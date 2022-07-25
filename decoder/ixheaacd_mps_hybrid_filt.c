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
#include <math.h>
#include <string.h>
#include "ixheaacd_type_def.h"
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_config.h"

#include "ixheaacd_mps_polyphase.h"
#include "ixheaacd_mps_dec.h"
#include "ixheaacd_mps_interface.h"

#include "ixheaacd_mps_polyphase.h"
#include "ixheaacd_constants.h"
#include "ixheaacd_basic_ops32.h"

#include "ixheaacd_mps_hybfilter.h"

extern const FLOAT32 ixheaacd_ia_mps_hyb_filter_coeff_8[QMF_HYBRID_FILT_ORDER];
extern const FLOAT32 ixheaacd_mps_hyb_filter_coeff_2[QMF_HYBRID_FILT_ORDER];

extern const FLOAT32 ixheaacd_sine[8][8];
extern const FLOAT32 ixheaacd_cosine[8][8];

static VOID ixheaacd_mps_hyb_filt_type1(
    ia_cmplx_flt_struct *input, ia_cmplx_flt_struct output[8][MAX_TIME_SLOTS],
    WORD32 num_samples, const FLOAT32 *filt_coeff)

{
  WORD32 i, n, q;

  FLOAT32 in_re, in_im;
  FLOAT32 coeff;
  FLOAT32 acc_re_l, acc_re_h, acc_im_l, acc_im_h;

  for (i = 0; i < num_samples; i++) {
    FLOAT32 x0_re[13], x0_im[13], x0_1_re[8], x0_1_im[8];
    FLOAT32 acc_re_val[8], acc_im_val[8];
    for (n = 0; n < QMF_HYBRID_FILT_ORDER; n++)  // x0 = x[n]*Cf[n]
    {
      in_re = input[n + i].re;
      in_im = input[n + i].im;

      coeff = filt_coeff[QMF_HYBRID_FILT_ORDER - 1 - n];

      x0_re[n] = coeff * in_re;
      x0_im[n] = coeff * in_im;
    }

    // x0_2 series

    x0_1_re[0] = x0_re[6];
    x0_1_im[0] = x0_im[6];

    x0_1_re[1] = x0_re[7];
    x0_1_im[1] = x0_im[7];

    x0_1_re[2] = x0_re[8] - x0_re[0];
    x0_1_im[2] = x0_im[8] - x0_im[0];

    x0_1_re[3] = x0_re[9] - x0_re[1];
    x0_1_im[3] = x0_im[9] - x0_im[1];

    x0_1_re[4] = x0_re[10] - x0_re[2];
    x0_1_im[4] = x0_im[10] - x0_im[2];

    x0_1_re[5] = x0_re[11] - x0_re[3];
    x0_1_im[5] = x0_im[11] - x0_im[3];

    x0_1_re[6] = x0_re[12] - x0_re[4];
    x0_1_im[6] = x0_im[12] - x0_im[4];

    x0_1_re[7] = -(x0_re[5]);
    x0_1_im[7] = -(x0_im[5]);

    // acc_re_im_val
    acc_re_val[0] = x0_1_re[0];
    acc_re_val[1] = x0_1_re[1] - x0_1_re[7];
    acc_re_val[2] = x0_1_re[2] - x0_1_re[6];
    acc_re_val[3] = x0_1_re[3] - x0_1_re[5];
    acc_re_val[4] = x0_1_im[1] + x0_1_im[7];
    acc_re_val[5] = x0_1_im[2] + x0_1_im[6];
    acc_re_val[6] = x0_1_im[3] + x0_1_im[5];
    acc_re_val[7] = x0_1_im[4];

    acc_im_val[0] = x0_1_im[0];
    acc_im_val[1] = x0_1_im[1] - x0_1_im[7];
    acc_im_val[2] = x0_1_im[2] - x0_1_im[6];
    acc_im_val[3] = x0_1_im[3] - x0_1_im[5];
    acc_im_val[4] = x0_1_re[1] + x0_1_re[7];
    acc_im_val[5] = x0_1_re[2] + x0_1_re[6];
    acc_im_val[6] = x0_1_re[3] + x0_1_re[5];
    acc_im_val[7] = x0_1_re[4];

    for (q = 0; q < 4; q++) {
      acc_re_l = 0;
      acc_im_l = 0;
      acc_re_h = 0;
      acc_im_h = 0;

      // X_re
      acc_re_l += acc_re_val[0];
      acc_re_l += acc_re_val[1] * ixheaacd_cosine[q][1];
      acc_re_l += acc_re_val[2] * ixheaacd_cosine[q][2];
      acc_re_l += acc_re_val[3] * ixheaacd_cosine[q][3];

      acc_re_h = acc_re_l;

      acc_re_l -= acc_re_val[4] * ixheaacd_sine[q][1];
      acc_re_l -= acc_re_val[5] * ixheaacd_sine[q][2];
      acc_re_l -= acc_re_val[6] * ixheaacd_sine[q][3];
      acc_re_l -= acc_re_val[7] * ixheaacd_sine[q][4];

      acc_re_h = acc_re_h - (acc_re_l - acc_re_h);

      // X_im
      acc_im_l += acc_im_val[0];
      acc_im_l += acc_im_val[1] * ixheaacd_cosine[q][1];
      acc_im_l += acc_im_val[2] * ixheaacd_cosine[q][2];
      acc_im_l += acc_im_val[3] * ixheaacd_cosine[q][3];

      acc_im_h = acc_im_l;

      acc_im_l += acc_im_val[4] * ixheaacd_sine[q][1];
      acc_im_l += acc_im_val[5] * ixheaacd_sine[q][2];
      acc_im_l += acc_im_val[6] * ixheaacd_sine[q][3];
      acc_im_l += acc_im_val[7] * ixheaacd_sine[q][4];

      acc_im_h = acc_im_h - (acc_im_l - acc_im_h);

      output[q][i].re = acc_re_l;
      output[q][i].im = acc_im_l;

      output[7 - q][i].re = acc_re_h;
      output[7 - q][i].im = acc_im_h;
    }
  }
}

