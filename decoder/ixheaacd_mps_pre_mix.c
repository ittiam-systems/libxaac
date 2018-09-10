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

#include <ixheaacd_type_def.h>
#include "ixheaacd_constants.h"
#include <ixheaacd_basic_ops32.h>
#include <ixheaacd_basic_ops40.h>

#include <math.h>
#include <memory.h>

#include <assert.h>

#undef ABS_THR
#define ABS_THR 1.0e-9f

#define ICC_FIX
#define UNSINGULARIZE_FIX
#define QUANTIZE_PARS_FIX

#define PI 3.14159265358979f

#define ONE_IN_Q28 (268435456)
#define MINUS_ONE_IN_Q28 (-268435456)
#define PI_Q28 (843314856)
#define PI_Q27 (421657428)
#define PI_BY_8_Q28 (105414352)

extern const WORD32 ixheaacd_im_weight_Q28[16][8][31];
extern const WORD32 ixheaacd_re_weight_Q28[16][8][31];
extern const WORD32 ixheaacd_beta_Q28[16][8][31];
extern const WORD32 ixheaacd_weight_Q28[16][8][31];
extern const WORD32 ixheaacd_c_l_table_Q31[31];
extern const WORD32 ixheaacd_sin_table_Q31[16][31];
extern const WORD32 ixheaacd_cos_table_Q31[16][31];
extern const WORD32 ixheaacd_atan_table_Q28[16][8][31];
extern WORD32 ixheaacd_ipd_de_quant_table_q28[16];

#define P_PI 3.1415926535897932
#define PI_IN_Q28 843314880

extern WORD32 ixheaacd_ipd_de_quant_table_q28[16];

#define P_PI 3.1415926535897932
#define PI_IN_Q28 843314880

static WORD32 ixheaacd_mps_phase_wraping(WORD32 phase) {
  const WORD32 pi_2 = 2 * PI_IN_Q28;

  while (phase < 0) phase += pi_2;
  while (phase >= pi_2) phase -= pi_2;
  assert((phase >= 0) && (phase < pi_2));

  return phase;
}

static VOID ixheaacd_mps_buffer_pre_and_mix_matrix(
    ia_mps_dec_state_struct *self) {
  WORD32 pb, row, col;

  for (pb = 0; pb < self->bs_param_bands; pb++) {
    for (row = 0; row < MAX_M_INPUT; row++) {
      for (col = 0; col < MAX_M_OUTPUT; col++) {
        self->m1_param_re_prev[pb][row][col] =
            self->m1_param_re[self->num_parameter_sets_prev - 1][pb][row][col];
        self->m1_param_im_prev[pb][row][col] =
            self->m1_param_im[self->num_parameter_sets_prev - 1][pb][row][col];
        self->m2_decor_re_prev[pb][row][col] =
            self->m2_decor_re[self->num_parameter_sets_prev - 1][pb][row][col];
        self->m2_decor_im_prev[pb][row][col] =
            self->m2_decor_im[self->num_parameter_sets_prev - 1][pb][row][col];
        self->m2_resid_re_prev[pb][row][col] =
            self->m2_resid_re[self->num_parameter_sets_prev - 1][pb][row][col];
        self->m2_resid_im_prev[pb][row][col] =
            self->m2_resid_im[self->num_parameter_sets_prev - 1][pb][row][col];
      }
    }
  }

  for (pb = 0; pb < self->bs_param_bands; pb++) {
    self->phase_l_prev[pb] =
        self->phase_l[self->num_parameter_sets_prev - 1][pb];
    self->phase_r_prev[pb] =
        self->phase_r[self->num_parameter_sets_prev - 1][pb];
  }
}

VOID ixheaacd_fix_to_float_int(WORD32 *inp, FLOAT32 *out, WORD32 length,
                               FLOAT32 q_fac) {
  WORD32 i;
  FLOAT32 m_qfac = 1.0f / q_fac;

  for (i = 0; i < length; i++) out[i] = (FLOAT32)(inp[i]) * m_qfac;
}

