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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "ixheaacd_type_def.h"
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_defines.h"
#include "ixheaacd_aac_rom.h"
#include "ixheaacd_pulsedata.h"
#include "ixheaacd_pns.h"
#include "ixheaacd_channelinfo.h"
#include "ixheaacd_common_rom.h"
#include "ixheaacd_sbrdecsettings.h"
#include "ixheaacd_sbr_scale.h"
#include "ixheaacd_env_extr_part.h"
#include "ixheaacd_sbr_rom.h"
#include "ixheaacd_hybrid.h"
#include "ixheaacd_ps_dec.h"
#include "ixheaacd_config.h"
#include "ixheaacd_qmf_dec.h"
#include "ixheaacd_audioobjtypes.h"
#include "ixheaacd_mps_polyphase.h"
#include "ixheaacd_mps_dec.h"
#include "ixheaacd_mps_interface.h"

#include "ixheaacd_mps_polyphase.h"

#include "ixheaacd_mps_decor.h"
#include "ixheaacd_mps_hybfilter.h"
#include "ixheaacd_error_standards.h"
#include "ixheaacd_constants.h"

static const WORD32 ixheaacd_decorr_delay[] = {11, 10, 5, 2};
static const WORD32 ixheaacd_decorr_delay_ldmps[] = {8, 7, 2, 1};

static const WORD32 ixheaacd_qmf_split_freq_0[] = {3, 15, 24, 65};
static const WORD32 ixheaacd_qmf_split_freq_1[] = {3, 50, 65, 65};
static const WORD32 ixheaacd_qmf_split_freq_2[] = {0, 15, 65, 65};


static const WORD32 ixheaacd_qmf_split_freq_0_ldmps[] = {0, 15, 24, 65};
static const WORD32 ixheaacd_qmf_split_freq_1_ldmps[] = {0, 50, 65, 65};
static const WORD32 ixheaacd_qmf_split_freq_2_ldmps[] = {0, 15, 65, 65};

extern const WORD32 ixheaacd_mps_gain_set_indx[29];

static const FLOAT32
    ixheaacd_lattice_coeff_0_filt_den_coeff[DECORR_FILT_0_ORD + 1] = {
        1.000000f, -0.314818f, -0.256828f, -0.173641f, -0.115077f, 0.000599f,
        0.033343f, 0.122672f,  -0.356362f, 0.128058f,  0.089800f};
static const FLOAT32
    ixheaacd_lattice_coeff_0_filt_num_coeff[DECORR_FILT_0_ORD + 1] = {
        0.089800f,  0.128058f,  -0.356362f, 0.122672f,  0.033343f, 0.000599f,
        -0.115077f, -0.173641f, -0.256828f, -0.314818f, 1.000000f};

static const FLOAT32
    ixheaacd_lattice_coeff_1_filt_den_coeff[DECORR_FILT_1_ORD + 1] = {
        1.000000f, -0.287137f, -0.088940f, 0.123204f, -0.126111f,
        0.064218f, 0.045768f,  -0.016264f, -0.122100f};
static const FLOAT32
    ixheaacd_lattice_coeff_1_filt_num_coeff[DECORR_FILT_1_ORD + 1] = {
        -0.122100f, -0.016264f, 0.045768f,  0.064218f, -0.126111f,
        0.123204f,  -0.088940f, -0.287137f, 1.000000f};

static const FLOAT32
    ixheaacd_lattice_coeff_2_filt_den_coeff[DECORR_FILT_2_ORD + 1] = {
        1.000000f, 0.129403f, -0.032633f, 0.035700f};
static const FLOAT32
    ixheaacd_lattice_coeff_2_filt_num_coeff[DECORR_FILT_2_ORD + 1] = {
        0.035700f, -0.032633f, 0.129403f, 1.000000f};

static const FLOAT32
    ixheaacd_lattice_coeff_3_filt_den_coeff[DECORR_FILT_3_ORD + 1] = {
        1.000000f, 0.034742f, -0.013000f};
static const FLOAT32
    ixheaacd_lattice_coeff_3_filt_num_coeff[DECORR_FILT_3_ORD + 1] = {
        -0.013000f, 0.034742f, 1.000000f};

static const FLOAT32
    ixheaacd_lattice_coeff_1_filt_num_ldmps[DECORR_FILT_1_ORD_LD_MPS + 1] = {
        (0.3355999887f),  (0.0024894588f),  (-0.1572290659f), (0.2807503343f),
        (-0.1942857355f), (0.3840600252f),  (-0.4084388912f), (-0.1750483066f),
        (0.5559588671f),  (-0.4935829639f), (0.0567415841f),  (-0.0658148378f),
        (0.3378961682f),  (0.2284426540f),  (-0.7025330663f), (1.0000000000f)};

