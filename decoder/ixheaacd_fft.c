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
#include <stdlib.h>
#include <stdio.h>

#include <ixheaacd_type_def.h>
#include "ixheaacd_interface.h"
#include "ixheaacd_constants.h"
#include <ixheaacd_basic_ops32.h>
#include "ixheaacd_function_selector.h"

extern const WORD32 ixheaacd_twiddle_table_fft_32x32[514];
extern const WORD32 ixheaacd_twiddle_table_3pr[1155];
extern const WORD32 ixheaacd_twiddle_table_3pi[1155];
extern const WORD8 ixheaacd_mps_dig_rev[16];

#define PLATFORM_INLINE __inline

#define DIG_REV(i, m, j)                                    \
  do {                                                      \
    unsigned _ = (i);                                       \
    _ = ((_ & 0x33333333) << 2) | ((_ & ~0x33333333) >> 2); \
    _ = ((_ & 0x0F0F0F0F) << 4) | ((_ & ~0x0F0F0F0F) >> 4); \
    _ = ((_ & 0x00FF00FF) << 8) | ((_ & ~0x00FF00FF) >> 8); \
    (j) = _ >> (m);                                         \
  } while (0)

static PLATFORM_INLINE WORD32 ixheaacd_mult32(WORD32 a, WORD32 b) {
  WORD32 result;
  WORD64 temp_result;

  temp_result = (WORD64)a * (WORD64)b;
  result = (WORD32)(temp_result >> 31);

  return (result);
}

static PLATFORM_INLINE WORD32 ixheaacd_mac32(WORD32 a, WORD32 b, WORD32 c) {
  WORD32 result;

  result = a + ixheaacd_mult32(b, c);

  return (result);
}

static PLATFORM_INLINE WORD32 ixheaacd_mult32_shl(WORD32 a, WORD32 b) {
  WORD32 result;
  WORD64 temp_result;

  temp_result = (WORD64)a * (WORD64)b;
  result = (WORD32)(temp_result >> 32);

  return (result << 1);
}

