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
#ifndef IXHEAACD_BASIC_OPS40_H
#define IXHEAACD_BASIC_OPS40_H
#define lo64(a) (((unsigned *)&a)[0])
#define hi64(a) (((WORD32 *)&a)[1])

static PLATFORM_INLINE WORD16 norm40(WORD40 *in) {
  WORD16 expo;
  WORD32 tempo;

  if (0 == (*in)) return 31;

  if (((*in) <= 0x7fffffff) && ((WORD40)(*in) >= (WORD40)0xFFFFFFFF80000000)) {
    tempo = (WORD32)(*in);
    expo = ixheaacd_norm32(tempo);
    *in = tempo << expo;

    return (expo);
  }

  tempo = (WORD32)((*in) >> 31);
  expo = 31 - (ixheaacd_norm32(tempo));
  *in = (*in) >> expo;

  return (-expo);
}

static PLATFORM_INLINE WORD32 add32_shr40(WORD32 a, WORD32 b) {
  WORD40 sum;

  sum = (WORD40)a + (WORD40)b;
  sum = sum >> 1;

  return ((WORD32)sum);
}

static PLATFORM_INLINE WORD32 sub32_shr40(WORD32 a, WORD32 b) {
  WORD40 sum;

  sum = (WORD40)a - (WORD40)b;
  sum = sum >> 1;

  return ((WORD32)sum);
}

static PLATFORM_INLINE WORD32 ixheaacd_mult32x16in32_shl(WORD32 a, WORD16 b) {
  WORD32 result;
  WORD64 temp_result;

  temp_result = (WORD64)a * (WORD64)b;

  result = (WORD32)(temp_result >> 16);

  return (result << 1);
}

static PLATFORM_INLINE WORD32 mult32x16hin32_shl(WORD32 a, WORD32 b) {
  WORD32 product;
  WORD64 temp_product;

  temp_product = (WORD64)a * (WORD64)(b >> 16);
  product = (WORD32)(temp_product >> 16);

  return (product << 1);
}

static PLATFORM_INLINE WORD32 ixheaacd_mult32x16in32(WORD32 a, WORD16 b) {
  WORD32 result;
  WORD64 temp_result;

  temp_result = (WORD64)a * (WORD64)b;

  result = (WORD32)(temp_result >> 16);

  return (result);
}

static PLATFORM_INLINE WORD32 ixheaacd_mult32x16in32_shl_sat(WORD32 a,
                                                             WORD16 b) {
  WORD32 result;

  if (a == (WORD32)0x80000000 && b == (WORD16)0x8000) {
    result = (WORD32)0x7fffffff;
  } else {
    result = ixheaacd_mult32x16in32_shl(a, b);
  }

  return (result);
}

static PLATFORM_INLINE WORD32 ixheaacd_mult32_shl(WORD32 a, WORD32 b) {
  WORD32 result;
  WORD64 temp_result;

  temp_result = (WORD64)a * (WORD64)b;
  result = (WORD32)(temp_result >> 32);

  return (result << 1);
}

static PLATFORM_INLINE WORD32 ixheaacd_mult32(WORD32 a, WORD32 b) {
  WORD32 result;
  WORD64 temp_result;

  temp_result = (WORD64)a * (WORD64)b;
  result = (WORD32)(temp_result >> 32);

  return (result);
}

static PLATFORM_INLINE WORD32 ixheaacd_mult32_shl_sat(WORD32 a, WORD32 b) {
  WORD32 result;

  if (a == (WORD32)0x80000000 && b == (WORD32)0x80000000) {
    result = 0x7fffffff;
  } else {
    result = ixheaacd_mult32_shl(a, b);
  }

  return (result);
}

static PLATFORM_INLINE WORD32 ixheaacd_mac32x16in32(WORD32 a, WORD32 b,
                                                    WORD16 c) {
  WORD32 result;

  result = a + ixheaacd_mult32x16in32(b, c);

  return (result);
}

static PLATFORM_INLINE WORD32 ixheaacd_mac32x16in32_shl(WORD32 a, WORD32 b,
                                                        WORD16 c) {
  WORD32 result;

  result = a + ixheaacd_mult32x16in32_shl(b, c);

  return (result);
}

static PLATFORM_INLINE WORD32 mac32x16in32_shl_sat(WORD32 a, WORD32 b,
                                                   WORD16 c) {
  return (ixheaacd_add32_sat(a, ixheaacd_mult32x16in32_shl_sat(b, c)));
}

static PLATFORM_INLINE WORD32 ixheaacd_mac32(WORD32 a, WORD32 b, WORD32 c) {
  WORD32 result;

  result = a + ixheaacd_mult32(b, c);

  return (result);
}

static PLATFORM_INLINE WORD32 mac32_shl(WORD32 a, WORD32 b, WORD32 c) {
  WORD32 result;

  result = a + ixheaacd_mult32_shl(b, c);

  return (result);
}

static PLATFORM_INLINE WORD32 mac32_shl_sat(WORD32 a, WORD32 b, WORD32 c) {
  return (ixheaacd_add32_sat(a, ixheaacd_mult32_shl_sat(b, c)));
}

static PLATFORM_INLINE WORD32 msu32x16in32(WORD32 a, WORD32 b, WORD16 c) {
  WORD32 result;

  result = a - ixheaacd_mult32x16in32(b, c);

  return (result);
}

static PLATFORM_INLINE WORD32 msu32x16in32_shl(WORD32 a, WORD32 b, WORD16 c) {
  WORD32 result;

  result = a - ixheaacd_mult32x16in32_shl(b, c);

  return (result);
}