VOID ixheaacd_pre_and_mix_matrix_calculation(ia_mps_dec_state_struct *self) {
  WORD32 ps, pb;
  ia_mps_bs_frame *curr_bit_stream = &(self->bs_frame);
  WORD32 h_imag[2 * MAX_PARAMETER_BANDS];
  WORD32
  h_real[6 * MAX_PARAMETER_BANDS];

  ixheaacd_mps_buffer_pre_and_mix_matrix(self);

  for (ps = 0; ps < self->num_parameter_sets; ps++) {
    WORD32 *h_im = &h_imag[0];
    WORD32 *h_re = &h_real[0];
    memset(h_real, 0, 6 * MAX_PARAMETER_BANDS * sizeof(WORD32));
    memset(h_imag, 0, 2 * MAX_PARAMETER_BANDS * sizeof(WORD32));

    switch (self->config->bs_phase_coding) {
      case 0:
        if (self->residual_coding) {
          ixheaacd_mps_par2umx_pred(self, curr_bit_stream, h_imag, h_real, ps,
                                    self->res_bands);
        } else {
          ixheaacd_mps_par2umx_ps(self, curr_bit_stream, h_real, ps);
        }

        break;
      case 1:
        ixheaacd_mps_par2umx_ps_ipd_opd(self, curr_bit_stream, h_real, ps);
        break;
      case 2:
        ixheaacd_mps_par2umx_pred(self, curr_bit_stream, h_imag, h_real, ps,
                                  self->res_bands);
        break;
    }

    for (pb = 0; pb < self->bs_param_bands; pb++) {
      self->m1_param_re[ps][pb][0][0] = 1073741824;
      self->m1_param_re[ps][pb][1][0] = 1073741824;

      self->m1_param_im[ps][pb][0][0] = 0;
      self->m1_param_im[ps][pb][1][0] = 0;

      self->m2_resid_re[ps][pb][0][0] = *h_re++;
      self->m2_resid_im[ps][pb][0][0] = *h_im++;
      self->m2_resid_im[ps][pb][0][1] = 0;

      self->m2_resid_re[ps][pb][1][0] = *h_re++;
      self->m2_resid_im[ps][pb][1][0] = *h_im++;
      self->m2_resid_im[ps][pb][1][1] = 0;

      self->m2_decor_re[ps][pb][0][0] = 0;
      self->m2_decor_im[ps][pb][0][0] = 0;
      self->m2_decor_re[ps][pb][0][1] = *h_re++;
      self->m2_decor_im[ps][pb][0][1] = 0;

      self->m2_decor_re[ps][pb][1][0] = 0;
      self->m2_decor_im[ps][pb][1][0] = 0;
      self->m2_decor_re[ps][pb][1][1] = *h_re++;
      self->m2_decor_im[ps][pb][1][1] = 0;

      self->m2_resid_re[ps][pb][0][1] = *h_re++;
      self->m2_resid_re[ps][pb][1][1] = *h_re++;
    }
  }
  ixheaacd_mps_smoothing_opd(self);

  ixheaacd_fix_to_float_int(&self->phase_l_fix[0][0], &self->phase_l[0][0],
                            MAX_PARAMETER_SETS_MPS * MAX_PARAMETER_BANDS,
                            268435456.0f);
  ixheaacd_fix_to_float_int(&self->phase_r_fix[0][0], &self->phase_r[0][0],
                            MAX_PARAMETER_SETS_MPS * MAX_PARAMETER_BANDS,
                            268435456.0f);
}

static VOID ixheaacd_mps_par2umx_ps_core(WORD32 cld[MAX_PARAMETER_BANDS],
                                         WORD32 icc[MAX_PARAMETER_BANDS],
                                         WORD32 ott_band_count,
                                         WORD32 *h_real) {
  WORD32 band;
  WORD32 c_l_temp, c_r_temp, cld_idx, icc_idx, temp;

  for (band = 0; band < ott_band_count; band++) {
    cld_idx = *cld++ + 15;
    icc_idx = *icc++;

    c_l_temp = (ixheaacd_c_l_table_Q31[cld_idx]);
    c_r_temp = (ixheaacd_c_l_table_Q31[30 - cld_idx]);

    temp = ixheaacd_cos_table_Q31[icc_idx][cld_idx];
    *h_real++ = ixheaacd_mult32(temp, c_l_temp) >> 2;

    temp = ixheaacd_cos_table_Q31[icc_idx][30 - cld_idx];
    *h_real++ = ixheaacd_mult32(temp, c_r_temp) >> 2;

    temp = ixheaacd_sin_table_Q31[icc_idx][cld_idx];
    *h_real++ = ixheaacd_mult32(temp, c_l_temp) >> 2;

    temp = -ixheaacd_sin_table_Q31[icc_idx][30 - cld_idx];
    *h_real++ = ixheaacd_mult32(temp, c_r_temp) >> 2;

    h_real += 2;
  }
}

