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
#include "ixheaacd_sbr_common.h"
#include <ixheaacd_type_def.h>

#include "ixheaacd_constants.h"
#include <ixheaacd_basic_ops32.h>
#include <ixheaacd_basic_ops16.h>
#include <ixheaacd_basic_ops40.h>
#include "ixheaacd_basic_ops.h"

#include "ixheaacd_defines.h"
#include "ixheaacd_common_rom.h"
#include "ixheaacd_basic_funcs.h"
#include <ixheaacd_aac_rom.h>
#include "ixheaacd_aac_imdct.h"
#include "ixheaacd_intrinsics.h"

#include <ixheaacd_basic_op.h>
#include "ixheaacd_function_selector.h"

#include "ixheaacd_audioobjtypes.h"
#include "ixheaacd_tns.h"

#define DIG_REV(i, m, j)                                      \
  do {                                                        \
    unsigned _ = (i);                                         \
    _ = ((_ & 0x33333333) << 2) | ((_ & ~0x33333333) >> 2);   \
    _ = ((_ & 0x0F0F0F0F) << 4) | ((_ & ~0x0F0F0F0F) >> 4);   \
    _ = ((_ & 0x00FF00FF) << 8) | ((_ & ~0x00FF00FF) >> 8);   \
    _ = ((_ & 0x0000FFFF) << 16) | ((_ & ~0x0000FFFF) >> 16); \
    (j) = _ >> (m);                                           \
  } while (0)

#define MPYHIRC(x, y)                                                         \
                                                                              \
  (((WORD32)((short)(x >> 16) * (unsigned short)(y & 0x0000FFFF) + 0x4000) >> \
    15) +                                                                     \
   ((WORD32)((short)(x >> 16) * (short)((y) >> 16)) << 1))

#define MPYLUHS(x, y) \
  ((WORD32)((unsigned short)(x & 0x0000FFFF) * (short)(y >> 16)))

#define MDCT_LEN 480
#define FFT15X2 30
#define MDCT_LEN_BY2 240
#define FFT5 5
#define FFT16 16
#define FFT15 15
#define FFT16X2 32

WORD32 ixheaacd_fft5out[FFT15X2];

static PLATFORM_INLINE WORD32 ixheaacd_shr32_drc(WORD32 a, WORD32 b) {
  WORD32 out_val;

  b = ((UWORD32)(b << 24) >> 24);
  if (b >= 31) {
    if (a < 0)
      out_val = -1;
    else
      out_val = 0;
  } else {
    a = ixheaacd_add32_sat(a, (1 << (b - 1)));
    out_val = (WORD32)a >> b;
  }

  return out_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_mult32x16hin32_drc(WORD32 a, WORD32 b) {
  WORD32 result;
  WORD64 temp_result;
  temp_result = (WORD64)a * (WORD64)(b >> 16);
  result = (WORD32)(temp_result >> 16);
  return (result);
}

static PLATFORM_INLINE WORD32 ixheaacd_mult32x16lin32(WORD32 a, WORD32 b) {
  WORD32 result;
  WORD64 temp_result;
  temp_result = (WORD64)a * (WORD64)(((b & 0xFFFF) << 16) >> 16);
  result = (WORD32)(temp_result >> 16);
  return (result);
}

static PLATFORM_INLINE WORD32 ixheaacd_mac32x16lin32(WORD32 a, WORD32 b,
                                                     WORD32 c) {
  WORD32 result;
  result = a + ixheaacd_mult32x16lin32(b, c);
  return (result);
}

static PLATFORM_INLINE WORD32 ixheaacd_mult32x16lin32_drc(WORD32 a, WORD32 b) {
  WORD32 result;
  WORD64 temp_result;
  temp_result = (WORD64)a * (WORD64)(((b & 0xFFFF) << 16) >> 16);
  if (temp_result < (WORD64)MIN_32)
    result = MIN_32;
  else if (temp_result > (WORD64)MAX_32)
    result = MAX_32;
  else
    result = (WORD32)(temp_result);
  return (result);
}

WORD16 ixheaacd_neg_expo_inc_dec(WORD16 neg_expo) { return (neg_expo + 2); }

WORD16 ixheaacd_neg_expo_inc_arm(WORD16 neg_expo) { return (neg_expo + 3); }

VOID ixheaacd_pretwiddle_compute_dec(
    WORD32 *spec_data1, WORD32 *spec_data2, WORD32 *out_ptr,
    ia_aac_dec_imdct_tables_struct *ptr_imdct_tables, WORD npoints4,
    WORD32 neg_expo) {
  WORD32 i;
  WORD32 tempr, tempi;
  WORD32 tempr1, tempi1;
  WORD32 npoints2 = npoints4 * 2;
  WORD32 *out_ptr1 = out_ptr + (npoints2 << 1) - 1;
  const WORD16 *cos_sin_ptr = ptr_imdct_tables->cosine_array_2048_256;

  WORD16 cos = 0, cos1 = 0, sin = 0, sin1 = 0;
  if (neg_expo < 0) {
    neg_expo = -neg_expo;
    if (npoints4 == 256) {
      cos = *cos_sin_ptr++;
      sin = *cos_sin_ptr++;
    } else if (npoints4 == 32) {
      cos = *cos_sin_ptr++;
      sin = *cos_sin_ptr;
      cos_sin_ptr += 15;
    }
    tempr = *spec_data1++;
    tempi = *spec_data2--;

    *out_ptr =
        ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr, cos), tempi, sin);

    *out_ptr = ixheaacd_shl32(*out_ptr, neg_expo);
    out_ptr++;

    *out_ptr = ixheaacd_sub32(ixheaacd_mult32x16in32(tempi, cos),
                              ixheaacd_mult32x16in32(tempr, sin));

    *out_ptr = ixheaacd_shl32(*out_ptr, neg_expo);
    out_ptr++;

    for (i = 0; i < npoints4 - 1; i++) {
      if (npoints4 == 256) {
        sin = *cos_sin_ptr++;
        cos = *cos_sin_ptr++;
      } else if (npoints4 == 32) {
        sin = *cos_sin_ptr++;
        cos = *cos_sin_ptr;
        cos_sin_ptr += 15;
      }

      tempi1 = *spec_data1++;
      tempr = *spec_data1++;
      tempr1 = *spec_data2--;
      tempi = *spec_data2--;

      *out_ptr1 = ixheaacd_sub32(ixheaacd_mult32x16in32(tempi1, cos),
                                 ixheaacd_mult32x16in32(tempr1, sin));

      *out_ptr1 = ixheaacd_shl32(*out_ptr1, neg_expo);
      out_ptr1--;

      *out_ptr1 = ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr1, cos),
                                        tempi1, sin);
      *out_ptr1 = ixheaacd_shl32(*out_ptr1, neg_expo);
      out_ptr1--;

      *out_ptr =
          ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr, sin), tempi, cos);
      *out_ptr = ixheaacd_shl32(*out_ptr, neg_expo);
      out_ptr++;

      *out_ptr = ixheaacd_sub32(ixheaacd_mult32x16in32(tempi, sin),
                                ixheaacd_mult32x16in32(tempr, cos));
      *out_ptr = ixheaacd_shl32(*out_ptr, neg_expo);
      out_ptr++;
    }
    cos1 = *cos_sin_ptr++;
    sin1 = *cos_sin_ptr;

    tempr1 = *spec_data2;
    tempi1 = *spec_data1;

    *out_ptr1 = ixheaacd_sub32(ixheaacd_mult32x16in32(tempi1, cos1),
                               ixheaacd_mult32x16in32(tempr1, sin1));
    *out_ptr1 = ixheaacd_shl32(*out_ptr1, neg_expo);
    out_ptr1--;

    *out_ptr1 = ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr1, cos1),
                                      tempi1, sin1);
    *out_ptr1 = ixheaacd_shl32(*out_ptr1, neg_expo);
    out_ptr1--;

  } else {
    if (npoints4 == 256) {
      cos = *cos_sin_ptr++;
      sin = *cos_sin_ptr++;

    } else if (npoints4 == 32) {
      cos = *cos_sin_ptr++;
      sin = *cos_sin_ptr;
      cos_sin_ptr += 15;
    }
    tempr = *spec_data1++;
    tempi = *spec_data2--;

    *out_ptr =
        ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr, cos), tempi, sin);
    *out_ptr = ixheaacd_shr32(*out_ptr, neg_expo);
    out_ptr++;

    *out_ptr = ixheaacd_sub32(ixheaacd_mult32x16in32(tempi, cos),
                              ixheaacd_mult32x16in32(tempr, sin));

    *out_ptr = ixheaacd_shr32(*out_ptr, neg_expo);
    out_ptr++;

    for (i = 0; i < npoints4 - 1; i++) {
      if (npoints4 == 256) {
        sin = *cos_sin_ptr++;
        cos = *cos_sin_ptr++;
      } else if (npoints4 == 32) {
        sin = *cos_sin_ptr++;
        cos = *cos_sin_ptr;
        cos_sin_ptr += 15;
      }

      tempi1 = *spec_data1++;
      tempr = *spec_data1++;
      tempr1 = *spec_data2--;
      tempi = *spec_data2--;

      *out_ptr1 = ixheaacd_sub32(ixheaacd_mult32x16in32(tempi1, cos),
                                 ixheaacd_mult32x16in32(tempr1, sin));
      *out_ptr1 = ixheaacd_shr32(*out_ptr1, neg_expo);
      out_ptr1--;

      *out_ptr1 = ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr1, cos),
                                        tempi1, sin);
      *out_ptr1 = ixheaacd_shr32(*out_ptr1, neg_expo);
      out_ptr1--;

      *out_ptr =
          ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr, sin), tempi, cos);
      *out_ptr = ixheaacd_shr32(*out_ptr, neg_expo);
      out_ptr++;

      *out_ptr = ixheaacd_sub32(ixheaacd_mult32x16in32(tempi, sin),
                                ixheaacd_mult32x16in32(tempr, cos));
      *out_ptr = ixheaacd_shr32(*out_ptr, neg_expo);
      out_ptr++;
    }
    cos1 = *cos_sin_ptr++;
    sin1 = *cos_sin_ptr;

    tempr1 = *spec_data2;
    tempi1 = *spec_data1;

    *out_ptr1 = ixheaacd_sub32(ixheaacd_mult32x16in32(tempi1, cos1),
                               ixheaacd_mult32x16in32(tempr1, sin1));
    *out_ptr1 = ixheaacd_shr32(*out_ptr1, neg_expo);
    out_ptr1--;

    *out_ptr1 = ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr1, cos1),
                                      tempi1, sin1);
    *out_ptr1 = ixheaacd_shr32(*out_ptr1, neg_expo);
    out_ptr1--;
  }
}

