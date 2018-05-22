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
#include <ixheaacd_type_def.h>
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_config.h"

#include "ixheaacd_mps_polyphase.h"
#include "ixheaacd_mps_dec.h"
#include "ixheaacd_mps_interface.h"

#include "ixheaacd_mps_polyphase.h"

#include "ixheaacd_mps_hybfilter.h"

extern WORD32 ixheaacd_ia_mps_hyb_filter_coeff_8[QMF_HYBRID_FILT_ORDER];
extern WORD32 ixheaacd_mps_hyb_filter_coeff_2[QMF_HYBRID_FILT_ORDER];
extern WORD32 ixheaacd_cosine[8][13];
extern WORD32 ixheaacd_sine[8][13];
extern WORD32 ixheaacd_cosine2[2][13];

static WORD32 ixheaacd_mps_mult32_local(WORD32 a, WORD32 b, WORD16 shift) {
  WORD64 temp;

  temp = (WORD64)a * (WORD64)b;
  temp = temp >> shift;
  return (WORD32)temp;
}

static VOID ixheaacd_mps_hyb_filt_type1(
    ia_cmplx_w32_struct *input, ia_cmplx_w32_struct output[8][MAX_TIME_SLOTS],
    WORD32 num_samples, WORD32 *filt_coeff)

{
  WORD32 i, n, q;

  WORD32 modulation_fac_re, modulation_fac_im;
  WORD32 in_re, in_im;
  WORD32 temp;
  WORD32 coeff;
  WORD64 acc_re, acc_im;

  WORD16 shift = 8;

  for (i = 0; i < num_samples; i++) {
    for (q = 0; q < 8; q++) {
      acc_re = 0;
      acc_im = 0;
      for (n = 0; n < QMF_HYBRID_FILT_ORDER; n++) {
        modulation_fac_re = ixheaacd_cosine[q][n];
        modulation_fac_im = ixheaacd_sine[q][n];

        in_re = (WORD32)(input[n + i].re);
        in_im = (WORD32)(input[n + i].im);

        in_re = in_re << shift;
        in_im = in_im << shift;

        coeff = filt_coeff[QMF_HYBRID_FILT_ORDER - 1 - n];

        temp = ixheaacd_mps_mult32_local(in_re, modulation_fac_re, 30) -
               ixheaacd_mps_mult32_local(in_im, modulation_fac_im, 30);

        if (temp >= 1073741823)
          temp = 1073741823;
        else if (temp <= -1073741824)
          temp = -1073741824;

        temp = ixheaacd_mps_mult32_local(coeff, temp, 30);
        acc_re = acc_re + (WORD64)temp;

        temp = ixheaacd_mps_mult32_local(in_im, modulation_fac_re, 30) +
               ixheaacd_mps_mult32_local(in_re, modulation_fac_im, 30);

        if (temp >= 1073741823)
          temp = 1073741823;
        else if (temp <= -1073741824)
          temp = -1073741824;

        temp = ixheaacd_mps_mult32_local(coeff, temp, 30);
        acc_im = acc_im + (WORD64)temp;
      }

      output[q][i].re = (WORD32)(acc_re >> shift);
      output[q][i].im = (WORD32)(acc_im >> shift);
    }
  }
}

static VOID ixheaacd_mps_hyb_filt_type2(
    ia_cmplx_w32_struct *input, ia_cmplx_w32_struct output[2][MAX_TIME_SLOTS],
    WORD32 num_samples, WORD32 *filt_coeff)

{
  WORD32 i, n, q;

  WORD32 modulation_fac_re;
  WORD32 in_re, in_im;
  WORD32 temp;
  WORD32 coeff;
  WORD64 acc_re, acc_im;

  WORD16 shift = 8;

  for (i = 0; i < num_samples; i++) {
    for (q = 0; q < 2; q++) {
      acc_re = 0;
      acc_im = 0;
      for (n = 0; n < QMF_HYBRID_FILT_ORDER; n++) {
        modulation_fac_re = ixheaacd_cosine2[q][n];

        in_re = (WORD32)(input[n + i].re);
        in_im = (WORD32)(input[n + i].im);

        in_re = in_re << shift;
        in_im = in_im << shift;

        coeff = filt_coeff[QMF_HYBRID_FILT_ORDER - 1 - n];

        temp = ixheaacd_mps_mult32_local(in_re, modulation_fac_re, 30);

        if (temp >= 1073741823)
          temp = 1073741823;
        else if (temp <= -1073741824)
          temp = -1073741824;

        temp = ixheaacd_mps_mult32_local(coeff, temp, 30);
        acc_re = acc_re + (WORD64)temp;

        temp = ixheaacd_mps_mult32_local(in_im, modulation_fac_re, 30);

        if (temp >= 1073741823)
          temp = 1073741823;
        else if (temp <= -1073741824)
          temp = -1073741824;

        temp = ixheaacd_mps_mult32_local(coeff, temp, 30);
        acc_im = acc_im + (WORD64)temp;
      }

      output[q][i].re = (WORD32)(acc_re >> shift);
      output[q][i].im = (WORD32)(acc_im >> shift);
    }
  }
}

