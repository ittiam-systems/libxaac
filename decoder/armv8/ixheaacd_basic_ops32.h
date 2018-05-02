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

#if 0
//returns the minima of 2 32 bit variables
static PLATFORM_INLINE WORD32 ixheaacd_min32(WORD32 a, WORD32 b)
{
	WORD32 min_val;
	asm (
		"CMP %w[a], %w[b]\n\t"
		"CSEL %w[min_val], %w[b], %w[a], GT\n"
		: [min_val] "=r" (min_val), [a] "+r" (a)
		: [b] "r" (b)
		: "cc"
		);
	return (min_val);
}

//returns the maxima of 2 32 bit variables 
static PLATFORM_INLINE WORD32 ixheaacd_max32(WORD32 a, WORD32 b)
{
	WORD32 max_val;
	asm (
		"CMP %w[a], %w[b]\n"
		"CSEL %w[max_val], %w[b], %w[a], LT\n"
		: [max_val] "=r" (max_val), [a] "+r" (a)
		: [b] "r" (b)
		: "cc"
		);
	return (max_val);
}
#else
static PLATFORM_INLINE WORD32 ixheaacd_min32(WORD32 a, WORD32 b) {
  WORD32 min_val;

  min_val = (a < b) ? a : b;

  return min_val;
}

// returns the maxima of 2 32 bit variables
static PLATFORM_INLINE WORD32 ixheaacd_max32(WORD32 a, WORD32 b) {
  WORD32 max_val;

  max_val = (a > b) ? a : b;

  return max_val;
}

#endif

// shifts a 32-bit value left by specificed bits
static PLATFORM_INLINE WORD32 ixheaacd_shl32(WORD32 a, WORD b) {
  WORD32 out_val;

  b = ((UWORD32)(b << 24) >> 24); /* Mod 8 */
  if (b > 31)
    out_val = 0;
  else
    out_val = (WORD32)a << b;

  return out_val;
}