VOID ixheaacd_post_twiddle_dec(WORD32 out_ptr[], WORD32 spec_data[],
                               ia_aac_dec_imdct_tables_struct *ptr_imdct_tables,
                               WORD npoints) {
  WORD i;
  WORD16 cos, cos1, sin, sin1;
  WORD32 *spec_data1 = spec_data + npoints - 1;
  WORD32 *out_ptr1 = out_ptr + npoints - 1;
  WORD16 adjust = 50, adjust1 = -50;
  const WORD16 *cos_sin_ptr = ptr_imdct_tables->cosine_array_2048_256;

  if (npoints == 1024) {
    WORD32 tempr, tempi, outi, outr, temp1, temp2;
    tempr = *spec_data++;
    tempi = *spec_data++;

    cos = *cos_sin_ptr;
    cos_sin_ptr++;
    sin = *cos_sin_ptr;
    cos_sin_ptr++;

    outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, sin),
                          ixheaacd_mult32x16in32(tempi, cos));
    outr =
        ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr, cos), tempi, sin);

    temp1 = ixheaacd_mult32x16in32(outi, adjust1);
    temp2 = ixheaacd_mult32x16in32(outr, adjust);

    outr = outr + temp1;
    outi = outi + temp2;
    *out_ptr1-- = outi;
    *out_ptr++ = outr;

    for (i = 0; i < (npoints / 2 - 2); i++) {
      sin = *cos_sin_ptr++;
      cos = *cos_sin_ptr++;

      tempi = *spec_data1--;
      tempr = *spec_data1--;

      outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, sin),
                            ixheaacd_mult32x16in32(tempi, cos));
      outr =
          ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr, cos), tempi, sin);

      temp1 = ixheaacd_mult32x16in32(outi, adjust1);
      temp2 = ixheaacd_mult32x16in32(outr, adjust);

      outr = outr + temp1;
      outi = outi + temp2;

      *out_ptr++ = outi;
      *out_ptr1-- = outr;

      i++;
      tempr = *spec_data++;
      tempi = *spec_data++;

      outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, cos),
                            ixheaacd_mult32x16in32(tempi, sin));
      outr =
          ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr, sin), tempi, cos);

      temp1 = ixheaacd_mult32x16in32(outi, adjust1);
      temp2 = ixheaacd_mult32x16in32(outr, adjust);

      outr = outr + temp1;
      outi = outi + temp2;

      *out_ptr1-- = outi;
      *out_ptr++ = outr;
    }
    cos1 = *cos_sin_ptr++;
    sin1 = *cos_sin_ptr;

    tempi = *spec_data1--;
    tempr = *spec_data1--;

    outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, sin1),
                          ixheaacd_mult32x16in32(tempi, cos1));
    outr =
        ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr, cos1), tempi, sin1);

    temp1 = ixheaacd_mult32x16in32(outi, adjust1);
    temp2 = ixheaacd_mult32x16in32(outr, adjust);

    outr = outr + temp1;
    outi = outi + temp2;

    *out_ptr++ = outi;
    *out_ptr1-- = outr;
  } else if (npoints == 128) {
    WORD32 tempr, tempi, outi, outr, temp1, temp2;
    tempr = *spec_data++;
    tempi = *spec_data++;

    cos = *cos_sin_ptr++;
    sin = *cos_sin_ptr;
    cos_sin_ptr += 15;

    outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, sin),
                          ixheaacd_mult32x16in32(tempi, cos));
    outr =
        ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr, cos), tempi, sin);

    temp1 = ixheaacd_mult32x16in32(outi, -(201 << 1));
    temp2 = ixheaacd_mult32x16in32(outr, 201 << 1);

    outr = outr + temp1;
    outi = outi + temp2;
    *out_ptr1-- = outi;
    *out_ptr++ = outr;

    for (i = 0; i < (npoints / 2 - 2); i++) {
      sin = *cos_sin_ptr++;
      cos = *cos_sin_ptr;
      cos_sin_ptr += 15;

      tempi = *spec_data1--;
      tempr = *spec_data1--;

      outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, sin),
                            ixheaacd_mult32x16in32(tempi, cos));
      outr =
          ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr, cos), tempi, sin);

      temp1 = ixheaacd_mult32x16in32(outi, -(201 << 1));
      temp2 = ixheaacd_mult32x16in32(outr, 201 << 1);

      outr = outr + temp1;
      outi = outi + temp2;

      *out_ptr++ = outi;
      *out_ptr1-- = outr;

      i++;
      tempr = *spec_data++;
      tempi = *spec_data++;

      outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, cos),
                            ixheaacd_mult32x16in32(tempi, sin));
      outr =
          ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr, sin), tempi, cos);

      temp1 = ixheaacd_mult32x16in32(outi, -(201 << 1));
      temp2 = ixheaacd_mult32x16in32(outr, 201 << 1);

      outr = outr + temp1;
      outi = outi + temp2;

      *out_ptr1-- = outi;
      *out_ptr++ = outr;
    }
    cos1 = *cos_sin_ptr++;
    sin1 = *cos_sin_ptr;

    tempi = *spec_data1--;
    tempr = *spec_data1--;

    outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, sin1),
                          ixheaacd_mult32x16in32(tempi, cos1));
    outr =
        ixheaacd_mac32x16in32(ixheaacd_mult32x16in32(tempr, cos1), tempi, sin1);

    temp1 = ixheaacd_mult32x16in32(outi, -(201 << 1));
    temp2 = ixheaacd_mult32x16in32(outr, 201 << 1);

    outr = outr + temp1;
    outi = outi + temp2;

    *out_ptr++ = outi;
    *out_ptr1-- = outr;
  }
}