VOID ixheaacd_mps_qmf_hybrid_analysis_init(ia_mps_hybrid_filt_struct *handle) {
  memset(handle->lf_buffer, 0,
         QMF_BANDS_TO_HYBRID * BUFFER_LEN_LF_MPS * sizeof(ia_cmplx_w32_struct));
  memset(handle->hf_buffer, 0, MAX_NUM_QMF_BANDS_MPS * BUFFER_LEN_HF_MPS *
                                   sizeof(ia_cmplx_flt_struct));
}

VOID ixheaacd_mps_qmf_hybrid_analysis(
    ia_mps_hybrid_filt_struct *handle,
    ia_cmplx_flt_struct in_qmf[MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS_MPS_NEW],
    WORD32 num_bands, WORD32 num_samples,
    ia_cmplx_flt_struct hyb[MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS]) {
  WORD32 lf_samples_shift;
  WORD32 hf_samples_shift;
  WORD32 lf_qmf_bands;
  WORD32 k, n;

  ia_cmplx_w32_struct scratch[MAX_HYBRID_ONLY_BANDS_PER_QMF][MAX_TIME_SLOTS];

  lf_samples_shift = BUFFER_LEN_LF_MPS - num_samples;
  hf_samples_shift = BUFFER_LEN_HF_MPS - num_samples;

  lf_qmf_bands = QMF_BANDS_TO_HYBRID;

  for (k = 0; k < lf_qmf_bands; k++) {
    for (n = 0; n < lf_samples_shift; n++) {
      handle->lf_buffer[k][n].re = handle->lf_buffer[k][n + num_samples].re;
      handle->lf_buffer[k][n].im = handle->lf_buffer[k][n + num_samples].im;
    }
  }

  for (k = 0; k < lf_qmf_bands; k++) {
    for (n = 0; n < num_samples; n++) {
      handle->lf_buffer[k][n + lf_samples_shift].re = (WORD32)(in_qmf[n][k].re);
      handle->lf_buffer[k][n + lf_samples_shift].im = (WORD32)(in_qmf[n][k].im);
    }
  }

  for (k = 0; k < num_bands - lf_qmf_bands; k++) {
    for (n = 0; n < hf_samples_shift; n++) {
      handle->hf_buffer[k][n].re = handle->hf_buffer[k][n + num_samples].re;
      handle->hf_buffer[k][n].im = handle->hf_buffer[k][n + num_samples].im;
    }
  }

  for (k = 0; k < num_bands - lf_qmf_bands; k++) {
    for (n = 0; n < num_samples; n++) {
      handle->hf_buffer[k][n + hf_samples_shift].re =
          (in_qmf[n][k + lf_qmf_bands].re);
      handle->hf_buffer[k][n + hf_samples_shift].im =
          (in_qmf[n][k + lf_qmf_bands].im);
    }
  }

  ixheaacd_mps_hyb_filt_type1(
      &(handle->lf_buffer[0][lf_samples_shift + 1 - QMF_HYBRID_FILT_ORDER]),
      scratch, num_samples, ixheaacd_ia_mps_hyb_filter_coeff_8);

  for (k = 0; k < 2; k++) {
    for (n = 0; n < num_samples; n++) {
      hyb[n][k].re = (FLOAT32)scratch[k + 6][n].re;
      hyb[n][k + 2].re = (FLOAT32)scratch[k][n].re;
      hyb[n][k + 4].re = (FLOAT32)scratch[k + 2][n].re;
      hyb[n][k + 4].re += (FLOAT32)scratch[5 - k][n].re;

      hyb[n][k].im = (FLOAT32)scratch[k + 6][n].im;
      hyb[n][k + 2].im = (FLOAT32)scratch[k][n].im;
      hyb[n][k + 4].im = (FLOAT32)scratch[k + 2][n].im;
      hyb[n][k + 4].im += (FLOAT32)scratch[5 - k][n].im;
    }
  }

  ixheaacd_mps_hyb_filt_type2(
      &(handle->lf_buffer[1][lf_samples_shift + 1 - QMF_HYBRID_FILT_ORDER]),
      scratch, num_samples, ixheaacd_mps_hyb_filter_coeff_2);

  for (k = 0; k < 2; k++) {
    for (n = 0; n < num_samples; n++) {
      hyb[n][k + 6].re = (FLOAT32)scratch[1 - k][n].re;
      hyb[n][k + 6].im = (FLOAT32)scratch[1 - k][n].im;
    }
  }

  ixheaacd_mps_hyb_filt_type2(
      &(handle->lf_buffer[2][lf_samples_shift + 1 - QMF_HYBRID_FILT_ORDER]),
      scratch, num_samples, ixheaacd_mps_hyb_filter_coeff_2);

  for (k = 0; k < 2; k++) {
    for (n = 0; n < num_samples; n++) {
      hyb[n][k + 8].re = (FLOAT32)scratch[k][n].re;
      hyb[n][k + 8].im = (FLOAT32)scratch[k][n].im;
    }
  }

  for (k = 0; k < num_bands - lf_qmf_bands; k++) {
    for (n = 0; n < num_samples; n++) {
      hyb[n][k + 10].re = (handle->hf_buffer[k][n + hf_samples_shift].re);
      hyb[n][k + 10].im = (handle->hf_buffer[k][n + hf_samples_shift].im);
    }
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

    for (k = 3; k < num_bands; k++) {
      in_qmf[n][k].re = hyb[n][k - 3 + 10].re;
      in_qmf[n][k].im = hyb[n][k - 3 + 10].im;
    }
  }
}