// shifts a 32-bit value right by specificed bits
static PLATFORM_INLINE WORD32 ixheaacd_shr32(WORD32 a, WORD b) {
  WORD32 out_val;

  b = ((UWORD32)(b << 24) >> 24); /* Mod 8 */
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

// shifts a 32-bit value left by specificed bits and saturates it to 32 bits
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

// shifts a 32-bit value left by specificed bits, shifts
// it right if specified no. of bits is negative

static PLATFORM_INLINE WORD32 ixheaacd_shl32_dir(WORD32 a, WORD b) {
  WORD32 out_val;

  if (b < 0) {
    out_val = ixheaacd_shr32(a, -b);
  } else {
    out_val = ixheaacd_shl32(a, b);
  }

  return out_val;
}

// shifts a 32-bit value left by specificed bits with sat,
// shifts it right if specified no. of bits is negative

static PLATFORM_INLINE WORD32 ixheaacd_shl32_dir_sat(WORD32 a, WORD b) {
  WORD32 out_val;

  if (b < 0) {
    out_val = ixheaacd_shr32(a, -b);
  } else {
    out_val = ixheaacd_shl32_sat(a, b);
  }

  return out_val;
}

// shifts a 32-bit value right by specificed bits, shifts
// it left if specified no. of bits is negative
static PLATFORM_INLINE WORD32 ixheaacd_shr32_dir(WORD32 a, WORD b) {
  WORD32 out_val;

  if (b < 0) {
    out_val = ixheaacd_shl32(a, -b);
  } else {
    out_val = ixheaacd_shr32(a, b);
  }

  return out_val;
}

// shifts a 32-bit value right by specificed bits, shifts
// it left with sat if specified no. of bits is negative
static PLATFORM_INLINE WORD32 shr32_dir_sat(WORD32 a, WORD b) {
  WORD32 out_val;

  if (b < 0) {
    out_val = ixheaacd_shl32_sat(a, -b);
  } else {
    out_val = ixheaacd_shr32(a, b);
  }

  return out_val;
}

// multiplies two 16 bit numbers and returns their 32-bit result
static PLATFORM_INLINE WORD32 ixheaacd_mult16x16in32(WORD16 a, WORD16 b) {
  WORD32 product;

  product = (WORD32)a * (WORD32)b;

  return product;
}

// multiplies two 32 bit numbers considering their last
// 16 bits and returns their 32-bit result
static PLATFORM_INLINE WORD32 mult16x16in32_32(WORD32 a, WORD32 b) {
  WORD32 product;
  asm("AND %w[a], %w[a], #0x0000FFFF\n"
      "SXTH %w[a], %w[a]\n"
      "AND %w[b], %w[b], #0x0000FFFF\n"
      "SXTH %w[b], %w[b]\n"
      "MUL %w[product], %w[a], %w[b]\n"
      : [product] "=r"(product)
      : [b] "r"(b), [a] "r"(a));
  return product;
}

// multiplies two 16 bit numbers and returns their 32-bit
// result after removing 1 redundant sign bit
static PLATFORM_INLINE WORD32 ixheaacd_mult16x16in32_shl(WORD16 a, WORD16 b) {
  WORD32 product;

  product = ixheaacd_shl32(ixheaacd_mult16x16in32(a, b), 1);

  return product;
}

// multiplies two 16 bit numbers and returns their 32-bit
// result after removing 1 redundant sign bit with saturation
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

// adds 2 32 bit variables
static PLATFORM_INLINE WORD32 ixheaacd_add32(WORD32 a, WORD32 b) {
  WORD32 sum;

  sum = (WORD32)a + (WORD32)b;

  return sum;
}

// subtract 2 32 bit variables
static PLATFORM_INLINE WORD32 ixheaacd_sub32(WORD32 a, WORD32 b) {
  WORD32 diff;

  diff = (WORD32)a - (WORD32)b;

  return diff;
}

// adds 2 32 bit variables with saturation
static PLATFORM_INLINE WORD32 ixheaacd_add32_sat(WORD32 a, WORD32 b) {
  WORD32 sum;

  sum = ixheaacd_add32(a, b);

  if ((((WORD32)a ^ (WORD32)b) & (WORD32)MIN_32) == 0) {
    if (((WORD32)sum ^ (WORD32)a) & (WORD32)MIN_32) {
      sum = (a < 0) ? MIN_32 : MAX_32;
    }
  }

  return sum;
}

// subtract 2 32 bit variables
static PLATFORM_INLINE WORD32 ixheaacd_sub32_sat(WORD32 a, WORD32 b) {
  WORD32 diff;

  diff = ixheaacd_sub32(a, b);

  if ((((WORD32)a ^ (WORD32)b) & (WORD32)MIN_32) != 0) {
    if (((WORD32)diff ^ (WORD32)a) & (WORD32)MIN_32) {
      diff = (a < 0L) ? MIN_32 : MAX_32;
    }
  }

  return (diff);
}

// returns number of redundant sign bits in a 32-bit value.
// return zero for a value of zero
static PLATFORM_INLINE WORD ixheaacd_norm32(WORD32 a) {
#if 1
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
#else
  WORD32 norm_val, temp;
  asm("ASR %w[temp], %w[a], #31\n"
      "EOR  %w[norm_val], %w[a], %w[temp]\n"
      "CLZ %w[norm_val], %w[norm_val]\n"
      "SUB  %w[norm_val], %w[norm_val], #1\n"
      : [norm_val] "=r"(norm_val), [temp] "+r"(temp)
      : [a] "r"(a)
      : "cc");
#endif
  return norm_val;
}

static PLATFORM_INLINE WORD ixheaacd_pnorm32(WORD32 a) {
  WORD32 norm_val;
  asm("CLZ %w[norm_val], %w[a]\n"
      "SUB  %w[norm_val], %w[norm_val], #1\n"
      : [norm_val] "=r"(norm_val)
      : [a] "r"(a));
  return norm_val;
}

// returns the position of the most significant bit for negative numbers.
// ignores leading zeros to determine the position of most significant bit.
static PLATFORM_INLINE WORD bin_expo32(WORD32 a) {
  WORD bin_expo_val;

  bin_expo_val = 31 - ixheaacd_norm32(a);

  return bin_expo_val;
}

// returns the absolute value of 32-bit number
static PLATFORM_INLINE WORD32 ixheaacd_abs32(WORD32 a) {
  WORD32 abs_val;
  asm("CMP %w[a], #0\n"
      "NEG %w[abs_val], %w[a]\n"
      "CSEL %w[abs_val], %w[abs_val], %w[a], LT\n"
      : [abs_val] "=r"(abs_val), [a] "+r"(a)
      :
      : "cc");
  return (abs_val);
}

// returns the absolute value of 32-bit number
static PLATFORM_INLINE WORD32 ixheaacd_abs32_nrm(WORD32 a) {
  WORD32 abs_val, temp;
  asm("ASR %w[temp], %w[a], #31\n"
      "EOR  %w[abs_val], %w[a], %w[temp]\n"
      : [abs_val] "=r"(abs_val), [temp] "+r"(temp)
      : [a] "r"(a)
      : "cc");
  return abs_val;
}

#if 0
//returns the absolute value of 32-bit number with saturation
static PLATFORM_INLINE WORD32 ixheaacd_abs32_sat(WORD32 a)
{
    WORD32 abs_val,temp;
    asm (
		"ADDS %w[abs_val], %w[a], #0\n"
		"NEG %w[temp], %w[abs_val]\n"
		"CSEL %w[abs_val], %w[temp], %w[a], MI\n"
		"CMP %w[abs_val], #0\n"
		"MOV %w[temp], #2147483647\n" 
		"CSEL %w[abs_val], %w[temp], %w[abs_val], LT\n"
		: [abs_val] "=r" (abs_val), [temp] "+r" (temp)
        : [a] "r" (a)
		: "cc"
	);
    return abs_val;
}

//returns the negated value of 32-bit number
static PLATFORM_INLINE WORD32 ixheaacd_negate32(WORD32 a)
{
    WORD32 neg_val;
    asm (
        "NEG %w[neg_val], %w[a]\n"
		: [neg_val] "=r" (neg_val)
		: [a] "r" (a)	
	);
    return neg_val;
}

//returns the negated value of 32-bit number with saturation
static PLATFORM_INLINE WORD32 ixheaacd_negate32_sat(WORD32 a)
{
    WORD32 neg_val,temp;
    asm (
		"NEGS %w[neg_val], %w[a]\n"
		"MOV %w[temp], #0x7FFFFFFF\n"
		"CSEL %w[neg_val], %w[temp], %w[neg_val], VS\n"
		: [neg_val] "=r" (neg_val), [temp] "+r" (temp)
        : [a] "r" (a)
		:"cc"
	);
    return neg_val;
}
#else

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

  neg_val = -a;
  if (a == MIN_32) {
    neg_val = MAX_32;
  }

  return neg_val;
}
#endif