VOID ixheaacd_mps_complex_fft_64_dec(WORD32 *ptr_x, WORD32 *fin_re,
                                     WORD32 *fin_im, WORD32 nlength) {
  WORD32 i, j, k, n_stages;
  WORD32 h2, x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
  WORD32 del, nodespacing, in_loop_cnt;
  WORD32 y[128];
  WORD32 npoints = nlength;
  WORD32 *ptr_y = y;
  const WORD32 *ptr_w;
  n_stages = 30 - ixheaacd_norm32(npoints);

  n_stages = n_stages >> 1;

  ptr_w = ixheaacd_twiddle_table_fft_32x32;

  for (i = 0; i < npoints; i += 4) {
    WORD32 *inp = ptr_x;
    h2 = ixheaacd_mps_dig_rev[i >> 2];
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
    x2r = x0r - (x2r << 1);
    x2i = x0i - (x2i << 1);
    x1r = x1r + x3r;
    x1i = x1i + x3i;
    x3r = x1r - (x3r << 1);
    x3i = x1i - (x3i << 1);

    x0r = x0r + x1r;
    x0i = x0i + x1i;
    x1r = x0r - (x1r << 1);
    x1i = x0i - (x1i << 1);
    x2r = x2r + x3i;
    x2i = x2i - x3r;
    x3i = x2r - (x3i << 1);
    x3r = x2i + (x3r << 1);

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
    const WORD32 *twiddles = ptr_w;
    WORD32 *data = ptr_y;
    WORD32 w1h, w2h, w3h, w1l, w2l, w3l;
    WORD32 sec_loop_cnt;

    for (k = in_loop_cnt; k != 0; k--) {
      x0r = (*data);
      x0i = (*(data + 1));
      data += (del << 1);

      x1r = (*data);
      x1i = (*(data + 1));
      data += (del << 1);

      x2r = (*data);
      x2i = (*(data + 1));
      data += (del << 1);

      x3r = (*data);
      x3i = (*(data + 1));
      data -= 3 * (del << 1);

      x0r = x0r + x2r;
      x0i = x0i + x2i;
      x2r = x0r - (x2r << 1);
      x2i = x0i - (x2i << 1);
      x1r = x1r + x3r;
      x1i = x1i + x3i;
      x3r = x1r - (x3r << 1);
      x3i = x1i - (x3i << 1);

      x0r = x0r + x1r;
      x0i = x0i + x1i;
      x1r = x0r - (x1r << 1);
      x1i = x0i - (x1i << 1);
      x2r = x2r + x3i;
      x2i = x2i - x3r;
      x3i = x2r - (x3i << 1);
      x3r = x2i + (x3r << 1);

      *data = x0r;
      *(data + 1) = x0i;
      data += (del << 1);

      *data = x2r;
      *(data + 1) = x2i;
      data += (del << 1);

      *data = x1r;
      *(data + 1) = x1i;
      data += (del << 1);

      *data = x3i;
      *(data + 1) = x3r;
      data += (del << 1);
    }
    data = ptr_y + 2;

    sec_loop_cnt = (nodespacing * del);
    sec_loop_cnt = (sec_loop_cnt / 4) + (sec_loop_cnt / 8) -
                   (sec_loop_cnt / 16) + (sec_loop_cnt / 32) -
                   (sec_loop_cnt / 64) + (sec_loop_cnt / 128) -
                   (sec_loop_cnt / 256);
    j = nodespacing;

    for (j = nodespacing; j <= sec_loop_cnt; j += nodespacing) {
      w1h = *(twiddles + 2 * j);
      w1l = *(twiddles + 2 * j + 1);
      w2h = *(twiddles + 2 * (j << 1));
      w2l = *(twiddles + 2 * (j << 1) + 1);
      w3h = *(twiddles + 2 * j + 2 * (j << 1));
      w3l = *(twiddles + 2 * j + 2 * (j << 1) + 1);

      for (k = in_loop_cnt; k != 0; k--) {
        WORD32 tmp;
        WORD32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

        data += (del << 1);

        x1r = *data;
        x1i = *(data + 1);
        data += (del << 1);

        x2r = *data;
        x2i = *(data + 1);
        data += (del << 1);

        x3r = *data;
        x3i = *(data + 1);
        data -= 3 * (del << 1);

        tmp = (ixheaacd_mult32(x1r, w1l) - ixheaacd_mult32(x1i, w1h));
        x1i = ixheaacd_mac32(ixheaacd_mult32(x1r, w1h), x1i, w1l);
        x1r = tmp;

        tmp = (ixheaacd_mult32(x2r, w2l) - ixheaacd_mult32(x2i, w2h));
        x2i = ixheaacd_mac32(ixheaacd_mult32(x2r, w2h), x2i, w2l);
        x2r = tmp;

        tmp = (ixheaacd_mult32(x3r, w3l) - ixheaacd_mult32(x3i, w3h));
        x3i = ixheaacd_mac32(ixheaacd_mult32(x3r, w3h), x3i, w3l);
        x3r = tmp;

        x0r = (*data);
        x0i = (*(data + 1));

        x0r = x0r + (x2r);
        x0i = x0i + (x2i);
        x2r = x0r - (x2r << 1);
        x2i = x0i - (x2i << 1);
        x1r = x1r + x3r;
        x1i = x1i + x3i;
        x3r = x1r - (x3r << 1);
        x3i = x1i - (x3i << 1);

        x0r = x0r + (x1r);
        x0i = x0i + (x1i);
        x1r = x0r - (x1r << 1);
        x1i = x0i - (x1i << 1);
        x2r = x2r + (x3i);
        x2i = x2i - (x3r);
        x3i = x2r - (x3i << 1);
        x3r = x2i + (x3r << 1);

        *data = x0r;
        *(data + 1) = x0i;
        data += (del << 1);

        *data = x2r;
        *(data + 1) = x2i;
        data += (del << 1);

        *data = x1r;
        *(data + 1) = x1i;
        data += (del << 1);

        *data = x3i;
        *(data + 1) = x3r;
        data += (del << 1);
      }
      data -= 2 * npoints;
      data += 2;
    }
    for (; j <= (nodespacing * del) >> 1; j += nodespacing) {
      w1h = *(twiddles + 2 * j);
      w2h = *(twiddles + 2 * (j << 1));
      w3h = *(twiddles + 2 * j + 2 * (j << 1) - 512);
      w1l = *(twiddles + 2 * j + 1);
      w2l = *(twiddles + 2 * (j << 1) + 1);
      w3l = *(twiddles + 2 * j + 2 * (j << 1) - 511);

      for (k = in_loop_cnt; k != 0; k--) {
        WORD32 tmp;
        WORD32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

        data += (del << 1);

        x1r = *data;
        x1i = *(data + 1);
        data += (del << 1);

        x2r = *data;
        x2i = *(data + 1);
        data += (del << 1);

        x3r = *data;
        x3i = *(data + 1);
        data -= 3 * (del << 1);

        tmp = (ixheaacd_mult32(x1r, w1l) - ixheaacd_mult32(x1i, w1h));
        x1i = ixheaacd_mac32(ixheaacd_mult32(x1r, w1h), x1i, w1l);
        x1r = tmp;

        tmp = (ixheaacd_mult32(x2r, w2l) - ixheaacd_mult32(x2i, w2h));
        x2i = ixheaacd_mac32(ixheaacd_mult32(x2r, w2h), x2i, w2l);
        x2r = tmp;

        tmp = (ixheaacd_mult32(x3r, w3h) + ixheaacd_mult32(x3i, w3l));
        x3i = -ixheaacd_mult32(x3r, w3l) + ixheaacd_mult32(x3i, w3h);
        x3r = tmp;

        x0r = (*data);
        x0i = (*(data + 1));

        x0r = x0r + (x2r);
        x0i = x0i + (x2i);
        x2r = x0r - (x2r << 1);
        x2i = x0i - (x2i << 1);
        x1r = x1r + x3r;
        x1i = x1i + x3i;
        x3r = x1r - (x3r << 1);
        x3i = x1i - (x3i << 1);

        x0r = x0r + (x1r);
        x0i = x0i + (x1i);
        x1r = x0r - (x1r << 1);
        x1i = x0i - (x1i << 1);
        x2r = x2r + (x3i);
        x2i = x2i - (x3r);
        x3i = x2r - (x3i << 1);
        x3r = x2i + (x3r << 1);

        *data = x0r;
        *(data + 1) = x0i;
        data += (del << 1);

        *data = x2r;
        *(data + 1) = x2i;
        data += (del << 1);

        *data = x1r;
        *(data + 1) = x1i;
        data += (del << 1);

        *data = x3i;
        *(data + 1) = x3r;
        data += (del << 1);
      }
      data -= 2 * npoints;
      data += 2;
    }
    for (; j <= sec_loop_cnt * 2; j += nodespacing) {
      w1h = *(twiddles + 2 * j);
      w2h = *(twiddles + 2 * (j << 1) - 512);
      w3h = *(twiddles + 2 * j + 2 * (j << 1) - 512);
      w1l = *(twiddles + 2 * j + 1);
      w2l = *(twiddles + 2 * (j << 1) - 511);
      w3l = *(twiddles + 2 * j + 2 * (j << 1) - 511);

      for (k = in_loop_cnt; k != 0; k--) {
        WORD32 tmp;
        WORD32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

        data += (del << 1);

        x1r = *data;
        x1i = *(data + 1);
        data += (del << 1);

        x2r = *data;
        x2i = *(data + 1);
        data += (del << 1);

        x3r = *data;
        x3i = *(data + 1);
        data -= 3 * (del << 1);

        tmp = (ixheaacd_mult32(x1r, w1l) - ixheaacd_mult32(x1i, w1h));
        x1i = ixheaacd_mac32(ixheaacd_mult32(x1r, w1h), x1i, w1l);
        x1r = tmp;

        tmp = (ixheaacd_mult32(x2r, w2h) + ixheaacd_mult32(x2i, w2l));
        x2i = -ixheaacd_mult32(x2r, w2l) + ixheaacd_mult32(x2i, w2h);
        x2r = tmp;

        tmp = (ixheaacd_mult32(x3r, w3h) + ixheaacd_mult32(x3i, w3l));
        x3i = -ixheaacd_mult32(x3r, w3l) + ixheaacd_mult32(x3i, w3h);
        x3r = tmp;

        x0r = (*data);
        x0i = (*(data + 1));

        x0r = x0r + (x2r);
        x0i = x0i + (x2i);
        x2r = x0r - (x2r << 1);
        x2i = x0i - (x2i << 1);
        x1r = x1r + x3r;
        x1i = x1i + x3i;
        x3r = x1r - (x3r << 1);
        x3i = x1i - (x3i << 1);

        x0r = x0r + (x1r);
        x0i = x0i + (x1i);
        x1r = x0r - (x1r << 1);
        x1i = x0i - (x1i << 1);
        x2r = x2r + (x3i);
        x2i = x2i - (x3r);
        x3i = x2r - (x3i << 1);
        x3r = x2i + (x3r << 1);

        *data = x0r;
        *(data + 1) = x0i;
        data += (del << 1);

        *data = x2r;
        *(data + 1) = x2i;
        data += (del << 1);

        *data = x1r;
        *(data + 1) = x1i;
        data += (del << 1);

        *data = x3i;
        *(data + 1) = x3r;
        data += (del << 1);
      }
      data -= 2 * npoints;
      data += 2;
    }
    for (; j < nodespacing * del; j += nodespacing) {
      w1h = *(twiddles + 2 * j);
      w2h = *(twiddles + 2 * (j << 1) - 512);
      w3h = *(twiddles + 2 * j + 2 * (j << 1) - 1024);
      w1l = *(twiddles + 2 * j + 1);
      w2l = *(twiddles + 2 * (j << 1) - 511);
      w3l = *(twiddles + 2 * j + 2 * (j << 1) - 1023);

      for (k = in_loop_cnt; k != 0; k--) {
        WORD32 tmp;
        WORD32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

        data += (del << 1);

        x1r = *data;
        x1i = *(data + 1);
        data += (del << 1);

        x2r = *data;
        x2i = *(data + 1);
        data += (del << 1);

        x3r = *data;
        x3i = *(data + 1);
        data -= 3 * (del << 1);

        tmp = (ixheaacd_mult32(x1r, w1l) - ixheaacd_mult32(x1i, w1h));
        x1i = ixheaacd_mac32(ixheaacd_mult32(x1r, w1h), x1i, w1l);
        x1r = tmp;

        tmp = (ixheaacd_mult32(x2r, w2h) + ixheaacd_mult32(x2i, w2l));
        x2i = -ixheaacd_mult32(x2r, w2l) + ixheaacd_mult32(x2i, w2h);
        x2r = tmp;

        tmp = (-ixheaacd_mult32(x3r, w3l) + ixheaacd_mult32(x3i, w3h));
        x3i = ixheaacd_mac32(ixheaacd_mult32(x3r, w3h), x3i, w3l);
        x3r = tmp;

        x0r = (*data);
        x0i = (*(data + 1));

        x0r = x0r + (x2r);
        x0i = x0i + (x2i);
        x2r = x0r - (x2r << 1);
        x2i = x0i - (x2i << 1);
        x1r = x1r + x3r;
        x1i = x1i - x3i;
        x3r = x1r - (x3r << 1);
        x3i = x1i + (x3i << 1);

        x0r = x0r + (x1r);
        x0i = x0i + (x1i);
        x1r = x0r - (x1r << 1);
        x1i = x0i - (x1i << 1);
        x2r = x2r + (x3i);
        x2i = x2i - (x3r);
        x3i = x2r - (x3i << 1);
        x3r = x2i + (x3r << 1);

        *data = x0r;
        *(data + 1) = x0i;
        data += (del << 1);

        *data = x2r;
        *(data + 1) = x2i;
        data += (del << 1);

        *data = x1r;
        *(data + 1) = x1i;
        data += (del << 1);

        *data = x3i;
        *(data + 1) = x3r;
        data += (del << 1);
      }
      data -= 2 * npoints;
      data += 2;
    }
    nodespacing >>= 2;
    del <<= 2;
    in_loop_cnt >>= 2;
  }

  for (i = 0; i < 2 * nlength; i += 2) {
    fin_re[i] = y[i];
    fin_im[i] = y[i + 1];
  }

  return;
}

