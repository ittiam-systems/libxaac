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
#include <math.h>
#include <stdlib.h>
#include <ixheaacd_type_def.h>
#include "ixheaacd_constants.h"
#include <ixheaacd_basic_ops32.h>
#include <ixheaacd_basic_ops40.h>

VOID ixheaacd_mps_pre_matrix_mix_matrix_smoothing(
    ia_mps_dec_state_struct *self) {
  int smooth_band;
  int delta, one_minus_delta;

  int ps = 0, pb, row, col;
  int res_bands = 0;
  int *p_smoothing_data;

  if (self->residual_coding) res_bands = self->max_res_bands;

  p_smoothing_data = &self->smoothing_data[ps][res_bands];

  delta = self->param_slot_diff[ps] * self->inv_smoothing_time[ps];
  one_minus_delta = 1073741824 - delta;

  for (pb = res_bands; pb < self->bs_param_bands; pb++) {
    smooth_band = *p_smoothing_data++;
    if (smooth_band) {
      for (row = 0; row < MAX_M_OUTPUT; row++) {
        for (col = 0; col < MAX_M_INPUT; col++) {
          self->m1_param_re[ps][pb][row][col] =
              (ixheaacd_mult32(delta, self->m1_param_re[ps][pb][row][col]) +
               ixheaacd_mult32(one_minus_delta,
                               self->m1_param_re_prev[pb][row][col]))
              << 2;
          self->m1_param_im[ps][pb][row][col] =
              (ixheaacd_mult32(delta, self->m1_param_im[ps][pb][row][col]) +
               ixheaacd_mult32(one_minus_delta,
                               self->m1_param_im_prev[pb][row][col]))
              << 2;
          self->m2_decor_re[ps][pb][row][col] =
              (ixheaacd_mult32(delta, self->m2_decor_re[ps][pb][row][col]) +
               ixheaacd_mult32(one_minus_delta,
                               self->m2_decor_re_prev[pb][row][col]))
              << 2;
          self->m2_decor_im[ps][pb][row][col] =
              (ixheaacd_mult32(delta, self->m2_decor_im[ps][pb][row][col]) +
               ixheaacd_mult32(one_minus_delta,
                               self->m2_decor_im_prev[pb][row][col]))
              << 2;
          self->m2_resid_re[ps][pb][row][col] =
              (ixheaacd_mult32(delta, self->m2_resid_re[ps][pb][row][col]) +
               ixheaacd_mult32(one_minus_delta,
                               self->m2_resid_re_prev[pb][row][col]))
              << 2;
          self->m2_resid_im[ps][pb][row][col] =
              (ixheaacd_mult32(delta, self->m2_resid_im[ps][pb][row][col]) +
               ixheaacd_mult32(one_minus_delta,
                               self->m2_resid_im_prev[pb][row][col]))
              << 2;
        }
      }
    }
  }

  for (ps = 1; ps < self->num_parameter_sets; ps++) {
    delta = self->param_slot_diff[ps] * self->inv_smoothing_time[ps];
    one_minus_delta = 1073741824 - delta;

    p_smoothing_data = &self->smoothing_data[ps][res_bands];

    for (pb = res_bands; pb < self->bs_param_bands; pb++) {
      smooth_band = *p_smoothing_data++;
      if (smooth_band) {
        for (row = 0; row < MAX_M_OUTPUT; row++) {
          for (col = 0; col < MAX_M_INPUT; col++) {
            self->m1_param_re[ps][pb][row][col] =
                (ixheaacd_mult32(delta, self->m1_param_re[ps][pb][row][col]) +
                 ixheaacd_mult32(one_minus_delta,
                                 self->m1_param_re[ps - 1][pb][row][col]))
                << 2;
            self->m1_param_im[ps][pb][row][col] =
                (ixheaacd_mult32(delta, self->m1_param_im[ps][pb][row][col]) +
                 ixheaacd_mult32(one_minus_delta,
                                 self->m1_param_im[ps - 1][pb][row][col]))
                << 2;
            self->m2_resid_re[ps][pb][row][col] =
                (ixheaacd_mult32(delta, self->m2_resid_re[ps][pb][row][col]) +
                 ixheaacd_mult32(one_minus_delta,
                                 self->m2_resid_re[ps - 1][pb][row][col]))
                << 2;
            self->m2_decor_re[ps][pb][row][col] =
                (ixheaacd_mult32(delta, self->m2_decor_re[ps][pb][row][col]) +
                 ixheaacd_mult32(one_minus_delta,
                                 self->m2_decor_re[ps - 1][pb][row][col]))
                << 2;
            self->m2_decor_im[ps][pb][row][col] =
                (ixheaacd_mult32(delta, self->m2_decor_im[ps][pb][row][col]) +
                 ixheaacd_mult32(one_minus_delta,
                                 self->m2_decor_im[ps - 1][pb][row][col]))
                << 2;
            self->m2_resid_im[ps][pb][row][col] =
                (ixheaacd_mult32(delta, self->m2_resid_im[ps][pb][row][col]) +
                 ixheaacd_mult32(one_minus_delta,
                                 self->m2_resid_im[ps - 1][pb][row][col]))
                << 2;
          }
        }
      }
    }
  }
}