static VOID ixheaacd_mps_hyb_filt_type2(
    ia_cmplx_flt_struct *input, ia_cmplx_flt_struct output[2][MAX_TIME_SLOTS],
    WORD32 num_samples, const FLOAT32 *filt_coeff)

{
  WORD32 i, n;

  FLOAT32 in_re, in_im;
  FLOAT32 coeff;
  FLOAT32 acc_re[2], acc_im[2];

  for (i = 0; i < num_samples; i++) {
    FLOAT32 x_0_re[13], x_0_im[13];

    for (n = 1; n < 6; n = n + 2) {
      in_re = input[n + i].re;
      in_im = input[n + i].im;

      in_re += input[12 - n + i].re;
      in_im += input[12 - n + i].im;

      coeff = filt_coeff[QMF_HYBRID_FILT_ORDER - 1 - n];

      x_0_re[n] = coeff * in_re;
      x_0_im[n] = coeff * in_im;
    }

    n = 6;
    in_re = input[n + i].re;
    in_im = input[n + i].im;

    coeff = filt_coeff[QMF_HYBRID_FILT_ORDER - 1 - n];

    x_0_re[n] = coeff * in_re;
    x_0_im[n] = coeff * in_im;

    x_0_re[1] = x_0_re[1] + x_0_re[3] + x_0_re[5];
    x_0_im[1] = x_0_im[1] + x_0_im[3] + x_0_im[5];

    acc_re[0] = x_0_re[6] + x_0_re[1];
    acc_im[0] = x_0_im[6] + x_0_im[1];

    acc_re[1] = x_0_re[6] - x_0_re[1];
    acc_im[1] = x_0_im[6] - x_0_im[1];

    output[0][i].re = acc_re[0];
    output[0][i].im = acc_im[0];

    output[1][i].re = acc_re[1];
    output[1][i].im = acc_im[1];
  }
}

VOID ixheaacd_mps_qmf_hybrid_analysis_init(ia_mps_hybrid_filt_struct *handle) {
  memset(handle->lf_buffer, 0,
         QMF_BANDS_TO_HYBRID * BUFFER_LEN_LF_MPS * sizeof(ia_cmplx_w32_struct));
  memset(handle->hf_buffer, 0, MAX_NUM_QMF_BANDS_MPS * BUFFER_LEN_HF_MPS *
                                   sizeof(ia_cmplx_flt_struct));
}

