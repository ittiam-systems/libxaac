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
#include <string.h>
#include "ixheaacd_sbr_common.h"
#include <ixheaacd_type_def.h>

#include "ixheaacd_constants.h"
#include "ixheaacd_basic_ops32.h"
#include "ixheaacd_basic_ops16.h"
#include "ixheaacd_basic_ops40.h"
#include "ixheaacd_basic_ops.h"

#include "ixheaacd_intrinsics.h"
#include "ixheaacd_common_rom.h"
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_sbrdecsettings.h"
#include "ixheaacd_sbr_scale.h"
#include "ixheaacd_lpp_tran.h"
#include "ixheaacd_env_extr_part.h"
#include "ixheaacd_sbr_rom.h"
#include "ixheaacd_hybrid.h"
#include "ixheaacd_ps_dec.h"
#include "ixheaacd_env_extr.h"
#include "ixheaacd_qmf_dec.h"

#include <ixheaacd_basic_op.h>
#include "ixheaacd_env_calc.h"

#include "ixheaacd_interface.h"
#include "ixheaacd_function_selector.h"
#include "ixheaacd_audioobjtypes.h"

#define mult16x16_16(a, b) ixheaacd_mult16((a), (b))
#define mac16x16(a, b, c) ixheaacd_mac16x16in32((a), (b), (c))
#define mpy_32x16(a, b) fixmuldiv2_32x16b((a), (b))
#define mpy_16x16(a, b) ixheaacd_mult16x16in32((a), (b))
#define mpy_32x32(a, b) ixheaacd_mult32((a), (b))
#define mpy_32x16H_n(a, b) ixheaacd_mult32x16hin32((a), (b))
#define msu16x16(a, b, c) msu16x16in32((a), (b), (c))

#define DCT3_LEN (32)
#define DCT2_LEN (64)

#define LP_SHIFT_VAL 7
#define HQ_SHIFT_64 4
#define RADIXSHIFT 1
#define ROUNDING_SPECTRA 1
#define HQ_SHIFT_VAL 4

VOID ixheaacd_dct3_32(WORD32 *input, WORD32 *output,
                      const WORD16 *main_twidle_fwd, const WORD16 *post_tbl,
                      const WORD16 *w_16, const WORD32 *p_table) {
  WORD32 n, k;

  WORD32 temp1[6];
  WORD32 temp2[4];
  WORD16 twid_re, twid_im;
  WORD32 *ptr_reverse, *ptr_forward, *p_out, *ptr_out1;
  const WORD16 *twidle_fwd, *twidle_rev;

  ptr_forward = &input[49];
  ptr_reverse = &input[47];

  p_out = output;
  twidle_fwd = main_twidle_fwd;
  twidle_fwd += 4;

  *p_out++ = input[48] >> LP_SHIFT_VAL;
  *p_out++ = 0;

  for (n = 1; n < DCT3_LEN / 2; n++) {
    temp1[0] = *ptr_forward++;
    temp1[1] = *ptr_reverse--;
    temp1[0] = ixheaacd_add32(ixheaacd_shr32(temp1[0], LP_SHIFT_VAL),
                              ixheaacd_shr32(temp1[1], LP_SHIFT_VAL));

    temp1[2] = *(ptr_forward - 33);
    temp1[3] = *(ptr_reverse - 31);
    temp1[1] = ixheaacd_sub32(ixheaacd_shr32(temp1[2], LP_SHIFT_VAL),
                              ixheaacd_shr32(temp1[3], LP_SHIFT_VAL));
    twid_re = *twidle_fwd++;

    twid_im = *twidle_fwd;
    twidle_fwd += 3;

    *p_out++ = ixheaacd_mult32x16in32(temp1[0], twid_re) +
               ixheaacd_mult32x16in32(temp1[1], twid_im);
    *p_out++ = -ixheaacd_mult32x16in32(temp1[1], twid_re) +
               ixheaacd_mult32x16in32(temp1[0], twid_im);
  }

  twid_re = *twidle_fwd++;

  twid_im = *twidle_fwd;
  twidle_fwd += 3;

  temp1[1] = *ptr_reverse--;
  temp1[0] = *(ptr_reverse - 31);
  temp1[1] = ixheaacd_sub32(ixheaacd_shr32(temp1[1], LP_SHIFT_VAL),
                            ixheaacd_shr32(temp1[0], LP_SHIFT_VAL));

  temp1[0] = temp1[1];

  temp2[2] = ixheaacd_mult32x16in32(temp1[0], twid_re) +
             ixheaacd_mult32x16in32(temp1[1], twid_im);
  temp2[3] = -ixheaacd_mult32x16in32(temp1[1], twid_re) +
             ixheaacd_mult32x16in32(temp1[0], twid_im);

  ptr_forward = output;
  ptr_reverse = &output[DCT3_LEN - 1];

  temp2[0] = *ptr_forward++;
  temp2[1] = *ptr_forward--;

  temp1[0] = -temp2[1] - temp2[3];
  temp1[1] = temp2[0] - temp2[2];
  temp2[0] = (temp2[0] + temp2[2] + temp1[0]);
  temp2[1] = (temp2[1] - temp2[3] + temp1[1]);

  temp2[0] >>= 1;
  temp2[1] >>= 1;

  *ptr_forward++ = temp2[0];
  *ptr_forward++ = temp2[1];

  twidle_fwd = post_tbl + 2;
  twidle_rev = post_tbl + 14;

  for (n = 1; n < DCT3_LEN / 4; n++) {
    temp2[0] = *ptr_forward++;
    temp2[1] = *ptr_forward--;
    temp2[3] = *ptr_reverse--;
    temp2[2] = *ptr_reverse++;

    twid_re = *twidle_rev;
    twidle_rev -= 2;
    twid_im = *twidle_fwd;
    twidle_fwd += 2;

    temp1[0] = temp2[0] - temp2[2];
    temp1[1] = (temp2[0] + temp2[2]);

    temp1[2] = temp2[1] + temp2[3];
    temp1[3] = (temp2[1] - temp2[3]);

    temp1[4] = ixheaacd_mult32x16in32(temp1[0], twid_re) +
               ixheaacd_mult32x16in32(temp1[2], twid_im);
    temp1[5] = -ixheaacd_mult32x16in32(temp1[2], twid_re) +
               ixheaacd_mult32x16in32(temp1[0], twid_im);

    temp1[1] >>= 1;
    temp1[3] >>= 1;

    *ptr_forward++ = temp1[1] - temp1[4];
    *ptr_forward++ = temp1[3] + temp1[5];

    *ptr_reverse-- = -temp1[3] + temp1[5];
    *ptr_reverse-- = temp1[1] + temp1[4];
  }
  temp2[0] = *ptr_forward++;
  temp2[1] = *ptr_forward--;
  temp2[3] = *ptr_reverse--;
  temp2[2] = *ptr_reverse++;

  twid_re = -*twidle_rev;
  twidle_rev -= 2;
  twid_im = *twidle_fwd;
  twidle_fwd += 2;

  temp1[0] = temp2[0] - temp2[2];
  temp1[1] = (temp2[0] + temp2[2]);

  temp1[2] = temp2[1] + temp2[3];
  temp1[3] = (temp2[1] - temp2[3]);

  temp1[4] = ixheaacd_mult32x16in32(temp1[0], twid_re) -
             ixheaacd_mult32x16in32(temp1[2], twid_im);
  temp1[5] = ixheaacd_mult32x16in32(temp1[2], twid_re) +
             ixheaacd_mult32x16in32(temp1[0], twid_im);

  temp1[1] >>= 1;
  temp1[3] >>= 1;
  *ptr_forward++ = temp1[1] + temp1[4];
  *ptr_forward++ = temp1[3] + temp1[5];

  ixheaacd_radix4bfly(w_16, output, 1, 4);
  ixheaacd_postradixcompute4(input, output, p_table, 16);

  output[0] = input[0];
  output[2] = input[1];

  p_out = input + 2;
  ptr_forward = output + 1;
  ptr_reverse = output + 30;
  ptr_out1 = input + 18;

  for (k = (DCT3_LEN / 4) - 1; k != 0; k--) {
    WORD32 tempre, tempim;

    tempre = *p_out++;
    tempim = *p_out++;
    *ptr_forward = (tempim);
    ptr_forward += 2;
    *ptr_forward = (tempre);
    ptr_forward += 2;

    tempre = *ptr_out1++;
    tempim = *ptr_out1++;
    *ptr_reverse = (tempim);
    ptr_reverse -= 2;
    *ptr_reverse = (tempre);
    ptr_reverse -= 2;
  }

  {
    WORD32 tempre, tempim;
    tempre = *p_out++;
    tempim = *p_out++;
    *ptr_forward = (tempim);
    ptr_forward += 2;
    *ptr_forward = (tempre);
    ptr_forward += 2;
  }

  return;
}
VOID ixheaacd_dct2_64(WORD32 *x, WORD32 *X,
                      ia_qmf_dec_tables_struct *qmf_dec_tables_ptr,
                      WORD16 *filter_states) {
  ixheaacd_pretwdct2(x, X);

  ixheaacd_sbr_imdct_using_fft(qmf_dec_tables_ptr->w1024, 32, X, x,
                               qmf_dec_tables_ptr->dig_rev_table2_128,
                               qmf_dec_tables_ptr->dig_rev_table2_128,
                               qmf_dec_tables_ptr->dig_rev_table2_128,
                               qmf_dec_tables_ptr->dig_rev_table2_128);

  ixheaacd_fftposttw(x, qmf_dec_tables_ptr);

  ixheaacd_posttwdct2(x, filter_states, qmf_dec_tables_ptr);

  return;
}

