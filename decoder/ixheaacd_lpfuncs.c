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
#include <ixheaacd_basic_ops32.h>
#include <ixheaacd_basic_ops16.h>
#include <ixheaacd_basic_ops40.h>
#include "ixheaacd_basic_ops.h"
#include "ixheaacd_bitbuffer.h"

#include <ixheaacd_basic_op.h>
#include "ixheaacd_intrinsics.h"

#include "ixheaacd_defines.h"

#include <ixheaacd_aac_rom.h>

#include "ixheaacd_definitions.h"

#include "ixheaacd_error_codes.h"

#include "ixheaacd_pulsedata.h"

#include "ixheaacd_pns.h"
#include "ixheaacd_drc_data_struct.h"

#include "ixheaacd_lt_predict.h"
#include "ixheaacd_channelinfo.h"
#include "ixheaacd_cnst.h"
#include "ixheaacd_drc_dec.h"

#include "ixheaacd_sbrdecoder.h"

#include "ixheaacd_block.h"
#include "ixheaacd_channel.h"

#include "ixheaacd_sbr_payload.h"
#include "ixheaacd_common_rom.h"

#include <ixheaacd_type_def.h>

#include "ixheaacd_sbrdecsettings.h"
#include "ixheaacd_sbr_scale.h"
#include "ixheaacd_env_extr_part.h"
#include <ixheaacd_sbr_rom.h>

#include "ixheaacd_lpp_tran.h"
#include "ixheaacd_hybrid.h"
#include "ixheaacd_ps_dec.h"

#include "ixheaacd_env_extr.h"
#include "ixheaacd_adts.h"
#include "ixheaacd_audioobjtypes.h"
#include "ixheaacd_memory_standards.h"

#include "ixheaacd_latmdemux.h"

#include "ixheaacd_aacdec.h"
#include "ixheaacd_mps_polyphase.h"
#include "ixheaacd_config.h"
#include "ixheaacd_mps_dec.h"

#include "ixheaacd_struct_def.h"

#include "ixheaacd_tns.h"
#include "ixheaacd_aac_imdct.h"

#include "ixheaacd_multichannel.h"
#include "ixheaacd_function_selector.h"