VOID ixheaacd_post_twid_overlap_add_dec(
    WORD16 pcm_out[], WORD32 spec_data[],
    ia_aac_dec_imdct_tables_struct *ptr_imdct_tables, WORD npoints,
    WORD32 *ptr_overlap_buf, WORD16 q_shift, const WORD16 *window,
    WORD16 ch_fac) {
  WORD i;
  WORD16 cos, cos1, sin, sin1;
  WORD32 size = npoints / 2;
  WORD16 *pcmout1 = pcm_out + (ch_fac * size);
  const WORD16 *cos_sin_ptr = ptr_imdct_tables->cosine_array_2048_256;

  pcm_out = pcmout1 - ch_fac;
  spec_data += size;

  if (q_shift > 0) {
    WORD32 tempr, tempi, outr, outi, win1, accu, temp1, temp2;
    WORD16 adjust, adjust1;
    WORD32 overlap_data;

    tempr = *(spec_data - size);
    tempi = *(spec_data - size + 1);
    adjust = 50;
    adjust1 = -50;
    cos = *cos_sin_ptr++;
    sin = *cos_sin_ptr++;
    outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, sin),
                          ixheaacd_mult32x16in32(tempi, cos));
    outr = ixheaacd_add32(ixheaacd_mult32x16in32(tempr, cos),
                          ixheaacd_mult32x16in32(tempi, sin));

    overlap_data = *ptr_overlap_buf;

    temp1 = ixheaacd_mult32x16in32(outi, adjust1);
    temp2 = ixheaacd_mult32x16in32(outr, adjust);

    outr = outr + temp1;
    outi = outi + temp2;

    *ptr_overlap_buf++ = ixheaacd_shr32_drc(outr, 16 - q_shift);

    win1 = *((WORD32 *)window + size - 1);
    accu = ixheaacd_sub32_sat(
        ixheaacd_shl32_sat(ixheaacd_mult32x16lin32(outi, win1), q_shift),
        ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)(win1 >> 16)));

    *pcm_out = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));

    pcm_out -= ch_fac;
    accu = ixheaacd_sub32_sat(
        ixheaacd_shl32_sat(
            ixheaacd_mult32x16hin32_drc(ixheaacd_negate32(outi), win1),
            q_shift),
        ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)(win1)));

    *pcmout1 = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));

    pcmout1 += ch_fac;

    for (i = size - 2; i != 0;) {
      sin = *cos_sin_ptr++;
      cos = *cos_sin_ptr++;

      tempr = *(spec_data + i);
      tempi = *(spec_data + i + 1);

      outr = ixheaacd_add32(ixheaacd_mult32x16in32(tempr, cos),
                            ixheaacd_mult32x16in32(tempi, sin));
      outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, sin),
                            ixheaacd_mult32x16in32(tempi, cos));

      temp1 = ixheaacd_mult32x16in32(outi, adjust1);
      temp2 = ixheaacd_mult32x16in32(outr, adjust);

      outr = outr + temp1;
      outi = outi + temp2;

      overlap_data = *ptr_overlap_buf;

      *ptr_overlap_buf++ = ixheaacd_shr32_drc(outi, 16 - q_shift);

      win1 = *((WORD32 *)window + i);
      accu = ixheaacd_sub32_sat(
          ixheaacd_shl32_sat(ixheaacd_mult32x16lin32(outr, win1), q_shift),
          ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)(win1 >> 16)));
      *pcm_out = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));
      pcm_out -= ch_fac;
      accu = ixheaacd_sub32_sat(
          ixheaacd_shl32_sat(
              ixheaacd_mult32x16hin32_drc(ixheaacd_negate32(outr), win1),
              q_shift),
          ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)win1));
      *pcmout1 = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));
      pcmout1 += ch_fac;

      tempr = *(spec_data - i);
      tempi = *(spec_data - i + 1);

      i -= 2;

      outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, cos),
                            ixheaacd_mult32x16in32(tempi, sin));
      outr = ixheaacd_add32(ixheaacd_mult32x16in32(tempr, sin),
                            ixheaacd_mult32x16in32(tempi, cos));

      overlap_data = *ptr_overlap_buf;

      temp1 = ixheaacd_mult32x16in32(outi, adjust1);

      temp2 = ixheaacd_mult32x16in32(outr, adjust);

      outr = outr + temp1;
      outi = outi + temp2;

      *ptr_overlap_buf++ = ixheaacd_shr32_drc(outr, 16 - q_shift);

      win1 = *((WORD32 *)window + i + 1);
      accu = ixheaacd_sub32_sat(
          ixheaacd_shl32_sat(ixheaacd_mult32x16lin32(outi, win1), q_shift),
          ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)(win1 >> 16)));
      *pcm_out = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));
      pcm_out -= ch_fac;
      accu = ixheaacd_sub32_sat(
          ixheaacd_shl32_sat(
              ixheaacd_mult32x16hin32_drc(ixheaacd_negate32(outi), win1),
              q_shift),
          ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)(win1)));
      *pcmout1 = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));
      pcmout1 += ch_fac;
    }
    cos1 = *cos_sin_ptr++;
    sin1 = *cos_sin_ptr;

    tempr = *(spec_data + i);
    tempi = *(spec_data + i + 1);

    outr = ixheaacd_add32(ixheaacd_mult32x16in32(tempr, cos1),
                          ixheaacd_mult32x16in32(tempi, sin1));
    outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, sin1),
                          ixheaacd_mult32x16in32(tempi, cos1));

    temp1 = ixheaacd_mult32x16in32(outi, adjust1);

    temp2 = ixheaacd_mult32x16in32(outr, adjust);

    outr = outr + temp1;
    outi = outi + temp2;

    overlap_data = *ptr_overlap_buf;

    *ptr_overlap_buf++ = ixheaacd_shr32_drc(outi, 16 - q_shift);
    win1 = *((WORD32 *)window + i);
    accu = ixheaacd_sub32_sat(
        ixheaacd_shl32_sat(ixheaacd_mult32x16lin32(outr, win1), q_shift),
        ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)(win1 >> 16)));
    *pcm_out = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));
    pcm_out -= ch_fac;
    accu = ixheaacd_sub32_sat(
        ixheaacd_shl32_sat(
            ixheaacd_mult32x16hin32_drc(ixheaacd_negate32(outr), win1),
            q_shift),
        ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)win1));
    *pcmout1 = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));
    pcmout1 += ch_fac;
  } else {
    q_shift = -q_shift;
    {
      WORD32 tempr, tempi, temp1, temp2, outr, outi, win1, accu;
      WORD16 adjust, adjust1;
      WORD16 overlap_data;
      tempr = *(spec_data - size);
      tempi = *(spec_data - size + 1);

      adjust = 50;
      adjust1 = -50;
      cos = *cos_sin_ptr++;
      sin = *cos_sin_ptr++;

      outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, sin),
                            ixheaacd_mult32x16in32(tempi, cos));
      outr = ixheaacd_add32(ixheaacd_mult32x16in32(tempr, cos),
                            ixheaacd_mult32x16in32(tempi, sin));

      overlap_data = *ptr_overlap_buf;

      temp1 = ixheaacd_mult32x16in32(outi, adjust1);
      temp2 = ixheaacd_mult32x16in32(outr, adjust);

      outr = outr + temp1;
      outi = outi + temp2;

      *ptr_overlap_buf++ = ixheaacd_shr32_drc(outr, 16 + q_shift);

      win1 = *((WORD32 *)window + size - 1);
      accu = ixheaacd_sub32_sat(
          ixheaacd_shr32(ixheaacd_mult32x16lin32(outi, win1), q_shift),
          ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)(win1 >> 16)));

      *pcm_out = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));

      pcm_out -= ch_fac;
      accu = ixheaacd_sub32_sat(
          ixheaacd_shr32(
              ixheaacd_mult32x16hin32_drc(ixheaacd_negate32(outi), win1),
              q_shift),
          ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)(win1)));

      *pcmout1 = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));
      pcmout1 += ch_fac;

      for (i = size - 2; i != 0;) {
        sin = *cos_sin_ptr++;
        cos = *cos_sin_ptr++;

        tempr = *(spec_data + i);
        tempi = *(spec_data + i + 1);

        outr = ixheaacd_add32(ixheaacd_mult32x16in32(tempr, cos),
                              ixheaacd_mult32x16in32(tempi, sin));
        outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, sin),
                              ixheaacd_mult32x16in32(tempi, cos));

        overlap_data = *ptr_overlap_buf;

        temp1 = ixheaacd_mult32x16in32(outi, adjust1);

        temp2 = ixheaacd_mult32x16in32(outr, adjust);
        outr = outr + temp1;
        outi = outi + temp2;
        *ptr_overlap_buf++ = ixheaacd_shr32_drc(outi, 16 + q_shift);

        win1 = *((WORD32 *)window + i);
        accu = ixheaacd_sub32_sat(
            ixheaacd_shr32(ixheaacd_mult32x16lin32(outr, win1), q_shift),
            ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)(win1 >> 16)));

        *pcm_out = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));
        pcm_out -= ch_fac;
        accu = ixheaacd_sub32_sat(
            ixheaacd_shr32(
                ixheaacd_mult32x16hin32_drc(ixheaacd_negate32(outr), win1),
                q_shift),
            ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)win1));
        *pcmout1 = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));
        pcmout1 += ch_fac;

        tempr = *(spec_data - i);
        tempi = *(spec_data - i + 1);
        i -= 2;

        outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, cos),
                              ixheaacd_mult32x16in32(tempi, sin));
        outr = ixheaacd_add32(ixheaacd_mult32x16in32(tempr, sin),
                              ixheaacd_mult32x16in32(tempi, cos));

        overlap_data = *ptr_overlap_buf;

        temp1 = ixheaacd_mult32x16in32(outi, adjust1);
        temp2 = ixheaacd_mult32x16in32(outr, adjust);

        outr = outr + temp1;
        outi = outi + temp2;

        *ptr_overlap_buf++ = ixheaacd_shr32_drc(outr, 16 + q_shift);

        win1 = *((WORD32 *)window + i + 1);
        accu = ixheaacd_sub32_sat(
            ixheaacd_shr32(ixheaacd_mult32x16lin32(outi, win1), q_shift),
            ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)(win1 >> 16)));

        *pcm_out = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));
        pcm_out -= ch_fac;
        accu = ixheaacd_sub32_sat(
            ixheaacd_shr32(
                ixheaacd_mult32x16hin32_drc(ixheaacd_negate32(outi), win1),
                q_shift),
            ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)(win1)));

        *pcmout1 = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));
        pcmout1 += ch_fac;
      }
      cos1 = *cos_sin_ptr++;
      sin1 = *cos_sin_ptr++;

      tempr = *(spec_data + i);
      tempi = *(spec_data + i + 1);

      outr = ixheaacd_add32(ixheaacd_mult32x16in32(tempr, cos1),
                            ixheaacd_mult32x16in32(tempi, sin1));
      outi = ixheaacd_sub32(ixheaacd_mult32x16in32(tempr, sin1),
                            ixheaacd_mult32x16in32(tempi, cos1));

      overlap_data = *ptr_overlap_buf;

      temp1 = ixheaacd_mult32x16in32(outi, adjust1);

      temp2 = ixheaacd_mult32x16in32(outr, adjust);

      outr = outr + temp1;
      outi = outi + temp2;

      *ptr_overlap_buf++ = ixheaacd_shr32_drc(outi, 16 + q_shift);

      win1 = *((WORD32 *)window + i);
      accu = ixheaacd_sub32_sat(
          ixheaacd_shr32(ixheaacd_mult32x16lin32(outr, win1), q_shift),
          ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)(win1 >> 16)));

      *pcm_out = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));
      pcm_out -= ch_fac;
      accu = ixheaacd_sub32_sat(
          ixheaacd_shr32(
              ixheaacd_mult32x16hin32_drc(ixheaacd_negate32(outr), win1),
              q_shift),
          ixheaacd_mult32x16lin32_drc(overlap_data, (WORD16)win1));
      *pcmout1 = ixheaacd_round16(ixheaacd_shl32_sat(accu, 2));
      pcmout1 += ch_fac;
    }
  }
}

VOID ixheaacd_imdct_using_fft_dec(
    ia_aac_dec_imdct_tables_struct *ptr_imdct_tables, WORD32 npoints,
    WORD32 *ptr_x, WORD32 *ptr_y)

