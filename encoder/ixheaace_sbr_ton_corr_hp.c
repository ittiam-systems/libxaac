/******************************************************************************
 *                                                                            *
 * Copyright (C) 2023 The Android Open Source Project
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

#include <string.h>

#include "ixheaac_type_def.h"
#include "ixheaac_constants.h"
#include "ixheaace_aac_constants.h"
#include "ixheaac_basic_ops32.h"
#include "ixheaac_basic_ops16.h"
#include "ixheaac_basic_ops40.h"
#include "ixheaac_basic_ops.h"

#include "ixheaace_sbr_header.h"
#include "ixheaace_sbr_def.h"
#include "ixheaace_resampler.h"
#include "ixheaace_sbr_rom.h"
#include "ixheaace_common_rom.h"
#include "ixheaace_sbr_qmf_enc.h"
#include "ixheaace_sbr_tran_det.h"
#include "ixheaace_sbr_frame_info_gen.h"
#include "ixheaace_sbr_env_est.h"
#include "ixheaace_sbr_code_envelope.h"
#include "ixheaace_sbr_hbe.h"
#include "ixheaace_sbr_main.h"
#include "ixheaace_sbr_missing_harmonics_det.h"
#include "ixheaace_sbr_inv_filtering_estimation.h"
#include "ixheaace_sbr_noise_floor_est.h"
#include "ixheaace_sbr_ton_corr.h"
#include "ixheaace_sbr.h"

#include "ixheaace_sbr_misc.h"
#include <math.h>

static VOID ixheaace_calc_auto_corr_second_order(ixheaace_acorr_coeffs *pstr_ac,
                                                 FLOAT32 **ptr_real, FLOAT32 **ptr_imag,
                                                 WORD32 bd, WORD32 len, WORD32 is_ld_sbr) {
  WORD32 j, jminus1, jminus2;
  float rel = 1.0f / (1.0f + RELAXATION);

  memset(pstr_ac, 0, sizeof(ixheaace_acorr_coeffs));
  if (is_ld_sbr) {
    FLOAT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;

    tmp1 = tmp3 = tmp5 = tmp7 = tmp8 = 0.0f;

    tmp7 += ptr_real[0][bd] * ptr_real[-2][bd] + ptr_imag[0][bd] * ptr_imag[-2][bd];

    tmp8 += ptr_real[-2][bd] * ptr_imag[0][bd] - ptr_real[0][bd] * ptr_imag[-2][bd];

    for (j = 0; j < 5; j++) {
      jminus1 = j - 1;

      tmp1 += ptr_real[jminus1][bd] * ptr_real[jminus1][bd] +
              ptr_imag[jminus1][bd] * ptr_imag[jminus1][bd];

      tmp3 += ptr_real[jminus1][bd] * ptr_real[j][bd] + ptr_imag[jminus1][bd] * ptr_imag[j][bd];

      tmp5 += ptr_imag[j][bd] * ptr_real[jminus1][bd] - ptr_real[j][bd] * ptr_imag[jminus1][bd];

      tmp7 += ptr_real[j + 1][bd] * ptr_real[jminus1][bd] +
              ptr_imag[j + 1][bd] * ptr_imag[jminus1][bd];

      tmp8 += ptr_imag[j + 1][bd] * ptr_real[jminus1][bd] -
              ptr_real[j + 1][bd] * ptr_imag[jminus1][bd];
    }

    tmp2 = ptr_real[-2][bd] * ptr_real[-2][bd] + ptr_imag[-2][bd] * ptr_imag[-2][bd];
    tmp2 += tmp1;

    tmp1 += ptr_real[4][bd] * ptr_real[4][bd] + ptr_imag[4][bd] * ptr_imag[4][bd];

    tmp0 = ptr_real[5][bd] * ptr_real[5][bd] + ptr_imag[5][bd] * ptr_imag[5][bd];
    tmp0 -= (ptr_real[-1][bd] * ptr_real[-1][bd] + ptr_imag[-1][bd] * ptr_imag[-1][bd]);
    tmp0 += tmp1;

    tmp4 = ptr_real[-1][bd] * ptr_real[-2][bd] + ptr_imag[-1][bd] * ptr_imag[-2][bd];
    tmp4 += tmp3;

    tmp3 += ptr_real[5][bd] * ptr_real[4][bd] + ptr_imag[5][bd] * ptr_imag[4][bd];

    tmp6 = ptr_imag[-1][bd] * ptr_real[-2][bd] - ptr_real[-1][bd] * ptr_imag[-2][bd];
    tmp6 += tmp5;

    tmp5 += ptr_imag[5][bd] * ptr_real[4][bd] - ptr_real[5][bd] * ptr_imag[4][bd];

    pstr_ac->r00r = tmp0 / 2;
    pstr_ac->r11r = tmp1 / 2;
    pstr_ac->r22r = tmp2 / 2;
    pstr_ac->r01r = tmp3 / 2;
    pstr_ac->r12r = tmp4 / 2;
    pstr_ac->r01i = tmp5 / 2;
    pstr_ac->r12i = tmp6 / 2;
    pstr_ac->r02r = tmp7 / 2;
    pstr_ac->r02i = tmp8 / 2;

    pstr_ac->det = ((pstr_ac->r11r * pstr_ac->r22r) - (pstr_ac->r12r * pstr_ac->r12r) -
                    (pstr_ac->r12i * pstr_ac->r12i)) /
                   2;
  } else {
    for (j = 0; j < (len - 2 - 1); j++) {
      jminus1 = j - 1;
      jminus2 = jminus1 - 1;

      pstr_ac->r00r += ptr_real[j][bd] * ptr_real[j][bd] + ptr_imag[j][bd] * ptr_imag[j][bd];

      pstr_ac->r11r += ptr_real[jminus1][bd] * ptr_real[jminus1][bd] +
                       ptr_imag[jminus1][bd] * ptr_imag[jminus1][bd];

      pstr_ac->r01r +=
          ptr_real[j][bd] * ptr_real[jminus1][bd] + ptr_imag[j][bd] * ptr_imag[jminus1][bd];

      pstr_ac->r01i +=
          ptr_imag[j][bd] * ptr_real[jminus1][bd] - ptr_real[j][bd] * ptr_imag[jminus1][bd];

      pstr_ac->r02r +=
          ptr_real[j][bd] * ptr_real[jminus2][bd] + ptr_imag[j][bd] * ptr_imag[jminus2][bd];

      pstr_ac->r02i +=
          ptr_imag[j][bd] * ptr_real[jminus2][bd] - ptr_real[j][bd] * ptr_imag[jminus2][bd];
    }

    pstr_ac->r22r =
        pstr_ac->r11r + ptr_real[-2][bd] * ptr_real[-2][bd] + ptr_imag[-2][bd] * ptr_imag[-2][bd];

    pstr_ac->r12r =
        pstr_ac->r01r + ptr_real[-1][bd] * ptr_real[-2][bd] + ptr_imag[-1][bd] * ptr_imag[-2][bd];

    pstr_ac->r12i =
        pstr_ac->r01i + ptr_imag[-1][bd] * ptr_real[-2][bd] - ptr_real[-1][bd] * ptr_imag[-2][bd];

    jminus1 = j - 1;
    jminus2 = jminus1 - 1;

    pstr_ac->r00r += ptr_real[j][bd] * ptr_real[j][bd] + ptr_imag[j][bd] * ptr_imag[j][bd];

    pstr_ac->r11r += ptr_real[jminus1][bd] * ptr_real[jminus1][bd] +
                     ptr_imag[jminus1][bd] * ptr_imag[jminus1][bd];

    pstr_ac->r01r +=
        ptr_real[j][bd] * ptr_real[jminus1][bd] + ptr_imag[j][bd] * ptr_imag[jminus1][bd];

    pstr_ac->r01i +=
        ptr_imag[j][bd] * ptr_real[jminus1][bd] - ptr_real[j][bd] * ptr_imag[jminus1][bd];

    pstr_ac->r02r +=
        ptr_real[j][bd] * ptr_real[jminus2][bd] + ptr_imag[j][bd] * ptr_imag[jminus2][bd];

    pstr_ac->r02i +=
        ptr_imag[j][bd] * ptr_real[jminus2][bd] - ptr_real[j][bd] * ptr_imag[jminus2][bd];

    pstr_ac->det = pstr_ac->r11r * pstr_ac->r22r -
                   rel * (pstr_ac->r12r * pstr_ac->r12r + pstr_ac->r12i * pstr_ac->r12i);
  }
}

VOID ixheaace_calculate_tonality_quotas(ixheaace_pstr_sbr_ton_corr_est pstr_ton_corr,
                                        FLOAT32 **ptr_real, FLOAT32 **ptr_imag, WORD32 usb,
                                        WORD32 num_time_slots, WORD32 is_ld_sbr) {
  WORD32 i, k, r, time_index;
  FLOAT32 alphar[2], alphai[2], r01r, r02r, r11r, r12r, r01i, r02i, r12i, det, r00r;
  ixheaace_acorr_coeffs ac;
  FLOAT32 *ptr_energy_vec = pstr_ton_corr->energy_vec;
  FLOAT32 **ptr_quota_mtx = pstr_ton_corr->ptr_quota_mtx;

  WORD32 start_index_matrix = pstr_ton_corr->start_index_matrix;
  WORD32 tot_no_est = pstr_ton_corr->est_cnt;
  WORD32 no_est_per_frame = pstr_ton_corr->est_cnt_per_frame;
  WORD32 move = pstr_ton_corr->move;
  WORD32 num_qmf_ch = pstr_ton_corr->num_qmf_ch;
  WORD32 len = num_time_slots;
  WORD32 qm_len;
  WORD32 time_step = 0;
  WORD32 idx_val = 0;
  for (i = 0; i < move; i++) {
    memcpy(ptr_quota_mtx[i], ptr_quota_mtx[i + no_est_per_frame],
           num_qmf_ch * sizeof(ptr_quota_mtx[i][0]));
  }

  memmove(ptr_energy_vec, ptr_energy_vec + no_est_per_frame, move * sizeof(ptr_energy_vec[0]));
  memset(ptr_energy_vec + start_index_matrix, 0,
         (tot_no_est - start_index_matrix) * sizeof(ptr_energy_vec[0]));
  if (is_ld_sbr) {
    qm_len = 2 + pstr_ton_corr->buffer_offset;
    time_step = pstr_ton_corr->buffer_offset;
    len = pstr_ton_corr->buffer_offset - 2;
  } else {
    qm_len = 18;
  }

  for (r = 0; r < usb; r++) {
    k = 2;
    time_index = start_index_matrix;

    if (is_ld_sbr) {
      idx_val = 0;
      while (k <= qm_len) {
        ixheaace_calc_auto_corr_second_order(&ac, &ptr_real[k], &ptr_imag[k], r, len, is_ld_sbr);

        r00r = ac.r00r;
        r11r = ac.r11r;
        r12r = ac.r12r;
        r12i = ac.r12i;
        r01r = ac.r01r;
        r01i = ac.r01i;
        r02r = ac.r02r;
        r02i = ac.r02i;
        det = ac.det;

        FLOAT32 fac = 0.0f;
        if (det == 0) {
          alphar[1] = alphai[1] = 0.0f;
          alphar[0] = r01r / 4.0f;
          alphai[0] = r01i / 4.0f;
        } else {
          alphar[1] = ((r01r * r12r) / 2 - (r01i * r12i) / 2 - (r02r * r11r) / 2);
          alphai[1] = ((r01i * r12r) / 2 + (r01r * r12i) / 2 - (r02i * r11r) / 2);

          alphar[0] = ((r01r * det) / 2 + (alphar[1] * r12r / 2) + (alphai[1] * r12i / 2));
          alphai[0] = ((r01i * det) / 2 + (alphai[1] * r12r / 2) - (alphar[1] * r12i / 2));

          fac = ((det * r11r) * r00r);
        }

        if (fac == 0.0f) {
          ptr_quota_mtx[time_index][r] = 0.0f;
        } else {
          FLOAT32 temp1 = 0, temp2 = 0;
          temp1 = ((alphar[0] * r01r)) + ((alphai[0] * r01i)) - ((alphar[1] * r02r * r11r) / 2) -
                  ((alphai[1] * r02i * r11r) / 2);
          temp1 = (FLOAT32)(fabs(temp1));
          temp2 =
              ((fac * 0.524288f) / (IXHEAACE_2_POWER_10 * IXHEAACE_2_POWER_10)) + fac / 2 - temp1;
          temp2 = (FLOAT32)fabs(temp2);
          temp1 = temp1 * 0.5242879986f;
          ptr_quota_mtx[time_index][r] =
              (temp1 / temp2) / (IXHEAACE_2_POWER_10 * IXHEAACE_2_POWER_9);
        }
        ptr_energy_vec[time_index] += (r00r) / IXHEAACE_2_POWER_14;

        k += time_step;
        time_index++;
        idx_val += time_step;
      }
    } else {
      while (k <= qm_len) {
        ixheaace_calc_auto_corr_second_order(&ac, &ptr_real[k], &ptr_imag[k], r, len, is_ld_sbr);

        r00r = ac.r00r;
        r11r = ac.r11r;
        r12r = ac.r12r;
        r12i = ac.r12i;
        r01r = ac.r01r;
        r01i = ac.r01i;
        r02r = ac.r02r;
        r02i = ac.r02i;
        det = ac.det;

        if (det == 0) {
          alphar[1] = alphai[1] = 0;
        } else {
          alphar[1] = (r01r * r12r - r01i * r12i - r02r * r11r) / det;
          alphai[1] = (r01i * r12r + r01r * r12i - r02i * r11r) / det;
        }

        if (r11r == 0) {
          alphar[0] = alphai[0] = 0;
        } else {
          alphar[0] = -(r01r + alphar[1] * r12r + alphai[1] * r12i) / r11r;
          alphai[0] = -(r01i + alphai[1] * r12r - alphar[1] * r12i) / r11r;
        }
        if (r00r) {
          FLOAT32 tmp =
              -(alphar[0] * r01r + alphai[0] * r01i + alphar[1] * r02r + alphai[1] * r02i) /
              (r00r);

          ptr_quota_mtx[time_index][r] = tmp / ((FLOAT32)1.0 - tmp);
        } else {
          ptr_quota_mtx[time_index][r] = 0;
        }
        ptr_energy_vec[time_index] += r00r;

        k += is_ld_sbr ? 8 : 16;

        time_index++;
      }
    }
  }
}