VOID ixheaacd_complex_fft_p2_dec(WORD32 *xr, WORD32 *xi, WORD32 nlength,
                                 WORD32 fft_mode, WORD32 *preshift) {
  WORD32 i, j, k, n_stages;
  WORD32 h2, x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
  WORD32 del, nodespacing, in_loop_cnt;
  WORD32 not_power_4;
  WORD32 npts, shift;
  WORD32 dig_rev_shift;
  WORD32 ptr_x[1024];
  WORD32 y[1024];
  WORD32 npoints = nlength;
  WORD32 n = 0;
  WORD32 *ptr_y = y;
  const WORD32 *ptr_w;
  dig_rev_shift = ixheaacd_norm32(npoints) + 1 - 16;
  n_stages = 30 - ixheaacd_norm32(npoints);
  not_power_4 = n_stages & 1;

  n_stages = n_stages >> 1;

  npts = npoints;
  while (npts >> 1) {
    n++;
    npts = npts >> 1;
  }

  if (n % 2 == 0)
    shift = ((n + 4)) / 2;
  else
    shift = ((n + 3) / 2);

  for (i = 0; i < nlength; i++) {
    ptr_x[2 * i] = (xr[i] / (1 << (shift)));
    ptr_x[2 * i + 1] = (xi[i] / (1 << (shift)));
  }

  if (fft_mode == -1) {
    ptr_w = ixheaacd_twiddle_table_fft_32x32;

    for (i = 0; i < npoints; i += 4) {
      WORD32 *inp = ptr_x;

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
      x2r = x0r - (x2r << 1);
      x2i = x0i - (x2i << 1);
      x1r = x1r + x3r;
      x1i = x1i + x3i;
      x3r = x1r - (x3r << 1);
      x3i = x1i - (x3i << 1);

      x0r = x0r + x1r;
      x0i = x0i + x1i;
      x1r = x0r - (x1r << 1);
      x1i = x0i - (x1i << 1);
      x2r = x2r + x3i;
      x2i = x2i - x3r;
      x3i = x2r - (x3i << 1);
      x3r = x2i + (x3r << 1);

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
      const WORD32 *twiddles = ptr_w;
      WORD32 *data = ptr_y;
      WORD32 w1h, w2h, w3h, w1l, w2l, w3l;
      WORD32 sec_loop_cnt;

      for (k = in_loop_cnt; k != 0; k--) {
        x0r = (*data);
        x0i = (*(data + 1));
        data += (del << 1);

        x1r = (*data);
        x1i = (*(data + 1));
        data += (del << 1);

        x2r = (*data);
        x2i = (*(data + 1));
        data += (del << 1);

        x3r = (*data);
        x3i = (*(data + 1));
        data -= 3 * (del << 1);

        x0r = x0r + x2r;
        x0i = x0i + x2i;
        x2r = x0r - (x2r << 1);
        x2i = x0i - (x2i << 1);
        x1r = x1r + x3r;
        x1i = x1i + x3i;
        x3r = x1r - (x3r << 1);
        x3i = x1i - (x3i << 1);

        x0r = x0r + x1r;
        x0i = x0i + x1i;
        x1r = x0r - (x1r << 1);
        x1i = x0i - (x1i << 1);
        x2r = x2r + x3i;
        x2i = x2i - x3r;
        x3i = x2r - (x3i << 1);
        x3r = x2i + (x3r << 1);

        *data = x0r;
        *(data + 1) = x0i;
        data += (del << 1);

        *data = x2r;
        *(data + 1) = x2i;
        data += (del << 1);

        *data = x1r;
        *(data + 1) = x1i;
        data += (del << 1);

        *data = x3i;
        *(data + 1) = x3r;
        data += (del << 1);
      }
      data = ptr_y + 2;

      sec_loop_cnt = (nodespacing * del);
      sec_loop_cnt = (sec_loop_cnt / 4) + (sec_loop_cnt / 8) -
                     (sec_loop_cnt / 16) + (sec_loop_cnt / 32) -
                     (sec_loop_cnt / 64) + (sec_loop_cnt / 128) -
                     (sec_loop_cnt / 256);
      j = nodespacing;

      for (j = nodespacing; j <= sec_loop_cnt; j += nodespacing) {
        w1h = *(twiddles + 2 * j);
        w1l = *(twiddles + 2 * j + 1);
        w2h = *(twiddles + 2 * (j << 1));
        w2l = *(twiddles + 2 * (j << 1) + 1);
        w3h = *(twiddles + 2 * j + 2 * (j << 1));
        w3l = *(twiddles + 2 * j + 2 * (j << 1) + 1);

        for (k = in_loop_cnt; k != 0; k--) {
          WORD32 tmp;
          WORD32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

          data += (del << 1);

          x1r = *data;
          x1i = *(data + 1);
          data += (del << 1);

          x2r = *data;
          x2i = *(data + 1);
          data += (del << 1);

          x3r = *data;
          x3i = *(data + 1);
          data -= 3 * (del << 1);

          tmp = (ixheaacd_mult32(x1r, w1l) - ixheaacd_mult32(x1i, w1h));
          x1i = ixheaacd_mac32(ixheaacd_mult32(x1r, w1h), x1i, w1l);
          x1r = tmp;

          tmp = (ixheaacd_mult32(x2r, w2l) - ixheaacd_mult32(x2i, w2h));
          x2i = ixheaacd_mac32(ixheaacd_mult32(x2r, w2h), x2i, w2l);
          x2r = tmp;

          tmp = (ixheaacd_mult32(x3r, w3l) - ixheaacd_mult32(x3i, w3h));
          x3i = ixheaacd_mac32(ixheaacd_mult32(x3r, w3h), x3i, w3l);
          x3r = tmp;

          x0r = (*data);
          x0i = (*(data + 1));

          x0r = x0r + (x2r);
          x0i = x0i + (x2i);
          x2r = x0r - (x2r << 1);
          x2i = x0i - (x2i << 1);
          x1r = x1r + x3r;
          x1i = x1i + x3i;
          x3r = x1r - (x3r << 1);
          x3i = x1i - (x3i << 1);

          x0r = x0r + (x1r);
          x0i = x0i + (x1i);
          x1r = x0r - (x1r << 1);
          x1i = x0i - (x1i << 1);
          x2r = x2r + (x3i);
          x2i = x2i - (x3r);
          x3i = x2r - (x3i << 1);
          x3r = x2i + (x3r << 1);

          *data = x0r;
          *(data + 1) = x0i;
          data += (del << 1);

          *data = x2r;
          *(data + 1) = x2i;
          data += (del << 1);

          *data = x1r;
          *(data + 1) = x1i;
          data += (del << 1);

          *data = x3i;
          *(data + 1) = x3r;
          data += (del << 1);
        }
        data -= 2 * npoints;
        data += 2;
      }
      for (; j <= (nodespacing * del) >> 1; j += nodespacing) {
        w1h = *(twiddles + 2 * j);
        w2h = *(twiddles + 2 * (j << 1));
        w3h = *(twiddles + 2 * j + 2 * (j << 1) - 512);
        w1l = *(twiddles + 2 * j + 1);
        w2l = *(twiddles + 2 * (j << 1) + 1);
        w3l = *(twiddles + 2 * j + 2 * (j << 1) - 511);

        for (k = in_loop_cnt; k != 0; k--) {
          WORD32 tmp;
          WORD32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;
          data += (del << 1);

          x1r = *data;
          x1i = *(data + 1);
          data += (del << 1);

          x2r = *data;
          x2i = *(data + 1);
          data += (del << 1);

          x3r = *data;
          x3i = *(data + 1);
          data -= 3 * (del << 1);

          tmp = (ixheaacd_mult32(x1r, w1l) - ixheaacd_mult32(x1i, w1h));
          x1i = ixheaacd_mac32(ixheaacd_mult32(x1r, w1h), x1i, w1l);
          x1r = tmp;

          tmp = (ixheaacd_mult32(x2r, w2l) - ixheaacd_mult32(x2i, w2h));
          x2i = ixheaacd_mac32(ixheaacd_mult32(x2r, w2h), x2i, w2l);
          x2r = tmp;

          tmp = (ixheaacd_mult32(x3r, w3h) + ixheaacd_mult32(x3i, w3l));
          x3i = -ixheaacd_mult32(x3r, w3l) + ixheaacd_mult32(x3i, w3h);
          x3r = tmp;

          x0r = (*data);
          x0i = (*(data + 1));

          x0r = x0r + (x2r);
          x0i = x0i + (x2i);
          x2r = x0r - (x2r << 1);
          x2i = x0i - (x2i << 1);
          x1r = x1r + x3r;
          x1i = x1i + x3i;
          x3r = x1r - (x3r << 1);
          x3i = x1i - (x3i << 1);

          x0r = x0r + (x1r);
          x0i = x0i + (x1i);
          x1r = x0r - (x1r << 1);
          x1i = x0i - (x1i << 1);
          x2r = x2r + (x3i);
          x2i = x2i - (x3r);
          x3i = x2r - (x3i << 1);
          x3r = x2i + (x3r << 1);

          *data = x0r;
          *(data + 1) = x0i;
          data += (del << 1);

          *data = x2r;
          *(data + 1) = x2i;
          data += (del << 1);

          *data = x1r;
          *(data + 1) = x1i;
          data += (del << 1);

          *data = x3i;
          *(data + 1) = x3r;
          data += (del << 1);
        }
        data -= 2 * npoints;
        data += 2;
      }
      for (; j <= sec_loop_cnt * 2; j += nodespacing) {
        w1h = *(twiddles + 2 * j);
        w2h = *(twiddles + 2 * (j << 1) - 512);
        w3h = *(twiddles + 2 * j + 2 * (j << 1) - 512);
        w1l = *(twiddles + 2 * j + 1);
        w2l = *(twiddles + 2 * (j << 1) - 511);
        w3l = *(twiddles + 2 * j + 2 * (j << 1) - 511);

        for (k = in_loop_cnt; k != 0; k--) {
          WORD32 tmp;
          WORD32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

          data += (del << 1);

          x1r = *data;
          x1i = *(data + 1);
          data += (del << 1);

          x2r = *data;
          x2i = *(data + 1);
          data += (del << 1);

          x3r = *data;
          x3i = *(data + 1);
          data -= 3 * (del << 1);

          tmp = (ixheaacd_mult32(x1r, w1l) - ixheaacd_mult32(x1i, w1h));
          x1i = ixheaacd_mac32(ixheaacd_mult32(x1r, w1h), x1i, w1l);
          x1r = tmp;

          tmp = (ixheaacd_mult32(x2r, w2h) + ixheaacd_mult32(x2i, w2l));
          x2i = -ixheaacd_mult32(x2r, w2l) + ixheaacd_mult32(x2i, w2h);
          x2r = tmp;

          tmp = (ixheaacd_mult32(x3r, w3h) + ixheaacd_mult32(x3i, w3l));
          x3i = -ixheaacd_mult32(x3r, w3l) + ixheaacd_mult32(x3i, w3h);
          x3r = tmp;

          x0r = (*data);
          x0i = (*(data + 1));

          x0r = x0r + (x2r);
          x0i = x0i + (x2i);
          x2r = x0r - (x2r << 1);
          x2i = x0i - (x2i << 1);
          x1r = x1r + x3r;
          x1i = x1i + x3i;
          x3r = x1r - (x3r << 1);
          x3i = x1i - (x3i << 1);

          x0r = x0r + (x1r);
          x0i = x0i + (x1i);
          x1r = x0r - (x1r << 1);
          x1i = x0i - (x1i << 1);
          x2r = x2r + (x3i);
          x2i = x2i - (x3r);
          x3i = x2r - (x3i << 1);
          x3r = x2i + (x3r << 1);

          *data = x0r;
          *(data + 1) = x0i;
          data += (del << 1);

          *data = x2r;
          *(data + 1) = x2i;
          data += (del << 1);

          *data = x1r;
          *(data + 1) = x1i;
          data += (del << 1);

          *data = x3i;
          *(data + 1) = x3r;
          data += (del << 1);
        }
        data -= 2 * npoints;
        data += 2;
      }
      for (; j < nodespacing * del; j += nodespacing) {
        w1h = *(twiddles + 2 * j);
        w2h = *(twiddles + 2 * (j << 1) - 512);
        w3h = *(twiddles + 2 * j + 2 * (j << 1) - 1024);
        w1l = *(twiddles + 2 * j + 1);
        w2l = *(twiddles + 2 * (j << 1) - 511);
        w3l = *(twiddles + 2 * j + 2 * (j << 1) - 1023);

        for (k = in_loop_cnt; k != 0; k--) {
          WORD32 tmp;
          WORD32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

          data += (del << 1);

          x1r = *data;
          x1i = *(data + 1);
          data += (del << 1);

          x2r = *data;
          x2i = *(data + 1);
          data += (del << 1);

          x3r = *data;
          x3i = *(data + 1);
          data -= 3 * (del << 1);

          tmp = (ixheaacd_mult32(x1r, w1l) - ixheaacd_mult32(x1i, w1h));
          x1i = ixheaacd_mac32(ixheaacd_mult32(x1r, w1h), x1i, w1l);
          x1r = tmp;

          tmp = (ixheaacd_mult32(x2r, w2h) + ixheaacd_mult32(x2i, w2l));
          x2i = -ixheaacd_mult32(x2r, w2l) + ixheaacd_mult32(x2i, w2h);
          x2r = tmp;

          tmp = (-ixheaacd_mult32(x3r, w3l) + ixheaacd_mult32(x3i, w3h));
          x3i = ixheaacd_mac32(ixheaacd_mult32(x3r, w3h), x3i, w3l);
          x3r = tmp;

          x0r = (*data);
          x0i = (*(data + 1));

          x0r = x0r + (x2r);
          x0i = x0i + (x2i);
          x2r = x0r - (x2r << 1);
          x2i = x0i - (x2i << 1);
          x1r = x1r + x3r;
          x1i = x1i - x3i;
          x3r = x1r - (x3r << 1);
          x3i = x1i + (x3i << 1);

          x0r = x0r + (x1r);
          x0i = x0i + (x1i);
          x1r = x0r - (x1r << 1);
          x1i = x0i - (x1i << 1);
          x2r = x2r + (x3i);
          x2i = x2i - (x3r);
          x3i = x2r - (x3i << 1);
          x3r = x2i + (x3r << 1);

          *data = x0r;
          *(data + 1) = x0i;
          data += (del << 1);

          *data = x2r;
          *(data + 1) = x2i;
          data += (del << 1);

          *data = x1r;
          *(data + 1) = x1i;
          data += (del << 1);

          *data = x3i;
          *(data + 1) = x3r;
          data += (del << 1);
        }
        data -= 2 * npoints;
        data += 2;
      }
      nodespacing >>= 2;
      del <<= 2;
      in_loop_cnt >>= 2;
    }
    if (not_power_4) {
      const WORD32 *twiddles = ptr_w;
      nodespacing <<= 1;
      shift += 1;

      for (j = del / 2; j != 0; j--) {
        WORD32 w1h = *twiddles;
        WORD32 w1l = *(twiddles + 1);
        WORD32 tmp;
        twiddles += nodespacing * 2;

        x0r = *ptr_y;
        x0i = *(ptr_y + 1);
        ptr_y += (del << 1);

        x1r = *ptr_y;
        x1i = *(ptr_y + 1);

        tmp = (ixheaacd_mult32(x1r, w1l) - ixheaacd_mult32(x1i, w1h));
        x1i = ixheaacd_mac32(ixheaacd_mult32(x1r, w1h), x1i, w1l);
        x1r = tmp;

        *ptr_y = (x0r) / 2 - (x1r) / 2;
        *(ptr_y + 1) = (x0i) / 2 - (x1i) / 2;
        ptr_y -= (del << 1);

        *ptr_y = (x0r) / 2 + (x1r) / 2;
        *(ptr_y + 1) = (x0i) / 2 + (x1i) / 2;
        ptr_y += 2;
      }
      twiddles = ptr_w;
      for (j = del / 2; j != 0; j--) {
        WORD32 w1h = *twiddles;
        WORD32 w1l = *(twiddles + 1);
        WORD32 tmp;
        twiddles += nodespacing * 2;

        x0r = *ptr_y;
        x0i = *(ptr_y + 1);
        ptr_y += (del << 1);

        x1r = *ptr_y;
        x1i = *(ptr_y + 1);

        tmp = (ixheaacd_mult32(x1r, w1h) + ixheaacd_mult32(x1i, w1l));
        x1i = -ixheaacd_mult32(x1r, w1l) + ixheaacd_mult32(x1i, w1h);
        x1r = tmp;

        *ptr_y = (x0r) / 2 - (x1r) / 2;
        *(ptr_y + 1) = (x0i) / 2 - (x1i) / 2;
        ptr_y -= (del << 1);

        *ptr_y = (x0r) / 2 + (x1r) / 2;
        *(ptr_y + 1) = (x0i) / 2 + (x1i) / 2;
        ptr_y += 2;
      }
    }

  }

  else {
    ptr_w = ixheaacd_twiddle_table_fft_32x32;

    for (i = 0; i < npoints; i += 4) {
      WORD32 *inp = ptr_x;

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
      x2r = x0r - (x2r << 1);
      x2i = x0i - (x2i << 1);
      x1r = x1r + x3r;
      x1i = x1i + x3i;
      x3r = x1r - (x3r << 1);
      x3i = x1i - (x3i << 1);

      x0r = x0r + x1r;
      x0i = x0i + x1i;
      x1r = x0r - (x1r << 1);
      x1i = x0i - (x1i << 1);
      x2r = x2r - x3i;
      x2i = x2i + x3r;
      x3i = x2r + (x3i << 1);
      x3r = x2i - (x3r << 1);

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
      const WORD32 *twiddles = ptr_w;
      WORD32 *data = ptr_y;
      WORD32 w1h, w2h, w3h, w1l, w2l, w3l;
      WORD32 sec_loop_cnt;

      for (k = in_loop_cnt; k != 0; k--) {
        x0r = (*data);
        x0i = (*(data + 1));
        data += (del << 1);

        x1r = (*data);
        x1i = (*(data + 1));
        data += (del << 1);

        x2r = (*data);
        x2i = (*(data + 1));
        data += (del << 1);

        x3r = (*data);
        x3i = (*(data + 1));
        data -= 3 * (del << 1);

        x0r = x0r + x2r;
        x0i = x0i + x2i;
        x2r = x0r - (x2r << 1);
        x2i = x0i - (x2i << 1);
        x1r = x1r + x3r;
        x1i = x1i + x3i;
        x3r = x1r - (x3r << 1);
        x3i = x1i - (x3i << 1);

        x0r = ixheaacd_add32_sat(x0r , x1r);
        x0i = ixheaacd_add32_sat(x0i , x1i);
        x1r = ixheaacd_sub32_sat(x0r , (x1r << 1));
        x1i = ixheaacd_sub32_sat(x0i , (x1i << 1));
        x2r = ixheaacd_sub32_sat(x2r , x3i);
        x2i = ixheaacd_add32_sat(x2i , x3r);
        x3i = ixheaacd_add32_sat(x2r , (x3i << 1));
        x3r = ixheaacd_sub32_sat(x2i , (x3r << 1));

        *data = x0r;
        *(data + 1) = x0i;
        data += (del << 1);

        *data = x2r;
        *(data + 1) = x2i;
        data += (del << 1);

        *data = x1r;
        *(data + 1) = x1i;
        data += (del << 1);

        *data = x3i;
        *(data + 1) = x3r;
        data += (del << 1);
      }
      data = ptr_y + 2;

      sec_loop_cnt = (nodespacing * del);
      sec_loop_cnt = (sec_loop_cnt / 4) + (sec_loop_cnt / 8) -
                     (sec_loop_cnt / 16) + (sec_loop_cnt / 32) -
                     (sec_loop_cnt / 64) + (sec_loop_cnt / 128) -
                     (sec_loop_cnt / 256);
      j = nodespacing;

      for (j = nodespacing; j <= sec_loop_cnt; j += nodespacing) {
        w1h = *(twiddles + 2 * j);
        w2h = *(twiddles + 2 * (j << 1));
        w3h = *(twiddles + 2 * j + 2 * (j << 1));
        w1l = *(twiddles + 2 * j + 1);
        w2l = *(twiddles + 2 * (j << 1) + 1);
        w3l = *(twiddles + 2 * j + 2 * (j << 1) + 1);

        for (k = in_loop_cnt; k != 0; k--) {
          WORD32 tmp;
          WORD32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

          data += (del << 1);

          x1r = *data;
          x1i = *(data + 1);
          data += (del << 1);

          x2r = *data;
          x2i = *(data + 1);
          data += (del << 1);

          x3r = *data;
          x3i = *(data + 1);
          data -= 3 * (del << 1);

          tmp = (ixheaacd_mult32(x1r, w1l) + ixheaacd_mult32(x1i, w1h));
          x1i = ixheaacd_mac32(-ixheaacd_mult32(x1r, w1h), x1i, w1l);
          x1r = tmp;

          tmp = (ixheaacd_mult32(x2r, w2l) + ixheaacd_mult32(x2i, w2h));
          x2i = ixheaacd_mac32(-ixheaacd_mult32(x2r, w2h), x2i, w2l);
          x2r = tmp;

          tmp = (ixheaacd_mult32(x3r, w3l) + ixheaacd_mult32(x3i, w3h));
          x3i = ixheaacd_mac32(-ixheaacd_mult32(x3r, w3h), x3i, w3l);
          x3r = tmp;

          x0r = (*data);
          x0i = (*(data + 1));

          x0r = x0r + (x2r);
          x0i = x0i + (x2i);
          x2r = x0r - (x2r << 1);
          x2i = x0i - (x2i << 1);
          x1r = x1r + x3r;
          x1i = x1i + x3i;
          x3r = x1r - (x3r << 1);
          x3i = x1i - (x3i << 1);

          x0r = x0r + (x1r);
          x0i = x0i + (x1i);
          x1r = x0r - (x1r << 1);
          x1i = x0i - (x1i << 1);
          x2r = x2r - (x3i);
          x2i = x2i + (x3r);
          x3i = x2r + (x3i << 1);
          x3r = x2i - (x3r << 1);

          *data = x0r;
          *(data + 1) = x0i;
          data += (del << 1);

          *data = x2r;
          *(data + 1) = x2i;
          data += (del << 1);

          *data = x1r;
          *(data + 1) = x1i;
          data += (del << 1);

          *data = x3i;
          *(data + 1) = x3r;
          data += (del << 1);
        }
        data -= 2 * npoints;
        data += 2;
      }
      for (; j <= (nodespacing * del) >> 1; j += nodespacing) {
        w1h = *(twiddles + 2 * j);
        w2h = *(twiddles + 2 * (j << 1));
        w3h = *(twiddles + 2 * j + 2 * (j << 1) - 512);
        w1l = *(twiddles + 2 * j + 1);
        w2l = *(twiddles + 2 * (j << 1) + 1);
        w3l = *(twiddles + 2 * j + 2 * (j << 1) - 511);

        for (k = in_loop_cnt; k != 0; k--) {
          WORD32 tmp;
          WORD32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

          data += (del << 1);

          x1r = *data;
          x1i = *(data + 1);
          data += (del << 1);

          x2r = *data;
          x2i = *(data + 1);
          data += (del << 1);

          x3r = *data;
          x3i = *(data + 1);
          data -= 3 * (del << 1);

          tmp = (ixheaacd_mult32(x1r, w1l) + ixheaacd_mult32(x1i, w1h));
          x1i = ixheaacd_mac32(-ixheaacd_mult32(x1r, w1h), x1i, w1l);
          x1r = tmp;

          tmp = (ixheaacd_mult32(x2r, w2l) + ixheaacd_mult32(x2i, w2h));
          x2i = ixheaacd_mac32(-ixheaacd_mult32(x2r, w2h), x2i, w2l);
          x2r = tmp;

          tmp = (ixheaacd_mult32(x3r, w3h) - ixheaacd_mult32(x3i, w3l));
          x3i = ixheaacd_mult32(x3r, w3l) + ixheaacd_mult32(x3i, w3h);
          x3r = tmp;

          x0r = (*data);
          x0i = (*(data + 1));

          x0r = x0r + (x2r);
          x0i = x0i + (x2i);
          x2r = x0r - (x2r << 1);
          x2i = x0i - (x2i << 1);
          x1r = x1r + x3r;
          x1i = x1i + x3i;
          x3r = x1r - (x3r << 1);
          x3i = x1i - (x3i << 1);

          x0r = x0r + (x1r);
          x0i = x0i + (x1i);
          x1r = x0r - (x1r << 1);
          x1i = x0i - (x1i << 1);
          x2r = x2r - (x3i);
          x2i = x2i + (x3r);
          x3i = x2r + (x3i << 1);
          x3r = x2i - (x3r << 1);

          *data = x0r;
          *(data + 1) = x0i;
          data += (del << 1);

          *data = x2r;
          *(data + 1) = x2i;
          data += (del << 1);

          *data = x1r;
          *(data + 1) = x1i;
          data += (del << 1);

          *data = x3i;
          *(data + 1) = x3r;
          data += (del << 1);
        }
        data -= 2 * npoints;
        data += 2;
      }
      for (; j <= sec_loop_cnt * 2; j += nodespacing) {
        w1h = *(twiddles + 2 * j);
        w2h = *(twiddles + 2 * (j << 1) - 512);
        w3h = *(twiddles + 2 * j + 2 * (j << 1) - 512);
        w1l = *(twiddles + 2 * j + 1);
        w2l = *(twiddles + 2 * (j << 1) - 511);
        w3l = *(twiddles + 2 * j + 2 * (j << 1) - 511);

        for (k = in_loop_cnt; k != 0; k--) {
          WORD32 tmp;
          WORD32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

          data += (del << 1);

          x1r = *data;
          x1i = *(data + 1);
          data += (del << 1);

          x2r = *data;
          x2i = *(data + 1);
          data += (del << 1);

          x3r = *data;
          x3i = *(data + 1);
          data -= 3 * (del << 1);

          tmp = (ixheaacd_mult32(x1r, w1l) + ixheaacd_mult32(x1i, w1h));
          x1i = ixheaacd_mac32(-ixheaacd_mult32(x1r, w1h), x1i, w1l);
          x1r = tmp;

          tmp = (ixheaacd_mult32(x2r, w2h) - ixheaacd_mult32(x2i, w2l));
          x2i = ixheaacd_mult32(x2r, w2l) + ixheaacd_mult32(x2i, w2h);
          x2r = tmp;

          tmp = (ixheaacd_mult32(x3r, w3h) - ixheaacd_mult32(x3i, w3l));
          x3i = ixheaacd_mult32(x3r, w3l) + ixheaacd_mult32(x3i, w3h);
          x3r = tmp;

          x0r = (*data);
          x0i = (*(data + 1));

          x0r = x0r + (x2r);
          x0i = x0i + (x2i);
          x2r = x0r - (x2r << 1);
          x2i = x0i - (x2i << 1);
          x1r = x1r + x3r;
          x1i = x1i + x3i;
          x3r = x1r - (x3r << 1);
          x3i = x1i - (x3i << 1);

          x0r = x0r + (x1r);
          x0i = x0i + (x1i);
          x1r = x0r - (x1r << 1);
          x1i = x0i - (x1i << 1);
          x2r = x2r - (x3i);
          x2i = x2i + (x3r);
          x3i = x2r + (x3i << 1);
          x3r = x2i - (x3r << 1);

          *data = x0r;
          *(data + 1) = x0i;
          data += (del << 1);

          *data = x2r;
          *(data + 1) = x2i;
          data += (del << 1);

          *data = x1r;
          *(data + 1) = x1i;
          data += (del << 1);

          *data = x3i;
          *(data + 1) = x3r;
          data += (del << 1);
        }
        data -= 2 * npoints;
        data += 2;
      }
      for (; j < nodespacing * del; j += nodespacing) {
        w1h = *(twiddles + 2 * j);
        w2h = *(twiddles + 2 * (j << 1) - 512);
        w3h = *(twiddles + 2 * j + 2 * (j << 1) - 1024);
        w1l = *(twiddles + 2 * j + 1);
        w2l = *(twiddles + 2 * (j << 1) - 511);
        w3l = *(twiddles + 2 * j + 2 * (j << 1) - 1023);

        for (k = in_loop_cnt; k != 0; k--) {
          WORD32 tmp;
          WORD32 x0r, x0i, x1r, x1i, x2r, x2i, x3r, x3i;

          data += (del << 1);

          x1r = *data;
          x1i = *(data + 1);
          data += (del << 1);

          x2r = *data;
          x2i = *(data + 1);
          data += (del << 1);

          x3r = *data;
          x3i = *(data + 1);
          data -= 3 * (del << 1);

          tmp = (ixheaacd_mult32(x1r, w1l) + ixheaacd_mult32(x1i, w1h));
          x1i = ixheaacd_mac32(-ixheaacd_mult32(x1r, w1h), x1i, w1l);
          x1r = tmp;

          tmp = (ixheaacd_mult32(x2r, w2h) - ixheaacd_mult32(x2i, w2l));
          x2i = ixheaacd_mult32(x2r, w2l) + ixheaacd_mult32(x2i, w2h);
          x2r = tmp;

          tmp = (-ixheaacd_mult32(x3r, w3l) - ixheaacd_mult32(x3i, w3h));
          x3i = ixheaacd_mac32(-ixheaacd_mult32(x3r, w3h), x3i, w3l);
          x3r = tmp;

          x0r = (*data);
          x0i = (*(data + 1));

          x0r = x0r + (x2r);
          x0i = x0i + (x2i);
          x2r = x0r - (x2r << 1);
          x2i = x0i - (x2i << 1);
          x1r = x1r + x3r;
          x1i = x1i - x3i;
          x3r = x1r - (x3r << 1);
          x3i = x1i + (x3i << 1);

          x0r = x0r + (x1r);
          x0i = x0i + (x1i);
          x1r = x0r - (x1r << 1);
          x1i = x0i - (x1i << 1);
          x2r = x2r - (x3i);
          x2i = x2i + (x3r);
          x3i = x2r + (x3i << 1);
          x3r = x2i - (x3r << 1);

          *data = x0r;
          *(data + 1) = x0i;
          data += (del << 1);

          *data = x2r;
          *(data + 1) = x2i;
          data += (del << 1);

          *data = x1r;
          *(data + 1) = x1i;
          data += (del << 1);

          *data = x3i;
          *(data + 1) = x3r;
          data += (del << 1);
        }
        data -= 2 * npoints;
        data += 2;
      }
      nodespacing >>= 2;
      del <<= 2;
      in_loop_cnt >>= 2;
    }
    if (not_power_4) {
      const WORD32 *twiddles = ptr_w;
      nodespacing <<= 1;
      shift += 1;
      for (j = del / 2; j != 0; j--) {
        WORD32 w1h = *twiddles;
        WORD32 w1l = *(twiddles + 1);

        WORD32 tmp;
        twiddles += nodespacing * 2;

        x0r = *ptr_y;
        x0i = *(ptr_y + 1);
        ptr_y += (del << 1);

        x1r = *ptr_y;
        x1i = *(ptr_y + 1);

        tmp = (ixheaacd_mult32(x1r, w1l) + ixheaacd_mult32(x1i, w1h));
        x1i = ixheaacd_mac32(-ixheaacd_mult32(x1r, w1h), x1i, w1l);
        x1r = tmp;

        *ptr_y = (x0r) / 2 - (x1r) / 2;
        *(ptr_y + 1) = (x0i) / 2 - (x1i) / 2;
        ptr_y -= (del << 1);

        *ptr_y = (x0r) / 2 + (x1r) / 2;
        *(ptr_y + 1) = (x0i) / 2 + (x1i) / 2;
        ptr_y += 2;
      }
      twiddles = ptr_w;
      for (j = del / 2; j != 0; j--) {
        WORD32 w1h = *twiddles;
        WORD32 w1l = *(twiddles + 1);
        WORD32 tmp;
        twiddles += nodespacing * 2;

        x0r = *ptr_y;
        x0i = *(ptr_y + 1);
        ptr_y += (del << 1);

        x1r = *ptr_y;
        x1i = *(ptr_y + 1);

        tmp = (ixheaacd_mult32(x1r, w1h) - ixheaacd_mult32(x1i, w1l));
        x1i = ixheaacd_mult32(x1r, w1l) + ixheaacd_mult32(x1i, w1h);
        x1r = tmp;

        *ptr_y = (x0r) / 2 - (x1r) / 2;
        *(ptr_y + 1) = (x0i) / 2 - (x1i) / 2;
        ptr_y -= (del << 1);

        *ptr_y = (x0r) / 2 + (x1r) / 2;
        *(ptr_y + 1) = (x0i) / 2 + (x1i) / 2;
        ptr_y += 2;
      }
    }
  }

  for (i = 0; i < nlength; i++) {
    xr[i] = y[2 * i];
    xi[i] = y[2 * i + 1];
  }

  *preshift = shift - *preshift;
  return;
}