{
  WORD32 i, j, k, k1, n_stages;
  WORD32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i, x4r, x4i, x5r, x5i, x6r, x6i,
      x7r, x7i;
  WORD32 del, nodespacing, in_loop_cnt, tmp, twiddle_val, *ptr_tmp;
  const WORD32 *ptr_twiddle;
  WORD8 *ptr_dig_rev_table;
  n_stages = ixheaacd_norm32(npoints);

  n_stages = (30 - n_stages) / 3;

  ptr_tmp = ptr_y;

  ptr_twiddle = ptr_imdct_tables->fft_twiddle;
  ptr_dig_rev_table = ((npoints << 1) == 1024)
                          ? ptr_imdct_tables->dig_rev_table8_long
                          : ptr_imdct_tables->dig_rev_table8_short;

  for (i = npoints; i != 0; i -= 8) {
    WORD32 *data = ptr_x;
    data = data + (*ptr_dig_rev_table++ << 1);

    x0r = *data;
    x0i = *(data + 1);
    data += (npoints >> 1);

    x2r = *data;
    x2i = *(data + 1);
    data += (npoints >> 1);

    x4r = *data;
    x4i = *(data + 1);
    data += (npoints >> 1);

    x6r = *data;
    x6i = *(data + 1);
    data -= 5 * (npoints >> 2);

    x0r = x0r + x4r;
    x0i = x0i + x4i;
    x4r = x0r - (x4r << 1);
    x4i = x0i - (x4i << 1);

    x2r = x2r + x6r;
    x2i = x2i + x6i;
    x6r = x2r - (x6r << 1);
    x6i = x2i - (x6i << 1);

    x0r = x0r + x2r;
    x0i = x0i + x2i;
    x2r = x0r - (x2r << 1);
    x2i = x0i - (x2i << 1);

    x4r = x4r + x6i;
    x4i = x4i - x6r;
    tmp = x6r;
    x6r = x4r - (x6i << 1);
    x6i = x4i + (tmp << 1);

    x1r = *data;
    x1i = *(data + 1);
    data += (npoints >> 1);

    x3r = *data;
    x3i = *(data + 1);
    data += (npoints >> 1);

    x5r = *data;
    x5i = *(data + 1);
    data += (npoints >> 1);

    x7r = *data;
    x7i = *(data + 1);
    data -= 7 * (npoints >> 2);

    x1r = x1r + x5r;
    x1i = x1i + x5i;
    x5r = x1r - (x5r << 1);
    x5i = x1i - (x5i << 1);

    x3r = x3r + x7r;
    x3i = x3i + x7i;
    x7r = x3r - (x7r << 1);
    x7i = x3i - (x7i << 1);

    x1r = x1r + x3r;
    x1i = x1i + x3i;
    x3r = x1r - (x3r << 1);
    x3i = x1i - (x3i << 1);

    x5r = x5r + x5i;
    x5i = x5r - (x5i << 1);

    x7r = x7r + x7i;
    x7i = x7r - (x7i << 1);

    x7i = x5r - x7i;
    x5r = x7i - (x5r << 1);

    x5i = x7r - x5i;
    x7r = x5i - (x7r << 1);

    x7i = x7i << 1;
    x5r = x5r << 1;
    x5i = x5i << 1;
    x7r = x7r << 1;

    x0r = x0r + x1r;
    x0i = x0i + x1i;
    x1r = x0r - (x1r << 1);
    x1i = x0i - (x1i << 1);

    x2r = x2r + x3i;
    tmp = x2r - (x3i << 1);
    x2i = x2i - x3r;
    x3i = x2i + (x3r << 1);

    *ptr_tmp = x0r;
    *(ptr_tmp + 1) = x0i;
    ptr_tmp += 4;

    *ptr_tmp = x2r;
    *(ptr_tmp + 1) = x2i;
    ptr_tmp += 4;

    *ptr_tmp = x1r;
    *(ptr_tmp + 1) = x1i;
    ptr_tmp += 4;

    *ptr_tmp = tmp;
    *(ptr_tmp + 1) = x3i;
    ptr_tmp -= 10;

    tmp = 0x5A82;

    x7i = x4r + (ixheaacd_mult32x16lin32(x7i, tmp));
    x4r = x7i - (x4r << 1);

    x7r = x4i + (ixheaacd_mult32x16lin32(x7r, tmp));
    x4i = x7r - (x4i << 1);

    x5i = x6r + (ixheaacd_mult32x16lin32(x5i, tmp));
    x6r = x5i - (x6r << 1);

    x5r = x6i + (ixheaacd_mult32x16lin32(x5r, tmp));
    x6i = x5r - (x6i << 1);

    *ptr_tmp = x7i;
    *(ptr_tmp + 1) = x7r;
    ptr_tmp += 4;

    *ptr_tmp = x5i;
    *(ptr_tmp + 1) = x5r;
    ptr_tmp += 4;

    *ptr_tmp = -x4r;
    *(ptr_tmp + 1) = -x4i;
    ptr_tmp += 4;

    *ptr_tmp = -x6r;
    *(ptr_tmp + 1) = -x6i;
    ptr_tmp += 2;
  }

  del = 8;

  nodespacing = 64;
  in_loop_cnt = npoints >> 6;

  for (k1 = n_stages - 2; k1 > 0; k1--) {
    WORD32 *data = ptr_y;
    const WORD32 *twiddles;

    for (i = 0; i != npoints; i += 8 * del) {
      data = ptr_y + (i << 1);
      x0r = *data;
      x0i = *(data + 1);
      data += (del << 2);

      x2r = *data;
      x2i = *(data + 1);
      data += (del << 2);

      x4r = *data;
      x4i = *(data + 1);
      data += (del << 2);

      x6r = *data;
      x6i = *(data + 1);
      data -= 5 * (del << 1);

      x0r = x0r + x4r;
      x0i = x0i + x4i;
      x4r = x0r - (x4r << 1);
      x4i = x0i - (x4i << 1);

      x2r = x2r + x6r;
      x2i = x2i + x6i;
      x6r = x2r - (x6r << 1);
      x6i = x2i - (x6i << 1);

      x0r = x0r + x2r;
      x0i = x0i + x2i;
      x2r = x0r - (x2r << 1);
      x2i = x0i - (x2i << 1);

      x4r = x4r + x6i;
      x4i = x4i - x6r;
      tmp = x6r;
      x6r = x4r - (x6i << 1);
      x6i = x4i + (tmp << 1);

      x1r = *data;
      x1i = *(data + 1);
      data += (del << 2);

      x3r = *data;
      x3i = *(data + 1);
      data += (del << 2);

      x5r = *data;
      x5i = *(data + 1);
      data += (del << 2);

      x7r = *data;
      x7i = *(data + 1);
      data -= 7 * (del << 1);

      x1r = x1r + x5r;
      x1i = x1i + x5i;
      x5r = x1r - (x5r << 1);
      x5i = x1i - (x5i << 1);

      x3r = x3r + x7r;
      x3i = x3i + x7i;
      x7r = x3r - (x7r << 1);
      x7i = x3i - (x7i << 1);

      x1r = x1r + x3r;
      x1i = x1i + x3i;
      x3r = x1r - (x3r << 1);
      x3i = x1i - (x3i << 1);

      x5r = x5r + x5i;
      x5i = x5r - (x5i << 1);

      x7r = x7r + x7i;
      x7i = x7r - (x7i << 1);

      x7i = x5r - x7i;
      x5r = x7i - (x5r << 1);

      x5i = x7r - x5i;
      x7r = x5i - (x7r << 1);

      x7i = x7i << 1;
      x5r = x5r << 1;
      x5i = x5i << 1;
      x7r = x7r << 1;

      x0r = x0r + x1r;
      x0i = x0i + x1i;
      x1r = x0r - (x1r << 1);
      x1i = x0i - (x1i << 1);

      x2r = x2r + x3i;
      tmp = x2r - (x3i << 1);
      x2i = x2i - x3r;
      x3i = x2i + (x3r << 1);

      *data = x0r;
      *(data + 1) = x0i;
      data += (del << 2);

      *data = x2r;
      *(data + 1) = x2i;
      data += (del << 2);

      *data = x1r;
      *(data + 1) = x1i;
      data += (del << 2);

      *data = tmp;
      *(data + 1) = x3i;
      data -= 5 * (del << 1);

      tmp = 0x5A82;

      x7i = x4r + (ixheaacd_mult32x16lin32(x7i, tmp));
      x4r = x7i - (x4r << 1);
      x7r = x4i + (ixheaacd_mult32x16lin32(x7r, tmp));
      x4i = x7r - (x4i << 1);

      x5i = x6r + (ixheaacd_mult32x16lin32(x5i, tmp));
      x6r = x5i - (x6r << 1);
      x5r = x6i + (ixheaacd_mult32x16lin32(x5r, tmp));
      x6i = x5r - (x6i << 1);

      *data = x7i;
      *(data + 1) = x7r;
      data += (del << 2);

      *data = x5i;
      *(data + 1) = x5r;
      data += (del << 2);

      *data = -x4r;
      *(data + 1) = -x4i;
      data += (del << 2);

      *data = -x6r;
      *(data + 1) = -x6i;

      data -= 7 * (del << 1);
    }

    twiddles = ptr_twiddle;
    data = ptr_y;

    for (j = nodespacing; j < nodespacing * del; j += nodespacing) {
      data = data + 2;

      for (k = in_loop_cnt; k != 0; k--) {
        data += (del << 2);
        x2r = *data;
        x2i = *(data + 1);

        data += (del << 2);
        x4r = *data;
        x4i = *(data + 1);

        data += (del << 2);
        x6r = *data;
        x6i = *(data + 1);

        data -= 6 * (del << 1);

        twiddles += (j >> 2);

        twiddle_val = *(twiddles);

        tmp = (ixheaacd_mult32x16lin32(x2r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x2i, twiddle_val));
        x2i = (ixheaacd_mac32x16lin32(
                  ixheaacd_mult32x16hin32_drc(x2r, twiddle_val), x2i,
                  twiddle_val))
              << 1;
        x2r = tmp << 1;

        twiddles += (j >> 2);
        twiddle_val = *(twiddles);

        tmp = (ixheaacd_mult32x16lin32(x4r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x4i, twiddle_val));
        x4i = (ixheaacd_mac32x16lin32(
                  ixheaacd_mult32x16hin32_drc(x4r, twiddle_val), x4i,
                  twiddle_val))
              << 1;
        x4r = tmp << 1;

        twiddles += (j >> 2);
        twiddle_val = *(twiddles);

        tmp = (ixheaacd_mult32x16lin32(x6r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x6i, twiddle_val));
        x6i = (ixheaacd_mac32x16lin32(
                  ixheaacd_mult32x16hin32_drc(x6r, twiddle_val), x6i,
                  twiddle_val))
              << 1;
        x6r = tmp << 1;

        x0r = *data;
        x0i = *(data + 1);
        data += (del << 1);

        x0r = x0r + x4r;
        x0i = x0i + x4i;
        x4r = x0r - (x4r << 1);
        x4i = x0i - (x4i << 1);

        x2r = x2r + x6r;
        x2i = x2i + x6i;
        x6r = x2r - (x6r << 1);
        x6i = x2i - (x6i << 1);

        x0r = x0r + x2r;
        x0i = x0i + x2i;
        x2r = x0r - (x2r << 1);
        x2i = x0i - (x2i << 1);

        x4r = x4r + x6i;
        x4i = x4i - x6r;
        tmp = x6r;
        x6r = x4r - (x6i << 1);
        x6i = x4i + (tmp << 1);

        x1r = *data;
        x1i = *(data + 1);
        data += (del << 2);

        twiddles -= 5 * (j >> 3);
        twiddle_val = *(twiddles);

        tmp = (ixheaacd_mult32x16lin32(x1r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x1i, twiddle_val));
        x1i = (ixheaacd_mac32x16lin32(
                  ixheaacd_mult32x16hin32_drc(x1r, twiddle_val), x1i,
                  twiddle_val))
              << 1;
        x1r = tmp << 1;

        x3r = *data;
        x3i = *(data + 1);
        data += (del << 2);

        twiddles += (j >> 2);
        twiddle_val = *(twiddles);

        tmp = (ixheaacd_mult32x16lin32(x3r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x3i, twiddle_val));
        x3i = (ixheaacd_mac32x16lin32(
            ixheaacd_mult32x16hin32_drc(x3r, twiddle_val), x3i, twiddle_val));
        x3r = tmp;

        x5r = *data;
        x5i = *(data + 1);
        data += (del << 2);

        twiddles += (j >> 2);
        twiddle_val = *(twiddles);

        tmp = (ixheaacd_mult32x16lin32(x5r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x5i, twiddle_val));
        x5i = (ixheaacd_mac32x16lin32(
            ixheaacd_mult32x16hin32_drc(x5r, twiddle_val), x5i, twiddle_val));
        x5r = tmp;

        x7r = *data;
        x7i = *(data + 1);
        data -= 7 * (del << 1);

        twiddles += (j >> 2);
        twiddle_val = *(twiddles);
        twiddles -= 7 * (j >> 3);

        tmp = (ixheaacd_mult32x16lin32(x7r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x7i, twiddle_val));
        x7i = (ixheaacd_mac32x16lin32(
            ixheaacd_mult32x16hin32_drc(x7r, twiddle_val), x7i, twiddle_val));
        x7r = tmp;

        x1r = x1r + (x5r << 1);
        x1i = x1i + (x5i << 1);
        x5r = x1r - (x5r << 2);
        x5i = x1i - (x5i << 2);

        x3r = x3r + x7r;
        x3i = x3i + x7i;
        x7r = x3r - (x7r << 1);
        x7i = x3i - (x7i << 1);

        x1r = x1r + (x3r << 1);
        x1i = x1i + (x3i << 1);
        x3r = x1r - (x3r << 2);
        x3i = x1i - (x3i << 2);

        x5r = x5r + x5i;
        x5i = x5r - (x5i << 1);

        x7r = x7r + x7i;
        x7i = x7r - (x7i << 1);

        x7i = x5r - (x7i << 1);
        x5r = x7i - (x5r << 1);

        x5i = (x7r << 1) - x5i;
        x7r = x5i - (x7r << 2);

        x7i = x7i << 1;
        x5r = x5r << 1;
        x5i = x5i << 1;
        x7r = x7r << 1;

        x0r = x0r + x1r;
        x0i = x0i + x1i;
        x1r = x0r - (x1r << 1);
        x1i = x0i - (x1i << 1);

        x2r = x2r + x3i;
        tmp = x2r - (x3i << 1);
        x2i = x2i - x3r;
        x3i = x2i + (x3r << 1);

        *data = x0r;
        *(data + 1) = x0i;
        data += (del << 2);

        *data = x2r;
        *(data + 1) = x2i;
        data += (del << 2);

        *data = x1r;
        *(data + 1) = x1i;
        data += (del << 2);

        *data = tmp;
        *(data + 1) = x3i;
        data -= 5 * (del << 1);

        tmp = 0x5A82;

        x7i = x4r + (ixheaacd_mult32x16lin32(x7i, tmp));
        x4r = x7i - (x4r << 1);

        x7r = x4i + (ixheaacd_mult32x16lin32(x7r, tmp));
        x4i = x7r - (x4i << 1);

        x5i = x6r + (ixheaacd_mult32x16lin32(x5i, tmp));
        x6r = x5i - (x6r << 1);

        x5r = x6i + (ixheaacd_mult32x16lin32(x5r, tmp));
        x6i = x5r - (x6i << 1);

        *data = x7i;
        *(data + 1) = x7r;
        data += (del << 2);

        *data = x5i;
        *(data + 1) = x5r;
        data += (del << 2);

        *data = -x4r;
        *(data + 1) = -x4i;
        data += (del << 2);

        *data = -x6r;
        *(data + 1) = -x6i;

        data -= 7 * (del << 1);
        data += (del << 4);
      }
      data -= npoints << 1;
    }
    nodespacing >>= 3;
    del <<= 3;
    in_loop_cnt >>= 3;
  }

  {
    WORD32 *data = ptr_y;
    const WORD32 *twiddles;
    twiddles = ptr_twiddle;
    data = ptr_y;
    data = data - 2;

    for (j = 0; j < nodespacing * del; j += nodespacing) {
      data = data + 2;

      {
        data += (del << 2);
        x2r = *data;
        x2i = *(data + 1);

        data += (del << 2);
        x4r = *data;
        x4i = *(data + 1);

        data += (del << 2);
        x6r = *data;
        x6i = *(data + 1);

        data -= 6 * (del << 1);

        twiddles += (j >> 2);

        twiddle_val = *(twiddles);

        tmp = (ixheaacd_mult32x16lin32(x2r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x2i, twiddle_val));
        x2i = (ixheaacd_mac32x16lin32(
                  ixheaacd_mult32x16hin32_drc(x2r, twiddle_val), x2i,
                  twiddle_val))
              << 1;
        x2r = tmp << 1;

        twiddles += (j >> 2);
        twiddle_val = *(twiddles);

        tmp = (ixheaacd_mult32x16lin32(x4r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x4i, twiddle_val));
        x4i = (ixheaacd_mac32x16lin32(
                  ixheaacd_mult32x16hin32_drc(x4r, twiddle_val), x4i,
                  twiddle_val))
              << 1;
        x4r = tmp << 1;

        twiddles += (j >> 2);
        twiddle_val = *(twiddles);

        tmp = (ixheaacd_mult32x16lin32(x6r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x6i, twiddle_val));
        x6i = (ixheaacd_mac32x16lin32(
                  ixheaacd_mult32x16hin32_drc(x6r, twiddle_val), x6i,
                  twiddle_val))
              << 1;
        x6r = tmp << 1;

        x0r = *data;
        x0i = *(data + 1);
        data += (del << 1);

        x0r = x0r + x4r;
        x0i = x0i + x4i;
        x4r = x0r - (x4r << 1);
        x4i = x0i - (x4i << 1);

        x2r = x2r + x6r;
        x2i = x2i + x6i;
        x6r = x2r - (x6r << 1);
        x6i = x2i - (x6i << 1);

        x0r = x0r + x2r;
        x0i = x0i + x2i;
        x2r = x0r - (x2r << 1);
        x2i = x0i - (x2i << 1);

        x4r = x4r + x6i;
        x4i = x4i - x6r;
        tmp = x6r;
        x6r = x4r - (x6i << 1);
        x6i = x4i + (tmp << 1);

        x1r = *data;
        x1i = *(data + 1);
        data += (del << 2);

        twiddles -= 5 * (j >> 3);
        twiddle_val = *(twiddles);

        tmp = (ixheaacd_mult32x16lin32(x1r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x1i, twiddle_val));
        x1i = (ixheaacd_mac32x16lin32(
                  ixheaacd_mult32x16hin32_drc(x1r, twiddle_val), x1i,
                  twiddle_val))
              << 1;
        x1r = tmp << 1;

        x3r = *data;
        x3i = *(data + 1);
        data += (del << 2);

        twiddles += (j >> 2);
        twiddle_val = *(twiddles);

        tmp = (ixheaacd_mult32x16lin32(x3r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x3i, twiddle_val));
        x3i = (ixheaacd_mac32x16lin32(
            ixheaacd_mult32x16hin32_drc(x3r, twiddle_val), x3i, twiddle_val));
        x3r = tmp;

        x5r = *data;
        x5i = *(data + 1);
        data += (del << 2);

        twiddles += (j >> 2);
        twiddle_val = *(twiddles);

        tmp = (ixheaacd_mult32x16lin32(x5r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x5i, twiddle_val));
        x5i = (ixheaacd_mac32x16lin32(
            ixheaacd_mult32x16hin32_drc(x5r, twiddle_val), x5i, twiddle_val));
        x5r = tmp;

        x7r = *data;
        x7i = *(data + 1);
        data -= 7 * (del << 1);

        twiddles += (j >> 2);
        twiddle_val = *(twiddles);
        twiddles -= 7 * (j >> 3);

        tmp = (ixheaacd_mult32x16lin32(x7r, twiddle_val) -
               ixheaacd_mult32x16hin32_drc(x7i, twiddle_val));
        x7i = (ixheaacd_mac32x16lin32(
            ixheaacd_mult32x16hin32_drc(x7r, twiddle_val), x7i, twiddle_val));
        x7r = tmp;

        x1r = x1r + (x5r << 1);
        x1i = x1i + (x5i << 1);
        x5r = x1r - (x5r << 2);
        x5i = x1i - (x5i << 2);

        x3r = x3r + x7r;
        x3i = x3i + x7i;
        x7r = x3r - (x7r << 1);
        x7i = x3i - (x7i << 1);

        x1r = x1r + (x3r << 1);
        x1i = x1i + (x3i << 1);
        x3r = x1r - (x3r << 2);
        x3i = x1i - (x3i << 2);

        x5r = x5r + x5i;
        x5i = x5r - (x5i << 1);

        x7r = x7r + x7i;
        x7i = x7r - (x7i << 1);

        x7i = x5r - (x7i << 1);
        x5r = x7i - (x5r << 1);

        x5i = (x7r << 1) - x5i;
        x7r = x5i - (x7r << 2);

        x7i = x7i << 1;
        x5r = x5r << 1;
        x5i = x5i << 1;
        x7r = x7r << 1;

        x0r = x0r + x1r;
        x0i = x0i + x1i;
        x1r = x0r - (x1r << 1);
        x1i = x0i - (x1i << 1);

        x2r = x2r + x3i;
        tmp = x2r - (x3i << 1);
        x2i = x2i - x3r;
        x3i = x2i + (x3r << 1);

        *data = x0r;
        *(data + 1) = x0i;
        data += (del << 2);

        *data = x2r;
        *(data + 1) = x2i;
        data += (del << 2);

        *data = x1r;
        *(data + 1) = x1i;
        data += (del << 2);

        *data = tmp;
        *(data + 1) = x3i;
        data -= 5 * (del << 1);

        tmp = 0x5A82;

        x7i = x4r + (ixheaacd_mult32x16lin32(x7i, tmp));
        x4r = x7i - (x4r << 1);

        x7r = x4i + (ixheaacd_mult32x16lin32(x7r, tmp));
        x4i = x7r - (x4i << 1);

        x5i = x6r + (ixheaacd_mult32x16lin32(x5i, tmp));
        x6r = x5i - (x6r << 1);

        x5r = x6i + (ixheaacd_mult32x16lin32(x5r, tmp));
        x6i = x5r - (x6i << 1);

        *data = x7i;
        *(data + 1) = x7r;
        data += (del << 2);

        *data = x5i;
        *(data + 1) = x5r;
        data += (del << 2);

        *data = -x4r;
        *(data + 1) = -x4i;
        data += (del << 2);

        *data = -x6r;
        *(data + 1) = -x6i;

        data -= 7 * (del << 1);
        data += (del << 4);
      }
      data -= npoints << 1;
    }

    nodespacing >>= 3;
    del <<= 3;
    in_loop_cnt >>= 3;
  }
}