VOID ixheaacd_cos_sin_mod(WORD32 *subband,
                          ia_sbr_qmf_filter_bank_struct *qmf_bank,
                          WORD16 *p_twiddle, WORD32 *p_dig_rev_tbl) {
  WORD32 M = ixheaacd_shr32(qmf_bank->no_channels, 1);

  const WORD16 *p_sin;
  const WORD16 *p_sin_cos = &qmf_bank->cos_twiddle[0];
  WORD32 subband_tmp[128];

  ixheaacd_cos_sin_mod_loop1(subband, M, p_sin_cos, subband_tmp);

  if (M == 32) {
    ixheaacd_sbr_imdct_using_fft(
        (const WORD32 *)p_twiddle, 32, subband_tmp, subband,
        (UWORD8 *)p_dig_rev_tbl, (UWORD8 *)p_dig_rev_tbl,
        (UWORD8 *)p_dig_rev_tbl, (UWORD8 *)p_dig_rev_tbl);

    ixheaacd_sbr_imdct_using_fft(
        (const WORD32 *)p_twiddle, 32, &subband_tmp[64], &subband[64],
        (UWORD8 *)p_dig_rev_tbl, (UWORD8 *)p_dig_rev_tbl,
        (UWORD8 *)p_dig_rev_tbl, (UWORD8 *)p_dig_rev_tbl);

  } else {
    ixheaacd_sbr_imdct_using_fft(
        (const WORD32 *)p_twiddle, 16, subband_tmp, subband,
        (UWORD8 *)p_dig_rev_tbl, (UWORD8 *)p_dig_rev_tbl,
        (UWORD8 *)p_dig_rev_tbl, (UWORD8 *)p_dig_rev_tbl);

    ixheaacd_sbr_imdct_using_fft(
        (const WORD32 *)p_twiddle, 16, &subband_tmp[64], &subband[64],
        (UWORD8 *)p_dig_rev_tbl, (UWORD8 *)p_dig_rev_tbl,
        (UWORD8 *)p_dig_rev_tbl, (UWORD8 *)p_dig_rev_tbl);
  }

  p_sin = &qmf_bank->alt_sin_twiddle[0];
  ixheaacd_cos_sin_mod_loop2(subband, p_sin, M);
}

VOID ixheaacd_fwd_modulation(const WORD32 *p_time_in1, WORD32 *real_subband,
                             WORD32 *imag_subband,
                             ia_sbr_qmf_filter_bank_struct *qmf_bank,
                             ia_qmf_dec_tables_struct *qmf_dec_tables_ptr) {
  WORD32 i;
  const WORD32 *p_time_in2 = &p_time_in1[2 * NO_ANALYSIS_CHANNELS - 1];
  WORD32 temp1, temp2;
  WORD32 *t_real_subband = real_subband;
  WORD32 *t_imag_subband = imag_subband;
  const WORD16 *tcos;

  for (i = NO_ANALYSIS_CHANNELS - 1; i >= 0; i--) {
    temp1 = ixheaacd_shr32(*p_time_in1++, HQ_SHIFT_VAL);
    temp2 = ixheaacd_shr32(*p_time_in2--, HQ_SHIFT_VAL);

    *t_real_subband++ = ixheaacd_sub32_sat(temp1, temp2);
    ;
    *t_imag_subband++ = ixheaacd_add32(temp1, temp2);
    ;
  }

  ixheaacd_cos_sin_mod(real_subband, qmf_bank,
                       (WORD16 *)qmf_dec_tables_ptr->w1024,
                       (WORD32 *)qmf_dec_tables_ptr->dig_rev_table2_128);

  tcos = qmf_bank->t_cos;

  for (i = (qmf_bank->usb - qmf_bank->lsb - 1); i >= 0; i--) {
    WORD16 cosh, sinh;
    WORD32 re, im;

    re = *real_subband;
    im = *imag_subband;
    cosh = *tcos++;
    sinh = *tcos++;
    *real_subband++ = ixheaacd_add32(ixheaacd_mult32x16in32_shl(re, cosh),
                                     ixheaacd_mult32x16in32_shl(im, sinh));
    *imag_subband++ = ixheaacd_sub32_sat(ixheaacd_mult32x16in32_shl(im, cosh),
                                         ixheaacd_mult32x16in32_shl(re, sinh));
  }
}

