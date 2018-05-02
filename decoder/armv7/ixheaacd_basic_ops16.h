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
#ifndef IXHEAACD_BASIC_OPS16_H
#define IXHEAACD_BASIC_OPS16_H

static PLATFORM_INLINE WORD16 ixheaacd_sat16(WORD32 op1) {
  WORD32 var_out;
  __asm__(
      "   MOV %0, %1  \n\t"
      "	CMP  %0, #0x8000 \n\t"
      "	ITEE GE \n\t"
      "	MVNGE  %0, #0x8000 \n\t"
      "	CMNLT %0, #0x00008000 \n\t"
      "	MOVLT  %0, #0x00008000 \n\t"
      : "=r"(var_out)
      : "r"(op1)
      : "cc");
  return ((WORD16)var_out);
}

// add 2 16 bit variables and returns 16 bit result
static PLATFORM_INLINE WORD16 ixheaacd_add16(WORD16 op1, WORD16 op2) {
  WORD16 var_out;

  var_out = ((WORD16)(op1 + op2));
  return (var_out);
}

// add 2 16 bit variables and returns 16 bit result with saturation
static PLATFORM_INLINE WORD16 ixheaacd_add16_sat(WORD16 op1, WORD16 op2) {
  WORD16 var_out;
  WORD32 sum;

  sum = (WORD32)op1 + (WORD32)op2;
  var_out = ixheaacd_sat16(sum);
  return (var_out);
}

// subtract 2 16 bit variables and returns 16 bit result
static PLATFORM_INLINE WORD16 ixheaacd_sub16(WORD16 op1, WORD16 op2) {
  WORD16 var_out;

  var_out = ((WORD16)(op1 - op2));
  return (var_out);
}

// subtract 2 16 bit variables and returns 16 bit result with saturation
static PLATFORM_INLINE WORD16 ixheaacd_sub16_sat(WORD16 op1, WORD16 op2) {
  WORD16 var_out;
  WORD32 diff;

  diff = (WORD32)op1 - op2;
  var_out = ixheaacd_sat16(diff);
  return (var_out);
}

// multiply 2 16 bit variables and return 31 to 16 bits
static PLATFORM_INLINE WORD16 ixheaacd_mult16(WORD16 op1, WORD16 op2) {
  WORD16 var_out;

  var_out = ((WORD16)(((WORD32)op1 * (WORD32)op2) >> 16));
  return (var_out);
}

// multiply 2 16 bit variables and return 30 to 15 bits
static PLATFORM_INLINE WORD16 ixheaacd_mult16_shl(WORD16 op1, WORD16 op2) {
  WORD16 var_out;

  var_out = ((WORD16)(((WORD32)op1 * (WORD32)op2) >> 15));
  return (var_out);
}

// multiply 2 16 bit variables and return 30 to 15 bits with saturation
static PLATFORM_INLINE WORD16 ixheaacd_mult16_shl_sat(WORD16 op1, WORD16 op2) {
  WORD32 temp;
  __asm__(
      "        SMULBB  %0, %1, %2 \n\t"
      "        MOV %0, %0, ASR #15 \n\t"
      "        CMP  %0, #0x00008000 \n\t"
      "		 IT 	EQ \n\t"
      "	     SUBEQ  %0, %0,	#1 \n\t"
      : "=r"(temp)
      : "r"(op1), "r"(op2)
      : "cc");
  return (temp);
}

// shifts left a 16 bit variable by the shift value and returns a 16 bit result
static PLATFORM_INLINE WORD16 ixheaacd_shl16(WORD16 op1, WORD16 shift) {
  WORD16 var_out;

  var_out = (WORD16)(op1 << shift);
  return (var_out);
}

// shifts left a 16 bit variable by the shift value and returns a 16 bit value
// with saturation
static PLATFORM_INLINE WORD16 ixheaacd_shl16_sat(WORD16 op1, WORD16 shift) {
  WORD16 var_out;
  WORD32 temp;

  if (shift > 15) {
    shift = 15;
  }
  temp = (WORD32)(op1 << shift);
  var_out = ixheaacd_sat16(temp);
  return (var_out);
}

// shifts right a 16 bit variable by the shift value and returns a 16 bit value
static PLATFORM_INLINE WORD16 ixheaacd_shr16(WORD16 op1, WORD16 shift) {
  WORD16 var_out;

  var_out = ((WORD16)(op1 >> shift));
  return (var_out);
}

