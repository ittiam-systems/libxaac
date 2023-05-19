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
#include "ixheaace_adjust_threshold_data.h"
#include "iusace_cnst.h"
#include "iusace_rom.h"
#include "iusace_bitbuffer.h"

#include "iusace_config.h"
#include "iusace_fft.h"
#include "iusace_basic_ops_flt.h"
#include "ixheaac_constants.h"
#include "ixheaace_aac_constants.h"
#include "ixheaac_basic_ops32.h"
#include "ixheaace_common_utils.h"

#define DIG_REV(i, m, j)                                    \
  do {                                                      \
    unsigned _ = (i);                                       \
    _ = ((_ & 0x33333333) << 2) | ((_ & ~0x33333333) >> 2); \
    _ = ((_ & 0x0F0F0F0F) << 4) | ((_ & ~0x0F0F0F0F) >> 4); \
    _ = ((_ & 0x00FF00FF) << 8) | ((_ & ~0x00FF00FF) >> 8); \
    (j) = _ >> (m);                                         \
  } while (0)

static PLATFORM_INLINE WORD8 iusace_calc_norm(WORD32 a) {
  WORD8 norm_val;

  if (a == 0) {
    norm_val = 31;
  } else {
    if (a == (WORD32)0xffffffffL) {
      norm_val = 31;
    } else {
      if (a < 0) {
        a = ~a;
      }
      for (norm_val = 0; a < (WORD32)0x40000000L; norm_val++) {
        a <<= 1;
      }
    }
  }

  return norm_val;
}

static PLATFORM_INLINE VOID iusace_complex_3point_fft(FLOAT32 *ptr_in, FLOAT32 *ptr_out) {
  FLOAT32 add_r, sub_r;
  FLOAT32 add_i, sub_i;
  FLOAT32 x01r, x01i, temp;
  FLOAT32 p1, p2, p3, p4;
  FLOAT64 sinmu;

  sinmu = 0.866025403784439;

  x01r = ptr_in[0] + ptr_in[2];
  x01i = ptr_in[1] + ptr_in[3];

  add_r = ptr_in[2] + ptr_in[4];
  add_i = ptr_in[3] + ptr_in[5];

  sub_r = ptr_in[2] - ptr_in[4];
  sub_i = ptr_in[3] - ptr_in[5];

  p1 = add_r / (FLOAT32)2.0;
  p4 = add_i / (FLOAT32)2.0;
  p2 = (FLOAT32)((FLOAT64)sub_i * sinmu);
  p3 = (FLOAT32)((FLOAT64)sub_r * sinmu);

  temp = ptr_in[0] - p1;

  ptr_out[0] = x01r + ptr_in[4];
  ptr_out[1] = x01i + ptr_in[5];
  ptr_out[2] = temp + p2;
  ptr_out[3] = (ptr_in[1] - p3) - p4;
  ptr_out[4] = temp - p2;
  ptr_out[5] = (ptr_in[1] + p3) - p4;

  return;
}