VOID ixheaacd_mps_par2umx_ps(ia_mps_dec_state_struct *self,
                             ia_mps_bs_frame *curr_bit_stream, WORD32 *h_real,
                             WORD32 param_set_idx) {
  ixheaacd_mps_par2umx_ps_core(curr_bit_stream->cld_idx[param_set_idx],
                               curr_bit_stream->icc_idx[param_set_idx],
                               self->bs_param_bands, h_real);
}

static VOID ixheaacd_mps_opd_calc(ia_mps_dec_state_struct *self,
                                  ia_mps_bs_frame *curr_bit_stream,
                                  WORD32 param_set_idx,
                                  WORD32 opd[MAX_PARAMETER_BANDS]) {
  WORD32 band;

  for (band = 0; band < self->num_bands_ipd; band++) {
    WORD32 cld_idx = curr_bit_stream->cld_idx[param_set_idx][band] + 15;
    WORD32 ipd_idx = (curr_bit_stream->ipd_idx[param_set_idx][band]) & 15;
    WORD32 icc_idx = curr_bit_stream->icc_idx[param_set_idx][band];

    if ((cld_idx == 15) && (ipd_idx == 8))
      opd[band] = 0;
    else
      opd[band] = ixheaacd_atan_table_Q28[ipd_idx][icc_idx][cld_idx];
  }
}

VOID ixheaacd_mps_par2umx_ps_ipd_opd(ia_mps_dec_state_struct *self,
                                     ia_mps_bs_frame *curr_bit_stream,
                                     WORD32 *h_real, WORD32 param_set_idx) {
  WORD32 opd[MAX_PARAMETER_BANDS];
  WORD32 ott_band_count = self->bs_param_bands;
  WORD32 num_bands_ipd = self->num_bands_ipd;
  WORD32 band;

  ixheaacd_mps_par2umx_ps_core(curr_bit_stream->cld_idx[param_set_idx],
                               curr_bit_stream->icc_idx[param_set_idx],
                               ott_band_count, h_real);

  if (self->bs_phase_mode) {
    ixheaacd_mps_opd_calc(self, curr_bit_stream, param_set_idx, opd);

    for (band = 0; band < num_bands_ipd; band++) {
      WORD32 ipd_idx = curr_bit_stream->ipd_idx[param_set_idx][band] & 15;
      WORD32 ipd = ixheaacd_ipd_de_quant_table_q28[ipd_idx];

      self->phase_l_fix[param_set_idx][band] =
          ixheaacd_mps_phase_wraping(opd[band]);
      self->phase_r_fix[param_set_idx][band] =
          ixheaacd_mps_phase_wraping(opd[band] - ipd);
    }
  } else {
    num_bands_ipd = 0;
  }

  for (band = num_bands_ipd; band < ott_band_count; band++) {
    self->phase_l_fix[param_set_idx][band] = 0;
    self->phase_r_fix[param_set_idx][band] = 0;
  }
}