WORD32 ixheaacd_inverse_transform(
    WORD32 spec_data[], WORD32 scratch[],
    ia_aac_dec_imdct_tables_struct *ptr_imdct_tables, WORD32 expo,
    WORD32 npoints) {
  (*ixheaacd_pretwiddle_compute)(spec_data, spec_data + npoints - 1, scratch,
                                 ptr_imdct_tables, (npoints >> 2), expo);

  (*ixheaacd_imdct_using_fft)(ptr_imdct_tables, npoints >> 1, scratch,
                              spec_data);

  expo += 2;

  return expo;
}

VOID ixheaacd_mdct_480_ld(WORD32 *inp, WORD32 *scratch, WORD32 *mdct_scale,
                          WORD32 mdct_flag,
                          ia_aac_dec_imdct_tables_struct *imdct_tables_ptr,
                          WORD32 object_type) {
  WORD32 expo, neg_expo = 0, k;

  WORD32 const_mltfac = 1145324612;

  expo = (*ixheaacd_calc_max_spectral_line)(inp, MDCT_LEN) - 1;
  ;

  memcpy(scratch, inp, sizeof(WORD32) * MDCT_LEN);

  neg_expo = 7 - expo;

  ixheaacd_pre_twiddle(inp, scratch, 480, imdct_tables_ptr->cosine_array_960,
                       neg_expo);

  ixheaacd_fft_480_ld(inp, scratch, imdct_tables_ptr);

  if (object_type == AOT_ER_AAC_LD) {
    ixheaacd_post_twiddle_ld(inp, scratch, imdct_tables_ptr->cosine_array_960,
                             480);
  } else if (object_type == AOT_ER_AAC_ELD) {
    ixheaacd_post_twiddle_eld(inp + (480), scratch,
                              imdct_tables_ptr->cosine_array_960, 480);
  }

  if (0 == mdct_flag) {
    WORD32 *data = inp;

    if (object_type != AOT_ER_AAC_ELD) {
      for (k = MDCT_LEN - 1; k >= 0; k -= 2) {
        *data = ixheaacd_mult32_shl(*data, const_mltfac);
        data++;
        *data = ixheaacd_mult32_shl(*data, const_mltfac);
        data++;
      }
      neg_expo += 1;
    } else {
      data = inp + 480;
      for (k = (MDCT_LEN << 1) - 1; k >= 0; k -= 2) {
        *data = ixheaacd_mult32_shl(*data, const_mltfac);
        data++;
        *data = ixheaacd_mult32_shl(*data, const_mltfac);
        data++;
      }
      neg_expo += 1;
    }
  }

  *mdct_scale = neg_expo + 3;
}