static PLATFORM_INLINE WORD32 ixheaacd_shr32_drc(WORD32 a, WORD32 b) {
  WORD32 out_val;

  b = ((UWORD32)(b << 24) >> 24);
  if (b >= 31) {
    if (a < 0)
      out_val = -1;
    else
      out_val = 0;
  } else {
    a += (1 << (b - 1));
    out_val = (WORD32)a >> b;
  }

  return out_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_mult32x16in32_drc(WORD32 a, WORD16 b) {
  WORD32 result;
  WORD64 temp_result;

  temp_result = (WORD64)a * (WORD64)b;

  if (temp_result < (WORD64)MIN_32)
    result = MIN_32;

  else if (temp_result > (WORD64)MAX_32)
    result = MAX_32;

  else
    result = (WORD32)(temp_result);

  return (result);
}

VOID ixheaacd_process_win_seq(WORD32 *coef, WORD32 *prev, WORD16 *out,
                              const WORD16 *window_long,
                              const WORD16 *window_short, WORD16 q_shift,
                              WORD16 ch_fac, WORD16 flag) {
  WORD32 i, accu;
  WORD32 *coef_1;
  const WORD16 *temp_win_sh, *temp_win_long;
  WORD16 *out1, *out2;
  WORD32 *temp_prev;

  if (flag == 1) {
    for (i = 0; i < SIZE07; i++) {
      WORD32 temp1 = ixheaacd_shl32_dir_sat_limit(
          ixheaacd_mult32x16in32(coef[SIZE08 + i], window_long[2 * i]),
          (q_shift + 1));

      accu = temp1 + ((WORD32)prev[i] << 16);
      out[ch_fac * i] = ixheaacd_round16(accu << 1);

      accu = ixheaacd_shl32_dir_sat_limit(
          ixheaacd_mult32x16in32(-(coef[SIZE15 - 1 - i]),
                                 window_long[2 * (SIZE07 - i) - 1]),
          q_shift);
      out[ch_fac * (i + SIZE09)] = ixheaacd_round16(accu << 2);
    }

    temp_win_sh = &(window_short[0]);
    coef_1 = &(coef[SIZE15]);
    temp_win_long = &(window_long[SIZE14]);
    temp_prev = &(prev[SIZE08 - 1]);
    out1 = &(out[ch_fac * (SIZE07)]);
    out2 = &(out[ch_fac * (SIZE09 - 1)]);

  } else {
    for (i = 0; i < SIZE07; i++) {
      accu = ixheaacd_mult32x16in32_drc(
          prev[SIZE08 - 1 - i], ixheaacd_negate16(window_long[2 * i + 1]));

      out[ch_fac * i] = ixheaacd_round16(accu << 2);

      accu = (ixheaacd_shl32_dir_sat_limit(-(coef[SIZE15 - 1 - i]),
                                           (q_shift - 1)) -
              ixheaacd_mult32x16in32_drc(prev[i + SIZE01],
                                         window_long[2 * SIZE07 - 2 - 2 * i]));

      out[ch_fac * (SIZE09 + i)] = ixheaacd_round16(accu << 2);
    }

    temp_win_sh = &(window_long[SIZE14]);
    coef_1 = &(coef[SIZE15]);
    temp_win_long = &(window_short[0]);
    temp_prev = &(prev[SIZE01 - 1]);
    out1 = &(out[ch_fac * (SIZE07)]);
    out2 = &(out[ch_fac * (SIZE09 - 1)]);
  }

  for (i = SIZE01 - 1; i >= 0; i--) {
    WORD32 temp_coef = *coef_1++;
    WORD16 win1 = *temp_win_long++;
    WORD16 win2 = *temp_win_long++;
    WORD32 prev1 = *temp_prev--;
    WORD16 win4 = *temp_win_sh++;
    WORD16 win3 = *temp_win_sh++;
    accu = ixheaacd_shl32_dir_sat_limit(ixheaacd_mult32x16in32(temp_coef, win1),
                                        q_shift) -
           ixheaacd_mult32x16in32_drc(prev1, win3);
    *out1 = ixheaacd_round16(accu << 2);
    out1 += ch_fac;

    accu = (ixheaacd_shl32_dir_sat_limit(
                ixheaacd_mult32x16in32(ixheaacd_negate32_sat(temp_coef), win2),
                q_shift) -
            ixheaacd_mult32x16in32_drc(prev1, win4));
    *out2 = ixheaacd_round16(accu << 2);
    out2 -= ch_fac;
  }
}

static PLATFORM_INLINE VOID ixheaacd_long_short_win_process(
    WORD32 *current, WORD32 *prev, WORD16 *out, const WORD16 *short_window,
    const WORD16 *long_window_prev, WORD16 q_shift, WORD16 ch_fac,
    WORD32 flag) {
  WORD i;
  WORD32 accu;
  WORD32 *current_tmp1 = &(current[(SIZE03 - 1)]);
  WORD32 *current_tmp2 = &(current[-SIZE01]);
  const WORD16 *short_ptr = &(short_window[SIZE02 - 1]);

  for (i = SIZE01 - 1; i >= 0; i--) {
    WORD32 tmp1_cur = *current_tmp1--;
    WORD32 tmp2_cur = *current_tmp2++;
    WORD16 short1 = *short_ptr--;
    WORD16 short2 = *short_ptr--;
    accu =
        (ixheaacd_shl32_dir_sat_limit(
             (ixheaacd_mult32x16in32(tmp1_cur, short2) -
              ixheaacd_mult32x16in32(tmp2_cur, short1)),
             q_shift) -
         ixheaacd_mult32x16in32_drc(prev[i], long_window_prev[0 - 2 - 2 * i]));
    out[ch_fac * (0 + i)] = ixheaacd_round16(accu << 2);

    if (flag) {
      accu = (ixheaacd_shl32_dir_sat_limit(
                  (ixheaacd_mult32x16in32(-(tmp1_cur), short1) -
                   ixheaacd_mult32x16in32(tmp2_cur, short2)),
                  q_shift) -
              ixheaacd_mult32x16in32_drc(
                  prev[SIZE02 - 1 - i], long_window_prev[-2 * SIZE02 + 2 * i]));
      out[ch_fac * (SIZE02 - 1 - i)] = ixheaacd_round16(accu << 2);
    }
  }
}

VOID ixheaacd_long_short_win_seq(WORD32 *current, WORD32 *prev, WORD16 *out,
                                 const WORD16 *short_window,
                                 const WORD16 *short_window_prev,
                                 const WORD16 *long_window_prev, WORD16 q_shift,
                                 WORD16 ch_fac) {
  WORD32 i, flag;
  WORD32 accu;
  for (i = 0; i < SIZE07; i++) {
    accu = ixheaacd_mult32x16in32_drc(
        prev[SIZE08 - 1 - i], ixheaacd_negate16(long_window_prev[2 * i + 1]));
    out[ch_fac * i] = ixheaacd_round16(accu << 2);
  }

  for (i = 0; i < SIZE01; i++) {
    accu =
        (ixheaacd_shl32_dir_sat_limit(
             ixheaacd_mult32x16in32(current[SIZE01 + i],
                                    short_window_prev[2 * i]),
             q_shift) -
         ixheaacd_mult32x16in32_drc(prev[SIZE01 - 1 - i],
                                    long_window_prev[2 * SIZE07 + 1 + 2 * i]));
    out[ch_fac * (SIZE07 + i)] = ixheaacd_round16(accu << 2);
  }

  for (i = 0; i < SIZE01; i++) {
    accu = (ixheaacd_shl32_dir_sat_limit(
                ixheaacd_mult32x16in32(-(current[SIZE02 - 1 - i]),
                                       short_window_prev[SIZE02 - 2 * i - 1]),
                q_shift) -
            ixheaacd_mult32x16in32_drc(prev[i],
                                       long_window_prev[SIZE16 - 2 - (2 * i)]));
    out[ch_fac * (SIZE08 + i)] = ixheaacd_round16(accu << 2);
  }

  flag = 1;
  for (i = 0; i < 4; i++) {
    WORD32 inc = i * SIZE02;

    if (i == 3) {
      flag = 0;
    }

    ixheaacd_long_short_win_process(&current[SIZE01 + inc], &prev[SIZE01 + inc],
                                    &out[ch_fac * (SIZE09 + inc)], short_window,
                                    &long_window_prev[2 * (SIZE07 - inc)],
                                    q_shift, ch_fac, flag);
  }

  for (i = 0; i < SIZE01; i++) {
    accu = (ixheaacd_mult32x16in32(-(current[SIZE10 - 1 - i]),
                                   short_window[SIZE02 - 2 * i - 1]) -
            ixheaacd_mult32x16in32(current[SIZE06 + i],
                                   short_window[SIZE02 - 2 * i - 2]));
    prev[i] =
        ixheaacd_round16(ixheaacd_shl32_dir_sat_limit(accu, (q_shift + 1)));
  }
}

VOID ixheaacd_nolap1_32(WORD32 *coef,

                        WORD32 *out,

                        WORD16 q_shift, WORD16 ch_fac) {
  WORD32 i;

  for (i = 0; i < SIZE07; i++) {
    out[ch_fac * i] = ixheaacd_shr32_drc(
        ixheaacd_negate32_sat(coef[SIZE07 - 1 - i]), 16 - q_shift);
  }
}

VOID ixheaacd_neg_shift_spec_dec(WORD32 *coef, WORD16 *out, WORD16 q_shift,
                                 WORD16 ch_fac) {
  WORD32 i;

  for (i = 0; i < SIZE07; i++) {
    out[ch_fac * i] = ixheaacd_round16(ixheaacd_shl32_dir_sat_limit(
        ixheaacd_negate32_sat(coef[SIZE07 - 1 - i]), q_shift));
  }
}

VOID ixheaacd_spec_to_overlapbuf_dec(WORD32 *ptr_overlap_buf,
                                     WORD32 *ptr_spec_coeff, WORD32 q_shift,
                                     WORD32 size) {
  WORD32 i;
  for (i = 0; i < size; i++) {
    ptr_overlap_buf[i] = ixheaacd_shr32_drc(ptr_spec_coeff[i], 16 - q_shift);
  }
}

VOID ixheaacd_overlap_buf_out_dec(WORD16 *out_samples, WORD32 *ptr_overlap_buf,
                                  WORD32 size, const WORD16 ch_fac) {
  WORD32 i;

  for (i = 0; i < size; i++) {
    out_samples[ch_fac * i] = ixheaacd_shl16_sat((WORD16)ptr_overlap_buf[i], 1);
  }
}

VOID ixheaacd_overlap_out_copy_dec(WORD16 *out_samples, WORD32 *ptr_overlap_buf,
                                   WORD32 *ptr_overlap_buf1,
                                   const WORD16 ch_fac) {
  WORD32 i;

  for (i = 0; i < SIZE01; i++) {
    out_samples[ch_fac * i] = ixheaacd_shl16_sat((WORD16)ptr_overlap_buf[i], 1);
    ptr_overlap_buf[i] = ptr_overlap_buf1[i];
  }
}

VOID ixheaacd_imdct_process(ia_aac_dec_overlap_info *ptr_aac_dec_overlap_info,
                            WORD32 *ptr_spec_coeff,
                            ia_ics_info_struct *ptr_ics_info,
                            WORD16 out_samples[], const WORD16 ch_fac,
                            WORD32 *scratch,
                            ia_aac_dec_tables_struct *ptr_aac_tables,
                            WORD32 object_type) {
  WORD32 *ptr_overlap_buf;
  const WORD16 *ptr_long_window;
  const WORD16 *ptr_short_window;

  ptr_overlap_buf = ptr_aac_dec_overlap_info->ptr_overlap_buf;
  ptr_long_window =
      ptr_aac_dec_overlap_info
          ->ptr_long_window[(WORD32)ptr_aac_dec_overlap_info->window_shape];
  ptr_short_window =
      ptr_aac_dec_overlap_info
          ->ptr_short_window[(WORD32)ptr_aac_dec_overlap_info->window_shape];

  if (ptr_ics_info->window_sequence != EIGHT_SHORT_SEQUENCE) {
    WORD16 q_shift;
    WORD32 expo, imdct_scale;

    if ((512 == ptr_ics_info->frame_length) ||
        (480 == ptr_ics_info->frame_length)) {
      if (512 == ptr_ics_info->frame_length) {
        WORD32 *ld_cos_sin_ptr =
            (WORD32 *)ptr_aac_tables->pstr_imdct_tables->cosine_array_1024;

        ixheaacd_inverse_transform_512(
            ptr_spec_coeff, scratch, &imdct_scale, ld_cos_sin_ptr,
            ptr_aac_tables->pstr_imdct_tables, object_type);

      } else {
        ixheaacd_mdct_480_ld(ptr_spec_coeff, scratch, &imdct_scale, 0,
                             ptr_aac_tables->pstr_imdct_tables, object_type);
      }

      if (object_type == AOT_ER_AAC_ELD) {
        int i, N = (ptr_ics_info->frame_length << 1);

        for (i = 0; i < N / 2; i++) {
          ptr_spec_coeff[i] = -ptr_spec_coeff[i + N];
          ptr_spec_coeff[i + N + N / 2] = -ptr_spec_coeff[i + N / 2];
        }
      }
    } else

    {
      expo = (*ixheaacd_calc_max_spectral_line)(ptr_spec_coeff, 1024) - 1;

      expo = 8 - expo;

      imdct_scale = ixheaacd_inverse_transform(
          ptr_spec_coeff, scratch, ptr_aac_tables->pstr_imdct_tables, expo,
          1024);
    }

    q_shift = (31 + imdct_scale) + (-1 - 16 - 9);

    switch (ptr_ics_info->window_sequence) {
      case ONLY_LONG_SEQUENCE:

        switch (ptr_aac_dec_overlap_info->window_sequence) {
          case ONLY_LONG_SEQUENCE:
          case LONG_STOP_SEQUENCE:

            if (1024 == ptr_ics_info->frame_length) {
              (*ixheaacd_post_twid_overlap_add)(
                  out_samples, ptr_spec_coeff,
                  ptr_aac_tables->pstr_imdct_tables, 1024, ptr_overlap_buf,
                  q_shift, ptr_long_window, ch_fac);
            }
            if ((512 == ptr_ics_info->frame_length) ||
                (480 == ptr_ics_info->frame_length)) {
              if (object_type != AOT_ER_AAC_ELD) {
                if (512 == ptr_ics_info->frame_length) {
                  ixheaacd_lap1_512_480(ptr_spec_coeff, ptr_overlap_buf,
                                        out_samples, ptr_long_window, q_shift,
                                        SIZE04, ch_fac);
                  ixheaacd_spec_to_overlapbuf(ptr_overlap_buf, ptr_spec_coeff,
                                              q_shift, SIZE04);
                } else if (480 == ptr_ics_info->frame_length) {
                  ixheaacd_lap1_512_480(ptr_spec_coeff, ptr_overlap_buf,
                                        out_samples, ptr_long_window, q_shift,
                                        240, ch_fac);
                  ixheaacd_spec_to_overlapbuf(ptr_overlap_buf, ptr_spec_coeff,
                                              q_shift, 240);
                }
              } else {
                ixheaacd_eld_dec_windowing(
                    ptr_spec_coeff, ptr_long_window, ptr_ics_info->frame_length,
                    q_shift, ptr_overlap_buf, ch_fac, out_samples);
              }
            }
            break;

          case LONG_START_SEQUENCE:
          case EIGHT_SHORT_SEQUENCE:
            if (1024 == ptr_ics_info->frame_length) {
              (*ixheaacd_post_twiddle)(scratch, ptr_spec_coeff,
                                       ptr_aac_tables->pstr_imdct_tables, 1024);
            }

            ixheaacd_process_win_seq(scratch, ptr_overlap_buf, out_samples,
                                     ptr_long_window, ptr_short_window, q_shift,
                                     ch_fac, 1);

            if (512 == ptr_ics_info->frame_length) {
              ixheaacd_spec_to_overlapbuf(ptr_overlap_buf, ptr_spec_coeff,
                                          q_shift, SIZE04);
            } else if (480 == ptr_ics_info->frame_length) {
              ixheaacd_spec_to_overlapbuf(ptr_overlap_buf, ptr_spec_coeff,
                                          q_shift, 240);
            } else {
              (*ixheaacd_spec_to_overlapbuf)(ptr_overlap_buf, scratch, q_shift,
                                             SIZE08);
            }
            break;
        }

        break;

      case LONG_START_SEQUENCE:
        if (1024 == ptr_ics_info->frame_length) {
          (*ixheaacd_post_twiddle)(scratch, ptr_spec_coeff,
                                   ptr_aac_tables->pstr_imdct_tables, 1024);
        }
        switch (ptr_aac_dec_overlap_info->window_sequence) {
          case ONLY_LONG_SEQUENCE:
          case LONG_STOP_SEQUENCE:

            (*ixheaacd_over_lap_add1)(scratch, ptr_overlap_buf, out_samples,
                                      ptr_long_window, q_shift, SIZE08, ch_fac);

            break;

          case LONG_START_SEQUENCE:
          case EIGHT_SHORT_SEQUENCE:

            ixheaacd_process_win_seq(scratch, ptr_overlap_buf, out_samples,
                                     ptr_long_window, ptr_short_window, q_shift,
                                     ch_fac, 1);

            break;
        }

        ixheaacd_nolap1_32(&scratch[SIZE01], ptr_overlap_buf, q_shift, 1);

        (*ixheaacd_spec_to_overlapbuf)(&ptr_overlap_buf[SIZE07], scratch,
                                       q_shift, SIZE01);

        break;

      case LONG_STOP_SEQUENCE:
        if (1024 == ptr_ics_info->frame_length) {
          (*ixheaacd_post_twiddle)(scratch, ptr_spec_coeff,
                                   ptr_aac_tables->pstr_imdct_tables, 1024);
        }

        switch (ptr_aac_dec_overlap_info->window_sequence) {
          case EIGHT_SHORT_SEQUENCE:
          case LONG_START_SEQUENCE:

            (*ixheaacd_overlap_buf_out)(out_samples, ptr_overlap_buf, SIZE07,
                                        ch_fac);

            (*ixheaacd_over_lap_add1)(
                &scratch[SIZE14], &ptr_overlap_buf[SIZE07],
                &out_samples[ch_fac * (SIZE07)], ptr_short_window, q_shift,
                SIZE01, ch_fac);

            {
              WORD16 q_shift1 = q_shift + 1;
              (*ixheaacd_neg_shift_spec)(&scratch[SIZE08],
                                         &out_samples[ch_fac * SIZE09],
                                         q_shift1, ch_fac);
            }

            break;
          case ONLY_LONG_SEQUENCE:
          case LONG_STOP_SEQUENCE:

            ixheaacd_process_win_seq(scratch, ptr_overlap_buf, out_samples,
                                     ptr_long_window, ptr_short_window, q_shift,
                                     ch_fac, 0);
            break;
        }

        (*ixheaacd_spec_to_overlapbuf)(ptr_overlap_buf, scratch, q_shift,
                                       SIZE08);

        break;
    }

  } else {
    WORD16 q_shift, max_scale;
    WORD32 imdct_scale[8], i;
    const WORD16 *short_window;

    short_window = ptr_aac_dec_overlap_info
                       ->ptr_short_window[(WORD32)ptr_ics_info->window_shape];

    {
      WORD32 expo;

      expo = (*ixheaacd_calc_max_spectral_line)(ptr_spec_coeff, 1024) - 1;

      expo = 5 - expo;

      for (i = 0; i < MAX_WINDOWS; i++) {
        imdct_scale[i] = ixheaacd_inverse_transform(
            &ptr_spec_coeff[i * SIZE02], &scratch[i * SIZE02],
            ptr_aac_tables->pstr_imdct_tables, expo, 128);

        (*ixheaacd_post_twiddle)(&scratch[i * SIZE02],
                                 &ptr_spec_coeff[i * SIZE02],
                                 ptr_aac_tables->pstr_imdct_tables, 128);
      }
    }

    max_scale = 31 + imdct_scale[0];

    q_shift = max_scale + (-16 - 6 - 1);

    switch (ptr_aac_dec_overlap_info->window_sequence) {
      WORD32 overlap_buf_loc[SIZE01];

      case EIGHT_SHORT_SEQUENCE:
      case LONG_START_SEQUENCE:

        (*ixheaacd_overlap_buf_out)(out_samples, ptr_overlap_buf, SIZE07,
                                    ch_fac);

        (*ixheaacd_over_lap_add1)(&scratch[0], &ptr_overlap_buf[SIZE07],
                                  &out_samples[ch_fac * SIZE07],
                                  ptr_short_window, q_shift, SIZE01, ch_fac);

        for (i = 0; i < 3; i++) {
          WORD32 inc = (i * SIZE02);
          (*ixheaacd_spec_to_overlapbuf)(overlap_buf_loc, &scratch[inc],
                                         q_shift, SIZE01);

          (*ixheaacd_over_lap_add1)(&scratch[SIZE02 + inc], overlap_buf_loc,
                                    &out_samples[ch_fac * (SIZE09 + inc)],
                                    short_window, q_shift, SIZE01, ch_fac);
        }

        (*ixheaacd_over_lap_add2)(&scratch[SIZE08], &scratch[SIZE06],
                                  ptr_overlap_buf, short_window, q_shift,
                                  SIZE01, 1);

        (*ixheaacd_overlap_out_copy)(&out_samples[ch_fac * SIZE15],
                                     ptr_overlap_buf, &ptr_overlap_buf[SIZE01],
                                     ch_fac);

        break;

      case ONLY_LONG_SEQUENCE:
      case LONG_STOP_SEQUENCE:

        ixheaacd_long_short_win_seq(scratch, ptr_overlap_buf, out_samples,
                                    short_window, ptr_short_window,
                                    ptr_long_window, q_shift, ch_fac);

        break;
    }

    for (i = 0; i < 3; i++) {
      WORD32 inc = (i * SIZE02);
      (*ixheaacd_over_lap_add2)(&scratch[SIZE10 + inc], &scratch[SIZE08 + inc],
                                &ptr_overlap_buf[SIZE01 + inc], short_window,
                                q_shift, SIZE01, 1);
    }

    (*ixheaacd_spec_to_overlapbuf)(&ptr_overlap_buf[SIZE07], &scratch[SIZE14],
                                   q_shift, SIZE01);
  }

  ptr_aac_dec_overlap_info->window_shape = ptr_ics_info->window_shape;
  ptr_aac_dec_overlap_info->window_sequence = ptr_ics_info->window_sequence;
}
void ixheaacd_eld_dec_windowing(WORD32 *ptr_spect_coeff, const WORD16 *p_win,
                                WORD32 framesize, WORD16 q_shift,
                                WORD32 *p_overlap_buffer, const WORD16 stride,
                                WORD16 *out_samples)

{
  int i = 0;
  int loop_size;
  WORD32 *ptr_z = ptr_spect_coeff;

  WORD32 *ptr_out, *p_out2;
  WORD32 *p_overlap_buffer32 = (WORD32 *)p_overlap_buffer;
  WORD32 delay = framesize >> 2;

  ptr_z = ptr_spect_coeff + delay;
  p_win += delay;
  ptr_out = p_overlap_buffer32;

  q_shift = q_shift + 2;

  if (q_shift >= 0) {
    for (i = (delay)-1; i >= 0; i--) {
      WORD32 win_op;
      WORD32 win_ovadd_op;
      WORD16 win_val;

      win_val = *p_win++;

      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));

      win_ovadd_op =
          ixheaacd_add32(ixheaacd_shl32(win_op, q_shift), *ptr_out++);

      *out_samples = ixheaacd_round16(ixheaacd_shl32_sat(win_ovadd_op, 1));
      out_samples += stride;

      win_val = *p_win++;

      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));

      win_ovadd_op =
          ixheaacd_add32(ixheaacd_shl32(win_op, q_shift), *ptr_out++);

      *out_samples = ixheaacd_round16(ixheaacd_shl32_sat(win_ovadd_op, 1));
      out_samples += stride;
      win_val = *p_win++;

      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));

      win_ovadd_op =
          ixheaacd_add32(ixheaacd_shl32(win_op, q_shift), *ptr_out++);

      *out_samples = ixheaacd_round16(ixheaacd_shl32_sat(win_ovadd_op, 1));
      out_samples += stride;

      win_val = *p_win++;

      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));

      win_ovadd_op =
          ixheaacd_add32(ixheaacd_shl32(win_op, q_shift), *ptr_out++);

      *out_samples = ixheaacd_round16(ixheaacd_shl32_sat(win_ovadd_op, 1));
      out_samples += stride;
    }

    p_out2 = p_overlap_buffer32;
    loop_size = (((framesize * 3) - framesize) >> 2) - 1;

    for (i = loop_size; i >= 0; i--) {
      WORD32 win_op;
      WORD16 win_val;
      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_add32(ixheaacd_shl32(win_op, q_shift), *ptr_out++);

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_add32(ixheaacd_shl32(win_op, q_shift), *ptr_out++);

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_add32(ixheaacd_shl32(win_op, q_shift), *ptr_out++);

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_add32(ixheaacd_shl32(win_op, q_shift), *ptr_out++);
    }

    loop_size = ((((framesize << 2) - delay) - (framesize * 3)) >> 2) - 1;
    for (i = loop_size; i >= 0; i--) {
      WORD32 win_op;
      WORD16 win_val;

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_shl32(win_op, q_shift);

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_shl32(win_op, q_shift);

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_shl32(win_op, q_shift);

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_shl32(win_op, q_shift);
    }
  } else {
    q_shift = -q_shift;

    for (i = (delay)-1; i >= 0; i--) {
      WORD32 win_op;
      WORD32 win_ovadd_op;
      WORD16 win_val;

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));

      win_ovadd_op =
          ixheaacd_add32(ixheaacd_shr32(win_op, q_shift), *ptr_out++);

      *out_samples = ixheaacd_round16(ixheaacd_shl32(win_ovadd_op, 1));
      out_samples += stride;

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));

      win_ovadd_op =
          ixheaacd_add32(ixheaacd_shr32(win_op, q_shift), *ptr_out++);

      *out_samples = ixheaacd_round16(ixheaacd_shl32(win_ovadd_op, 1));
      out_samples += stride;

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));

      win_ovadd_op =
          ixheaacd_add32(ixheaacd_shr32(win_op, q_shift), *ptr_out++);

      *out_samples = ixheaacd_round16(ixheaacd_shl32(win_ovadd_op, 1));
      out_samples += stride;

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));

      win_ovadd_op =
          ixheaacd_add32(ixheaacd_shr32(win_op, q_shift), *ptr_out++);

      *out_samples = ixheaacd_round16(ixheaacd_shl32(win_ovadd_op, 1));
      out_samples += stride;
    }

    p_out2 = p_overlap_buffer32;
    loop_size = (((framesize * 3) - framesize) >> 2) - 1;

    for (i = loop_size; i >= 0; i--) {
      WORD32 win_op;
      WORD16 win_val;
      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_add32(ixheaacd_shr32(win_op, q_shift), *ptr_out++);

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_add32(ixheaacd_shr32(win_op, q_shift), *ptr_out++);

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_add32(ixheaacd_shr32(win_op, q_shift), *ptr_out++);

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_add32(ixheaacd_shr32(win_op, q_shift), *ptr_out++);
    }
    loop_size = ((((framesize << 2) - delay) - (framesize * 3)) >> 2) - 1;
    for (i = loop_size; i >= 0; i--) {
      WORD32 win_op;
      WORD16 win_val;
      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_shr32(win_op, q_shift);

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_shr32(win_op, q_shift);

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_shr32(win_op, q_shift);

      win_val = *p_win++;
      win_op = ixheaacd_mult32x16in32(*ptr_z++, (win_val));
      *p_out2++ = ixheaacd_shr32(win_op, q_shift);
    }
  }
}