// shifts left a 16 bit variable by the shift value
// if the value is positive else shifts right and returns a 16 bit

static PLATFORM_INLINE WORD16 shl16_dir(WORD16 op1, WORD16 shift) {
  WORD16 var_out;
  if (shift > 0) {
    var_out = ixheaacd_shl16(op1, shift);
  } else {
    var_out = ixheaacd_shr16(op1, (WORD16)(-shift));
  }
  return (var_out);
}

// shifts left a 16 bit variable by the shift value
// if the value is negative else shifts right and returns a 16 bit
static PLATFORM_INLINE WORD16 shr16_dir(WORD16 op1, WORD16 shift) {
  WORD16 var_out;

  if (shift < 0) {
    var_out = ixheaacd_shl16(op1, (WORD16)(-shift));
  } else {
    var_out = ixheaacd_shr16(op1, shift);
  }
  return (var_out);
}

// shifts left a 16 bit variable by the shift value
// if the value is positive else shifts right and returns a 16 bit with
// saturation
static PLATFORM_INLINE WORD16 shl16_dir_sat(WORD16 op1, WORD16 shift) {
  WORD16 var_out;
  if (shift > 0) {
    var_out = ixheaacd_shl16_sat(op1, shift);
  } else {
    var_out = ixheaacd_shr16(op1, (WORD16)(-shift));
  }
  return (var_out);
}

// shifts left a 16 bit variable by the shift value
// if the value is negative else shifts right and returns a 16 bit
static PLATFORM_INLINE WORD16 ixheaacd_shr16_dir_sat(WORD16 op1, WORD16 shift) {
  WORD16 var_out;

  if (shift < 0) {
    var_out = ixheaacd_shl16_sat(op1, (WORD16)(-shift));
  } else {
    var_out = ixheaacd_shr16(op1, shift);
  }
  return (var_out);
}

// finds a value which normalizes the input to 16 bit

static PLATFORM_INLINE WORD16 norm16(WORD16 var1) {
  WORD16 var_out;

  __asm__(
      "MOVS %1, %1,  LSL #16 \n\t"
      "MVNMIS %1, %1 \n\t"
      "MOVEQ %0, #16 \n\t"
      " CLZGT %0, %1 \n\t"
      "SUB %0, %0, #1 \n\t"
      : "=r"(var_out)
      : "r"(var1)
      : "cc");

  return (var_out);
}

// finds no. of significant bits excluding sign bit
// value 15 returned for zero
static PLATFORM_INLINE WORD16 bin_expo16(WORD16 op1) {
  WORD16 var_out;

  var_out = ((WORD16)(15 - norm16(op1)));
  return (var_out);
}

// returns a 16 bit absolute value of a given signed 16 bit value
static PLATFORM_INLINE WORD16 ixheaacd_abs16(WORD16 var1) {
  WORD16 var_out;
  __asm__(
      "	MOVS %0,  %1, LSL #16   \n\t"
      "	RSBLTS  %0 , %0, #0 \n\t"
      "	MOV %0, %0, ASR #16  \n\t"
      : "=r"(var_out)
      : "r"(var1)
      : "cc");
  return (var_out);
}

// returns a 16 bit absolute value of a given signed 16 bit value with
// saturation
static PLATFORM_INLINE WORD16 ixheaacd_abs16_sat(WORD16 var1) {
  WORD16 var_out;
  __asm__(
      "	MOVS %0,  %1, LSL #16   \n\t"
      "	RSBLTS  %0 , %0, #0 \n\t"
      "	MOVMI %0,  #0x7fffffff \n\t"
      "	MOV %0, %0, ASR #16  \n\t"
      : "=r"(var_out)
      : "r"(var1)
      : "cc");
  return (var_out);
}

// returns a 16 bit negative value of a given signed 16 bit value.
static PLATFORM_INLINE WORD16 ixheaacd_negate16(WORD16 op1) {
  WORD16 var_out;

  if (-32768 == op1) {
    var_out = MAX_16;
  } else {
    var_out = (WORD16)(-op1);
  }
  return (var_out);
}

// returns the minima of 2 16 bit variables
static PLATFORM_INLINE WORD16 ixheaacd_min16(WORD16 op1, WORD16 op2) {
  WORD16 var_out;

  var_out = op1 < op2 ? op1 : op2;
  return (var_out);
}