VOID ixheaacd_inverse_transform_512(
    WORD32 data[], WORD32 temp[], WORD32 *imdct_scale, WORD32 *cos_sin_ptr,
    ia_aac_dec_imdct_tables_struct *imdct_tables_ptr, WORD32 object_type) {
  WORD32 n;
  WORD32 npoints_2;
  WORD16 expo, neg_expo;

  n = 512;

  npoints_2 = n >> 1;

  expo = (*ixheaacd_calc_max_spectral_line)(data, n) - 1;

  memcpy(temp, data, sizeof(WORD32) * n);

  neg_expo = 7 - expo;

  ixheaacd_pre_twiddle(data, temp, n, cos_sin_ptr, neg_expo);

  (*ixheaacd_fft32x32_ld)(imdct_tables_ptr, npoints_2, data, temp);

  neg_expo = (*ixheaacd_neg_expo_inc)(neg_expo);

  *imdct_scale = neg_expo + 1;

  if (object_type == AOT_ER_AAC_ELD)
    ixheaacd_post_twiddle_eld((data + n), temp, cos_sin_ptr, n);
  else
    ixheaacd_post_twiddle_ld((data), temp, cos_sin_ptr, n);
}

VOID ixheaacd_fft_480_ld(WORD32 *inp, WORD32 *op,
                         ia_aac_dec_imdct_tables_struct *imdct_tables_ptr) {
  WORD32 i;
  WORD32 *buf1, *buf2;
  UWORD8 *re_arr_tab_sml_240_ptr;

  (*ixheaacd_aac_ld_dec_rearrange)(inp, op, MDCT_LEN_BY2,
                                   imdct_tables_ptr->re_arr_tab_16);

  buf1 = op;
  buf2 = inp;

  for (i = 0; i < FFT15; i++) {
    (*ixheaacd_fft32x32_ld2)(imdct_tables_ptr, 16, buf1, buf2);

    buf1 += (FFT16X2);
    buf2 += (FFT16X2);
  }
  re_arr_tab_sml_240_ptr = imdct_tables_ptr->re_arr_tab_sml_240;
  buf1 = inp;

  for (i = 0; i < FFT16; i++) {
    (*ixheaacd_fft_15_ld)(buf1, op, ixheaacd_fft5out, re_arr_tab_sml_240_ptr);
    re_arr_tab_sml_240_ptr += FFT15;
    buf1 += 2;
  }
}

VOID ixheaacd_pre_twiddle(WORD32 *xptr, WORD32 *data, WORD32 n,
                          WORD32 *cos_sin_ptr, WORD32 neg_expo) {
  WORD npoints_4, i;
  WORD32 tempr, tempi, temp;
  WORD32 c, c1, s, s1;
  WORD32 *in_ptr1, *in_ptr2;

  npoints_4 = n >> 2;

  in_ptr1 = data;
  in_ptr2 = data + n - 1;

  if (neg_expo >= 0) {
    for (i = npoints_4 - 1; i >= 0; i--) {
      c = *cos_sin_ptr++;
      c1 = *cos_sin_ptr++;
      s = *cos_sin_ptr++;
      s1 = *cos_sin_ptr++;

      tempr = *in_ptr1;
      tempi = *in_ptr2;

      in_ptr1 += 2;
      in_ptr2 -= 2;

      temp =
          -ixheaacd_add32(ixheaacd_mult32(tempr, c), ixheaacd_mult32(tempi, s));
      *xptr++ = ixheaacd_shr32(temp, neg_expo);

      temp =
          ixheaacd_sub32(ixheaacd_mult32(tempr, s), ixheaacd_mult32(tempi, c));
      *xptr++ = ixheaacd_shr32(temp, neg_expo);

      tempr = *in_ptr1;
      tempi = *in_ptr2;

      in_ptr1 += 2;
      in_ptr2 -= 2;

      temp = -ixheaacd_add32(ixheaacd_mult32(tempr, c1),
                             ixheaacd_mult32(tempi, s1));
      *xptr++ = ixheaacd_shr32(temp, neg_expo);

      temp = ixheaacd_sub32(ixheaacd_mult32(tempr, s1),
                            ixheaacd_mult32(tempi, c1));
      *xptr++ = ixheaacd_shr32(temp, neg_expo);
    }
  } else {
    neg_expo = -neg_expo;

    for (i = npoints_4 - 1; i >= 0; i--) {
      c = *cos_sin_ptr++;
      c1 = *cos_sin_ptr++;
      s = *cos_sin_ptr++;
      s1 = *cos_sin_ptr++;

      tempr = *in_ptr1;
      tempi = *in_ptr2;

      in_ptr1 += 2;
      in_ptr2 -= 2;

      temp =
          -ixheaacd_add32(ixheaacd_mult32(tempr, c), ixheaacd_mult32(tempi, s));
      *xptr++ = ixheaacd_shl32(temp, neg_expo);

      temp =
          ixheaacd_sub32(ixheaacd_mult32(tempr, s), ixheaacd_mult32(tempi, c));
      *xptr++ = ixheaacd_shl32(temp, neg_expo);

      tempr = *in_ptr1;
      tempi = *in_ptr2;

      in_ptr1 += 2;
      in_ptr2 -= 2;

      temp = -ixheaacd_add32(ixheaacd_mult32(tempr, c1),
                             ixheaacd_mult32(tempi, s1));
      *xptr++ = ixheaacd_shl32(temp, neg_expo);

      temp = ixheaacd_sub32(ixheaacd_mult32(tempr, s1),
                            ixheaacd_mult32(tempi, c1));
      *xptr++ = ixheaacd_shl32(temp, neg_expo);
    }
  }
}

VOID ixheaacd_post_twiddle_ld(WORD32 out[], WORD32 x[],
                              const WORD32 *cos_sin_ptr, WORD m) {
  WORD i;

  WORD32 *ptr_x = &x[0];
  WORD32 *ptr_out, *ptr_out1;

  ptr_out = &out[0];
  ptr_out1 = &out[m - 1];

  for (i = (m >> 2) - 1; i >= 0; i--) {
    WORD32 c, c1, s, s1;
    WORD32 re, im;

    c = *cos_sin_ptr++;
    c1 = *cos_sin_ptr++;
    s = *cos_sin_ptr++;
    s1 = *cos_sin_ptr++;

    re = *ptr_x++;
    im = *ptr_x++;

    *ptr_out1 = ixheaacd_sub32(ixheaacd_mult32(im, c), ixheaacd_mult32(re, s));

    *ptr_out = -ixheaacd_add32(ixheaacd_mult32(re, c), ixheaacd_mult32(im, s));

    ptr_out += 2;
    ptr_out1 -= 2;

    re = *ptr_x++;
    im = *ptr_x++;

    *ptr_out1 =
        ixheaacd_sub32(ixheaacd_mult32(im, c1), ixheaacd_mult32(re, s1));
    *ptr_out =
        -ixheaacd_add32(ixheaacd_mult32(re, c1), ixheaacd_mult32(im, s1));

    ptr_out += 2;
    ptr_out1 -= 2;
  }
}

VOID ixheaacd_post_twiddle_eld(WORD32 out[], WORD32 x[],
                               const WORD32 *cos_sin_ptr, WORD m) {
  WORD i = 0;

  WORD32 *ptr_x = &x[0];
  WORD32 *ptr_out_767, *ptr_out_256;
  WORD32 *ptr_out_768, *ptr_out_255;
  WORD32 *ptr_out_0, *ptr_out_1279;
  WORD32 tempr, tempi;

  ptr_out_767 = &out[m + (m >> 1) - 1 - 2 * i];
  ptr_out_256 = &out[(m >> 1) + 2 * i];

  ptr_out_768 = &out[m + (m >> 1) + 2 * i];
  ptr_out_255 = &out[(m >> 1) - 1 - 2 * i];

  for (i = 0; i < (m >> 3); i++) {
    WORD32 c, c1, s, s1;
    WORD32 re, im;

    c = *cos_sin_ptr++;
    c1 = *cos_sin_ptr++;
    s = *cos_sin_ptr++;
    s1 = *cos_sin_ptr++;

    re = *ptr_x++;
    im = *ptr_x++;

    tempi = ixheaacd_sub32(ixheaacd_mult32(im, c), ixheaacd_mult32(re, s));
    tempr = -ixheaacd_add32(ixheaacd_mult32(re, c), ixheaacd_mult32(im, s));

    *ptr_out_767 = tempr;
    *ptr_out_256 = tempi;

    *ptr_out_768 = *ptr_out_767;
    *ptr_out_255 = -*ptr_out_256;

    ptr_out_256 += 2;
    ptr_out_767 -= 2;
    ptr_out_768 += 2;
    ptr_out_255 -= 2;

    re = *ptr_x++;
    im = *ptr_x++;

    tempi = ixheaacd_sub32(ixheaacd_mult32(im, c1), ixheaacd_mult32(re, s1));
    tempr = -ixheaacd_add32(ixheaacd_mult32(re, c1), ixheaacd_mult32(im, s1));

    *ptr_out_767 = tempr;
    *ptr_out_256 = tempi;

    *ptr_out_768 = *ptr_out_767;
    *ptr_out_255 = -*ptr_out_256;

    ptr_out_256 += 2;
    ptr_out_767 -= 2;
    ptr_out_768 += 2;
    ptr_out_255 -= 2;
  }

  ptr_out_0 = &out[2 * 2 * i - (m >> 1)];
  ptr_out_1279 = &out[m + m + (m >> 1) - 1 - 2 * 2 * i];

  for (; i < (m >> 2); i++) {
    WORD32 c, c1, s, s1;
    WORD32 re, im;

    c = *cos_sin_ptr++;
    c1 = *cos_sin_ptr++;
    s = *cos_sin_ptr++;
    s1 = *cos_sin_ptr++;

    re = *ptr_x++;
    im = *ptr_x++;

    tempi = ixheaacd_sub32(ixheaacd_mult32(im, c), ixheaacd_mult32(re, s));
    tempr = -ixheaacd_add32(ixheaacd_mult32(re, c), ixheaacd_mult32(im, s));

    *ptr_out_767 = tempr;
    *ptr_out_256 = tempi;

    *ptr_out_0 = -*ptr_out_767;
    *ptr_out_1279 = *ptr_out_256;

    ptr_out_256 += 2;
    ptr_out_767 -= 2;
    ptr_out_0 += 2;
    ptr_out_1279 -= 2;

    re = *ptr_x++;
    im = *ptr_x++;

    tempi = ixheaacd_sub32(ixheaacd_mult32(im, c1), ixheaacd_mult32(re, s1));
    tempr = -ixheaacd_add32(ixheaacd_mult32(re, c1), ixheaacd_mult32(im, s1));

    *ptr_out_767 = tempr;
    *ptr_out_256 = tempi;

    *ptr_out_0 = -*ptr_out_767;
    *ptr_out_1279 = *ptr_out_256;

    ptr_out_256 += 2;
    ptr_out_767 -= 2;
    ptr_out_0 += 2;
    ptr_out_1279 -= 2;
  }
}