VOID ixheaacd_mps_qmf_hybrid_analysis_no_pre_mix(
    ia_mps_hybrid_filt_struct *handle,
    ia_cmplx_flt_struct in_qmf[MAX_NUM_QMF_BANDS_MPS_NEW][MAX_TIME_SLOTS],
    WORD32 num_bands, WORD32 num_samples,
    ia_cmplx_flt_struct v[MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS]) {
  WORD32 lf_samples_shift;
  WORD32 hf_samples_shift;
  WORD32 lf_qmf_bands;
  WORD32 k, n;

  ia_cmplx_flt_struct scratch[MAX_HYBRID_ONLY_BANDS_PER_QMF][MAX_TIME_SLOTS];

  lf_samples_shift = BUFFER_LEN_LF_MPS - num_samples;
  hf_samples_shift = BUFFER_LEN_HF_MPS - num_samples;

  lf_qmf_bands = QMF_BANDS_TO_HYBRID;

  for (k = 0; k < lf_qmf_bands; k++) {
    memmove(&handle->lf_buffer[k][0].re, &handle->lf_buffer[k][num_samples].re,
            2 * lf_samples_shift * sizeof(FLOAT32));
  }

  for (k = 0; k < lf_qmf_bands; k++) {
    memcpy(&handle->lf_buffer[k][lf_samples_shift].re, &in_qmf[k][0].re,
           2 * num_samples * sizeof(FLOAT32));
  }

  for (k = 0; k < MAX_NUM_QMF_BANDS_SAC / 2 - lf_qmf_bands; k++) {
    memmove(&handle->hf_buffer[k][0].re, &handle->hf_buffer[k][num_samples].re,
            2 * hf_samples_shift * sizeof(FLOAT32));
  }

  for (k = 0; k < num_bands - lf_qmf_bands; k++) {
    memcpy(&handle->hf_buffer[k][hf_samples_shift].re,
           &in_qmf[k + lf_qmf_bands][0].re, 2 * num_samples * sizeof(FLOAT32));
  }

  ixheaacd_mps_hyb_filt_type1(
      &(handle->lf_buffer[0][lf_samples_shift + 1 - QMF_HYBRID_FILT_ORDER]),
      scratch, num_samples, ixheaacd_ia_mps_hyb_filter_coeff_8);

  for (k = 0; k < 2; k++) {
    for (n = 0; n < num_samples; n++) {
      v[n][k].re = scratch[k + 6][n].re;
      v[n][k + 2].re = scratch[k][n].re;
      v[n][k + 4].re = scratch[k + 2][n].re;
      v[n][k + 4].re += scratch[5 - k][n].re;

      v[n][k].im = scratch[k + 6][n].im;
      v[n][k + 2].im = scratch[k][n].im;
      v[n][k + 4].im = scratch[k + 2][n].im;
      v[n][k + 4].im += scratch[5 - k][n].im;
    }
  }

  ixheaacd_mps_hyb_filt_type2(
      &(handle->lf_buffer[1][lf_samples_shift + 1 - QMF_HYBRID_FILT_ORDER]),
      scratch, num_samples, ixheaacd_mps_hyb_filter_coeff_2);

  for (k = 0; k < 2; k++) {
    for (n = 0; n < num_samples; n++) {
      v[n][k + 6].re = scratch[1 - k][n].re;
      v[n][k + 6].im = scratch[1 - k][n].im;
    }
  }

  ixheaacd_mps_hyb_filt_type2(
      &(handle->lf_buffer[2][lf_samples_shift + 1 - QMF_HYBRID_FILT_ORDER]),
      scratch, num_samples, ixheaacd_mps_hyb_filter_coeff_2);

  for (k = 0; k < 2; k++) {
    for (n = 0; n < num_samples; n++) {
      v[n][k + 8].re = scratch[k][n].re;
      v[n][k + 8].im = scratch[k][n].im;
    }
  }

  for (k = 0; k < num_bands - lf_qmf_bands; k++) {
    for (n = 0; n < num_samples; n++) {
      v[n][k + 10].re = (handle->hf_buffer[k][n + hf_samples_shift].re);
      v[n][k + 10].im = (handle->hf_buffer[k][n + hf_samples_shift].im);
    }
  }
}