static const FLOAT32
    ixheaacd_lattice_coeff_1_filt_den_ldmps[DECORR_FILT_1_ORD_LD_MPS + 1] = {
        (1.0000000000f),  (-0.7025330663f), (0.2284426540f),  (0.3378961682f),
        (-0.0658148378f), (0.0567415841f),  (-0.4935829639f), (0.5559588671f),
        (-0.1750483066f), (-0.4084388912f), (0.3840600252f),  (-0.1942857355f),
        (0.2807503343f),  (-0.1572290659f), (0.0024894588f),  (0.3355999887f)};

static const FLOAT32
    ixheaacd_lattice_coeff_2_filt_num_ldmps[DECORR_FILT_2_ORD_LD_MPS + 1] = {
        (-0.4623999894f), (0.2341193259f), (0.5163637400f), (-0.0253488291f),
        (-0.2871030867f), (0.0153170601f), (1.0000000000f)};

static const FLOAT32
    ixheaacd_lattice_coeff_2_filt_den_ldmps[DECORR_FILT_2_ORD_LD_MPS + 1] = {
        (1.0000000000f), (0.0153170601f), (-0.2871030867f), (-0.0253488291f),
        (0.5163637400f), (0.2341193259f), (-0.4623999894f)

};

static const FLOAT32
    ixheaacd_lattice_coeff_3_filt_num_ldmps[DECORR_FILT_3_ORD_LD_MPS + 1] = {
        (0.2468000054f), (0.0207958221f), (-0.3898491263f), (1.0000000000f)};

static const FLOAT32
    ixheaacd_lattice_coeff_3_filt_den_ldmps[DECORR_FILT_3_ORD_LD_MPS + 1] = {
        (1.0000000000f), (-0.3898491263f), (0.0207958221f), (0.2468000054f)};

extern WORD32
    ixheaacd_hybrid_band_71_to_processing_band_28_map[MAX_HYBRID_BANDS_MPS];
extern WORD32
    ixheaacd_hybrid_band_64_to_processing_band_23_map[MAX_HYBRID_BANDS_MPS];

static const WORD32 ixheaacd_hybrid_to_qmf_map[MAX_HYBRID_BANDS_MPS] = {
    0,  0,  0,  0,  0,  0,  1,  1,  2,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
    47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

static const WORD32 ixheaacd_hybrid_to_qmf_map_ldmps[MAX_HYBRID_BANDS_MPS] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
    54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70};

static void ixheaacd_mps_decor_filt_init(ia_mps_decor_filt_struct *self,
                                         WORD32 reverb_band,
                                         WORD32 object_type) {
  if (object_type == AOT_ER_AAC_ELD || object_type == AOT_ER_AAC_LD) {
    switch (reverb_band) {
      case 0:
        self->num_len = self->den_len = DECORR_FILT_0_ORD_LD_MPS + 1;
        self->num = NULL;
        self->den = NULL;

        break;
      case 1:
        self->num_len = self->den_len = DECORR_FILT_1_ORD_LD_MPS + 1;
        self->num = ixheaacd_lattice_coeff_1_filt_num_ldmps;
        self->den = ixheaacd_lattice_coeff_1_filt_den_ldmps;

        break;
      case 2:
        self->num_len = self->den_len = DECORR_FILT_2_ORD_LD_MPS + 1;
        self->num = ixheaacd_lattice_coeff_2_filt_num_ldmps;
        self->den = ixheaacd_lattice_coeff_2_filt_den_ldmps;
        break;
      case 3:
        self->num_len = self->den_len = DECORR_FILT_3_ORD_LD_MPS + 1;
        self->num = ixheaacd_lattice_coeff_3_filt_num_ldmps;
        self->den = ixheaacd_lattice_coeff_3_filt_den_ldmps;
        break;
    }
  } else {
    switch (reverb_band) {
      case 0:
        self->num_len = self->den_len = DECORR_FILT_0_ORD + 1;
        self->num = ixheaacd_lattice_coeff_0_filt_num_coeff;
        self->den = ixheaacd_lattice_coeff_0_filt_den_coeff;

        break;
      case 1:
        self->num_len = self->den_len = DECORR_FILT_1_ORD + 1;
        self->num = ixheaacd_lattice_coeff_1_filt_num_coeff;
        self->den = ixheaacd_lattice_coeff_1_filt_den_coeff;

        break;
      case 2:
        self->num_len = self->den_len = DECORR_FILT_2_ORD + 1;
        self->num = ixheaacd_lattice_coeff_2_filt_num_coeff;
        self->den = ixheaacd_lattice_coeff_2_filt_den_coeff;
        break;
      case 3:
        self->num_len = self->den_len = DECORR_FILT_3_ORD + 1;
        self->num = ixheaacd_lattice_coeff_3_filt_num_coeff;
        self->den = ixheaacd_lattice_coeff_3_filt_den_coeff;
        break;
    }
  }

  self->state_len = self->num_len;
  memset(self->state, 0,
         sizeof(ia_cmplx_flt_struct) * (MAX_DECORR_FIL_ORDER + 1));

  return;
}