VOID iusace_complex_fft_p2(FLOAT32 *ptr_x, WORD32 nlength, FLOAT32 *scratch_fft_p2_y) {
  WORD32 i, j, k, n_stages, h2;
  FLOAT32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
  FLOAT32 tmp;
  WORD32 del, nodespacing, in_loop_cnt;
  WORD32 not_power_4;
  WORD32 dig_rev_shift;
  FLOAT32 *y = scratch_fft_p2_y;
  WORD32 mpass = nlength;
  WORD32 npoints = nlength;
  FLOAT32 *ptr_y = y;
  const FLOAT64 *ptr_w;

  dig_rev_shift = iusace_calc_norm(mpass) + 1 - 16;
  n_stages = 30 - iusace_calc_norm(mpass);
  not_power_4 = n_stages & 1;

  n_stages = n_stages >> 1;

  ptr_w = iusace_twiddle_table_fft_32x32;

  if (dig_rev_shift < 0) {
    dig_rev_shift = 0;
  }

  for (i = 0; i < npoints; i += 4) {
    FLOAT32 *inp = ptr_x;
    FLOAT32 tmk;

    DIG_REV(i, dig_rev_shift, h2);
    if (not_power_4) {
      h2 += 1;
      h2 &= ~1;
    }
    inp += (h2);

    x0r = *inp;
    x0i = *(inp + 1);
    inp += (npoints >> 1);

    x1r = *inp;
    x1i = *(inp + 1);
    inp += (npoints >> 1);

    x2r = *inp;
    x2i = *(inp + 1);
    inp += (npoints >> 1);

    x3r = *inp;
    x3i = *(inp + 1);

    x0r = x0r + x2r;
    x0i = x0i + x2i;

    tmk = x0r - x2r;
    x2r = tmk - x2r;
    tmk = x0i - x2i;
    x2i = tmk - x2i;

    x1r = x1r + x3r;
    x1i = x1i + x3i;

    tmk = x1r - x3r;
    x3r = tmk - x3r;
    tmk = x1i - x3i;
    x3i = tmk - x3i;

    x0r = x0r + x1r;
    x0i = x0i + x1i;

    tmk = x0r - x1r;
    x1r = tmk - x1r;
    tmk = x0i - x1i;
    x1i = tmk - x1i;

    x2r = x2r + x3i;
    x2i = x2i - x3r;

    tmk = x2r - x3i;
    x3i = tmk - x3i;
    tmk = x2i + x3r;
    x3r = tmk + x3r;

    *ptr_y++ = x0r;
    *ptr_y++ = x0i;
    *ptr_y++ = x2r;
    *ptr_y++ = x2i;
    *ptr_y++ = x1r;
    *ptr_y++ = x1i;
    *ptr_y++ = x3i;
    *ptr_y++ = x3r;
  }
  ptr_y -= 2 * npoints;
  del = 4;
  nodespacing = 64;
  in_loop_cnt = npoints >> 4;
  for (i = n_stages - 1; i > 0; i--) {
    const FLOAT64 *twiddles = ptr_w;
    FLOAT32 *data = ptr_y;
    FLOAT64 w_1, w_2, w_3, w_4, w_5, w_6;
    WORD32 sec_loop_cnt;

    for (k = in_loop_cnt; k != 0; k--) {
      x0r = (*data);
      x0i = (*(data + 1));
      data += ((SIZE_T)del << 1);

      x1r = (*data);
      x1i = (*(data + 1));
      data += ((SIZE_T)del << 1);

      x2r = (*data);
      x2i = (*(data + 1));
      data += ((SIZE_T)del << 1);

      x3r = (*data);
      x3i = (*(data + 1));
      data -= 3 * (del << 1);

      x0r = x0r + x2r;
      x0i = x0i + x2i;
      x2r = x0r - (x2r * 2);
      x2i = x0i - (x2i * 2);
      x1r = x1r + x3r;
      x1i = x1i + x3i;
      x3r = x1r - (x3r * 2);
      x3i = x1i - (x3i * 2);

      x0r = x0r + x1r;
      x0i = x0i + x1i;
      x1r = x0r - (x1r * 2);
      x1i = x0i - (x1i * 2);
      x2r = x2r + x3i;
      x2i = x2i - x3r;
      x3i = x2r - (x3i * 2);
      x3r = x2i + (x3r * 2);

      *data = x0r;
      *(data + 1) = x0i;
      data += ((SIZE_T)del << 1);

      *data = x2r;
      *(data + 1) = x2i;
      data += ((SIZE_T)del << 1);

      *data = x1r;
      *(data + 1) = x1i;
      data += ((SIZE_T)del << 1);

      *data = x3i;
      *(data + 1) = x3r;
      data += ((SIZE_T)del << 1);
    }
    data = ptr_y + 2;

    sec_loop_cnt = (nodespacing * del);
    sec_loop_cnt = (sec_loop_cnt / 4) + (sec_loop_cnt / 8) - (sec_loop_cnt / 16) +
                   (sec_loop_cnt / 32) - (sec_loop_cnt / 64) + (sec_loop_cnt / 128) -
                   (sec_loop_cnt / 256);

    for (j = nodespacing; j <= sec_loop_cnt; j += nodespacing) {
      w_1 = *(twiddles + j);
      w_4 = *(twiddles + j + 257);
      w_2 = *(twiddles + ((SIZE_T)j << 1));
      w_5 = *(twiddles + ((SIZE_T)j << 1) + 257);
      w_3 = *(twiddles + j + ((SIZE_T)j << 1));
      w_6 = *(twiddles + j + ((SIZE_T)j << 1) + 257);

      for (k = in_loop_cnt; k != 0; k--) {
        data += ((SIZE_T)del << 1);

        x1r = *data;
        x1i = *(data + 1);
        data += ((SIZE_T)del << 1);

        x2r = *data;
        x2i = *(data + 1);
        data += ((SIZE_T)del << 1);

        x3r = *data;
        x3i = *(data + 1);
        data -= 3 * (del << 1);

        tmp = (FLOAT32)(ixheaace_dmult((FLOAT64)x1r, w_1) - ixheaace_dmult((FLOAT64)x1i, w_4));
        x1i = (FLOAT32)ixheaace_dmac(ixheaace_dmult((FLOAT64)x1r, w_4), (FLOAT64)x1i, w_1);
        x1r = tmp;

        tmp = (FLOAT32)(ixheaace_dmult((FLOAT64)x2r, w_2) - ixheaace_dmult((FLOAT64)x2i, w_5));
        x2i = (FLOAT32)ixheaace_dmac(ixheaace_dmult((FLOAT64)x2r, w_5), (FLOAT64)x2i, w_2);
        x2r = tmp;

        tmp = (FLOAT32)(ixheaace_dmult((FLOAT64)x3r, w_3) - ixheaace_dmult((FLOAT64)x3i, w_6));
        x3i = (FLOAT32)ixheaace_dmac(ixheaace_dmult((FLOAT64)x3r, w_6), (FLOAT64)x3i, w_3);
        x3r = tmp;

        x0r = (*data);
        x0i = (*(data + 1));

        x0r = x0r + (x2r);
        x0i = x0i + (x2i);
        x2r = x0r - (x2r * 2);
        x2i = x0i - (x2i * 2);
        x1r = x1r + x3r;
        x1i = x1i + x3i;
        x3r = x1r - (x3r * 2);
        x3i = x1i - (x3i * 2);

        x0r = x0r + (x1r);
        x0i = x0i + (x1i);
        x1r = x0r - (x1r * 2);
        x1i = x0i - (x1i * 2);
        x2r = x2r + (x3i);
        x2i = x2i - (x3r);
        x3i = x2r - (x3i * 2);
        x3r = x2i + (x3r * 2);

        *data = x0r;
        *(data + 1) = x0i;
        data += ((SIZE_T)del << 1);

        *data = x2r;
        *(data + 1) = x2i;
        data += ((SIZE_T)del << 1);

        *data = x1r;
        *(data + 1) = x1i;
        data += ((SIZE_T)del << 1);

        *data = x3i;
        *(data + 1) = x3r;
        data += ((SIZE_T)del << 1);
      }
      data -= 2 * npoints;
      data += 2;
    }
    for (; j <= (nodespacing * del) >> 1; j += nodespacing) {
      w_1 = *(twiddles + j);
      w_4 = *(twiddles + j + 257);
      w_2 = *(twiddles + ((SIZE_T)j << 1));
      w_5 = *(twiddles + ((SIZE_T)j << 1) + 257);
      w_3 = *(twiddles + j + ((SIZE_T)j << 1) - 256);
      w_6 = *(twiddles + j + ((SIZE_T)j << 1) + 1);

      for (k = in_loop_cnt; k != 0; k--) {
        data += ((SIZE_T)del << 1);

        x1r = *data;
        x1i = *(data + 1);
        data += ((SIZE_T)del << 1);

        x2r = *data;
        x2i = *(data + 1);
        data += ((SIZE_T)del << 1);

        x3r = *data;
        x3i = *(data + 1);
        data -= 3 * (del << 1);

        tmp = (FLOAT32)(ixheaace_dmult((FLOAT64)x1r, w_1) - ixheaace_dmult((FLOAT64)x1i, w_4));
        x1i = (FLOAT32)ixheaace_dmac(ixheaace_dmult((FLOAT64)x1r, w_4), (FLOAT64)x1i, w_1);
        x1r = tmp;

        tmp = (FLOAT32)(ixheaace_dmult((FLOAT64)x2r, w_2) - ixheaace_dmult((FLOAT64)x2i, w_5));
        x2i = (FLOAT32)ixheaace_dmac(ixheaace_dmult((FLOAT64)x2r, w_5), (FLOAT64)x2i, w_2);
        x2r = tmp;

        tmp = (FLOAT32)(ixheaace_dmult((FLOAT64)x3r, w_6) + ixheaace_dmult((FLOAT64)x3i, w_3));
        x3i = (FLOAT32)(-ixheaace_dmult((FLOAT64)x3r, w_3) + ixheaace_dmult((FLOAT64)x3i, w_6));
        x3r = tmp;

        x0r = (*data);
        x0i = (*(data + 1));

        x0r = x0r + (x2r);
        x0i = x0i + (x2i);
        x2r = x0r - (x2r * 2);
        x2i = x0i - (x2i * 2);
        x1r = x1r + x3r;
        x1i = x1i + x3i;
        x3r = x1r - (x3r * 2);
        x3i = x1i - (x3i * 2);

        x0r = x0r + (x1r);
        x0i = x0i + (x1i);
        x1r = x0r - (x1r * 2);
        x1i = x0i - (x1i * 2);
        x2r = x2r + (x3i);
        x2i = x2i - (x3r);
        x3i = x2r - (x3i * 2);
        x3r = x2i + (x3r * 2);

        *data = x0r;
        *(data + 1) = x0i;
        data += ((SIZE_T)del << 1);

        *data = x2r;
        *(data + 1) = x2i;
        data += ((SIZE_T)del << 1);

        *data = x1r;
        *(data + 1) = x1i;
        data += ((SIZE_T)del << 1);

        *data = x3i;
        *(data + 1) = x3r;
        data += ((SIZE_T)del << 1);
      }
      data -= 2 * npoints;
      data += 2;
    }
    for (; j <= sec_loop_cnt * 2; j += nodespacing) {
      w_1 = *(twiddles + j);
      w_4 = *(twiddles + j + 257);
      w_2 = *(twiddles + ((SIZE_T)j << 1) - 256);
      w_5 = *(twiddles + ((SIZE_T)j << 1) + 1);
      w_3 = *(twiddles + j + ((SIZE_T)j << 1) - 256);
      w_6 = *(twiddles + j + ((SIZE_T)j << 1) + 1);

      for (k = in_loop_cnt; k != 0; k--) {
        data += ((SIZE_T)del << 1);

        x1r = *data;
        x1i = *(data + 1);
        data += ((SIZE_T)del << 1);

        x2r = *data;
        x2i = *(data + 1);
        data += ((SIZE_T)del << 1);

        x3r = *data;
        x3i = *(data + 1);
        data -= 3 * (del << 1);

        tmp = (FLOAT32)(ixheaace_dmult((FLOAT64)x1r, w_1) - ixheaace_dmult((FLOAT64)x1i, w_4));
        x1i = (FLOAT32)ixheaace_dmac(ixheaace_dmult(x1r, w_4), x1i, w_1);
        x1r = tmp;

        tmp = (FLOAT32)(ixheaace_dmult((FLOAT64)x2r, w_5) + ixheaace_dmult((FLOAT64)x2i, w_2));
        x2i = (FLOAT32)(-ixheaace_dmult(x2r, w_2) + ixheaace_dmult(x2i, w_5));
        x2r = tmp;

        tmp = (FLOAT32)(ixheaace_dmult((FLOAT64)x3r, w_6) + ixheaace_dmult((FLOAT64)x3i, w_3));
        x3i = (FLOAT32)(-ixheaace_dmult((FLOAT64)x3r, w_3) + ixheaace_dmult((FLOAT64)x3i, w_6));
        x3r = tmp;

        x0r = (*data);
        x0i = (*(data + 1));

        x0r = x0r + (x2r);
        x0i = x0i + (x2i);
        x2r = x0r - (x2r * 2);
        x2i = x0i - (x2i * 2);
        x1r = x1r + x3r;
        x1i = x1i + x3i;
        x3r = x1r - (x3r * 2);
        x3i = x1i - (x3i * 2);

        x0r = x0r + (x1r);
        x0i = x0i + (x1i);
        x1r = x0r - (x1r * 2);
        x1i = x0i - (x1i * 2);
        x2r = x2r + (x3i);
        x2i = x2i - (x3r);
        x3i = x2r - (x3i * 2);
        x3r = x2i + (x3r * 2);

        *data = x0r;
        *(data + 1) = x0i;
        data += ((SIZE_T)del << 1);

        *data = x2r;
        *(data + 1) = x2i;
        data += ((SIZE_T)del << 1);

        *data = x1r;
        *(data + 1) = x1i;
        data += ((SIZE_T)del << 1);

        *data = x3i;
        *(data + 1) = x3r;
        data += ((SIZE_T)del << 1);
      }
      data -= 2 * npoints;
      data += 2;
    }
    for (; j < nodespacing * del; j += nodespacing) {
      w_1 = *(twiddles + j);
      w_4 = *(twiddles + j + 257);
      w_2 = *(twiddles + ((SIZE_T)j << 1) - 256);
      w_5 = *(twiddles + ((SIZE_T)j << 1) + 1);
      w_3 = *(twiddles + j + ((SIZE_T)j << 1) - 512);
      w_6 = *(twiddles + j + ((SIZE_T)j << 1) - 512 + 257);

      for (k = in_loop_cnt; k != 0; k--) {
        data += ((SIZE_T)del << 1);

        x1r = *data;
        x1i = *(data + 1);
        data += ((SIZE_T)del << 1);

        x2r = *data;
        x2i = *(data + 1);
        data += ((SIZE_T)del << 1);

        x3r = *data;
        x3i = *(data + 1);
        data -= 3 * ((SIZE_T)del << 1);

        tmp = (FLOAT32)(ixheaace_dmult((FLOAT64)x1r, w_1) - ixheaace_dmult((FLOAT64)x1i, w_4));
        x1i = (FLOAT32)ixheaace_dmac(ixheaace_dmult((FLOAT64)x1r, w_4), (FLOAT64)x1i, w_1);
        x1r = tmp;

        tmp = (FLOAT32)(ixheaace_dmult((FLOAT64)x2r, w_5) + ixheaace_dmult((FLOAT64)x2i, w_2));
        x2i = (FLOAT32)(-ixheaace_dmult((FLOAT64)x2r, w_2) + ixheaace_dmult((FLOAT64)x2i, w_5));
        x2r = tmp;

        tmp = (FLOAT32)(-ixheaace_dmult((FLOAT64)x3r, w_3) + ixheaace_dmult((FLOAT64)x3i, w_6));
        x3i = (FLOAT32)ixheaace_dmac(ixheaace_dmult((FLOAT64)x3r, w_6), (FLOAT64)x3i, w_3);
        x3r = tmp;

        x0r = (*data);
        x0i = (*(data + 1));

        x0r = x0r + (x2r);
        x0i = x0i + (x2i);
        x2r = x0r - (x2r * 2);
        x2i = x0i - (x2i * 2);
        x1r = x1r + x3r;
        x1i = x1i - x3i;
        x3r = x1r - (x3r * 2);
        x3i = x1i + (x3i * 2);

        x0r = x0r + (x1r);
        x0i = x0i + (x1i);
        x1r = x0r - (x1r * 2);
        x1i = x0i - (x1i * 2);
        x2r = x2r + (x3i);
        x2i = x2i - (x3r);
        x3i = x2r - (x3i * 2);
        x3r = x2i + (x3r * 2);

        *data = x0r;
        *(data + 1) = x0i;
        data += ((SIZE_T)del << 1);

        *data = x2r;
        *(data + 1) = x2i;
        data += ((SIZE_T)del << 1);

        *data = x1r;
        *(data + 1) = x1i;
        data += ((SIZE_T)del << 1);

        *data = x3i;
        *(data + 1) = x3r;
        data += ((SIZE_T)del << 1);
      }
      data -= 2 * npoints;
      data += 2;
    }
    nodespacing >>= 2;
    del <<= 2;
    in_loop_cnt >>= 2;
  }
  if (not_power_4) {
    const FLOAT64 *twiddles = ptr_w;
    nodespacing <<= 1;

    for (j = del / 2; j != 0; j--) {
      FLOAT64 w_1 = *twiddles;
      FLOAT64 w_4 = *(twiddles + 257);
      twiddles += nodespacing;

      x0r = *ptr_y;
      x0i = *(ptr_y + 1);
      ptr_y += ((SIZE_T)del << 1);

      x1r = *ptr_y;
      x1i = *(ptr_y + 1);

      tmp = (FLOAT32)(ixheaace_dmult((FLOAT64)x1r, w_1) - ixheaace_dmult((FLOAT64)x1i, w_4));
      x1i = (FLOAT32)ixheaace_dmac(ixheaace_dmult((FLOAT64)x1r, w_4), (FLOAT64)x1i, w_1);
      x1r = tmp;

      *ptr_y = (x0r) - (x1r);
      *(ptr_y + 1) = (x0i) - (x1i);
      ptr_y -= ((SIZE_T)del << 1);

      *ptr_y = (x0r) + (x1r);
      *(ptr_y + 1) = (x0i) + (x1i);
      ptr_y += 2;
    }
    twiddles = ptr_w;
    for (j = del / 2; j != 0; j--) {
      FLOAT64 w_1 = *twiddles;
      FLOAT64 w_4 = *(twiddles + 257);
      twiddles += nodespacing;

      x0r = *ptr_y;
      x0i = *(ptr_y + 1);
      ptr_y += ((SIZE_T)del << 1);

      x1r = *ptr_y;
      x1i = *(ptr_y + 1);

      tmp = (FLOAT32)(ixheaace_dmult((FLOAT64)x1r, w_4) + ixheaace_dmult((FLOAT64)x1i, w_1));
      x1i = (FLOAT32)(-ixheaace_dmult((FLOAT64)x1r, w_1) + ixheaace_dmult((FLOAT64)x1i, w_4));
      x1r = tmp;

      *ptr_y = (x0r) - (x1r);
      *(ptr_y + 1) = (x0i) - (x1i);
      ptr_y -= ((SIZE_T)del << 1);

      *ptr_y = (x0r) + (x1r);
      *(ptr_y + 1) = (x0i) + (x1i);
      ptr_y += 2;
    }
  }

  for (i = 0; i < nlength; i++) {
    *(ptr_x + 2 * i) = y[2 * i];
    *(ptr_x + 2 * i + 1) = y[2 * i + 1];
  }
}