VOID ixheaacd_cplx_anal_qmffilt(const WORD16 *time_sample_buf,
                                ia_sbr_scale_fact_struct *sbr_scale_factor,
                                WORD32 **qmf_real, WORD32 **qmf_imag,
                                ia_sbr_qmf_filter_bank_struct *qmf_bank,
                                ia_qmf_dec_tables_struct *qmf_dec_tables_ptr,
                                WORD32 ch_fac, WORD32 low_pow_flag,
                                WORD audio_object_type) {
  WORD32 i, k;
  WORD32 num_time_slots = qmf_bank->num_time_slots;

  WORD32 analysis_buffer[4 * NO_ANALYSIS_CHANNELS];
  WORD16 *filter_states = qmf_bank->core_samples_buffer;

  WORD16 *fp1, *fp2, *tmp;

  WORD16 *filter_1;
  WORD16 *filter_2;
  WORD16 *filt_ptr;
  if (audio_object_type != AOT_ER_AAC_ELD &&
      audio_object_type != AOT_ER_AAC_LD) {
    qmf_bank->filter_pos +=
        (qmf_dec_tables_ptr->qmf_c - qmf_bank->analy_win_coeff);
    qmf_bank->analy_win_coeff = qmf_dec_tables_ptr->qmf_c;
  } else {
    qmf_bank->filter_pos +=
        (qmf_dec_tables_ptr->qmf_c_eld3 - qmf_bank->analy_win_coeff);
    qmf_bank->analy_win_coeff = qmf_dec_tables_ptr->qmf_c_eld3;
  }

  filter_1 = qmf_bank->filter_pos;

  if (audio_object_type != AOT_ER_AAC_ELD &&
      audio_object_type != AOT_ER_AAC_LD) {
    filter_2 = filter_1 + 64;
  } else {
    filter_2 = filter_1 + 32;
  }

  sbr_scale_factor->st_lb_scale = 0;
  sbr_scale_factor->lb_scale = -10;
  if (!low_pow_flag) {
    if (audio_object_type != AOT_ER_AAC_ELD &&
        audio_object_type != AOT_ER_AAC_LD) {
      sbr_scale_factor->lb_scale = -8;
    } else {
      sbr_scale_factor->lb_scale = -9;
    }
    qmf_bank->cos_twiddle =
        (WORD16 *)qmf_dec_tables_ptr->sbr_sin_cos_twiddle_l32;
    qmf_bank->alt_sin_twiddle =
        (WORD16 *)qmf_dec_tables_ptr->sbr_alt_sin_twiddle_l32;
    if (audio_object_type != AOT_ER_AAC_ELD &&
        audio_object_type != AOT_ER_AAC_LD) {
      qmf_bank->t_cos = (WORD16 *)qmf_dec_tables_ptr->sbr_t_cos_sin_l32;
    } else {
      qmf_bank->t_cos =
          (WORD16 *)qmf_dec_tables_ptr->ixheaacd_sbr_t_cos_sin_l32_eld;
    }
  }

  fp1 = qmf_bank->anal_filter_states;
  fp2 = qmf_bank->anal_filter_states + NO_ANALYSIS_CHANNELS;

  if (audio_object_type == AOT_ER_AAC_ELD ||
      audio_object_type == AOT_ER_AAC_LD) {
    filter_2 = qmf_bank->filter_2;
    fp1 = qmf_bank->fp1_anal;
    fp2 = qmf_bank->fp2_anal;
  }

  for (i = 0; i < num_time_slots; i++) {
    for (k = 0; k < NO_ANALYSIS_CHANNELS; k++)
      filter_states[NO_ANALYSIS_CHANNELS - 1 - k] = time_sample_buf[ch_fac * k];

    if (audio_object_type != AOT_ER_AAC_ELD &&
        audio_object_type != AOT_ER_AAC_LD) {
      ixheaacd_sbr_qmfanal32_winadds(fp1, fp2, filter_1, filter_2,
                                     analysis_buffer, filter_states,
                                     time_sample_buf, ch_fac);
    }

    else {
      ixheaacd_sbr_qmfanal32_winadd_eld(fp1, fp2, filter_1, filter_2,
                                        analysis_buffer);
    }

    time_sample_buf += NO_ANALYSIS_CHANNELS * ch_fac;

    filter_states -= NO_ANALYSIS_CHANNELS;
    if (filter_states < qmf_bank->anal_filter_states) {
      filter_states = qmf_bank->anal_filter_states + 288;
    }

    tmp = fp1;
    fp1 = fp2;
    fp2 = tmp;
    if (audio_object_type != AOT_ER_AAC_ELD &&
        audio_object_type != AOT_ER_AAC_LD) {
      filter_1 += 64;
      filter_2 += 64;
    } else {
      filter_1 += 32;
      filter_2 += 32;
    }

    filt_ptr = filter_1;
    filter_1 = filter_2;
    filter_2 = filt_ptr;
    if (audio_object_type != AOT_ER_AAC_ELD &&
        audio_object_type != AOT_ER_AAC_LD) {
      if (filter_2 > (qmf_bank->analy_win_coeff + 640)) {
        filter_1 = (WORD16 *)qmf_bank->analy_win_coeff;
        filter_2 = (WORD16 *)qmf_bank->analy_win_coeff + 64;
      }
    } else {
      if (filter_2 > (qmf_bank->analy_win_coeff + 320)) {
        filter_1 = (WORD16 *)qmf_bank->analy_win_coeff;
        filter_2 = (WORD16 *)qmf_bank->analy_win_coeff + 32;
      }
    }

    if (!low_pow_flag) {
      ixheaacd_fwd_modulation(analysis_buffer, qmf_real[i], qmf_imag[i],
                              qmf_bank, qmf_dec_tables_ptr);
    } else {
      ixheaacd_dct3_32(
          (WORD32 *)analysis_buffer, qmf_real[i], qmf_dec_tables_ptr->dct23_tw,
          qmf_dec_tables_ptr->post_fft_tbl, qmf_dec_tables_ptr->w_16,
          qmf_dec_tables_ptr->dig_rev_table4_16);
    }
  }

  qmf_bank->filter_pos = filter_1;
  qmf_bank->core_samples_buffer = filter_states;

  if (audio_object_type == AOT_ER_AAC_ELD || audio_object_type == AOT_ER_AAC_LD)

  {
    qmf_bank->fp1_anal = fp1;
    qmf_bank->fp2_anal = fp2;
    qmf_bank->filter_2 = filter_2;
  }
}