static VOID ixheaacd_mps_allpass_apply(ia_mps_decor_filt_struct *self,
                                       ia_cmplx_flt_struct *input, WORD32 len,
                                       ia_cmplx_flt_struct *output) {
  WORD32 i, j;

  for (i = 0; i < len; i++) {
    output[i].re = self->state[0].re + input[i].re * self->num[0];
    output[i].im = self->state[0].im + input[i].im * self->num[0];

    for (j = 1; j < self->num_len; j++) {
      self->state[j - 1].re = self->state[j].re + self->num[j] * input[i].re -
                              self->den[j] * output[i].re;
      self->state[j - 1].im = self->state[j].im + self->num[j] * input[i].im -
                              self->den[j] * output[i].im;
    }
  }
}

static VOID ixheaacd_mps_decor_energy_adjustment(
    ixheaacd_mps_decor_energy_adjust_filt_struct *handle,
    ia_cmplx_flt_struct in[MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS],
    ia_cmplx_flt_struct out[MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS],
    WORD32 time_slots, WORD32 res_bands, WORD32 ldmps_present) {
  ixheaacd_mps_decor_energy_adjust_filt_struct *self =
      (ixheaacd_mps_decor_energy_adjust_filt_struct *)handle;
  FLOAT32 in_energy[MAX_PARAMETER_BANDS] = {0};
  FLOAT32 out_energy[MAX_PARAMETER_BANDS] = {0};
  FLOAT32 gain[MAX_PARAMETER_BANDS];
  WORD32 i, j, k, loop_counter;
  WORD32 *ptr_hybrid_band;

  if (ldmps_present == 1)
    ptr_hybrid_band = ixheaacd_hybrid_band_64_to_processing_band_23_map;
  else
    ptr_hybrid_band = ixheaacd_hybrid_band_71_to_processing_band_28_map;

  WORD32 start_param_band = 0, start_bin = 0;

  if (res_bands != NO_RES_BANDS) {
    start_bin = ixheaacd_mps_gain_set_indx[res_bands];
    start_param_band = res_bands;
  }

  for (i = 0; i < time_slots; i++) {
    memset(in_energy, 0, sizeof(FLOAT32) * MAX_PARAMETER_BANDS);
    memset(out_energy, 0, sizeof(FLOAT32) * MAX_PARAMETER_BANDS);

    for (j = start_bin; j < self->num_bins; j++) {
      k = ptr_hybrid_band[j];

      in_energy[k] += in[i][j].re * in[i][j].re + in[i][j].im * in[i][j].im;
      out_energy[k] +=
          out[i][j].re * out[i][j].re + out[i][j].im * out[i][j].im;
    }

    if (ldmps_present == 1)
      loop_counter = MAX_PARAMETER_BANDS;
    else
      loop_counter = MAX_PARAMETER_BANDS_MPS;

    for (k = start_param_band; k < loop_counter; k++) {
      self->smooth_in_energy[k] = self->smooth_in_energy[k] * DECOR_ALPHA +
                                  in_energy[k] * ONE_MINUS_DECOR_ALPHA;
      self->smooth_out_energy[k] = self->smooth_out_energy[k] * DECOR_ALPHA +
                                   out_energy[k] * ONE_MINUS_DECOR_ALPHA;

      gain[k] = 1.0f;

      if (self->smooth_out_energy[k] >
          self->smooth_in_energy[k] * DECOR_GAMMA) {
        gain[k] = (FLOAT32)sqrt(self->smooth_in_energy[k] * DECOR_GAMMA /
                                (self->smooth_out_energy[k] + ABS_THR));
      }

      if (self->smooth_in_energy[k] >
          self->smooth_out_energy[k] * DECOR_GAMMA) {
        gain[k] =
            min(2.0f, (FLOAT32)sqrt(self->smooth_in_energy[k] /
                                    (DECOR_GAMMA * self->smooth_out_energy[k] +
                                     ABS_THR)));
      }
    }

    for (j = start_bin; j < self->num_bins; j++) {
      k = ptr_hybrid_band[j];

      out[i][j].re *= gain[k];
      out[i][j].im *= gain[k];
    }
  }
}