static PLATFORM_INLINE void ixheaacd_complex_3point_fft(WORD32 *inp, WORD32 *op,
                                                        WORD32 sign_dir) {
  WORD32 add_r, sub_r;
  WORD32 add_i, sub_i;
  WORD32 temp_real, temp_imag, temp;

  WORD32 p1, p2, p3, p4;

  WORD32 sinmu;
  sinmu = -1859775393 * sign_dir;

  temp_real = ixheaacd_add32_sat(inp[0], inp[2]);
  temp_imag = ixheaacd_add32_sat(inp[1], inp[3]);

  add_r = ixheaacd_add32_sat(inp[2], inp[4]);
  add_i = ixheaacd_add32_sat(inp[3], inp[5]);

  sub_r = ixheaacd_sub32_sat(inp[2], inp[4]);
  sub_i = ixheaacd_sub32_sat(inp[3], inp[5]);

  p1 = add_r >> 1;
  p4 = add_i >> 1;
  p2 = ixheaacd_mult32_shl(sub_i, sinmu);
  p3 = ixheaacd_mult32_shl(sub_r, sinmu);

  temp = ixheaacd_sub32(inp[0], p1);

  op[0] = ixheaacd_add32_sat(temp_real, inp[4]);
  op[1] = ixheaacd_add32_sat(temp_imag, inp[5]);
  op[2] = ixheaacd_add32_sat(temp, p2);
  op[3] = ixheaacd_sub32_sat(ixheaacd_sub32_sat(inp[1], p3), p4);
  op[4] = ixheaacd_sub32_sat(temp, p2);
  op[5] = ixheaacd_sub32_sat(ixheaacd_add32_sat(inp[1], p3), p4);

  return;
}