VOID ixheaacd_inv_modulation_lp(WORD32 *qmf_real, WORD16 *filter_states,
                                ia_sbr_qmf_filter_bank_struct *syn_qmf,
                                ia_qmf_dec_tables_struct *qmf_dec_tables_ptr) {
  WORD32 L = syn_qmf->no_channels;
  const WORD32 M = (L >> 1);
  WORD32 *dct_in = qmf_real;
  WORD32 time_out[2 * NO_SYNTHESIS_CHANNELS];

  WORD32 ui_rem = ((WORD64)(&time_out[0]) % 8);
  WORD32 *ptime_out = (pVOID)((WORD8 *)&time_out[0] + 8 - ui_rem);

  if (L == 64)
    ixheaacd_dct2_64(dct_in, ptime_out, qmf_dec_tables_ptr, filter_states + M);
  else
    ixheaacd_dct2_32(dct_in, time_out, qmf_dec_tables_ptr, filter_states);

  filter_states[3 * M] = 0;
}

VOID ixheaacd_inv_emodulation(WORD32 *qmf_real,
                              ia_sbr_qmf_filter_bank_struct *syn_qmf,
                              ia_qmf_dec_tables_struct *qmf_dec_tables_ptr) {
  ixheaacd_cos_sin_mod(qmf_real, syn_qmf, (WORD16 *)qmf_dec_tables_ptr->w1024,
                       (WORD32 *)qmf_dec_tables_ptr->dig_rev_table2_128);
}

VOID ixheaacd_esbr_radix4bfly(const WORD32 *w, WORD32 *x, WORD32 index1,
                              WORD32 index) {
  int i;
  WORD32 l1, l2, h2, fft_jmp;
  WORD64 xt0_0, yt0_0, xt1_0, yt1_0, xt2_0, yt2_0;
  WORD64 xh0_0, xh1_0, xh20_0, xh21_0, xl0_0, xl1_0, xl20_0, xl21_0;
  WORD32 x_0, x_1, x_l1_0, x_l1_1, x_l2_0, x_l2_1;
  WORD32 x_h2_0, x_h2_1;
  WORD32 si10, si20, si30, co10, co20, co30;

  WORD64 mul_1, mul_2, mul_3, mul_4, mul_5, mul_6;
  WORD64 mul_7, mul_8, mul_9, mul_10, mul_11, mul_12;
  WORD32 *x_l1;
  WORD32 *x_l2;
  WORD32 *x_h2;
  const WORD32 *w_ptr = w;
  WORD32 i1;

  h2 = index << 1;
  l1 = index << 2;
  l2 = (index << 2) + (index << 1);

  x_l1 = &(x[l1]);
  x_l2 = &(x[l2]);
  x_h2 = &(x[h2]);

  fft_jmp = 6 * (index);

  for (i1 = 0; i1 < index1; i1++) {
    for (i = 0; i < index; i++) {
      si10 = (*w_ptr++);
      co10 = (*w_ptr++);
      si20 = (*w_ptr++);
      co20 = (*w_ptr++);
      si30 = (*w_ptr++);
      co30 = (*w_ptr++);

      x_0 = x[0];
      x_h2_0 = x[h2];
      x_l1_0 = x[l1];
      x_l2_0 = x[l2];

      xh0_0 = (WORD64)x_0 + (WORD64)x_l1_0;
      xl0_0 = (WORD64)x_0 - (WORD64)x_l1_0;

      xh20_0 = (WORD64)x_h2_0 + (WORD64)x_l2_0;
      xl20_0 = (WORD64)x_h2_0 - (WORD64)x_l2_0;

      x[0] = (WORD32)ixheaacd_add64_sat(xh0_0, xh20_0);
      xt0_0 = (WORD64)xh0_0 - (WORD64)xh20_0;

      x_1 = x[1];
      x_h2_1 = x[h2 + 1];
      x_l1_1 = x[l1 + 1];
      x_l2_1 = x[l2 + 1];

      xh1_0 = (WORD64)x_1 + (WORD64)x_l1_1;
      xl1_0 = (WORD64)x_1 - (WORD64)x_l1_1;

      xh21_0 = (WORD64)x_h2_1 + (WORD64)x_l2_1;
      xl21_0 = (WORD64)x_h2_1 - (WORD64)x_l2_1;

      x[1] = (WORD32)ixheaacd_add64_sat(xh1_0, xh21_0);
      yt0_0 = (WORD64)xh1_0 - (WORD64)xh21_0;

      xt1_0 = (WORD64)xl0_0 + (WORD64)xl21_0;
      xt2_0 = (WORD64)xl0_0 - (WORD64)xl21_0;

      yt2_0 = (WORD64)xl1_0 + (WORD64)xl20_0;
      yt1_0 = (WORD64)xl1_0 - (WORD64)xl20_0;

      mul_11 = ixheaacd_mult64(xt2_0, co30);
      mul_3 = ixheaacd_mult64(yt2_0, si30);
      x[l2] = (WORD32)((mul_3 + mul_11) >> 32) << RADIXSHIFT;

      mul_5 = ixheaacd_mult64(xt2_0, si30);
      mul_9 = ixheaacd_mult64(yt2_0, co30);
      x[l2 + 1] = (WORD32)((mul_9 - mul_5) >> 32) << RADIXSHIFT;

      mul_12 = ixheaacd_mult64(xt0_0, co20);
      mul_2 = ixheaacd_mult64(yt0_0, si20);
      x[l1] = (WORD32)((mul_2 + mul_12) >> 32) << RADIXSHIFT;

      mul_6 = ixheaacd_mult64(xt0_0, si20);
      mul_8 = ixheaacd_mult64(yt0_0, co20);
      x[l1 + 1] = (WORD32)((mul_8 - mul_6) >> 32) << RADIXSHIFT;

      mul_4 = ixheaacd_mult64(xt1_0, co10);
      mul_1 = ixheaacd_mult64(yt1_0, si10);
      x[h2] = (WORD32)((mul_1 + mul_4) >> 32) << RADIXSHIFT;

      mul_10 = ixheaacd_mult64(xt1_0, si10);
      mul_7 = ixheaacd_mult64(yt1_0, co10);
      x[h2 + 1] = (WORD32)((mul_7 - mul_10) >> 32) << RADIXSHIFT;

      x += 2;
    }
    x += fft_jmp;
    w_ptr = w_ptr - fft_jmp;
  }
}