static VOID iusace_complex_fft_p3(FLOAT32 *data, WORD32 nlength,
                                  iusace_scratch_mem *pstr_scratch) {
  WORD32 i, j;
  FLOAT32 *data_3 = pstr_scratch->p_fft_p3_data_3;
  FLOAT32 *y = pstr_scratch->p_fft_p3_y;
  WORD32 cnfac;
  WORD32 mpass = nlength;
  FLOAT32 *ptr_x = data;
  FLOAT32 *ptr_y = y;

  cnfac = 0;
  while (mpass % 3 == 0) {
    mpass /= 3;
    cnfac++;
  }

  for (i = 0; i < 3 * cnfac; i++) {
    for (j = 0; j < mpass; j++) {
      data_3[2 * j] = data[3 * (2 * j) + (2 * i)];
      data_3[2 * j + 1] = data[3 * (2 * j) + 1 + (2 * i)];
    }
    iusace_complex_fft_p2(data_3, mpass, pstr_scratch->p_fft_p2_y);

    for (j = 0; j < mpass; j++) {
      data[3 * (2 * j) + (2 * i)] = data_3[2 * j];
      data[3 * (2 * j) + 1 + (2 * i)] = data_3[2 * j + 1];
    }
  }

  {
    const FLOAT64 *w1r, *w1i;
    FLOAT32 tmp;
    w1r = iusace_twiddle_table_3pr;
    w1i = iusace_twiddle_table_3pi;

    for (i = 0; i < nlength; i += 3) {
      tmp = (FLOAT32)((FLOAT64)data[2 * i] * (*w1r) - (FLOAT64)data[2 * i + 1] * (*w1i));
      data[2 * i + 1] =
          (FLOAT32)((FLOAT64)data[2 * i] * (*w1i) + (FLOAT64)data[2 * i + 1] * (*w1r));
      data[2 * i] = tmp;

      w1r++;
      w1i++;

      tmp = (FLOAT32)((FLOAT64)data[2 * (i + 1)] * (*w1r) -
                      (FLOAT64)data[2 * (i + 1) + 1] * (*w1i));
      data[2 * (i + 1) + 1] = (FLOAT32)((FLOAT64)data[2 * (i + 1)] * (*w1i) +
                                        (FLOAT64)data[2 * (i + 1) + 1] * (*w1r));
      data[2 * (i + 1)] = tmp;

      w1r++;
      w1i++;

      tmp = (FLOAT32)((FLOAT64)data[2 * (i + 2)] * (*w1r) -
                      (FLOAT64)data[2 * (i + 2) + 1] * (*w1i));
      data[2 * (i + 2) + 1] = (FLOAT32)((FLOAT64)data[2 * (i + 2)] * (*w1i) +
                                        (FLOAT64)data[2 * (i + 2) + 1] * (*w1r));
      data[2 * (i + 2)] = tmp;

      w1r += 3 * (128 / mpass - 1) + 1;
      w1i += 3 * (128 / mpass - 1) + 1;
    }
  }

  for (i = 0; i < mpass; i++) {
    iusace_complex_3point_fft(ptr_x, ptr_y);

    ptr_x = ptr_x + 6;
    ptr_y = ptr_y + 6;
  }

  for (i = 0; i < mpass; i++) {
    data[2 * i] = y[6 * i];
    data[2 * i + 1] = y[6 * i + 1];
  }

  for (i = 0; i < mpass; i++) {
    data[2 * (i + mpass)] = y[6 * i + 2];
    data[2 * (i + mpass) + 1] = y[6 * i + 3];
  }

  for (i = 0; i < mpass; i++) {
    data[2 * (i + 2 * mpass)] = y[6 * i + 4];
    data[2 * (i + 2 * mpass) + 1] = y[6 * i + 5];
  }
}