VOID ixheaacd_mps_par2umx_pred(ia_mps_dec_state_struct *self,
                               ia_mps_bs_frame *curr_bit_stream, WORD32 *h_imag,
                               WORD32 *h_real, WORD32 param_set_idx,
                               WORD32 res_bands) {
  WORD32 band;

  for (band = 0; band < self->bs_param_bands; band++) {
    WORD32 cld_idx = curr_bit_stream->cld_idx[param_set_idx][band] + 15;
    WORD32 icc_idx = curr_bit_stream->icc_idx[param_set_idx][band];
    WORD32 ipd_idx = curr_bit_stream->ipd_idx[param_set_idx][band] & 15;

    if ((band < self->num_bands_ipd) && (cld_idx == 15) && (icc_idx == 0) &&
        (ipd_idx == 8)) {
      WORD32 gain = 111848107;
      *h_imag++ = 0;
      *h_imag++ = 0;

      if (band < res_bands) {
        *h_real++ = gain;
        *h_real++ = gain;
        h_real += 2;

        *h_real++ = gain;
        *h_real++ = -gain;
      } else {
        *h_real++ = gain;
        *h_real++ = -gain;

        h_real += 4;
      }
    } else {
      WORD32 weight_fix, re_weight_fix, im_weight_fix;

      weight_fix = ixheaacd_weight_Q28[ipd_idx][icc_idx][cld_idx];
      re_weight_fix = ixheaacd_re_weight_Q28[ipd_idx][icc_idx][cld_idx];
      im_weight_fix = ixheaacd_im_weight_Q28[ipd_idx][icc_idx][cld_idx];

      if (band < self->num_bands_ipd) {
        weight_fix = ixheaacd_weight_Q28[ipd_idx][icc_idx][cld_idx];
        re_weight_fix = ixheaacd_re_weight_Q28[ipd_idx][icc_idx][cld_idx];
        im_weight_fix = ixheaacd_im_weight_Q28[ipd_idx][icc_idx][cld_idx];
      } else {
        weight_fix = ixheaacd_weight_Q28[0][icc_idx][cld_idx];
        re_weight_fix = ixheaacd_re_weight_Q28[0][icc_idx][cld_idx];
        im_weight_fix = ixheaacd_im_weight_Q28[0][icc_idx][cld_idx];
      }

      *h_real++ = weight_fix - re_weight_fix;
      *h_imag++ = -im_weight_fix;
      *h_real++ = weight_fix + re_weight_fix;
      *h_imag++ = im_weight_fix;

      if (band < res_bands) {
        h_real += 2;

        *h_real++ = weight_fix;
        *h_real++ = -weight_fix;
      } else {
        WORD32 beta = ixheaacd_beta_Q28[ipd_idx][icc_idx][cld_idx];

        *h_real++ = beta;
        *h_real++ = -beta;
        h_real += 2;
      }
    }
  }
}

WORD32 ixheaacd_mps_apply_pre_matrix(ia_mps_dec_state_struct *self) {
  WORD32 ts, qs, row, col = 0;
  WORD32 err = 0;
  err = ixheaacd_mps_upmix_interp(
      self->m1_param_re, self->r_out_re_scratch_m1, self->m1_param_re_prev,
      (self->dir_sig_count + self->decor_sig_count), 1, self);
  if (err < 0) return err;
  err = ixheaacd_mps_upmix_interp(
      self->m1_param_im, self->r_out_im_scratch_m1, self->m1_param_im_prev,
      (self->dir_sig_count + self->decor_sig_count), 1, self);
  if (err < 0) return err;

  ixheaacd_fix_to_float_int(
      (WORD32 *)(self->r_out_re_scratch_m1), (FLOAT32 *)(self->r_out_re_in_m1),
      MAX_TIME_SLOTS * MAX_PARAMETER_BANDS * MAX_M_OUTPUT * MAX_M_INPUT,
      1073741824);
  ixheaacd_fix_to_float_int(
      (WORD32 *)self->r_out_im_scratch_m1, (FLOAT32 *)self->r_out_im_in_m1,
      MAX_TIME_SLOTS * MAX_PARAMETER_BANDS * MAX_M_OUTPUT * MAX_M_INPUT,
      1073741824);

  for (ts = 0; ts < self->time_slots; ts++) {
    for (qs = 0; qs < 2; qs++) {
      WORD32 sign = -1;
      WORD32 indx = self->hyb_band_to_processing_band_table[qs];
      for (row = 0; row < (self->dir_sig_count + self->decor_sig_count);
           row++) {
        FLOAT32 sum_real = 0.0f;
        FLOAT32 sum_imag = 0.0f;

        {
          FLOAT32 real = self->hyb_in[0][ts][qs].re *
                             self->r_out_re_in_m1[ts][indx][row][col] -
                         self->hyb_in[0][ts][qs].im *
                             self->r_out_im_in_m1[ts][indx][row][col] * sign;
          FLOAT32 imag = self->hyb_in[0][ts][qs].re *
                             self->r_out_im_in_m1[ts][indx][row][col] * sign +
                         self->hyb_in[0][ts][qs].im *
                             self->r_out_re_in_m1[ts][indx][row][col];
          sum_real += real;
          sum_imag += imag;
        }
        self->v[row][ts][qs].re = sum_real;
        self->v[row][ts][qs].im = sum_imag;
      }
    }
    for (qs = 2; qs < self->hyb_band_count; qs++) {
      WORD32 sign = 1;
      WORD32 indx = self->hyb_band_to_processing_band_table[qs];
      for (row = 0; row < (self->dir_sig_count + self->decor_sig_count);
           row++) {
        FLOAT32 sum_real = 0.0f;
        FLOAT32 sum_imag = 0.0f;

        {
          FLOAT32 real = self->hyb_in[0][ts][qs].re *
                             self->r_out_re_in_m1[ts][indx][row][col] -
                         self->hyb_in[0][ts][qs].im *
                             self->r_out_im_in_m1[ts][indx][row][col] * sign;
          FLOAT32 imag = self->hyb_in[0][ts][qs].re *
                             self->r_out_im_in_m1[ts][indx][row][col] * sign +
                         self->hyb_in[0][ts][qs].im *
                             self->r_out_re_in_m1[ts][indx][row][col];
          sum_real += real;
          sum_imag += imag;
        }
        self->v[row][ts][qs].re = sum_real;
        self->v[row][ts][qs].im = sum_imag;
      }
    }
  }
  return 0;
}