VOID ixheaacd_esbr_postradixcompute2(WORD32 *ptr_y, WORD32 *ptr_x,
                                     const WORD32 *pdig_rev_tbl,
                                     WORD32 npoints) {
  WORD32 i, k;
  WORD32 h2;
  WORD32 x_0, x_1, x_2, x_3;
  WORD32 x_4, x_5, x_6, x_7;
  WORD32 x_8, x_9, x_a, x_b, x_c, x_d, x_e, x_f;
  WORD32 n0, j0;
  WORD32 *x2, *x0;
  WORD32 *y0, *y1, *y2, *y3;

  y0 = ptr_y;
  y2 = ptr_y + (WORD32)npoints;
  x0 = ptr_x;
  x2 = ptr_x + (WORD32)(npoints >> 1);

  y1 = y0 + (WORD32)(npoints >> 2);
  y3 = y2 + (WORD32)(npoints >> 2);
  j0 = 8;
  n0 = npoints >> 1;

  for (k = 0; k < 2; k++) {
    for (i = 0; i<npoints>> 1; i += 8) {
      h2 = *pdig_rev_tbl++ >> 2;

      x_0 = *x0++;
      x_1 = *x0++;
      x_2 = *x0++;
      x_3 = *x0++;
      x_4 = *x0++;
      x_5 = *x0++;
      x_6 = *x0++;
      x_7 = *x0++;

      y0[h2] = ixheaacd_add32_sat(x_0, x_2);
      y0[h2 + 1] = ixheaacd_add32_sat(x_1, x_3);
      y1[h2] = ixheaacd_add32_sat(x_4, x_6);
      y1[h2 + 1] = ixheaacd_add32_sat(x_5, x_7);
      y2[h2] = ixheaacd_sub32_sat(x_0, x_2);
      y2[h2 + 1] = ixheaacd_sub32_sat(x_1, x_3);
      y3[h2] = ixheaacd_sub32_sat(x_4, x_6);
      y3[h2 + 1] = ixheaacd_sub32_sat(x_5, x_7);

      x_8 = *x2++;
      x_9 = *x2++;
      x_a = *x2++;
      x_b = *x2++;
      x_c = *x2++;
      x_d = *x2++;
      x_e = *x2++;
      x_f = *x2++;

      y0[h2 + 2] = ixheaacd_add32_sat(x_8, x_a);
      y0[h2 + 3] = ixheaacd_add32_sat(x_9, x_b);
      y1[h2 + 2] = ixheaacd_add32_sat(x_c, x_e);
      y1[h2 + 3] = ixheaacd_add32_sat(x_d, x_f);
      y2[h2 + 2] = ixheaacd_sub32_sat(x_8, x_a);
      y2[h2 + 3] = ixheaacd_sub32_sat(x_9, x_b);
      y3[h2 + 2] = ixheaacd_sub32_sat(x_c, x_e);
      y3[h2 + 3] = ixheaacd_sub32_sat(x_d, x_f);
    }
    x0 += (WORD32)npoints >> 1;
    x2 += (WORD32)npoints >> 1;
  }
}

VOID ixheaacd_esbr_postradixcompute4(WORD32 *ptr_y, WORD32 *ptr_x,
                                     const WORD32 *p_dig_rev_tbl,
                                     WORD32 npoints) {
  WORD32 i, k;
  WORD32 h2;
  WORD32 xh0_0, xh1_0, xl0_0, xl1_0;
  WORD32 xh0_1, xh1_1, xl0_1, xl1_1;
  WORD32 x_0, x_1, x_2, x_3;
  WORD32 xh0_2, xh1_2, xl0_2, xl1_2, xh0_3, xh1_3, xl0_3, xl1_3;
  WORD32 x_4, x_5, x_6, x_7;
  WORD32 x_8, x_9, x_a, x_b, x_c, x_d, x_e, x_f;
  WORD32 n00, n10, n20, n30, n01, n11, n21, n31;
  WORD32 n02, n12, n22, n32, n03, n13, n23, n33;
  WORD32 n0, j0;
  WORD32 *x2, *x0;
  WORD32 *y0, *y1, *y2, *y3;

  y0 = ptr_y;
  y2 = ptr_y + (WORD32)npoints;
  x0 = ptr_x;
  x2 = ptr_x + (WORD32)(npoints >> 1);

  y1 = y0 + (WORD32)(npoints >> 1);
  y3 = y2 + (WORD32)(npoints >> 1);

  j0 = 4;
  n0 = npoints >> 2;

  for (k = 0; k < 2; k++) {
    for (i = 0; i<npoints>> 1; i += 8) {
      h2 = *p_dig_rev_tbl++ >> 2;
      x_0 = *x0++;
      x_1 = *x0++;
      x_2 = *x0++;
      x_3 = *x0++;
      x_4 = *x0++;
      x_5 = *x0++;
      x_6 = *x0++;
      x_7 = *x0++;

      xh0_0 = x_0 + x_4;
      xh1_0 = x_1 + x_5;
      xl0_0 = x_0 - x_4;
      xl1_0 = x_1 - x_5;
      xh0_1 = x_2 + x_6;
      xh1_1 = x_3 + x_7;
      xl0_1 = x_2 - x_6;
      xl1_1 = x_3 - x_7;

      n00 = xh0_0 + xh0_1;
      n01 = xh1_0 + xh1_1;
      n10 = xl0_0 + xl1_1;
      n11 = xl1_0 - xl0_1;
      n20 = xh0_0 - xh0_1;
      n21 = xh1_0 - xh1_1;
      n30 = xl0_0 - xl1_1;
      n31 = xl1_0 + xl0_1;

      y0[h2] = n00;
      y0[h2 + 1] = n01;
      y1[h2] = n10;
      y1[h2 + 1] = n11;
      y2[h2] = n20;
      y2[h2 + 1] = n21;
      y3[h2] = n30;
      y3[h2 + 1] = n31;

      x_8 = *x2++;
      x_9 = *x2++;
      x_a = *x2++;
      x_b = *x2++;
      x_c = *x2++;
      x_d = *x2++;
      x_e = *x2++;
      x_f = *x2++;

      xh0_2 = x_8 + x_c;
      xh1_2 = x_9 + x_d;
      xl0_2 = x_8 - x_c;
      xl1_2 = x_9 - x_d;
      xh0_3 = x_a + x_e;
      xh1_3 = x_b + x_f;
      xl0_3 = x_a - x_e;
      xl1_3 = x_b - x_f;

      n02 = xh0_2 + xh0_3;
      n03 = xh1_2 + xh1_3;
      n12 = xl0_2 + xl1_3;
      n13 = xl1_2 - xl0_3;
      n22 = xh0_2 - xh0_3;
      n23 = xh1_2 - xh1_3;
      n32 = xl0_2 - xl1_3;
      n33 = xl1_2 + xl0_3;

      y0[h2 + 2] = n02;
      y0[h2 + 3] = n03;
      y1[h2 + 2] = n12;
      y1[h2 + 3] = n13;
      y2[h2 + 2] = n22;
      y2[h2 + 3] = n23;
      y3[h2 + 2] = n32;
      y3[h2 + 3] = n33;
    }
    x0 += (WORD32)npoints >> 1;
    x2 += (WORD32)npoints >> 1;
  }
}

