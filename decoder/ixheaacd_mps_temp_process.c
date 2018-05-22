/******************************************************************************
 *                                                                            *
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************
 * Originally developed and contributed by Ittiam Systems Pvt. Ltd, Bangalore
*/
#include <ixheaacd_type_def.h>
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_config.h"
#include "ixheaacd_mps_polyphase.h"
#include "ixheaacd_mps_dec.h"
#include "ixheaacd_mps_interface.h"

#include "ixheaacd_mps_process.h"

#include <math.h>
#include <float.h>
#include <memory.h>

#include <assert.h>

#include "ixheaacd_common_rom.h"
#include "ixheaacd_defines.h"

#include "ixheaacd_pns.h"

#include <ixheaacd_aac_rom.h>
#include "ixheaacd_pulsedata.h"

#include "ixheaacd_sbrdecsettings.h"
#include "ixheaacd_sbr_scale.h"
#include "ixheaacd_lpp_tran.h"
#include "ixheaacd_env_extr_part.h"
#include <ixheaacd_sbr_rom.h>
#include "ixheaacd_hybrid.h"
#include "ixheaacd_ps_dec.h"
#include "ixheaacd_env_extr.h"

#include "ixheaacd_qmf_dec.h"

#include "ixheaacd_env_calc.h"
#include "ixheaacd_sbr_const.h"
#include "ixheaacd_pvc_dec.h"
#include "ixheaacd_sbr_dec.h"

#define HP_SIZE (9)

#define STP_LPF_COEFF1 (0.950f)
#define STP_LPF_COEFF2 (0.450f)
#define STP_UPDATE_ENERGY_RATE (32)
#define STP_SCALE_LIMIT (2.82f)
#define STP_DAMP (0.1f)

#define max(a, b) ((a > b) ? (a) : (b))
#define min(a, b) ((a < b) ? (a) : (b))

static FLOAT32 ixheaacd_bp[BP_SIZE] = {
    0.0000f, 0.0005f, 0.0092f, 0.0587f, 0.2580f, 0.7392f, 0.9791f,
    0.9993f, 1.0000f, 1.0000f, 1.0000f, 1.0000f, 0.9999f, 0.9984f,
    0.9908f, 0.9639f, 0.8952f, 0.7711f, 0.6127f, 0.4609f, 0.3391f,
    0.2493f, 0.1848f, 0.1387f, 0.1053f};

static FLOAT32 ixheaacd_gf[BP_SIZE] = {
    0.f,         0.f,         0.f,         0.f,         0.f,
    0.f,         1e-008f,     8.1e-007f,   3.61e-006f,  8.41e-006f,
    1.6e-005f,   2.704e-005f, 3.969e-005f, 5.625e-005f, 7.396e-005f,
    9.801e-005f, 0.00012321f, 0.00015625f, 0.00019881f, 0.00024964f,
    0.00032041f, 0.00041209f, 0.00053824f, 0.00070756f, 0.00094249f};

static VOID ixheaacd_mps_temp_process_scale_calc(ia_mps_dec_state_struct* self,
                                                 WORD32 ts, FLOAT32* scale) {
  FLOAT32 dir_energy;
  FLOAT32 diff_energy[2];
  FLOAT32 temp;

  WORD32 ch, n;
  WORD32 left_ch = 0, right_ch = 1;

  if (self->subband_var.init_flag == 0) {
    for (ch = 0; ch < 2; ch++) {
      self->subband_var.tp_scale_last[ch] = 1.0f;
      self->subband_var.nrg_diff_prev[ch] = 32768 * 32768;
    }

    self->subband_var.nrg_dir_prev = 32768 * 32768;
    self->subband_var.init_flag = 1;
  }

  if (self->subband_var.update_old_ener == STP_UPDATE_ENERGY_RATE) {
    self->subband_var.update_old_ener = 1;
    self->subband_var.nrg_dir_prev = self->subband_var.nrg_dir;
    for (ch = 0; ch < self->out_ch_count; ch++)
      self->subband_var.nrg_diff_prev[ch] = self->subband_var.nrg_diff[ch];
  } else
    self->subband_var.update_old_ener++;

  dir_energy = 0;

  for (n = 6; n < BP_SIZE; n++) {
    FLOAT32 dir_left_re = self->hyb_dir_out[left_ch][ts][n + 7].re;
    FLOAT32 dir_right_re = self->hyb_dir_out[right_ch][ts][n + 7].re;
    FLOAT32 dir_left_im = self->hyb_dir_out[left_ch][ts][n + 7].im;
    FLOAT32 dir_right_im = self->hyb_dir_out[right_ch][ts][n + 7].im;

    temp = ((dir_left_re + dir_right_re) * (dir_left_re + dir_right_re)) +
           ((dir_left_im + dir_right_im) * (dir_left_im + dir_right_im));
    dir_energy += temp * ixheaacd_bp[n] * ixheaacd_bp[n] * ixheaacd_gf[n] *
                  ixheaacd_gf[n];
  }

  self->subband_var.nrg_dir =
      (FLOAT32)(STP_LPF_COEFF1 * self->subband_var.nrg_dir +
                (1.0 - STP_LPF_COEFF1) * dir_energy);

  dir_energy /= (self->subband_var.nrg_dir_prev + ABS_THR);

  for (ch = 0; ch < self->out_ch_count; ch++) {
    diff_energy[ch] = 0;
    for (n = 6; n < BP_SIZE; n++) {
      FLOAT32 diff_re = self->hyb_diff_out[ch][ts][n + 7].re;
      FLOAT32 diff_im = self->hyb_diff_out[ch][ts][n + 7].im;

      temp = (diff_re * diff_re) + (diff_im * diff_im);
      diff_energy[ch] += temp * ixheaacd_bp[n] * ixheaacd_bp[n] *
                         ixheaacd_gf[n] * ixheaacd_gf[n];
    }

    self->subband_var.nrg_diff[ch] =
        (FLOAT32)(STP_LPF_COEFF1 * self->subband_var.nrg_diff[ch] +
                  (1.0 - STP_LPF_COEFF1) * diff_energy[ch]);
    diff_energy[ch] /= (self->subband_var.nrg_diff_prev[ch] + ABS_THR);
  }

  scale[left_ch] = (FLOAT32)sqrt((dir_energy) / (diff_energy[left_ch] + 1e-9));
  scale[right_ch] =
      (FLOAT32)sqrt((dir_energy) / (diff_energy[right_ch] + 1e-9));

  for (ch = 0; ch < self->out_ch_count; ch++) {
    scale[ch] = STP_DAMP + (1 - STP_DAMP) * scale[ch];
  }

  for (ch = 0; ch < self->out_ch_count; ch++) {
    scale[ch] =
        min(max(scale[ch], (FLOAT32)(1.0 / STP_SCALE_LIMIT)), STP_SCALE_LIMIT);
  }

  for (ch = 0; ch < self->out_ch_count; ch++) {
    scale[ch] =
        (FLOAT32)(STP_LPF_COEFF2 * scale[ch] +
                  (1.0 - STP_LPF_COEFF2) * self->subband_var.tp_scale_last[ch]);
    self->subband_var.tp_scale_last[ch] = scale[ch];
  }
}