VOID ixheaacd_complex_fft_p3(WORD32 *xr, WORD32 *xi, WORD32 nlength,
                             WORD32 fft_mode, WORD32 *preshift) {
  WORD32 i, j;
  WORD32 shift = 0;
  WORD32 xr_3[384];
  WORD32 xi_3[384];
  WORD32 x[1024];
  WORD32 y[1024];
  WORD32 cnfac, npts;
  WORD32 mpass = nlength;
  WORD32 n = 0;
  WORD32 *ptr_x = x;
  WORD32 *ptr_y = y;

  cnfac = 0;
  while (mpass % 3 == 0) {
    mpass /= 3;
    cnfac++;
  }
  npts = mpass;

  for (i = 0; i < 3 * cnfac; i++) {
    for (j = 0; j < mpass; j++) {
      xr_3[j] = xr[3 * j + i];
      xi_3[j] = xi[3 * j + i];
    }

    (*ixheaacd_complex_fft_p2)(xr_3, xi_3, mpass, fft_mode, &shift);

    for (j = 0; j < mpass; j++) {
      xr[3 * j + i] = xr_3[j];
      xi[3 * j + i] = xi_3[j];
    }
  }

  while (npts >> 1) {
    n++;
    npts = npts >> 1;
  }

  if (n % 2 == 0)
    shift = ((n + 4)) / 2;
  else
    shift = ((n + 5) / 2);

  *preshift = shift - *preshift + 1;

  for (i = 0; i < nlength; i++) {
    ptr_x[2 * i] = (xr[i] >> 1);
    ptr_x[2 * i + 1] = (xi[i] >> 1);
  }

  {
    const WORD32 *w1r, *w1i;
    WORD32 tmp;
    w1r = ixheaacd_twiddle_table_3pr;
    w1i = ixheaacd_twiddle_table_3pi;

    if (fft_mode < 0) {
      for (i = 0; i < nlength; i += 3) {
        tmp = ixheaacd_sub32_sat(ixheaacd_mult32(ptr_x[2 * i], (*w1r)),
                                 ixheaacd_mult32(ptr_x[2 * i + 1], (*w1i)));
        ptr_x[2 * i + 1] =
            ixheaacd_add32_sat(ixheaacd_mult32(ptr_x[2 * i], (*w1i)),
                               ixheaacd_mult32(ptr_x[2 * i + 1], (*w1r)));
        ptr_x[2 * i] = tmp;

        w1r++;
        w1i++;

        tmp = ixheaacd_sub32_sat(ixheaacd_mult32(ptr_x[2 * i + 2], (*w1r)),
                                 ixheaacd_mult32(ptr_x[2 * i + 3], (*w1i)));
        ptr_x[2 * i + 3] =
            ixheaacd_add32_sat(ixheaacd_mult32(ptr_x[2 * i + 2], (*w1i)),
                               ixheaacd_mult32(ptr_x[2 * i + 3], (*w1r)));
        ptr_x[2 * i + 2] = tmp;

        w1r++;
        w1i++;

        tmp = ixheaacd_sub32_sat(ixheaacd_mult32(ptr_x[2 * i + 4], (*w1r)),
                                 ixheaacd_mult32(ptr_x[2 * i + 5], (*w1i)));
        ptr_x[2 * i + 5] =
            ixheaacd_add32_sat(ixheaacd_mult32(ptr_x[2 * i + 4], (*w1i)),
                               ixheaacd_mult32(ptr_x[2 * i + 5], (*w1r)));
        ptr_x[2 * i + 4] = tmp;

        w1r += 3 * (128 / mpass - 1) + 1;
        w1i += 3 * (128 / mpass - 1) + 1;
      }
    }

    else {
      for (i = 0; i < nlength; i += 3) {
        tmp = ixheaacd_add32_sat(ixheaacd_mult32(ptr_x[2 * i], (*w1r)),
                                 ixheaacd_mult32(ptr_x[2 * i + 1], (*w1i)));
        ptr_x[2 * i + 1] =
            ixheaacd_sub32_sat(ixheaacd_mult32(ptr_x[2 * i + 1], (*w1r)),
                               ixheaacd_mult32(ptr_x[2 * i], (*w1i)));
        ptr_x[2 * i] = tmp;

        w1r++;
        w1i++;

        tmp = ixheaacd_add32_sat(ixheaacd_mult32(ptr_x[2 * i + 2], (*w1r)),
                                 ixheaacd_mult32(ptr_x[2 * i + 3], (*w1i)));
        ptr_x[2 * i + 3] =
            ixheaacd_sub32_sat(ixheaacd_mult32(ptr_x[2 * i + 3], (*w1r)),
                               ixheaacd_mult32(ptr_x[2 * i + 2], (*w1i)));
        ptr_x[2 * i + 2] = tmp;

        w1r++;
        w1i++;

        tmp = ixheaacd_add32_sat(ixheaacd_mult32(ptr_x[2 * i + 4], (*w1r)),
                                 ixheaacd_mult32(ptr_x[2 * i + 5], (*w1i)));
        ptr_x[2 * i + 5] =
            ixheaacd_sub32_sat(ixheaacd_mult32(ptr_x[2 * i + 5], (*w1r)),
                               ixheaacd_mult32(ptr_x[2 * i + 4], (*w1i)));
        ptr_x[2 * i + 4] = tmp;

        w1r += 3 * (128 / mpass - 1) + 1;
        w1i += 3 * (128 / mpass - 1) + 1;
      }
    }
  }

  for (i = 0; i < mpass; i++) {
    ixheaacd_complex_3point_fft(ptr_x, ptr_y, fft_mode);

    ptr_x = ptr_x + 6;
    ptr_y = ptr_y + 6;
  }

  for (i = 0; i < mpass; i++) {
    xr[i] = y[6 * i];
    xi[i] = y[6 * i + 1];
  }

  for (i = 0; i < mpass; i++) {
    xr[mpass + i] = y[6 * i + 2];
    xi[mpass + i] = y[6 * i + 3];
  }

  for (i = 0; i < mpass; i++) {
    xr[2 * mpass + i] = y[6 * i + 4];
    xi[2 * mpass + i] = y[6 * i + 5];
  }
  return;
}

VOID ixheaacd_complex_fft(WORD32 *data_r, WORD32 *data_i, WORD32 nlength,
                          WORD32 fft_mode, WORD32 *preshift) {
  if (nlength & (nlength - 1)) {
    if ((nlength != 24) && (nlength != 48) && (nlength != 96) &&
        (nlength != 192) && (nlength != 384)) {
      printf("%d point FFT not supported", nlength);
      exit(0);
    }
    ixheaacd_complex_fft_p3(data_r, data_i, nlength, fft_mode, preshift);
  } else
    (*ixheaacd_complex_fft_p2)(data_r, data_i, nlength, fft_mode, preshift);

  return;
}