VOID ixheaacd_esbr_cos_sin_mod(WORD32 *subband,
                               ia_sbr_qmf_filter_bank_struct *qmf_bank,
                               WORD32 *p_twiddle, WORD32 *p_dig_rev_tbl) {
  WORD32 z;
  WORD32 temp[128];
  WORD32 scaleshift = 0;

  WORD32 re2, re3;
  WORD32 wim, wre;

  WORD32 i, M_2;
  WORD32 M = ixheaacd_shr32(qmf_bank->no_channels, 1);

  const WORD32 *p_sin;
  const WORD32 *p_sin_cos;

  WORD32 subband_tmp[128];
  WORD32 re;
  WORD32 im;
  WORD32 *psubband, *psubband1;
  WORD32 *psubband_t, *psubband1_t;
  WORD32 *psubband2, *psubband12;
  WORD32 *psubband_t2, *psubband1_t2;

  M_2 = ixheaacd_shr32(M, 1);

  p_sin_cos = qmf_bank->esbr_cos_twiddle;

  psubband = &subband[0];
  psubband1 = &subband[2 * M - 1];
  psubband_t = subband_tmp;
  psubband1_t = &subband_tmp[2 * M - 1];

  psubband2 = &subband[64];
  psubband12 = &subband[2 * M - 1 + 64];
  psubband_t2 = &subband_tmp[64];
  psubband1_t2 = &subband_tmp[2 * M - 1 + 64];

  for (i = (M_2 >> 1) - 1; i >= 0; i--) {
    re = *psubband++;
    im = *psubband1--;

    wim = *p_sin_cos++;
    wre = *p_sin_cos++;

    *psubband_t++ = (WORD32)(
        (ixheaacd_add64(ixheaacd_mult64(re, wre), ixheaacd_mult64(im, wim))) >>
        32);
    *psubband_t++ = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(im, wre),
                                                 ixheaacd_mult64(re, wim))) >>
                             32);

    re = *psubband2++;
    im = *psubband12--;

    *psubband_t2++ = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(im, wim),
                                                  ixheaacd_mult64(re, wre))) >>
                              32);
    *psubband_t2++ = (WORD32)(
        (ixheaacd_add64(ixheaacd_mult64(re, wim), ixheaacd_mult64(im, wre))) >>
        32);

    re = *psubband1--;
    im = *psubband++;

    wim = *p_sin_cos++;
    wre = *p_sin_cos++;

    *psubband1_t-- = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(im, wre),
                                                  ixheaacd_mult64(re, wim))) >>
                              32);
    *psubband1_t-- = (WORD32)(
        (ixheaacd_add64(ixheaacd_mult64(re, wre), ixheaacd_mult64(im, wim))) >>
        32);

    re = *psubband12--;
    im = *psubband2++;

    *psubband1_t2-- = (WORD32)(
        (ixheaacd_add64(ixheaacd_mult64(re, wim), ixheaacd_mult64(im, wre))) >>
        32);
    *psubband1_t2-- = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(im, wim),
                                                   ixheaacd_mult64(re, wre))) >>
                               32);

    re = *psubband++;
    im = *psubband1--;

    wim = *p_sin_cos++;
    wre = *p_sin_cos++;

    *psubband_t++ = (WORD32)(
        (ixheaacd_add64(ixheaacd_mult64(re, wre), ixheaacd_mult64(im, wim))) >>
        32);
    *psubband_t++ = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(im, wre),
                                                 ixheaacd_mult64(re, wim))) >>
                             32);

    re = *psubband2++;
    im = *psubband12--;

    *psubband_t2++ = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(im, wim),
                                                  ixheaacd_mult64(re, wre))) >>
                              32);
    *psubband_t2++ = (WORD32)(
        (ixheaacd_add64(ixheaacd_mult64(re, wim), ixheaacd_mult64(im, wre))) >>
        32);

    re = *psubband1--;
    im = *psubband++;
    ;

    wim = *p_sin_cos++;
    wre = *p_sin_cos++;

    *psubband1_t-- = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(im, wre),
                                                  ixheaacd_mult64(re, wim))) >>
                              32);
    *psubband1_t-- = (WORD32)(
        (ixheaacd_add64(ixheaacd_mult64(re, wre), ixheaacd_mult64(im, wim))) >>
        32);

    re = *psubband12--;
    im = *psubband2++;
    ;

    *psubband1_t2-- = (WORD32)(
        (ixheaacd_add64(ixheaacd_mult64(re, wim), ixheaacd_mult64(im, wre))) >>
        32);
    *psubband1_t2-- = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(im, wim),
                                                   ixheaacd_mult64(re, wre))) >>
                               32);
  }

  if (M == 32) {
    ixheaacd_esbr_radix4bfly(p_twiddle, subband_tmp, 1, 8);
    ixheaacd_esbr_radix4bfly(p_twiddle + 48, subband_tmp, 4, 2);
    ixheaacd_esbr_postradixcompute2(subband, subband_tmp, p_dig_rev_tbl, 32);

    ixheaacd_esbr_radix4bfly(p_twiddle, &subband_tmp[64], 1, 8);
    ixheaacd_esbr_radix4bfly(p_twiddle + 48, &subband_tmp[64], 4, 2);
    ixheaacd_esbr_postradixcompute2(&subband[64], &subband_tmp[64],
                                    p_dig_rev_tbl, 32);

  }

  else if (M == 16) {
    ixheaacd_esbr_radix4bfly(p_twiddle, subband_tmp, 1, 4);
    ixheaacd_esbr_postradixcompute4(subband, subband_tmp, p_dig_rev_tbl, 16);

    ixheaacd_esbr_radix4bfly(p_twiddle, &subband_tmp[64], 1, 4);
    ixheaacd_esbr_postradixcompute4(&subband[64], &subband_tmp[64],
                                    p_dig_rev_tbl, 16);

  }

  else if (M == 12) {
    for (z = 0; z < (qmf_bank->no_channels >> 1); z++) {
      temp[z] = subband_tmp[2 * z];
      temp[12 + z] = subband_tmp[2 * z + 1];
    }

    ixheaacd_complex_fft_p3(temp, &temp[12], 12, -1, &scaleshift);

    for (z = 0; z < (qmf_bank->no_channels >> 1); z++) {
      subband[2 * z] = temp[z];
      subband[2 * z + 1] = temp[z + 12];
    }
    scaleshift = 0;
    for (z = 0; z < (qmf_bank->no_channels >> 1); z++) {
      temp[z] = subband_tmp[64 + 2 * z];
      temp[12 + z] = subband_tmp[64 + 2 * z + 1];
    }

    ixheaacd_complex_fft_p3(temp, &temp[12], 12, -1, &scaleshift);

    for (z = 0; z < (qmf_bank->no_channels >> 1); z++) {
      subband[64 + 2 * z] = temp[z];
      subband[64 + 2 * z + 1] = temp[z + 12];
    }

  }

  else {
    for (z = 0; z < (qmf_bank->no_channels >> 1); z++) {
      temp[z] = subband_tmp[2 * z];
      temp[8 + z] = subband_tmp[2 * z + 1];
    }

    (*ixheaacd_complex_fft_p2)(temp, &temp[8], 8, -1, &scaleshift);

    for (z = 0; z < (qmf_bank->no_channels >> 1); z++) {
      subband[2 * z] = temp[z] << scaleshift;
      subband[2 * z + 1] = temp[z + 8] << scaleshift;
    }
    scaleshift = 0;
    for (z = 0; z < (qmf_bank->no_channels >> 1); z++) {
      temp[z] = subband_tmp[64 + 2 * z];
      temp[8 + z] = subband_tmp[64 + 2 * z + 1];
    }

    (*ixheaacd_complex_fft_p2)(temp, &temp[8], 8, -1, &scaleshift);

    for (z = 0; z < (qmf_bank->no_channels >> 1); z++) {
      subband[64 + 2 * z] = temp[z] << scaleshift;
      subband[64 + 2 * z + 1] = temp[8 + z] << scaleshift;
    }
  }

  psubband = &subband[0];
  psubband1 = &subband[2 * M - 1];

  re = *psubband1;

  *psubband = *psubband >> 1;
  psubband++;
  *psubband1 = ixheaacd_negate32(*psubband >> 1);
  psubband1--;

  p_sin = qmf_bank->esbr_alt_sin_twiddle;

  wim = *p_sin++;
  wre = *p_sin++;

  im = *psubband1;
  ;

  *psubband1-- = (WORD32)(
      (ixheaacd_add64(ixheaacd_mult64(re, wre), ixheaacd_mult64(im, wim))) >>
      32);
  *psubband++ = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(im, wre),
                                             ixheaacd_mult64(re, wim))) >>
                         32);

  psubband2 = &subband[64];
  psubband12 = &subband[2 * M - 1 + 64];

  re = *psubband12;
  ;

  *psubband12-- = ixheaacd_negate32_sat(*psubband2 >> 1);
  ;
  *psubband2 = psubband2[1] >> 1;
  ;
  psubband2++;

  im = *psubband12;
  ;

  *psubband2++ = ixheaacd_negate32_sat((WORD32)(
      (ixheaacd_add64(ixheaacd_mult64(re, wre), ixheaacd_mult64(im, wim))) >>
      32));
  *psubband12-- = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(re, wim),
                                               ixheaacd_mult64(im, wre))) >>
                           32);

  for (i = (M_2 - 2); i >= 0; i--) {
    im = psubband[0];
    ;
    re = psubband[1];
    ;
    re2 = *psubband1;
    ;

    *psubband++ = (WORD32)(
        (ixheaacd_add64(ixheaacd_mult64(re, wim), ixheaacd_mult64(im, wre))) >>
        32);
    *psubband1-- = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(im, wim),
                                                ixheaacd_mult64(re, wre))) >>
                            32);

    im = psubband2[0];
    ;
    re = psubband2[1];
    ;
    re3 = *psubband12;
    ;

    *psubband12-- = ixheaacd_negate32_sat((WORD32)(
        (ixheaacd_add64(ixheaacd_mult64(re, wim), ixheaacd_mult64(im, wre))) >>
        32));
    *psubband2++ = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(re, wre),
                                                ixheaacd_mult64(im, wim))) >>
                            32);

    wim = *p_sin++;
    wre = *p_sin++;
    im = psubband1[0];
    ;

    *psubband1-- = (WORD32)(
        (ixheaacd_add64(ixheaacd_mult64(re2, wre), ixheaacd_mult64(im, wim))) >>
        32);
    *psubband++ = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(im, wre),
                                               ixheaacd_mult64(re2, wim))) >>
                           32);

    im = psubband12[0];
    ;

    *psubband2++ = ixheaacd_negate32_sat((WORD32)(
        (ixheaacd_add64(ixheaacd_mult64(re3, wre), ixheaacd_mult64(im, wim))) >>
        32));
    *psubband12-- = (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(re3, wim),
                                                 ixheaacd_mult64(im, wre))) >>
                             32);
  }
}