VOID iusace_complex_fft_p3_no_scratch(FLOAT32 *data, WORD32 nlength) {
  WORD32 i, j;

  FLOAT32 data_3[800];
  FLOAT32 y[1024];
  FLOAT32 p_fft_p2_y[2048];
  WORD32 cnfac;
  WORD32 mpass = nlength;
  FLOAT32 *ptr_x = data;
  FLOAT32 *ptr_y = y;

  cnfac = 0;
  while (mpass % 3 == 0) {
    mpass /= 3;
    cnfac++;
  }

  for (i = 0; i < 3 * cnfac; i++) {
    for (j = 0; j < mpass; j++) {
      data_3[2 * j] = data[3 * (2 * j) + (2 * i)];
      data_3[2 * j + 1] = data[3 * (2 * j) + 1 + (2 * i)];
    }
    iusace_complex_fft_p2(data_3, mpass, p_fft_p2_y);

    for (j = 0; j < mpass; j++) {
      data[3 * (2 * j) + (2 * i)] = data_3[2 * j];
      data[3 * (2 * j) + 1 + (2 * i)] = data_3[2 * j + 1];
    }
  }

  {
    const FLOAT64 *w1r, *w1i;
    FLOAT32 tmp;
    w1r = iusace_twiddle_table_3pr;
    w1i = iusace_twiddle_table_3pi;

    for (i = 0; i < nlength; i += 3) {
      tmp = (FLOAT32)((FLOAT64)data[2 * i] * (*w1r) - (FLOAT64)data[2 * i + 1] * (*w1i));
      data[2 * i + 1] =
          (FLOAT32)((FLOAT64)data[2 * i] * (*w1i) + (FLOAT64)data[2 * i + 1] * (*w1r));
      data[2 * i] = tmp;

      w1r++;
      w1i++;

      tmp = (FLOAT32)((FLOAT64)data[2 * (i + 1)] * (*w1r) -
                      (FLOAT64)data[2 * (i + 1) + 1] * (*w1i));
      data[2 * (i + 1) + 1] = (FLOAT32)((FLOAT64)data[2 * (i + 1)] * (*w1i) +
                                        (FLOAT64)data[2 * (i + 1) + 1] * (*w1r));
      data[2 * (i + 1)] = tmp;

      w1r++;
      w1i++;

      tmp = (FLOAT32)((FLOAT64)data[2 * (i + 2)] * (*w1r) -
                      (FLOAT64)data[2 * (i + 2) + 1] * (*w1i));
      data[2 * (i + 2) + 1] = (FLOAT32)((FLOAT64)data[2 * (i + 2)] * (*w1i) +
                                        (FLOAT64)data[2 * (i + 2) + 1] * (*w1r));
      data[2 * (i + 2)] = tmp;

      w1r += 3 * (128 / mpass - 1) + 1;
      w1i += 3 * (128 / mpass - 1) + 1;
    }
  }

  for (i = 0; i < mpass; i++) {
    iusace_complex_3point_fft(ptr_x, ptr_y);

    ptr_x = ptr_x + 6;
    ptr_y = ptr_y + 6;
  }

  for (i = 0; i < mpass; i++) {
    data[2 * i] = y[6 * i];
    data[2 * i + 1] = y[6 * i + 1];
  }

  for (i = 0; i < mpass; i++) {
    data[2 * (i + mpass)] = y[6 * i + 2];
    data[2 * (i + mpass) + 1] = y[6 * i + 3];
  }

  for (i = 0; i < mpass; i++) {
    data[2 * (i + 2 * mpass)] = y[6 * i + 4];
    data[2 * (i + 2 * mpass) + 1] = y[6 * i + 5];
  }
}