VOID ixheaacd_mps_qmf_hybrid_analysis(
    ia_mps_hybrid_filt_struct *handle,
    ia_cmplx_flt_struct in_qmf[MAX_NUM_QMF_BANDS_MPS_NEW][MAX_TIME_SLOTS],
    WORD32 num_bands, WORD32 num_samples,
    ia_cmplx_flt_struct hyb[MAX_HYBRID_BANDS_MPS][MAX_TIME_SLOTS]) {
  WORD32 lf_samples_shift;
  WORD32 hf_samples_shift;
  WORD32 lf_qmf_bands;
  WORD32 k, n;

  ia_cmplx_flt_struct scratch[MAX_HYBRID_ONLY_BANDS_PER_QMF][MAX_TIME_SLOTS];

  lf_samples_shift = BUFFER_LEN_LF_MPS - num_samples;
  hf_samples_shift = BUFFER_LEN_HF_MPS - num_samples;

  lf_qmf_bands = QMF_BANDS_TO_HYBRID;

  for (k = 0; k < lf_qmf_bands; k++) {
    memmove(&handle->lf_buffer[k][0].re, &handle->lf_buffer[k][num_samples].re,
            2 * lf_samples_shift * sizeof(FLOAT32));
  }

  for (k = 0; k < lf_qmf_bands; k++) {
    memcpy(&handle->lf_buffer[k][lf_samples_shift].re, &in_qmf[k][0].re,
           2 * num_samples * sizeof(FLOAT32));
  }

  for (k = 0; k < MAX_NUM_QMF_BANDS_SAC / 2 - lf_qmf_bands; k++) {
    memmove(&handle->hf_buffer[k][0].re, &handle->hf_buffer[k][num_samples].re,
            2 * hf_samples_shift * sizeof(FLOAT32));
  }

  for (k = 0; k < num_bands - lf_qmf_bands; k++) {
    memcpy(&handle->hf_buffer[k][hf_samples_shift].re,
           &in_qmf[k + lf_qmf_bands][0].re, 2 * num_samples * sizeof(FLOAT32));
  }

  ixheaacd_mps_hyb_filt_type1(
      &(handle->lf_buffer[0][lf_samples_shift + 1 - QMF_HYBRID_FILT_ORDER]),
      scratch, num_samples, ixheaacd_ia_mps_hyb_filter_coeff_8);

  for (k = 0; k < 2; k++) {
    for (n = 0; n < num_samples; n++) {
      hyb[k][n].re = scratch[k + 6][n].re;
      hyb[k + 2][n].re = scratch[k][n].re;
      hyb[k + 4][n].re = scratch[k + 2][n].re;
      hyb[k + 4][n].re += scratch[5 - k][n].re;

      hyb[k][n].im = scratch[k + 6][n].im;
      hyb[k + 2][n].im = scratch[k][n].im;
      hyb[k + 4][n].im = scratch[k + 2][n].im;
      hyb[k + 4][n].im += scratch[5 - k][n].im;
    }
  }

  ixheaacd_mps_hyb_filt_type2(
      &(handle->lf_buffer[1][lf_samples_shift + 1 - QMF_HYBRID_FILT_ORDER]),
      scratch, num_samples, ixheaacd_mps_hyb_filter_coeff_2);

  for (k = 0; k < 2; k++) {
    for (n = 0; n < num_samples; n++) {
      hyb[k + 6][n].re = scratch[1 - k][n].re;
      hyb[k + 6][n].im = scratch[1 - k][n].im;
    }
  }

  ixheaacd_mps_hyb_filt_type2(
      &(handle->lf_buffer[2][lf_samples_shift + 1 - QMF_HYBRID_FILT_ORDER]),
      scratch, num_samples, ixheaacd_mps_hyb_filter_coeff_2);

  for (k = 0; k < 2; k++) {
    for (n = 0; n < num_samples; n++) {
      hyb[k + 8][n].re = scratch[k][n].re;
      hyb[k + 8][n].im = scratch[k][n].im;
    }
  }

  for (k = 0; k < num_bands - lf_qmf_bands; k++) {
    memcpy(&hyb[k + 10][0].re, &handle->hf_buffer[k][hf_samples_shift].re,
           2 * num_samples * sizeof(FLOAT32));
  }
}

VOID ixheaacd_mps_qmf_hybrid_synthesis(
    ia_cmplx_flt_struct hyb[MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS],
    WORD32 num_bands, WORD32 num_samples,
    ia_cmplx_flt_struct in_qmf[MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS_MPS]) {
  WORD32 k, n;

  for (n = 0; n < num_samples; n++) {
    in_qmf[n][0].re = hyb[n][0].re;
    in_qmf[n][0].im = hyb[n][0].im;

    for (k = 1; k < 6; k++) {
      in_qmf[n][0].re += hyb[n][k].re;
      in_qmf[n][0].im += hyb[n][k].im;
    }

    in_qmf[n][1].re = hyb[n][6].re + hyb[n][7].re;
    in_qmf[n][1].im = hyb[n][6].im + hyb[n][7].im;

    in_qmf[n][2].re = hyb[n][8].re + hyb[n][9].re;
    in_qmf[n][2].im = hyb[n][8].im + hyb[n][9].im;

    memcpy(&in_qmf[n][3].re, &hyb[n][10].re,
           2 * (num_bands - 3) * sizeof(FLOAT32));
  }
}