VOID ixheaacd_esbr_fwd_modulation(
    const WORD32 *time_sample_buf, WORD32 *real_subband, WORD32 *imag_subband,
    ia_sbr_qmf_filter_bank_struct *qmf_bank,
    ia_qmf_dec_tables_struct *qmf_dec_tables_ptr) {
  WORD32 i;
  const WORD32 *time_sample_buf1 =
      &time_sample_buf[2 * qmf_bank->no_channels - 1];
  WORD32 temp1, temp2;
  WORD32 *t_real_subband = real_subband;
  WORD32 *t_imag_subband = imag_subband;
  const WORD32 *tcos;

  for (i = qmf_bank->no_channels - 1; i >= 0; i--) {
    temp1 = ixheaacd_shr32(*time_sample_buf++, HQ_SHIFT_64);
    temp2 = ixheaacd_shr32(*time_sample_buf1--, HQ_SHIFT_64);

    *t_real_subband++ = ixheaacd_sub32_sat(temp1, temp2);
    ;
    *t_imag_subband++ = ixheaacd_add32(temp1, temp2);
    ;
  }

  ixheaacd_esbr_cos_sin_mod(real_subband, qmf_bank,
                            qmf_dec_tables_ptr->esbr_w_16,
                            qmf_dec_tables_ptr->dig_rev_table4_16);

  tcos = qmf_bank->esbr_t_cos;

  for (i = (qmf_bank->usb - qmf_bank->lsb - 1); i >= 0; i--) {
    WORD32 cosh, sinh;
    WORD32 re, im;

    re = *real_subband;
    im = *imag_subband;
    cosh = *tcos++;
    sinh = *tcos++;
    *real_subband++ = (WORD32)((ixheaacd_add64(ixheaacd_mult64(re, cosh),
                                               ixheaacd_mult64(im, sinh))) >>
                               31);
    *imag_subband++ =
        (WORD32)((ixheaacd_sub64_sat(ixheaacd_mult64(im, cosh),
                                     ixheaacd_mult64(re, sinh))) >>
                 31);
  }
}

VOID ixheaacd_esbr_qmfsyn64_winadd(WORD32 *tmp1, WORD32 *tmp2, WORD32 *inp1,
                                   WORD32 *sample_buffer, WORD32 ch_fac) {
  WORD32 k;

  for (k = 0; k < 64; k++) {
    WORD64 syn_out = 0;

    syn_out =
        ixheaacd_add64(syn_out, ixheaacd_mult64(tmp1[0 + k], inp1[k + 0]));
    syn_out =
        ixheaacd_add64(syn_out, ixheaacd_mult64(tmp1[256 + k], inp1[k + 128]));
    syn_out =
        ixheaacd_add64(syn_out, ixheaacd_mult64(tmp1[512 + k], inp1[k + 256]));
    syn_out =
        ixheaacd_add64(syn_out, ixheaacd_mult64(tmp1[768 + k], inp1[k + 384]));
    syn_out =
        ixheaacd_add64(syn_out, ixheaacd_mult64(tmp1[1024 + k], inp1[k + 512]));

    syn_out =
        ixheaacd_add64(syn_out, ixheaacd_mult64(tmp2[128 + k], inp1[k + 64]));
    syn_out =
        ixheaacd_add64(syn_out, ixheaacd_mult64(tmp2[384 + k], inp1[k + 192]));
    syn_out =
        ixheaacd_add64(syn_out, ixheaacd_mult64(tmp2[640 + k], inp1[k + 320]));
    syn_out =
        ixheaacd_add64(syn_out, ixheaacd_mult64(tmp2[896 + k], inp1[k + 448]));
    syn_out =
        ixheaacd_add64(syn_out, ixheaacd_mult64(tmp2[1152 + k], inp1[k + 576]));

    sample_buffer[ch_fac * k] = (WORD32)(syn_out >> 31);
  }
}