VOID iusace_complex_fft(FLOAT32 *data, WORD32 nlength, iusace_scratch_mem *pstr_scratch) {
  if (nlength & (nlength - 1)) {
    iusace_complex_fft_p3(data, nlength, pstr_scratch);
  } else {
    iusace_complex_fft_p2(data, nlength, pstr_scratch->p_fft_p2_y);
  }
}

VOID iusace_complex_fft_2048(FLOAT32 *ptr_x, FLOAT32 *scratch_fft) {
  WORD32 i;
  FLOAT32 re, im, c_v, s_v, tmp_re, tmp_im;
  FLOAT32 *ptr_re, *ptr_im, *ptr_re_h, *ptr_im_h;
  FLOAT32 *ptr_cos_val, *ptr_sin_val;
  iusace_complex_fft_p2(ptr_x, 1024, scratch_fft);
  iusace_complex_fft_p2(ptr_x + 2048, 1024, scratch_fft);

  ptr_re = ptr_x;
  ptr_im = ptr_x + 1;
  ptr_re_h = ptr_x + 2048;
  ptr_im_h = ptr_x + 2048 + 1;
  ptr_cos_val = (FLOAT32 *)&iusace_twiddle_cos_2048[0];
  ptr_sin_val = (FLOAT32 *)&iusace_twiddle_sin_2048[0];
  for (i = 0; i < 1024; i++) {
    re = *ptr_re_h;
    im = *ptr_im_h;
    c_v = ptr_cos_val[i];
    s_v = ptr_sin_val[i];
    tmp_re = (re * c_v) + (im * s_v);
    tmp_im = -(re * s_v) + (im * c_v);
    re = *ptr_re;
    im = *ptr_im;

    *ptr_re = re + tmp_re;
    *ptr_im = im + tmp_im;
    *ptr_re_h = re - tmp_re;
    *ptr_im_h = im - tmp_im;

    ptr_re += 2;
    ptr_im += 2;
    ptr_re_h += 2;
    ptr_im_h += 2;
  }
}
