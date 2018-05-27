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
#ifndef IXHEAACD_BASIC_OPS32_H
#define IXHEAACD_BASIC_OPS32_H

static PLATFORM_INLINE WORD32 ixheaacd_min32(WORD32 a, WORD32 b) {
  WORD32 min_val;

  min_val = (a < b) ? a : b;

  return min_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_max32(WORD32 a, WORD32 b) {
  WORD32 max_val;

  max_val = (a > b) ? a : b;

  return max_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_shl32(WORD32 a, WORD b) {
  WORD32 out_val;

  b = ((UWORD32)(b << 24) >> 24);
  if (b > 31)
    out_val = 0;
  else
    out_val = (WORD32)a << b;

  return out_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_shr32(WORD32 a, WORD b) {
  WORD32 out_val;

  b = ((UWORD32)(b << 24) >> 24);
  if (b >= 31) {
    if (a < 0)
      out_val = -1;
    else
      out_val = 0;
  } else {
    out_val = (WORD32)a >> b;
  }

  return out_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_shl32_sat(WORD32 a, WORD b) {
  WORD32 out_val = a;
  for (; b > 0; b--) {
    if (a > (WORD32)0X3fffffffL) {
      out_val = MAX_32;
      break;
    } else if (a < (WORD32)0xc0000000L) {
      out_val = MIN_32;
      break;
    }

    a = ixheaacd_shl32(a, 1);
    out_val = a;
  }
  return (out_val);
}

static PLATFORM_INLINE WORD32 ixheaacd_shl32_dir(WORD32 a, WORD b) {
  WORD32 out_val;

  if (b < 0) {
    out_val = ixheaacd_shr32(a, -b);
  } else {
    out_val = ixheaacd_shl32(a, b);
  }

  return out_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_shl32_dir_sat(WORD32 a, WORD b) {
  WORD32 out_val;

  if (b < 0) {
    out_val = ixheaacd_shr32(a, -b);
  } else {
    out_val = ixheaacd_shl32_sat(a, b);
  }

  return out_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_shr32_dir(WORD32 a, WORD b) {
  WORD32 out_val;

  if (b < 0) {
    out_val = ixheaacd_shl32(a, -b);
  } else {
    out_val = ixheaacd_shr32(a, b);
  }

  return out_val;
}

static PLATFORM_INLINE WORD32 shr32_dir_sat(WORD32 a, WORD b) {
  WORD32 out_val;

  if (b < 0) {
    out_val = ixheaacd_shl32_sat(a, -b);
  } else {
    out_val = ixheaacd_shr32(a, b);
  }

  return out_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_mult16x16in32(WORD16 a, WORD16 b) {
  WORD32 product;

  product = (WORD32)a * (WORD32)b;

  return product;
}

static PLATFORM_INLINE WORD32 mult16x16in32_32(WORD32 a, WORD32 b) {
  WORD32 product;

  product = (WORD32)a * (WORD32)b;

  return product;
}

static PLATFORM_INLINE WORD32 ixheaacd_mult16x16in32_shl(WORD16 a, WORD16 b) {
  WORD32 product;

  product = ixheaacd_shl32(ixheaacd_mult16x16in32(a, b), 1);

  return product;
}

static PLATFORM_INLINE WORD32 ixheaacd_mult16x16in32_shl_sat(WORD16 a,
                                                             WORD16 b) {
  WORD32 product;
  product = (WORD32)a * (WORD32)b;
  if (product != (WORD32)0x40000000L) {
    product = ixheaacd_shl32(product, 1);
  } else {
    product = MAX_32;
  }
  return product;
}

static PLATFORM_INLINE WORD32 ixheaacd_add32(WORD32 a, WORD32 b) {
  WORD32 sum;

  sum = (WORD32)a + (WORD32)b;

  return sum;
}

static PLATFORM_INLINE WORD32 ixheaacd_sub32(WORD32 a, WORD32 b) {
  WORD32 diff;

  diff = (WORD32)a - (WORD32)b;

  return diff;
}

static PLATFORM_INLINE WORD32 ixheaacd_add32_sat(WORD32 a, WORD32 b) {
  WORD64 sum;

  sum = (WORD64)a + (WORD64)b;

  if ((((WORD32)a ^ (WORD32)b) & (WORD32)MIN_32) == 0) {
    if (((WORD32)sum ^ (WORD32)a) & (WORD32)MIN_32) {
      sum = (a < 0) ? MIN_32 : MAX_32;
    }
  }

  return (WORD32)sum;
}

static PLATFORM_INLINE WORD32 ixheaacd_add32_sat3(WORD32 a, WORD32 b,
                                                  WORD32 c) {
  WORD64 sum;

  sum = (WORD64)a + (WORD64)b;

  sum = (WORD64)sum + (WORD64)c;

  if (sum > MAX_32) {
    sum = MAX_32;
  }
  if (sum < MIN_32) {
    sum = MIN_32;
  }

  return (WORD32)sum;
}

static PLATFORM_INLINE WORD32 ixheaacd_sub32_sat(WORD32 a, WORD32 b) {
  WORD64 diff;

  diff = (WORD64)a - (WORD64)b;

  if ((((WORD32)a ^ (WORD32)b) & (WORD32)MIN_32) != 0) {
    if (((WORD32)diff ^ (WORD32)a) & (WORD32)MIN_32) {
      diff = (a < 0L) ? MIN_32 : MAX_32;
    }
  }

  return (WORD32)diff;
}

static PLATFORM_INLINE WORD ixheaacd_norm32(WORD32 a) {
  WORD norm_val;

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

static PLATFORM_INLINE WORD ixheaacd_pnorm32(WORD32 a) {
  WORD norm_val;

  if (a == 0) {
    norm_val = 31;
  } else {
    for (norm_val = 0; a < (WORD32)0x40000000L; norm_val++) {
      a <<= 1;
    }
  }

  return norm_val;
}

static PLATFORM_INLINE WORD bin_expo32(WORD32 a) {
  WORD bin_expo_val;

  bin_expo_val = 31 - ixheaacd_norm32(a);

  return bin_expo_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_abs32(WORD32 a) {
  WORD32 abs_val;

  abs_val = a;

  if (a < 0) {
    abs_val = -a;
  }

  return abs_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_abs32_nrm(WORD32 a) {
  WORD32 abs_val;

  abs_val = a;

  if (a < 0) {
    abs_val = ~a;
  }

  return abs_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_abs32_sat(WORD32 a) {
  WORD32 abs_val;

  abs_val = a;

  if (a == MIN_32) {
    abs_val = MAX_32;
  } else if (a < 0) {
    abs_val = -a;
  }

  return abs_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_negate32(WORD32 a) {
  WORD32 neg_val;

  neg_val = -a;

  return neg_val;
}

static PLATFORM_INLINE WORD32 ixheaacd_negate32_sat(WORD32 a) {
  WORD32 neg_val;


  if (a == MIN_32)
  {
    neg_val = MAX_32;
  }
  else
  {
    neg_val = -a;
  }
  return neg_val;
}

static PLATFORM_INLINE WORD32 div32(WORD32 a, WORD32 b, WORD *q_format) {
  WORD32 quotient;
  UWORD32 mantissa_nr, mantissa_dr;
  WORD16 sign = 0;

  LOOPINDEX i;
  WORD q_nr, q_dr;

  mantissa_nr = a;
  mantissa_dr = b;
  quotient = 0;

  if ((a < 0) && (0 != b)) {
    a = -a;
    sign = (WORD16)(sign ^ -1);
  }

  if (b < 0) {
    b = -b;
    sign = (WORD16)(sign ^ -1);
  }

  if (0 == b) {
    *q_format = 0;
    return (a);
  }

  quotient = 0;

  q_nr = ixheaacd_norm32(a);
  mantissa_nr = (UWORD32)a << (q_nr);
  q_dr = ixheaacd_norm32(b);
  mantissa_dr = (UWORD32)b << (q_dr);
  *q_format = (WORD)(30 + q_nr - q_dr);

  for (i = 0; i < 31; i++) {
    quotient = quotient << 1;

    if (mantissa_nr >= mantissa_dr) {
      mantissa_nr = mantissa_nr - mantissa_dr;
      quotient += 1;
    }

    mantissa_nr = (UWORD32)mantissa_nr << 1;
  }

  if (sign < 0) {
    quotient = -quotient;
  }

  return quotient;
}

static PLATFORM_INLINE WORD32 ixheaacd_mac16x16in32(WORD32 a, WORD16 b,
                                                    WORD16 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32(b, c);

  acc = ixheaacd_add32(a, acc);

  return acc;
}

static PLATFORM_INLINE WORD32 mac16x16hin32(WORD32 a, WORD32 b, WORD32 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32((WORD16)b, (WORD16)(c >> 16));

  acc = ixheaacd_add32(a, acc);

  return acc;
}

static PLATFORM_INLINE WORD32 ixheaacd_mac16x16in32_shl(WORD32 a, WORD16 b,
                                                        WORD16 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32_shl(b, c);

  acc = ixheaacd_add32(a, acc);

  return acc;
}

static PLATFORM_INLINE WORD32 ixheaacd_mac16x16in32_shl_sat(WORD32 a, WORD16 b,
                                                            WORD16 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32_shl_sat(b, c);

  acc = ixheaacd_add32_sat(a, acc);

  return acc;
}

static PLATFORM_INLINE WORD32 msu16x16in32(WORD32 a, WORD16 b, WORD16 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32(b, c);

  acc = ixheaacd_sub32(a, acc);

  return acc;
}

static PLATFORM_INLINE WORD32 msu16x16in32_shl(WORD32 a, WORD16 b, WORD16 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32_shl(b, c);

  acc = ixheaacd_sub32(a, acc);

  return acc;
}

static PLATFORM_INLINE WORD32 msu16x16in32_shl_sat(WORD32 a, WORD16 b,
                                                   WORD16 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32_shl_sat(b, c);

  acc = ixheaacd_sub32_sat(a, acc);

  return acc;
}

static PLATFORM_INLINE WORD32 add32_shr(WORD32 a, WORD32 b) {
  WORD32 sum;

  a = ixheaacd_shr32(a, 1);
  b = ixheaacd_shr32(b, 1);

  sum = ixheaacd_add32(a, b);

  return sum;
}

static PLATFORM_INLINE WORD32 sub32_shr(WORD32 a, WORD32 b) {
  WORD32 diff;

  a = ixheaacd_shr32(a, 1);
  b = ixheaacd_shr32(b, 1);

  diff = ixheaacd_sub32(a, b);

  return diff;
}
#endif