// returns the maxima of 2 16 bit variables
static PLATFORM_INLINE WORD16 ixheaacd_max16(WORD16 op1, WORD16 op2) {
  WORD16 var_out;

  var_out = op1 > op2 ? op1 : op2;
  return (var_out);
}

/*****************************************************************************/
/*                                                                           */
/*  function name : div16                                                    */
/*                                                                           */
/*  description   : divides 2 16 bit variables and returns the quotient      */
/*                  the q-format of the result is modified                   */
/*                  ( op1/op2  to 14 bits precision)                         */
/*                                                                           */
/*  inputs        : WORD16 op1, WORD16 op2, WORD16 *q_format                 */
/*                                                                           */
/*  globals       : none                                                     */
/*                                                                           */
/*  processing    : non-restoration type algorithm(shift & substract)        */
/*                                                                           */
/*  outputs       : WORD16 *q_format                                         */
/*                                                                           */
/*  returns       : WORD16 var_out                                           */
/*                                                                           */
/*  issues        : none                                                     */
/*                                                                           */
/*  revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       author                changes                     */
/*        11 11 2003       preethi               modified(bug fixes)         */
/*        15 11 2004       tejaswi/vishal        modified(bug fixes/cleanup) */
/*                                                                           */
/*****************************************************************************/

// divides 2 16 bit variables and returns the quotient
static PLATFORM_INLINE WORD16 div16(WORD16 op1, WORD16 op2, WORD16 *q_format) {
  WORD32 quotient;
  UWORD16 mantissa_nr, mantissa_dr;
  WORD16 sign = 0;

  LOOPIDX i;
  WORD16 q_nr, q_dr;

  mantissa_nr = op1;
  mantissa_dr = op2;
  quotient = 0;

  if (op1 < 0 && op2 != 0) {
    op1 = -op1;
    sign = (WORD16)(sign ^ -1);
  }

  if (op2 < 0) {
    op2 = -op2;
    sign = (WORD16)(sign ^ -1);
  }

  if (op2 == 0) {
    *q_format = 0;
    return (op1);
  }

  quotient = 0;

  q_nr = norm16(op1);
  mantissa_nr = (UWORD16)op1 << (q_nr);
  q_dr = norm16(op2);
  mantissa_dr = (UWORD16)op2 << (q_dr);
  *q_format = (WORD16)(14 + q_nr - q_dr);

  for (i = 0; i < 15; i++) {
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

  return (WORD16)quotient;
}

// multiply 2 16 bit variables, add 31 to 16 bits to acc
static PLATFORM_INLINE WORD16 mac16(WORD16 c, WORD16 op1, WORD16 op2) {
  WORD16 var_out;

  var_out = ixheaacd_mult16(op1, op2);
  var_out = ixheaacd_add16(c, var_out);
  return (var_out);
}

// multiply 2 16 bit variables, add 31 to 16 bits to acc with saturation
static PLATFORM_INLINE WORD16 mac16_sat(WORD16 c, WORD16 op1, WORD16 op2) {
  WORD16 var_out;

  var_out = ixheaacd_mult16(op1, op2);
  var_out = ixheaacd_add16_sat(c, var_out);
  return (var_out);
}

// multiply 2 16 bit variables, add 30 to 15 bits to acc
static PLATFORM_INLINE WORD16 mac16_shl(WORD16 c, WORD16 op1, WORD16 op2) {
  WORD16 var_out;

  var_out = ixheaacd_mult16_shl(op1, op2);
  var_out = ixheaacd_add16(c, var_out);
  return (var_out);
}

// multiply 2 16 bit variables, add 30 to 15 bits to acc with saturation
static PLATFORM_INLINE WORD16 mac16_shl_sat(WORD16 c, WORD16 op1, WORD16 op2) {
  WORD16 var_out;
  WORD32 temp;

  temp = ((WORD32)op1 * (WORD32)op2) >> 15;
  temp += c;
  var_out = ixheaacd_sat16(temp);
  return (var_out);
}

// rounds a 32 bit variable to a 16 bit variable with saturation
static PLATFORM_INLINE WORD16 ixheaacd_round16(WORD32 op1) {
  WORD16 var_out;
  __asm__(
      "        ADDS %0, %1, #0x8000 \n\t"
      "		 IT VS \n\t"
      "        MVNVS %0, #0x80000000 \n\t"
      "        MOV %0, %0, ASR #16 \n\t"
      : "=r"(var_out)
      : "r"(op1)
      : "cc");
  return (var_out);
}
#endif