WORD32 ixheaacd_mps_apply_mix_matrix(ia_mps_dec_state_struct *self) {
  WORD32 ts, qs, row, col;
  WORD32 complex_m2 = ((self->config->bs_phase_coding != 0));
  WORD32 phase_interpolation = (self->config->bs_phase_coding == 1);
  WORD32 err = 0;
  err = ixheaacd_mps_upmix_interp(
      self->m2_decor_re, self->r_diff_out_re_fix_in_m2, self->m2_decor_re_prev,
      self->out_ch_count, (self->dir_sig_count + self->decor_sig_count), self);
  if (err < 0) return err;
  err = ixheaacd_mps_upmix_interp(
      self->m2_resid_re, self->r_out_re_fix_in_m2, self->m2_resid_re_prev,
      self->out_ch_count, (self->dir_sig_count + self->decor_sig_count), self);
  if (err < 0) return err;
  ixheaacd_fix_to_float_int(
      (WORD32 *)self->r_out_re_fix_in_m2, (FLOAT32 *)self->r_out_re_in_m2,
      MAX_TIME_SLOTS * MAX_PARAMETER_BANDS * MAX_M_OUTPUT * MAX_M_INPUT,
      268435456);
  ixheaacd_fix_to_float_int(
      (WORD32 *)self->r_diff_out_re_fix_in_m2,
      (FLOAT32 *)self->r_out_diff_re_in_m2,
      MAX_TIME_SLOTS * MAX_PARAMETER_BANDS * MAX_M_OUTPUT * MAX_M_INPUT,
      268435456);

  if (complex_m2 && !phase_interpolation) {
    err = ixheaacd_mps_upmix_interp(
        self->m2_decor_im, self->r_diff_out_im_fix_in_m2,
        self->m2_decor_im_prev, self->out_ch_count,
        (self->dir_sig_count + self->decor_sig_count), self);
    if (err < 0) return err;
    err = ixheaacd_mps_upmix_interp(
        self->m2_resid_im, self->r_out_im_fix_in_m2, self->m2_resid_im_prev,
        self->out_ch_count, (self->dir_sig_count + self->decor_sig_count),
        self);
    if (err < 0) return err;
    ixheaacd_fix_to_float_int(
        (WORD32 *)self->r_diff_out_im_fix_in_m2,
        (FLOAT32 *)self->r_out_diff_im_in_m2,
        MAX_TIME_SLOTS * MAX_PARAMETER_BANDS * MAX_M_OUTPUT * MAX_M_INPUT,
        268435456);
    ixheaacd_fix_to_float_int(
        (WORD32 *)self->r_out_im_fix_in_m2, (FLOAT32 *)self->r_out_im_in_m2,
        MAX_TIME_SLOTS * MAX_PARAMETER_BANDS * MAX_M_OUTPUT * MAX_M_INPUT,
        268435456);
  }

  if (phase_interpolation) {
    ixheaacd_mps_phase_interpolation(
        self->phase_l, self->phase_r, self->phase_l_prev, self->phase_r_prev,
        self->r_out_ph_re_in_m2, self->r_out_ph_im_in_m2, self);

    for (ts = 0; ts < self->time_slots; ts++) {
      WORD32 pb;
      for (pb = 0; pb < self->bs_param_bands; pb++) {
        for (row = 0; row < self->out_ch_count; row++) {
          for (col = 0; col < (self->dir_sig_count + self->decor_sig_count);
               col++) {
            self->r_out_im_in_m2[ts][pb][row][col] =
                self->r_out_re_in_m2[ts][pb][row][col] *
                self->r_out_ph_im_in_m2[ts][pb][row];
            self->r_out_re_in_m2[ts][pb][row][col] =
                self->r_out_re_in_m2[ts][pb][row][col] *
                self->r_out_ph_re_in_m2[ts][pb][row];

            self->r_out_diff_im_in_m2[ts][pb][row][col] =
                self->r_out_diff_re_in_m2[ts][pb][row][col] *
                self->r_out_ph_im_in_m2[ts][pb][row];
            self->r_out_diff_re_in_m2[ts][pb][row][col] =
                self->r_out_diff_re_in_m2[ts][pb][row][col] *
                self->r_out_ph_re_in_m2[ts][pb][row];
          }
        }
      }
    }
  }

  for (ts = 0; ts < self->time_slots; ts++) {
    for (qs = 0; qs < self->hyb_band_count; qs++) {
      WORD32 indx = self->hyb_band_to_processing_band_table[qs];
      for (row = 0; row < self->out_ch_count; row++) {
        FLOAT32 sum_re_dir = 0;
        FLOAT32 sum_re_diff = 0;
        FLOAT32 sum_im_dir = 0;
        FLOAT32 sum_im_diff = 0;
        for (col = 0; col < (self->dir_sig_count + self->decor_sig_count);
             col++) {
          sum_re_dir += self->w_dir[col][ts][qs].re *
                        self->r_out_re_in_m2[ts][indx][row][col];
          sum_im_dir += self->w_dir[col][ts][qs].im *
                        self->r_out_re_in_m2[ts][indx][row][col];
          sum_re_diff += self->w_diff[col][ts][qs].re *
                         self->r_out_diff_re_in_m2[ts][indx][row][col];
          sum_im_diff += self->w_diff[col][ts][qs].im *
                         self->r_out_diff_re_in_m2[ts][indx][row][col];
        }
        self->hyb_dir_out[row][ts][qs].re = sum_re_dir;
        self->hyb_dir_out[row][ts][qs].im = sum_im_dir;
        self->hyb_diff_out[row][ts][qs].re = sum_re_diff;
        self->hyb_diff_out[row][ts][qs].im = sum_im_diff;
      }
    }
  }

  if (complex_m2) {
    for (ts = 0; ts < self->time_slots; ts++) {
      for (qs = 0; qs < 2; qs++) {
        WORD32 indx = self->hyb_band_to_processing_band_table[qs];
        for (row = 0; row < self->out_ch_count; row++) {
          FLOAT32 sum_re_dir = self->hyb_dir_out[row][ts][qs].re;
          FLOAT32 sum_im_dir = self->hyb_dir_out[row][ts][qs].im;
          FLOAT32 sum_re_diff = self->hyb_diff_out[row][ts][qs].re;
          FLOAT32 sum_im_diff = self->hyb_diff_out[row][ts][qs].im;
          for (col = 0; col < (self->dir_sig_count + self->decor_sig_count);
               col++) {
            sum_re_dir += self->w_dir[col][ts][qs].im *
                          self->r_out_im_in_m2[ts][indx][row][col];
            sum_im_dir -= self->w_dir[col][ts][qs].re *
                          self->r_out_im_in_m2[ts][indx][row][col];
            sum_re_diff += self->w_diff[col][ts][qs].im *
                           self->r_out_diff_im_in_m2[ts][indx][row][col];
            sum_im_diff -= self->w_diff[col][ts][qs].re *
                           self->r_out_diff_im_in_m2[ts][indx][row][col];
          }
          self->hyb_dir_out[row][ts][qs].re = sum_re_dir;
          self->hyb_dir_out[row][ts][qs].im = sum_im_dir;
          self->hyb_diff_out[row][ts][qs].re = sum_re_diff;
          self->hyb_diff_out[row][ts][qs].im = sum_im_diff;
        }
      }
      for (qs = 2; qs < self->hyb_band_count; qs++) {
        WORD32 indx = self->hyb_band_to_processing_band_table[qs];
        for (row = 0; row < self->out_ch_count; row++) {
          FLOAT32 sum_re_dir = self->hyb_dir_out[row][ts][qs].re;
          FLOAT32 sum_im_dir = self->hyb_dir_out[row][ts][qs].im;
          FLOAT32 sum_re_diff = self->hyb_diff_out[row][ts][qs].re;
          FLOAT32 sum_im_diff = self->hyb_diff_out[row][ts][qs].im;
          for (col = 0; col < (self->dir_sig_count + self->decor_sig_count);
               col++) {
            sum_re_dir -= self->w_dir[col][ts][qs].im *
                          self->r_out_im_in_m2[ts][indx][row][col];
            sum_im_dir += self->w_dir[col][ts][qs].re *
                          self->r_out_im_in_m2[ts][indx][row][col];
            sum_re_diff -= self->w_diff[col][ts][qs].im *
                           self->r_out_diff_im_in_m2[ts][indx][row][col];
            sum_im_diff += self->w_diff[col][ts][qs].re *
                           self->r_out_diff_im_in_m2[ts][indx][row][col];
          }
          self->hyb_dir_out[row][ts][qs].re = sum_re_dir;
          self->hyb_dir_out[row][ts][qs].im = sum_im_dir;
          self->hyb_diff_out[row][ts][qs].re = sum_re_diff;
          self->hyb_diff_out[row][ts][qs].im = sum_im_diff;
        }
      }
    }
  }
  return 0;
}