VOID ixheaacd_shiftrountine(WORD32 *qmf_real, WORD32 *qmf_imag, WORD32 len,
                            WORD32 common_shift) {
  WORD32 treal, timag;
  WORD32 j;

  if (common_shift < 0) {
    WORD32 cshift = -common_shift;
    cshift = ixheaacd_min32(cshift, 31);
    for (j = len - 1; j >= 0; j--) {
      treal = *qmf_real;
      timag = *qmf_imag;

      treal = (ixheaacd_shr32(treal, cshift));
      timag = (ixheaacd_shr32(timag, cshift));

      *qmf_real++ = treal;
      *qmf_imag++ = timag;
    }
  } else {
    for (j = len - 1; j >= 0; j--) {
      treal = (ixheaacd_shl32_sat(*qmf_real, common_shift));
      timag = (ixheaacd_shl32_sat(*qmf_imag, common_shift));
      *qmf_real++ = treal;
      *qmf_imag++ = timag;
    }
  }
}

VOID ixheaacd_shiftrountine_with_rnd_hq(WORD32 *qmf_real, WORD32 *qmf_imag,
                                        WORD32 *filter_states, WORD32 len,
                                        WORD32 shift) {
  WORD32 *filter_states_rev = filter_states + len;
  WORD32 treal, timag;
  WORD32 j;

  for (j = (len - 1); j >= 0; j -= 2) {
    WORD32 r1, r2, i1, i2;
    i2 = qmf_imag[j];
    r2 = qmf_real[j];
    r1 = *qmf_real++;
    i1 = *qmf_imag++;

    timag = ixheaacd_add32_sat(i1, r1);
    timag = (ixheaacd_shl32_sat(timag, shift));
    filter_states_rev[j] = timag;

    treal = ixheaacd_sub32_sat(i2, r2);
    treal = (ixheaacd_shl32_sat(treal, shift));
    filter_states[j] = treal;

    treal = ixheaacd_sub32_sat(i1, r1);
    treal = (ixheaacd_shl32_sat(treal, shift));
    *filter_states++ = treal;

    timag = ixheaacd_add32_sat(i2, r2);
    timag = (ixheaacd_shl32_sat(timag, shift));
    *filter_states_rev++ = timag;
  }
}

VOID ixheaacd_radix4bfly(const WORD16 *w, WORD32 *x, WORD32 index1,
                         WORD32 index) {
  int i;
  WORD32 l1, l2, h2, fft_jmp;
  WORD32 xt0_0, yt0_0, xt1_0, yt1_0, xt2_0, yt2_0;
  WORD32 xh0_0, xh1_0, xh20_0, xh21_0, xl0_0, xl1_0, xl20_0, xl21_0;
  WORD32 x_0, x_1, x_l1_0, x_l1_1, x_l2_0, x_l2_1;
  WORD32 x_h2_0, x_h2_1;
  WORD16 si10, si20, si30, co10, co20, co30;

  WORD32 mul_1, mul_2, mul_3, mul_4, mul_5, mul_6;
  WORD32 mul_7, mul_8, mul_9, mul_10, mul_11, mul_12;
  WORD32 *x_l1;
  WORD32 *x_l2;
  WORD32 *x_h2;
  const WORD16 *w_ptr = w;
  WORD32 i1;

  h2 = index << 1;
  l1 = index << 2;
  l2 = (index << 2) + (index << 1);

  x_l1 = &(x[l1]);
  x_l2 = &(x[l2]);
  x_h2 = &(x[h2]);

  fft_jmp = 6 * (index);

  for (i1 = 0; i1 < index1; i1++) {
    for (i = 0; i < index; i++) {
      si10 = (*w_ptr++);
      co10 = (*w_ptr++);
      si20 = (*w_ptr++);
      co20 = (*w_ptr++);
      si30 = (*w_ptr++);
      co30 = (*w_ptr++);

      x_0 = x[0];
      x_h2_0 = x[h2];
      x_l1_0 = x[l1];
      x_l2_0 = x[l2];

      xh0_0 = x_0 + x_l1_0;
      xl0_0 = x_0 - x_l1_0;

      xh20_0 = x_h2_0 + x_l2_0;
      xl20_0 = x_h2_0 - x_l2_0;

      x[0] = xh0_0 + xh20_0;
      xt0_0 = xh0_0 - xh20_0;

      x_1 = x[1];
      x_h2_1 = x[h2 + 1];
      x_l1_1 = x[l1 + 1];
      x_l2_1 = x[l2 + 1];

      xh1_0 = x_1 + x_l1_1;
      xl1_0 = x_1 - x_l1_1;

      xh21_0 = x_h2_1 + x_l2_1;
      xl21_0 = x_h2_1 - x_l2_1;

      x[1] = xh1_0 + xh21_0;
      yt0_0 = xh1_0 - xh21_0;

      xt1_0 = xl0_0 + xl21_0;
      xt2_0 = xl0_0 - xl21_0;

      yt2_0 = xl1_0 + xl20_0;
      yt1_0 = xl1_0 - xl20_0;

      mul_11 = ixheaacd_mult32x16in32(xt2_0, co30);
      mul_3 = ixheaacd_mult32x16in32(yt2_0, si30);
      x[l2] = (mul_3 + mul_11) << RADIXSHIFT;

      mul_5 = ixheaacd_mult32x16in32(xt2_0, si30);
      mul_9 = ixheaacd_mult32x16in32(yt2_0, co30);
      x[l2 + 1] = (mul_9 - mul_5) << RADIXSHIFT;

      mul_12 = ixheaacd_mult32x16in32(xt0_0, co20);
      mul_2 = ixheaacd_mult32x16in32(yt0_0, si20);
      x[l1] = (mul_2 + mul_12) << RADIXSHIFT;

      mul_6 = ixheaacd_mult32x16in32(xt0_0, si20);
      mul_8 = ixheaacd_mult32x16in32(yt0_0, co20);
      x[l1 + 1] = (mul_8 - mul_6) << RADIXSHIFT;

      mul_4 = ixheaacd_mult32x16in32(xt1_0, co10);
      mul_1 = ixheaacd_mult32x16in32(yt1_0, si10);
      x[h2] = (mul_1 + mul_4) << RADIXSHIFT;

      mul_10 = ixheaacd_mult32x16in32(xt1_0, si10);
      mul_7 = ixheaacd_mult32x16in32(yt1_0, co10);
      x[h2 + 1] = (mul_7 - mul_10) << RADIXSHIFT;

      x += 2;
    }
    x += fft_jmp;
    w_ptr = w_ptr - fft_jmp;
  }
}
