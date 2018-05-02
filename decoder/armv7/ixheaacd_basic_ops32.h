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

// returns the minima of 2 32 bit variables
static PLATFORM_INLINE WORD32 ixheaacd_min32(WORD32 a, WORD32 b) {
  WORD32 min_val;

  __asm__ __volatile__(
      "  		CMP		%1, 	%2 \n\t"
      "		ITE GT \n\t"
      "		MOVGT	%0, 	%2 \n\t"
      "		MOVLE	%0, 	%1 \n\t"
      : "=r"(min_val)
      : "r"(a), "r"(b)
      : "cc");

  return min_val;
}

// returns the maxima of 2 32 bit variables
static PLATFORM_INLINE WORD32 ixheaacd_max32(WORD32 a, WORD32 b) {
  WORD32 max_val;
  __asm__ __volatile__(
      "  		CMP		%1, 	%2 \n\t"
      "		MOVLE	%0, 	%2 \n\t"
      "		MOVGT	%0, 	%1 \n\t"
      : "=r"(max_val)
      : "r"(a), "r"(b)
      : "cc");

  return max_val;
}

// shifts a 32-bit value left by specificed bits
static PLATFORM_INLINE WORD32 ixheaacd_shl32(WORD32 a, WORD b) {
  WORD32 out_val;
  __asm__(

      "          MOV   %0,  %1,  LSL %2 \n\t"
      : "=r"(out_val)
      : "r"(a), "r"(b));

  return (out_val);
}

// shifts a 32-bit value right by specificed bits
static PLATFORM_INLINE WORD32 ixheaacd_shr32(WORD32 a, WORD b) {
  WORD32 out_val;
  __asm__(

      "          MOV   %0,  %1,  ASR %2 \n\t"
      : "=r"(out_val)
      : "r"(a), "r"(b));

  return out_val;
}

// shifts a 32-bit value left by specificed bits and saturates it to 32 bits
static PLATFORM_INLINE WORD32 ixheaacd_shl32_sat(WORD32 a, WORD b) {
  WORD32 out_val = a;
  // WORD32 dummy1=0/*,dummy2=0*/;

  __asm__ __volatile__(
      "			RSBS   r3,  %2,  #31 \n\t"
      "		    MOVS  r3,  %1,  ASR r3 \n\t"
      "			ITT LT \n\t"
      "		    CMNLT r3,  #1 \n\t"
      "		    MOVLT %0,  #0x80000000 \n\t"
      "			IT GT \n\t"
      "		    MOVGT %0,  #0x7fffffff \n\t"
      "			IT EQ \n\t"
      "           MOVEQ %0,  %1,  LSL %2 \n\t"
      : "=r"(out_val)
      : "r"(a), "r"(b)
      : "cc", "r3");

  return (out_val);
}

// shifts a 32-bit value left by specificed bits, shifts
// it right if specified no. of bits is negative

static PLATFORM_INLINE WORD32 ixheaacd_shl32_dir(WORD32 a, WORD b) {
  WORD32 out_val = 0;
  // WORD32	dummy=0;

  __asm__ __volatile__(

      "		RSBS    r3,  %2,  #0 \n\t"
      "		MOVMI   %0,  %1,  LSL %2 \n\t"
      "		MOVPL   %0,  %1,  ASR r3 \n\t"
      : "=r"(out_val)
      : "r"(a), "r"((WORD)b)
      : "cc", "r3");
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
  WORD32 out_val = 0;
  __asm__ __volatile__(
      "		RSBS    r3,  %2,  #0 \n\t"
      "		IT MI \n\t"
      "		MOVMI   %0,  %1,  ASR %2 \n\t"
      "		IT PL \n\t"
      "		MOVPL   %0,  %1,  LSL r3 \n\t"
      : "=r"(out_val)
      : "r"(a), "r"(b)
      : "cc", "r3");

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
  __asm__(

      "                SMULBB   %0 ,  %1,  %2 \n\t"
      : "=r"(product)
      : "r"(a), "r"(b));
  return product;
}

// multiplies two 16 bit numbers and returns their 32-bit
// result after removing 1 redundant sign bit
static PLATFORM_INLINE WORD32 ixheaacd_mult16x16in32_shl(WORD16 a, WORD16 b) {
  WORD32 product;
  __asm__(

      "                SMULBB   %0 ,  %1,  %2 \n\t"
      "				MOV		%0,	%0,	LSL #1 \n\t"
      : "=r"(product)
      : "r"(a), "r"(b));
  return product;
}