static PLATFORM_INLINE WORD32 msu32x16in32_shl_sat(WORD32 a, WORD32 b,
                                                   WORD16 c) {
  return (ixheaacd_sub32_sat(a, ixheaacd_mult32x16in32_shl_sat(b, c)));
}

static PLATFORM_INLINE WORD32 msu32(WORD32 a, WORD32 b, WORD32 c) {
  WORD32 result;

  result = a - ixheaacd_mult32(b, c);

  return (result);
}

static PLATFORM_INLINE WORD32 msu32_shl(WORD32 a, WORD32 b, WORD32 c) {
  WORD32 result;

  result = a - ixheaacd_mult32_shl(b, c);

  return (result);
}

static PLATFORM_INLINE WORD32 msu32_shl_sat(WORD32 a, WORD32 b, WORD32 c) {
  return (ixheaacd_sub32_sat(a, ixheaacd_mult32_shl_sat(b, c)));
}

static PLATFORM_INLINE WORD32 mac3216_arr40(WORD32 *x, WORD16 *y,
                                            LOOPINDEX length, WORD16 *q_val) {
  LOOPINDEX i;
  WORD40 sum = 0;

  for (i = 0; i < length; i++) {
    sum += (WORD40)(ixheaacd_mult32x16in32(x[i], y[i]));
  }

  *q_val = norm40(&sum);

  return (WORD32)sum;
}

static PLATFORM_INLINE WORD32 mac32_arr40(WORD32 *x, WORD32 *y,
                                          LOOPINDEX length, WORD16 *q_val) {
  LOOPINDEX i;
  WORD40 sum = 0;

  for (i = 0; i < length; i++) {
    sum += (WORD40)(ixheaacd_mult32(x[i], y[i]));
  }

  *q_val = norm40(&sum);

  return ((WORD32)sum);
}

static PLATFORM_INLINE WORD32 mac16_arr40(WORD16 *x, WORD16 *y,
                                          LOOPINDEX length, WORD16 *q_val) {
  LOOPINDEX i;
  WORD40 sum = 0;

  for (i = 0; i < length; i++) {
    sum += (WORD40)((WORD32)x[i] * (WORD32)y[i]);
  }

  *q_val = norm40(&sum);

  return ((WORD32)sum);
}

static PLATFORM_INLINE WORD32 add32_arr40(WORD32 *in_arr, LOOPINDEX length,
                                          WORD16 *q_val) {
  LOOPINDEX i;
  WORD40 sum = 0;

  for (i = 0; i < length; i++) {
    sum += (WORD40)in_arr[i];
  }

  *q_val = norm40(&sum);

  return ((WORD32)sum);
}

static PLATFORM_INLINE WORD64 ixheaacd_mult32x32in64(WORD32 a, WORD32 b) {
  WORD64 result;

  result = (WORD64)a * (WORD64)b;

  return (result);
}

static PLATFORM_INLINE WORD64 ixheaacd_mac32x32in64(WORD64 sum, WORD32 a,
                                                    WORD32 b) {
  sum += (WORD64)a * (WORD64)b;

  return (sum);
}

static PLATFORM_INLINE WORD64 ixheaacd_mac32x32in64_7(WORD64 sum,
                                                      const WORD32 *a,
                                                      const WORD16 *b) {
  sum = (WORD64)a[0] * (WORD64)b[0];
  sum += (WORD64)a[1] * (WORD64)b[1];
  sum += (WORD64)a[2] * (WORD64)b[2];
  sum += (WORD64)a[3] * (WORD64)b[3];
  sum += (WORD64)a[4] * (WORD64)b[4];
  sum += (WORD64)a[5] * (WORD64)b[5];
  sum += (WORD64)a[6] * (WORD64)b[6];

  return (sum);
}

static PLATFORM_INLINE WORD64 ixheaacd_mac32x32in64_n(WORD64 sum,
                                                      const WORD32 *a,
                                                      const WORD16 *b,
                                                      WORD32 n) {
  WORD32 k;

  sum += (WORD64)a[0] * (WORD64)b[0];
  for (k = 1; k < n; k++) sum += (WORD64)a[k] * (WORD64)b[k];
  return (sum);
}

static PLATFORM_INLINE WORD64 ixheaacd_mult64(WORD32 a, WORD32 b) {
  WORD64 result;
  result = (WORD64)a * (WORD64)b;
  return (result);
}

static PLATFORM_INLINE WORD64 ixheaacd_add64(WORD64 a, WORD64 b) {
  WORD64 result;
  result = a + b;
  return (result);
}

static PLATFORM_INLINE WORD64 ixheaacd_sub64(WORD64 a, WORD64 b) {
  WORD64 diff;

  diff = (WORD64)a - (WORD64)b;

  return diff;
}

static PLATFORM_INLINE WORD64 ixheaacd_sub64_sat(WORD64 a, WORD64 b) {
  WORD64 diff;

  diff = ixheaacd_sub64(a, b);

  if ((((WORD64)a ^ (WORD64)b) & (WORD64)MIN_64) != 0) {
    if (((WORD64)diff ^ (WORD64)a) & (WORD64)MIN_64) {
      diff = (a < 0L) ? MIN_64 : MAX_64;
    }
  }

  return (diff);
}

static PLATFORM_INLINE WORD32 ixheaacd_mul32_sh(WORD32 a, WORD32 b,
                                                WORD8 shift) {
  WORD32 result;
  WORD64 temp_result;

  temp_result = (WORD64)a * (WORD64)b;
  result = (WORD32)(temp_result >> shift);

  return (result);
}

#endif