static VOID ixheaacd_mps_subbandtp(ia_mps_dec_state_struct* self, WORD32 ts) {
  FLOAT32 scale[2];
  WORD32 ch, n;
  WORD32 no_scaling;
  FLOAT32 temp;
  const WORD32 ixheaacd_hybrid_to_qmf_map[] = {0, 0, 0, 0, 0, 0, 1, 1, 2, 2};

  ixheaacd_mps_temp_process_scale_calc(self, ts, scale);

  for (ch = 0; ch < self->out_ch_count; ch++) {
    no_scaling = 1;

    if ((self->config->bs_temp_shape_config == 1) ||
        (self->config->bs_temp_shape_config == 2))
      no_scaling = !self->temp_shape_enable_ch_stp[ch];

    if (no_scaling == 1) {
      for (n = 0; n < self->hyb_band_count; n++) {
        self->hyb_dir_out[ch][ts][n].re += self->hyb_diff_out[ch][ts][n].re;
        self->hyb_dir_out[ch][ts][n].im += self->hyb_diff_out[ch][ts][n].im;
      }
    } else {
      for (n = 0; n < 10; n++) {
        temp =
            (FLOAT32)(scale[ch] * ixheaacd_bp[ixheaacd_hybrid_to_qmf_map[n]]);
        self->hyb_dir_out[ch][ts][n].re +=
            (self->hyb_diff_out[ch][ts][n].re * temp);
        self->hyb_dir_out[ch][ts][n].im +=
            (self->hyb_diff_out[ch][ts][n].im * temp);
      }
      for (; n < HP_SIZE - 3 + 10; n++) {
        temp = (FLOAT32)(scale[ch] * ixheaacd_bp[n + 3 - 10]);
        self->hyb_dir_out[ch][ts][n].re +=
            (self->hyb_diff_out[ch][ts][n].re * temp);
        self->hyb_dir_out[ch][ts][n].im +=
            (self->hyb_diff_out[ch][ts][n].im * temp);
      }
      for (; n < self->hyb_band_count; n++) {
        temp = (FLOAT32)(scale[ch]);
        self->hyb_dir_out[ch][ts][n].re +=
            (self->hyb_diff_out[ch][ts][n].re * temp);
        self->hyb_dir_out[ch][ts][n].im +=
            (self->hyb_diff_out[ch][ts][n].im * temp);
      }
    }
  }
}

VOID ixheaacd_mps_temp_process(ia_mps_dec_state_struct* self) {
  WORD32 ch, ts, hyb;

  for (ch = 0; ch < self->out_ch_count; ch++) {
    for (ts = 0; ts < self->time_slots; ts++) {
      for (hyb = 0; hyb < HYBRID_BAND_BORDER; hyb++) {
        self->hyb_dir_out[ch][ts][hyb].re += self->hyb_diff_out[ch][ts][hyb].re;
        self->hyb_dir_out[ch][ts][hyb].im += self->hyb_diff_out[ch][ts][hyb].im;
        self->hyb_diff_out[ch][ts][hyb].re = 0;
        self->hyb_diff_out[ch][ts][hyb].im = 0;
      }
    }
  }

  for (ts = 0; ts < self->time_slots; ts++) ixheaacd_mps_subbandtp(self, ts);

  ixheaacd_mps_qmf_hyb_synthesis(self);

  for (ch = 0; ch < self->out_ch_count; ch++) {
    ixheaacd_sbr_dec_from_mps(&self->qmf_out_dir[ch][0][0].re,
                              self->p_sbr_dec[ch], self->p_sbr_frame[ch],
                              self->p_sbr_header[ch]);
  }

  ixheaacd_mps_synt_calc(self);
}