static PLATFORM_INLINE WORD32 ixheaacd_mult32_shl2(WORD32 a, WORD32 b) {
  WORD32 result;
  WORD64 temp_result;

  temp_result = (WORD64)a * (WORD64)b;
  result = (WORD32)(temp_result >> 30);

  return (result);
}

WORD32 ixheaacd_mps_upmix_interp(
    WORD32 m_matrix[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                   [MAX_M_INPUT],
    WORD32 r_matrix[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                   [MAX_M_INPUT],
    WORD32 m_matrix_prev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
    WORD32 num_rows, WORD32 num_cols, ia_mps_dec_state_struct *self) {
  WORD32 ts, ps, pb, row, col, i;

  for (pb = 0; pb < self->bs_param_bands; pb++) {
    for (row = 0; row < num_rows; row++) {
      for (col = 0; col < num_cols; col++) {
        ps = 0;
        ts = 0;
        if (MAX_TIME_SLOTS < (self->param_slot_diff[0])) return -1;
        for (i = 1; i <= (WORD32)self->param_slot_diff[0]; i++) {
          WORD32 alpha = i * self->inv_param_slot_diff_Q30[ps];
          WORD32 one_minus_alpha = 1073741824 - alpha;
          r_matrix[ts][pb][row][col] =
              ((ixheaacd_mult32_shl2(m_matrix_prev[pb][row][col],
                                     one_minus_alpha) +
                ixheaacd_mult32_shl2(alpha, m_matrix[ps][pb][row][col])));
          ts++;
        }

        for (ps = 1; ps < self->num_parameter_sets; ps++) {
          if (MAX_TIME_SLOTS < (ts + self->param_slot_diff[ps])) return -1;
          for (i = 1; i <= (WORD32)self->param_slot_diff[ps]; i++) {
            WORD32 alpha = i * self->inv_param_slot_diff_Q30[ps];
            WORD32 one_minus_alpha = 1073741824 - alpha;
            r_matrix[ts][pb][row][col] =
                ((ixheaacd_mult32_shl2(m_matrix[ps - 1][pb][row][col],
                                       one_minus_alpha) +
                  ixheaacd_mult32_shl2(alpha, m_matrix[ps][pb][row][col])));
            ts++;
          }
        }
      }
    }
  }
  return 0;
}

static FLOAT32 ixheaacd_mps_angle_interpolation(FLOAT32 angle1, FLOAT32 angle2,
                                                FLOAT32 alpha) {
  while (angle2 - angle1 > (FLOAT32)P_PI)
    angle1 = angle1 + 2.0f * (FLOAT32)P_PI;
  while (angle1 - angle2 > (FLOAT32)P_PI)
    angle2 = angle2 + 2.0f * (FLOAT32)P_PI;

  return (1 - alpha) * angle1 + alpha * angle2;
}

VOID ixheaacd_mps_phase_interpolation(
    FLOAT32 pl[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS],
    FLOAT32 pr[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS],
    FLOAT32 pl_prev[MAX_PARAMETER_BANDS], FLOAT32 pr_prev[MAX_PARAMETER_BANDS],
    FLOAT32 r_re[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2],
    FLOAT32 r_im[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2],
    ia_mps_dec_state_struct *self) {
  WORD32 ts, ps, pb;
  WORD32 i;
  for (pb = 0; pb < self->bs_param_bands; pb++) {
    ps = 0;
    ts = 0;
    for (i = 1; i <= self->param_slot_diff[ps]; i++) {
      FLOAT32 alpha = (FLOAT32)i * self->inv_param_slot_diff[ps];
      FLOAT32 t;

      t = ixheaacd_mps_angle_interpolation(pl_prev[pb], pl[ps][pb], alpha);
      r_re[ts][pb][0] = (FLOAT32)cos(t);
      r_im[ts][pb][0] = (FLOAT32)sin(t);

      t = ixheaacd_mps_angle_interpolation(pr_prev[pb], pr[ps][pb], alpha);
      r_re[ts][pb][1] = (FLOAT32)cos(t);
      r_im[ts][pb][1] = (FLOAT32)sin(t);
      ts++;
    }

    for (ps = 1; ps < self->num_parameter_sets; ps++) {
      for (i = 1; i <= self->param_slot_diff[ps]; i++) {
        FLOAT32 alpha = (FLOAT32)i * self->inv_param_slot_diff[ps];
        FLOAT32 t;

        t = ixheaacd_mps_angle_interpolation(pl[ps - 1][pb], pl[ps][pb], alpha);
        r_re[ts][pb][0] = (FLOAT32)cos(t);
        r_im[ts][pb][0] = (FLOAT32)sin(t);

        t = ixheaacd_mps_angle_interpolation(pr[ps - 1][pb], pr[ps][pb], alpha);
        r_re[ts][pb][1] = (FLOAT32)cos(t);
        r_im[ts][pb][1] = (FLOAT32)sin(t);
        ts++;
      }
    }
  }
}

VOID ixheaacd_mps_init_pre_and_post_matrix(ia_mps_dec_state_struct *self) {
  memset(self->m1_param_re_prev, 0,
         MAX_PARAMETER_BANDS * MAX_M_OUTPUT * MAX_M_INPUT * sizeof(WORD32));
  memset(self->m1_param_im_prev, 0,
         MAX_PARAMETER_BANDS * MAX_M_OUTPUT * MAX_M_INPUT * sizeof(WORD32));
  memset(self->m1_param_re_prev, 0,
         MAX_PARAMETER_BANDS * MAX_M_OUTPUT * MAX_M_INPUT * sizeof(WORD32));
  memset(self->m2_decor_re_prev, 0,
         MAX_PARAMETER_BANDS * MAX_M_OUTPUT * MAX_M_INPUT * sizeof(WORD32));
  memset(self->m2_resid_re_prev, 0,
         MAX_PARAMETER_BANDS * MAX_M_OUTPUT * MAX_M_INPUT * sizeof(WORD32));
  memset(self->m2_resid_im_prev, 0,
         MAX_PARAMETER_BANDS * MAX_M_OUTPUT * MAX_M_INPUT * sizeof(WORD32));
}