#define ONE_BY_128_IN_Q30 (8388608)
#define ONE_IN_Q30 (1073741824)
#define PI_IN_Q27 (421657440)
#define FIFTY_X_PI_BY_180_Q27 (117127067)
#define TWENTY_FIVE_X_PI_BY_180_Q27 (58563533)

VOID ixheaacd_mps_smoothing_opd(ia_mps_dec_state_struct *self) {
  int ps, pb;
  int delta, one_minus_delta;

  if (self->opd_smoothing_mode == 0) {
    for (pb = 0; pb < self->bs_param_bands; pb++) {
      self->opd_smooth.smooth_l_phase[pb] =
          self->phase_l_fix[self->num_parameter_sets - 1][pb] >> 1;
      self->opd_smooth.smooth_r_phase[pb] =
          self->phase_r_fix[self->num_parameter_sets - 1][pb] >> 1;
    }
    return;
  }
  for (ps = 0; ps < self->num_parameter_sets; ps++) {
    int thr = self->bs_frame.ipd_data.bs_quant_coarse_xxx[ps]
                  ? FIFTY_X_PI_BY_180_Q27
                  : TWENTY_FIVE_X_PI_BY_180_Q27;

    delta = self->param_slot_diff[ps] * ONE_BY_128_IN_Q30;
    one_minus_delta = ONE_IN_Q30 - delta;

    for (pb = 0; pb < self->bs_param_bands; pb++) {
      int ltemp, rtemp, tmp;

      ltemp = self->phase_l_fix[ps][pb] >> 1;
      rtemp = self->phase_r_fix[ps][pb] >> 1;

      while (ltemp > self->opd_smooth.smooth_l_phase[pb] + PI_IN_Q27)
        ltemp -= 2 * PI_IN_Q27;
      while (ltemp < self->opd_smooth.smooth_l_phase[pb] - PI_IN_Q27)
        ltemp += 2 * PI_IN_Q27;
      while (rtemp > self->opd_smooth.smooth_r_phase[pb] + PI_IN_Q27)
        rtemp -= 2 * PI_IN_Q27;
      while (rtemp < self->opd_smooth.smooth_r_phase[pb] - PI_IN_Q27)
        rtemp += 2 * PI_IN_Q27;

      self->opd_smooth.smooth_l_phase[pb] =
          (ixheaacd_mult32_shl(delta, ltemp) +
           ixheaacd_mult32_shl(one_minus_delta,
                               self->opd_smooth.smooth_l_phase[pb]))
          << 1;
      self->opd_smooth.smooth_r_phase[pb] =
          (ixheaacd_mult32_shl(delta, rtemp) +
           ixheaacd_mult32_shl(one_minus_delta,
                               self->opd_smooth.smooth_r_phase[pb]))
          << 1;

      tmp = (ltemp - rtemp) - (self->opd_smooth.smooth_l_phase[pb] -
                               self->opd_smooth.smooth_r_phase[pb]);
      while (tmp > PI_IN_Q27) tmp -= 2 * PI_IN_Q27;
      while (tmp < -PI_IN_Q27) tmp += 2 * PI_IN_Q27;

      if (ixheaacd_abs32(tmp) > thr) {
        self->opd_smooth.smooth_l_phase[pb] = ltemp;
        self->opd_smooth.smooth_r_phase[pb] = rtemp;
      }

      while (self->opd_smooth.smooth_l_phase[pb] > 2 * PI_IN_Q27)
        self->opd_smooth.smooth_l_phase[pb] -= 2 * PI_IN_Q27;
      while (self->opd_smooth.smooth_l_phase[pb] < 0)
        self->opd_smooth.smooth_l_phase[pb] += 2 * PI_IN_Q27;
      while (self->opd_smooth.smooth_r_phase[pb] > 2 * PI_IN_Q27)
        self->opd_smooth.smooth_r_phase[pb] -= 2 * PI_IN_Q27;
      while (self->opd_smooth.smooth_r_phase[pb] < 0)
        self->opd_smooth.smooth_r_phase[pb] += 2 * PI_IN_Q27;

      self->phase_l_fix[ps][pb] = self->opd_smooth.smooth_l_phase[pb] << 1;
      self->phase_r_fix[ps][pb] = self->opd_smooth.smooth_r_phase[pb] << 1;
    }
  }
}