VOID ixheaacd_fft32x32_ld_dec(ia_aac_dec_imdct_tables_struct *imdct_tables_ptr,
                              WORD32 npoints, WORD32 *ptr_x, WORD32 *ptr_y) {
  WORD32 i, j, l1, l2, h2, predj, tw_offset, stride, fft_jmp;
  WORD32 xt0_0, yt0_0, xt1_0, yt1_0, xt2_0, yt2_0;
  WORD32 xh0_0, xh1_0, xh20_0, xh21_0, xl0_0, xl1_0, xl20_0, xl21_0;
  WORD32 xh0_1, xh1_1, xl0_1, xl1_1;
  WORD32 x_0, x_1, x_2, x_3, x_l1_0, x_l1_1, x_l2_0, x_l2_1;
  WORD32 xh0_2, xh1_2, xl0_2, xl1_2, xh0_3, xh1_3, xl0_3, xl1_3;
  WORD32 x_4, x_5, x_6, x_7, x_h2_0, x_h2_1;
  WORD32 x_8, x_9, x_a, x_b, x_c, x_d, x_e, x_f;
  WORD32 si10, si20, si30, co10, co20, co30;
  WORD32 *w;
  WORD32 *x, *x2, *x0;
  WORD32 *y0, *y1, *y2, *y3;
  WORD32 n00, n10, n20, n30, n01, n11, n21, n31;
  WORD32 n02, n12, n22, n32, n03, n13, n23, n33;
  WORD32 n0, j0;
  WORD32 radix;
  WORD32 norm;
  WORD32 m;
  WORD32 *ptr_w;

  if (npoints == 256)
    ptr_w = imdct_tables_ptr->w_256;
  else
    ptr_w = imdct_tables_ptr->w_16;

  for (i = 31, m = 1; (npoints & (1 << i)) == 0; i--, m++)
    ;
  radix = m & 1 ? 2 : 4;
  norm = m - 2;

  stride = npoints;
  tw_offset = 0;
  fft_jmp = 6 * stride;

  while (stride > radix) {
    j = 0;
    fft_jmp >>= 2;

    h2 = stride >> 1;
    l1 = stride;
    l2 = stride + (stride >> 1);

    x = ptr_x;
    w = ptr_w + tw_offset;
    tw_offset += fft_jmp;

    stride >>= 2;

    for (i = 0; i < npoints; i += 4) {
      co10 = w[j + 1];
      si10 = w[j + 0];
      co20 = w[j + 3];
      si20 = w[j + 2];
      co30 = w[j + 5];
      si30 = w[j + 4];

      x_0 = x[0];
      x_1 = x[1];
      x_l1_0 = x[l1];
      x_l1_1 = x[l1 + 1];
      x_l2_0 = x[l2];
      x_l2_1 = x[l2 + 1];
      x_h2_0 = x[h2];
      x_h2_1 = x[h2 + 1];

      xh0_0 = x_0 + x_l1_0;
      xh1_0 = x_1 + x_l1_1;
      xl0_0 = x_0 - x_l1_0;
      xl1_0 = x_1 - x_l1_1;
      xh20_0 = x_h2_0 + x_l2_0;
      xh21_0 = x_h2_1 + x_l2_1;
      xl20_0 = x_h2_0 - x_l2_0;
      xl21_0 = x_h2_1 - x_l2_1;

      x0 = x;
      x2 = x0;

      j += 6;
      x += 2;
      predj = (j - fft_jmp);
      if (!predj) x += fft_jmp;
      if (!predj) j = 0;

      x0[0] = xh0_0 + xh20_0;
      x0[1] = xh1_0 + xh21_0;
      xt0_0 = xh0_0 - xh20_0;
      yt0_0 = xh1_0 - xh21_0;
      xt1_0 = xl0_0 + xl21_0;
      yt2_0 = xl1_0 + xl20_0;
      xt2_0 = xl0_0 - xl21_0;
      yt1_0 = xl1_0 - xl20_0;

      x2[h2] =
          MPYHIRC(si10, yt1_0) + MPYHIRC(co10, xt1_0) +
          (((MPYLUHS(si10, yt1_0) + MPYLUHS(co10, xt1_0) + 0x8000) >> 16) << 1);

      x2[h2 + 1] =
          MPYHIRC(co10, yt1_0) - MPYHIRC(si10, xt1_0) +
          (((MPYLUHS(co10, yt1_0) - MPYLUHS(si10, xt1_0) + 0x8000) >> 16) << 1);

      x2[l1] =
          MPYHIRC(si20, yt0_0) + MPYHIRC(co20, xt0_0) +
          (((MPYLUHS(si20, yt0_0) + MPYLUHS(co20, xt0_0) + 0x8000) >> 16) << 1);

      x2[l1 + 1] =
          MPYHIRC(co20, yt0_0) - MPYHIRC(si20, xt0_0) +
          (((MPYLUHS(co20, yt0_0) - MPYLUHS(si20, xt0_0) + 0x8000) >> 16) << 1);

      x2[l2] =
          MPYHIRC(si30, yt2_0) + MPYHIRC(co30, xt2_0) +
          (((MPYLUHS(si30, yt2_0) + MPYLUHS(co30, xt2_0) + 0x8000) >> 16) << 1);

      x2[l2 + 1] =
          MPYHIRC(co30, yt2_0) - MPYHIRC(si30, xt2_0) +
          (((MPYLUHS(co30, yt2_0) - MPYLUHS(si30, xt2_0) + 0x8000) >> 16) << 1);
    }
  }

  y0 = ptr_y;
  y2 = ptr_y + (WORD32)npoints;
  x0 = ptr_x;
  x2 = ptr_x + (WORD32)(npoints >> 1);

  if (radix == 2) {
    y1 = y0 + (WORD32)(npoints >> 2);
    y3 = y2 + (WORD32)(npoints >> 2);
    l1 = norm + 1;
    j0 = 8;
    n0 = npoints >> 1;
  } else {
    y1 = y0 + (WORD32)(npoints >> 1);
    y3 = y2 + (WORD32)(npoints >> 1);
    l1 = norm + 2;
    j0 = 4;
    n0 = npoints >> 2;
  }

  j = 0;

  for (i = 0; i < npoints; i += 8) {
    DIG_REV(j, l1, h2);

    x_0 = x0[0];
    x_1 = x0[1];
    x_2 = x0[2];
    x_3 = x0[3];
    x_4 = x0[4];
    x_5 = x0[5];
    x_6 = x0[6];
    x_7 = x0[7];
    x0 += 8;

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

    if (radix == 2) {
      n00 = x_0 + x_2;
      n01 = x_1 + x_3;
      n20 = x_0 - x_2;
      n21 = x_1 - x_3;
      n10 = x_4 + x_6;
      n11 = x_5 + x_7;
      n30 = x_4 - x_6;
      n31 = x_5 - x_7;
    }

    y0[2 * h2] = n00;
    y0[2 * h2 + 1] = n01;
    y1[2 * h2] = n10;
    y1[2 * h2 + 1] = n11;
    y2[2 * h2] = n20;
    y2[2 * h2 + 1] = n21;
    y3[2 * h2] = n30;
    y3[2 * h2 + 1] = n31;

    x_8 = x2[0];
    x_9 = x2[1];
    x_a = x2[2];
    x_b = x2[3];
    x_c = x2[4];
    x_d = x2[5];
    x_e = x2[6];
    x_f = x2[7];
    x2 += 8;

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

    if (radix == 2) {
      n02 = x_8 + x_a;
      n03 = x_9 + x_b;
      n22 = x_8 - x_a;
      n23 = x_9 - x_b;
      n12 = x_c + x_e;
      n13 = x_d + x_f;
      n32 = x_c - x_e;
      n33 = x_d - x_f;
    }

    y0[2 * h2 + 2] = n02;
    y0[2 * h2 + 3] = n03;
    y1[2 * h2 + 2] = n12;
    y1[2 * h2 + 3] = n13;
    y2[2 * h2 + 2] = n22;
    y2[2 * h2 + 3] = n23;
    y3[2 * h2 + 2] = n32;
    y3[2 * h2 + 3] = n33;

    j += j0;

    if (j == n0) {
      j += n0;
      x0 += (WORD32)npoints >> 1;
      x2 += (WORD32)npoints >> 1;
    }
  }
}

VOID ixheaacd_rearrange_dec(WORD32 *ip, WORD32 *op, WORD32 mdct_len_2,
                            UWORD8 *re_arr_tab) {
  WORD32 n, i = 0;

  for (n = 0; n < mdct_len_2; n++) {
    WORD32 idx = re_arr_tab[n] << 1;

    op[i++] = ip[idx];
    op[i++] = ip[idx + 1];
  }
}