// multiplies two 16 bit numbers and returns their 32-bit
// result after removing 1 redundant sign bit with saturation
static PLATFORM_INLINE WORD32 ixheaacd_mult16x16in32_shl_sat(WORD16 a,
                                                             WORD16 b) {
  WORD32 product;
  __asm__(

      "                SMULBB   %0 ,  %1,  %2 \n\t"
      "				QADD		%0,	%0,	%0 \n\t"
      : "=r"(product)
      : "r"(a), "r"(b));
  return product;
}

// adds 2 32 bit variables
static PLATFORM_INLINE WORD32 ixheaacd_add32(WORD32 a, WORD32 b) {
  WORD32 sum;
  __asm__(

      "                ADD   %0 ,  %1,  %2 \n\t"
      : "=r"(sum)
      : "r"(a), "r"(b));
  return (sum);
}

// subtract 2 32 bit variables
static PLATFORM_INLINE WORD32 ixheaacd_sub32(WORD32 a, WORD32 b) {
  WORD32 diff;
  __asm__(

      "                SUB   %0 ,  %1,  %2 \n\t"
      : "=r"(diff)
      : "r"(a), "r"(b));
  return (diff);
}

// adds 2 32 bit variables with saturation
static PLATFORM_INLINE WORD32 ixheaacd_add32_sat(WORD32 a, WORD32 b) {
  WORD32 sum;
  __asm__(

      "                QADD   %0 ,  %1,  %2 \n\t"
      : "=r"(sum)
      : "r"(a), "r"(b));
  return (sum);
}

// subtract 2 32 bit variables
static PLATFORM_INLINE WORD32 ixheaacd_sub32_sat(WORD32 a, WORD32 b) {
  WORD32 diff;
  __asm__(

      "                QSUB   %0 ,  %1,  %2 \n\t"
      : "=r"(diff)
      : "r"(a), "r"(b));
  return (diff);
}

// returns number of redundant sign bits in a 32-bit value.
// return zero for a value of zero
static PLATFORM_INLINE WORD ixheaacd_norm32(WORD32 a) {
  WORD32 norm_val;
  __asm__(
      "		eor 	%0 , %1,   %1,asr #31 \n\t"
      "		CLZ 	%0,   %0 \n\t"
      "		SUB 	%0,   %0, #1 \n\t"
      : "=r"(norm_val)
      : "r"(a));
  return norm_val;
}

static PLATFORM_INLINE WORD ixheaacd_pnorm32(WORD32 a) {
  WORD32 norm_val;
  __asm__(

      "		CLZ 	%0,   %1 \n\t"
      "		SUB 	%0,   %0, #1 \n\t"
      : "=r"(norm_val)
      : "r"(a));
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

  abs_val = a;

  if (a < 0) {
    abs_val = -a;
  }

  return abs_val;
}

// returns the absolute value of 32-bit number
static PLATFORM_INLINE WORD32 ixheaacd_abs32_nrm(WORD32 a) {
  WORD32 abs_val;

  abs_val = a;

  if (a < 0) {
    abs_val = ~a;
  }

  return abs_val;
}

// returns the absolute value of 32-bit number with saturation
static PLATFORM_INLINE WORD32 ixheaacd_abs32_sat(WORD32 a) {
  WORD32 abs_val;
  __asm__ __volatile__(

      "		MOVS 	%0 ,  %1  \n\t"
      "		IT MI \n\t"
      "		RSBSMI 	%0 ,  %1 ,  #0  \n\t"
      "		IT MI \n\t"
      "       MOVMI 	%0 ,  #0x7fffffff  \n\t"
      : "=r"(abs_val)
      : "r"(a)
      : "cc");

  return abs_val;
}

// returns the negated value of 32-bit number
static PLATFORM_INLINE WORD32 ixheaacd_negate32(WORD32 a) {
  WORD32 neg_val;
  __asm__("        RSB %0, %1, #0 \n\t" : "=r"(neg_val) : "r"(a));
  return neg_val;
}

// returns the negated value of 32-bit number with saturation
static PLATFORM_INLINE WORD32 ixheaacd_negate32_sat(WORD32 a) {
  WORD32 neg_val;
  __asm__(
      "       RSBS %0, %1, #0 \n\t"
      "		IT VS \n\t"
      "		MVNVS %0, #0x80000000 \n\t"
      : "=r"(neg_val)
      : "r"(a)
      : "cc");
  return neg_val;
}

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
  __asm__(

      "		SMLABB  %0, %2, %3, %1 \n\t"
      : "=r"(acc)
      : "r"(a), "r"(b), "r"(c)

          );

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