// divides 2 32 bit variables and returns the quotient
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

// multiplies two 16 bit numbers and accumulates their result in a 32 bit
// variable
static PLATFORM_INLINE WORD32 ixheaacd_mac16x16in32(WORD32 a, WORD16 b,
                                                    WORD16 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32(b, c);

  acc = ixheaacd_add32(a, acc);

  return acc;
}

// multiplies lower 16 bit of one data with upper 16 bit of
// other and accumulates their result in a 32 bit variable
static PLATFORM_INLINE WORD32 mac16x16hin32(WORD32 a, WORD32 b, WORD32 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32((WORD16)b, (WORD16)(c >> 16));

  acc = ixheaacd_add32(a, acc);

  return acc;
}

// multiplies two 16 bit numbers and accumulates their result in a 32 bit
// variable
static PLATFORM_INLINE WORD32 ixheaacd_mac16x16in32_shl(WORD32 a, WORD16 b,
                                                        WORD16 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32_shl(b, c);

  acc = ixheaacd_add32(a, acc);

  return acc;
}

// multiplies two 16 bit numbers and accumulates their
// result in a 32 bit variable with saturation

static PLATFORM_INLINE WORD32 ixheaacd_mac16x16in32_shl_sat(WORD32 a, WORD16 b,
                                                            WORD16 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32_shl_sat(b, c);

  acc = ixheaacd_add32_sat(a, acc);

  return acc;
}

// multiplies two 16 bit numbers and subtracts their
// result from a 32 bit variable
static PLATFORM_INLINE WORD32 msu16x16in32(WORD32 a, WORD16 b, WORD16 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32(b, c);

  acc = ixheaacd_sub32(a, acc);

  return acc;
}

// multiplies two 16 bit numbers and subtracts their
// result from a 32 bit variable after removing a redundant sign bit in the
// product
static PLATFORM_INLINE WORD32 msu16x16in32_shl(WORD32 a, WORD16 b, WORD16 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32_shl(b, c);

  acc = ixheaacd_sub32(a, acc);

  return acc;
}

// multiplies two 16 bit numbers and subtracts their
// result from a 32 bit variable with saturation
// after removing a redundant sign bit in the product
static PLATFORM_INLINE WORD32 msu16x16in32_shl_sat(WORD32 a, WORD16 b,
                                                   WORD16 c) {
  WORD32 acc;

  acc = ixheaacd_mult16x16in32_shl_sat(b, c);

  acc = ixheaacd_sub32_sat(a, acc);

  return acc;
}

// adding two 32 bit numbers and taking care of overflow
// by downshifting both numbers before addition
static PLATFORM_INLINE WORD32 add32_shr(WORD32 a, WORD32 b) {
  WORD32 sum;

  a = ixheaacd_shr32(a, 1);
  b = ixheaacd_shr32(b, 1);

  sum = ixheaacd_add32(a, b);

  return sum;
}

// subtracting two 32 bit numbers and taking care of
// overflow by downshifting both numbers before addition

static PLATFORM_INLINE WORD32 sub32_shr(WORD32 a, WORD32 b) {
  WORD32 diff;

  a = ixheaacd_shr32(a, 1);
  b = ixheaacd_shr32(b, 1);

  diff = ixheaacd_sub32(a, b);

  return diff;
}
#endif