VOID ixheaacd_fft_15_ld_dec(WORD32 *inp, WORD32 *op, WORD32 *fft3out,
                            UWORD8 *re_arr_tab_sml_240_ptr) {
  WORD32 i, n, idx;
  WORD32 *buf1, *buf2, *buf1a;
  WORD32 add_r, sub_r;
  WORD32 add_i, sub_i;
  WORD32 x01_real, x_01_imag, temp;
  WORD32 p1, p2, p3, p4;

  WORD32 sinmu = 1859775393;
  WORD32 cos_51 = 2042378317;
  WORD32 cos_52 = -1652318768;
  WORD32 cos_53 = -780119100;
  WORD32 cos_54 = 1200479854;
  WORD32 cos_55 = -1342177280;

  WORD32 r1, r2, r3, r4;
  WORD32 s1, s2, s3, s4, t, temp1, temp2;
  WORD32 *fft3outptr = fft3out;

  WORD32 xr_0, xr_1, xr_2;
  WORD32 xi_0, xi_1, xi_2;

  buf2 = fft3out;
  buf1 = buf1a = fft3out;
  n = 0;

  {
    *buf1++ = inp[0];
    *buf1++ = inp[1];

    *buf1++ = inp[96];
    *buf1++ = inp[97];

    *buf1++ = inp[192];
    *buf1++ = inp[193];

    *buf1++ = inp[288];
    *buf1++ = inp[289];

    *buf1++ = inp[384];
    *buf1++ = inp[385];

    r1 = ixheaacd_add32_sat(buf1a[2], buf1a[8]);
    r4 = ixheaacd_sub32_sat(buf1a[2], buf1a[8]);
    r3 = ixheaacd_add32_sat(buf1a[4], buf1a[6]);
    r2 = ixheaacd_sub32_sat(buf1a[4], buf1a[6]);

    t = ixheaacd_mult32_shl(ixheaacd_sub32_sat(r1, r3), cos_54);

    r1 = ixheaacd_add32_sat(r1, r3);

    temp1 = ixheaacd_add32_sat(buf1a[0], r1);

    r1 = ixheaacd_add32_sat(
        temp1, ixheaacd_shl32_sat((ixheaacd_mult32_shl(r1, cos_55)), 1));

    r3 = ixheaacd_sub32_sat(r1, t);
    r1 = ixheaacd_add32_sat(r1, t);

    t = ixheaacd_mult32_shl(ixheaacd_add32_sat(r4, r2), cos_51);
    r4 = ixheaacd_add32_sat(
        t, ixheaacd_shl32_sat(ixheaacd_mult32_shl(r4, cos_52), 1));
    r2 = ixheaacd_add32_sat(t, ixheaacd_mult32_shl(r2, cos_53));

    s1 = ixheaacd_add32_sat(buf1a[3], buf1a[9]);
    s4 = ixheaacd_sub32_sat(buf1a[3], buf1a[9]);
    s3 = ixheaacd_add32_sat(buf1a[5], buf1a[7]);
    s2 = ixheaacd_sub32_sat(buf1a[5], buf1a[7]);

    t = ixheaacd_mult32_shl(ixheaacd_sub32_sat(s1, s3), cos_54);
    s1 = ixheaacd_add32_sat(s1, s3);

    temp2 = ixheaacd_add32_sat(buf1a[1], s1);

    s1 = ixheaacd_add32_sat(
        temp2, ixheaacd_shl32_sat((ixheaacd_mult32_shl(s1, cos_55)), 1));

    s3 = ixheaacd_sub32_sat(s1, t);
    s1 = ixheaacd_add32_sat(s1, t);

    t = ixheaacd_mult32_shl(ixheaacd_add32_sat(s4, s2), cos_51);
    s4 = ixheaacd_add32_sat(
        t, ixheaacd_shl32_sat((ixheaacd_mult32_shl(s4, cos_52)), 1));
    s2 = ixheaacd_add32_sat(t, (ixheaacd_mult32_shl(s2, cos_53)));

    *buf2++ = temp1;
    *buf2++ = temp2;
    *buf2++ = ixheaacd_add32_sat(r1, s2);
    *buf2++ = ixheaacd_sub32_sat(s1, r2);
    *buf2++ = ixheaacd_sub32_sat(r3, s4);
    *buf2++ = ixheaacd_add32_sat(s3, r4);
    *buf2++ = ixheaacd_add32_sat(r3, s4);
    *buf2++ = ixheaacd_sub32_sat(s3, r4);
    *buf2++ = ixheaacd_sub32_sat(r1, s2);
    *buf2++ = ixheaacd_add32_sat(s1, r2);
    buf1a = buf1;

    *buf1++ = inp[160];
    *buf1++ = inp[161];

    *buf1++ = inp[256];
    *buf1++ = inp[257];

    *buf1++ = inp[352];
    *buf1++ = inp[353];

    *buf1++ = inp[448];
    *buf1++ = inp[449];

    *buf1++ = inp[64];
    *buf1++ = inp[65];

    r1 = ixheaacd_add32_sat(buf1a[2], buf1a[8]);
    r4 = ixheaacd_sub32_sat(buf1a[2], buf1a[8]);
    r3 = ixheaacd_add32_sat(buf1a[4], buf1a[6]);
    r2 = ixheaacd_sub32_sat(buf1a[4], buf1a[6]);

    t = ixheaacd_mult32_shl(ixheaacd_sub32_sat(r1, r3), cos_54);

    r1 = ixheaacd_add32_sat(r1, r3);

    temp1 = ixheaacd_add32_sat(buf1a[0], r1);

    r1 = ixheaacd_add32_sat(
        temp1, ixheaacd_shl32_sat((ixheaacd_mult32_shl(r1, cos_55)), 1));

    r3 = ixheaacd_sub32_sat(r1, t);
    r1 = ixheaacd_add32_sat(r1, t);

    t = ixheaacd_mult32_shl(ixheaacd_add32_sat(r4, r2), cos_51);
    r4 = ixheaacd_add32_sat(
        t, ixheaacd_shl32_sat(ixheaacd_mult32_shl(r4, cos_52), 1));
    r2 = ixheaacd_add32_sat(t, ixheaacd_mult32_shl(r2, cos_53));

    s1 = ixheaacd_add32_sat(buf1a[3], buf1a[9]);
    s4 = ixheaacd_sub32_sat(buf1a[3], buf1a[9]);
    s3 = ixheaacd_add32_sat(buf1a[5], buf1a[7]);
    s2 = ixheaacd_sub32_sat(buf1a[5], buf1a[7]);

    t = ixheaacd_mult32_shl(ixheaacd_sub32_sat(s1, s3), cos_54);

    s1 = ixheaacd_add32_sat(s1, s3);

    temp2 = ixheaacd_add32_sat(buf1a[1], s1);

    s1 = ixheaacd_add32_sat(
        temp2, ixheaacd_shl32_sat((ixheaacd_mult32_shl(s1, cos_55)), 1));

    s3 = ixheaacd_sub32_sat(s1, t);
    s1 = ixheaacd_add32_sat(s1, t);

    t = ixheaacd_mult32_shl(ixheaacd_add32_sat(s4, s2), cos_51);
    s4 = ixheaacd_add32_sat(
        t, ixheaacd_shl32_sat((ixheaacd_mult32_shl(s4, cos_52)), 1));
    s2 = ixheaacd_add32_sat(t, (ixheaacd_mult32_shl(s2, cos_53)));

    *buf2++ = temp1;
    *buf2++ = temp2;
    *buf2++ = ixheaacd_add32_sat(r1, s2);
    *buf2++ = ixheaacd_sub32_sat(s1, r2);
    *buf2++ = ixheaacd_sub32_sat(r3, s4);
    *buf2++ = ixheaacd_add32_sat(s3, r4);
    *buf2++ = ixheaacd_add32_sat(r3, s4);
    *buf2++ = ixheaacd_sub32_sat(s3, r4);
    *buf2++ = ixheaacd_sub32_sat(r1, s2);
    *buf2++ = ixheaacd_add32_sat(s1, r2);
    buf1a = buf1;
    ;

    *buf1++ = inp[320];
    *buf1++ = inp[321];

    *buf1++ = inp[416];
    *buf1++ = inp[417];

    *buf1++ = inp[32];
    *buf1++ = inp[33];

    *buf1++ = inp[128];
    *buf1++ = inp[129];

    *buf1++ = inp[224];
    *buf1++ = inp[225];

    r1 = ixheaacd_add32_sat(buf1a[2], buf1a[8]);
    r4 = ixheaacd_sub32_sat(buf1a[2], buf1a[8]);
    r3 = ixheaacd_add32_sat(buf1a[4], buf1a[6]);
    r2 = ixheaacd_sub32_sat(buf1a[4], buf1a[6]);

    t = ixheaacd_mult32_shl(ixheaacd_sub32_sat(r1, r3), cos_54);

    r1 = ixheaacd_add32_sat(r1, r3);

    temp1 = ixheaacd_add32_sat(buf1a[0], r1);

    r1 = ixheaacd_add32_sat(
        temp1, ixheaacd_shl32_sat((ixheaacd_mult32_shl(r1, cos_55)), 1));

    r3 = ixheaacd_sub32_sat(r1, t);
    r1 = ixheaacd_add32_sat(r1, t);

    t = ixheaacd_mult32_shl(ixheaacd_add32_sat(r4, r2), cos_51);
    r4 = ixheaacd_add32_sat(
        t, ixheaacd_shl32_sat(ixheaacd_mult32_shl(r4, cos_52), 1));
    r2 = ixheaacd_add32_sat(t, ixheaacd_mult32_shl(r2, cos_53));

    s1 = ixheaacd_add32_sat(buf1a[3], buf1a[9]);
    s4 = ixheaacd_sub32_sat(buf1a[3], buf1a[9]);
    s3 = ixheaacd_add32_sat(buf1a[5], buf1a[7]);
    s2 = ixheaacd_sub32_sat(buf1a[5], buf1a[7]);

    t = ixheaacd_mult32_shl(ixheaacd_sub32_sat(s1, s3), cos_54);

    s1 = ixheaacd_add32_sat(s1, s3);

    temp2 = ixheaacd_add32_sat(buf1a[1], s1);

    s1 = ixheaacd_add32_sat(
        temp2, ixheaacd_shl32_sat((ixheaacd_mult32_shl(s1, cos_55)), 1));

    s3 = ixheaacd_sub32_sat(s1, t);
    s1 = ixheaacd_add32_sat(s1, t);

    t = ixheaacd_mult32_shl(ixheaacd_add32_sat(s4, s2), cos_51);
    s4 = ixheaacd_add32_sat(
        t, ixheaacd_shl32_sat((ixheaacd_mult32_shl(s4, cos_52)), 1));
    s2 = ixheaacd_add32_sat(t, (ixheaacd_mult32_shl(s2, cos_53)));

    *buf2++ = temp1;
    *buf2++ = temp2;
    *buf2++ = ixheaacd_add32_sat(r1, s2);
    *buf2++ = ixheaacd_sub32_sat(s1, r2);
    *buf2++ = ixheaacd_sub32_sat(r3, s4);
    *buf2++ = ixheaacd_add32_sat(s3, r4);
    *buf2++ = ixheaacd_add32_sat(r3, s4);
    *buf2++ = ixheaacd_sub32_sat(s3, r4);
    *buf2++ = ixheaacd_sub32_sat(r1, s2);
    *buf2++ = ixheaacd_add32_sat(s1, r2);
    buf1a = buf1;
    ;
  }

  n = 0;
  for (i = 0; i < FFT5; i++) {
    xr_0 = fft3outptr[0];
    xi_0 = fft3outptr[1];

    xr_1 = fft3outptr[10];
    xi_1 = fft3outptr[11];

    xr_2 = fft3outptr[20];
    xi_2 = fft3outptr[21];

    x01_real = ixheaacd_add32_sat(xr_0, xr_1);
    x_01_imag = ixheaacd_add32_sat(xi_0, xi_1);

    add_r = ixheaacd_add32_sat(xr_1, xr_2);
    add_i = ixheaacd_add32_sat(xi_1, xi_2);

    sub_r = ixheaacd_sub32_sat(xr_1, xr_2);
    sub_i = ixheaacd_sub32_sat(xi_1, xi_2);

    p1 = add_r >> 1;

    p2 = ixheaacd_mult32_shl(sub_i, sinmu);
    p3 = ixheaacd_mult32_shl(sub_r, sinmu);

    p4 = add_i >> 1;

    temp = ixheaacd_sub32_sat(xr_0, p1);
    temp1 = ixheaacd_add32_sat(xi_0, p3);
    temp2 = ixheaacd_sub32_sat(xi_0, p3);

    idx = re_arr_tab_sml_240_ptr[n++] << 1;
    op[idx] = ixheaacd_add32_sat(x01_real, xr_2);
    op[idx + 1] = ixheaacd_add32_sat(x_01_imag, xi_2);

    idx = re_arr_tab_sml_240_ptr[n++] << 1;
    op[idx] = ixheaacd_add32_sat(temp, p2);
    op[idx + 1] = ixheaacd_sub32_sat(temp2, p4);

    idx = re_arr_tab_sml_240_ptr[n++] << 1;
    op[idx] = ixheaacd_sub32_sat(temp, p2);
    op[idx + 1] = ixheaacd_sub32_sat(temp1, p4);
    fft3outptr += 2;
  }
}