IA_ERRORCODE ixheaacd_mps_decor_init(ia_mps_decor_struct_handle self,
                                     WORD32 subbands, WORD32 decor_config,
                                     WORD32 object_type) {
  WORD32 i, reverb_band;
  const WORD32 *splitfreq;
  const WORD32 *ptr_ixheaacd_hybrid_to_qmf_map;
  const WORD32 *ptr_decorr_delay;
  if (object_type == AOT_ER_AAC_ELD || object_type == AOT_ER_AAC_LD) {
    ptr_ixheaacd_hybrid_to_qmf_map = ixheaacd_hybrid_to_qmf_map_ldmps;
    ptr_decorr_delay = ixheaacd_decorr_delay_ldmps;
    switch (decor_config) {
      case 0:
        splitfreq = ixheaacd_qmf_split_freq_0_ldmps;
        break;
      case 1:
        splitfreq = ixheaacd_qmf_split_freq_1_ldmps;
        break;
      case 2:
        splitfreq = ixheaacd_qmf_split_freq_2_ldmps;
        break;
      default:
        return IA_FATAL_ERROR;
    }
  } else {
    ptr_ixheaacd_hybrid_to_qmf_map = ixheaacd_hybrid_to_qmf_map;
    ptr_decorr_delay = ixheaacd_decorr_delay;
    switch (decor_config) {
      case 0:
        splitfreq = ixheaacd_qmf_split_freq_0;
        break;
      case 1:
        splitfreq = ixheaacd_qmf_split_freq_1;
        break;
      case 2:
        splitfreq = ixheaacd_qmf_split_freq_2;
        break;
      default:
        return IA_FATAL_ERROR;
    }
  }

  self->num_bins = subbands;
  if (self->num_bins > MAX_HYBRID_BANDS_MPS) return IA_FATAL_ERROR;

  for (i = 0; i < self->num_bins; i++) {
    reverb_band = 0;
    while ((reverb_band < 3) &&
           (ptr_ixheaacd_hybrid_to_qmf_map[i] >= (splitfreq[reverb_band] - 1)))
      reverb_band++;

    self->delay_sample_count[i] = ptr_decorr_delay[reverb_band];
    ixheaacd_mps_decor_filt_init(&self->filter[i], reverb_band, object_type);
  }

  self->decor_nrg_smooth.num_bins = self->num_bins;

  return IA_NO_ERROR;
}

VOID ixheaacd_mps_decor_apply(
    ia_mps_decor_struct_handle self,
    ia_cmplx_flt_struct in[MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS],
    ia_cmplx_flt_struct out[MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS],
    WORD32 length, WORD32 res_bands, WORD32 ldmps_present) {
  WORD32 idx, sb_sample, index = 0;

  ia_cmplx_flt_struct scratch[MAX_TIME_SLOTS];

  if (res_bands != NO_RES_BANDS) index = ixheaacd_mps_gain_set_indx[res_bands];

  for (idx = index; idx < self->num_bins; idx++) {
    for (sb_sample = 0; sb_sample < length; sb_sample++) {
      self->decor_delay_buffer[idx][self->delay_sample_count[idx] + sb_sample]
          .re = in[sb_sample][idx].re;
      self->decor_delay_buffer[idx][self->delay_sample_count[idx] + sb_sample]
          .im = in[sb_sample][idx].im;
    }
    ixheaacd_mps_allpass_apply(&self->filter[idx],
                               self->decor_delay_buffer[idx], length, scratch);

    for (sb_sample = 0; sb_sample < length; sb_sample++) {
      out[sb_sample][idx].re = scratch[sb_sample].re;
      out[sb_sample][idx].im = scratch[sb_sample].im;
    }

    for (sb_sample = 0; sb_sample < self->delay_sample_count[idx];
         sb_sample++) {
      self->decor_delay_buffer[idx][sb_sample].re =
          self->decor_delay_buffer[idx][length + sb_sample].re;
      self->decor_delay_buffer[idx][sb_sample].im =
          self->decor_delay_buffer[idx][length + sb_sample].im;
    }
  }

  ixheaacd_mps_decor_energy_adjustment(&self->decor_nrg_smooth, in, out, length,
                                       res_bands,
                                       ldmps_present);
}
