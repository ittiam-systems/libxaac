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
#ifndef IXHEAACD_BASIC_OP_H
#define IXHEAACD_BASIC_OP_H

#define add_d(a, b) ((a) + (b))
#define sub_d(a, b) ((a) - (b))
#define ixheaacd_cbrt_calc(a) cbrt(1.0f / a)

static PLATFORM_INLINE WORD32 msu32x16in32_dual(WORD32 a, WORD16 c1, WORD32 b,
                                                WORD16 c2) {
  WORD32 result;
  WORD32 temp_result;
  UWORD32 a_lsb;
  WORD32 a_msb;
  UWORD32 b_lsb;
  WORD32 b_msb;

  a_lsb = a & 65535;
  a_msb = a >> 16;

  b_lsb = b & 65535;
  b_msb = b >> 16;
  temp_result = ((UWORD32)a_lsb * (UWORD32)c1);
  temp_result = temp_result - (UWORD32)b_lsb * (UWORD32)c2;
  temp_result = ((WORD32)temp_result) >> 16;
  result = temp_result + ((a_msb * (WORD32)c1) - (b_msb * (WORD32)c2));

  return (result);
}

static PLATFORM_INLINE WORD32 mac32x16in32_dual(WORD32 a, WORD16 c1, WORD32 b,
                                                WORD16 c2) {
  WORD32 result;
  WORD32 temp_result;
  UWORD32 a_lsb;
  WORD32 a_msb;
  UWORD32 b_lsb;
  WORD32 b_msb;

  a_lsb = a & 65535;
  a_msb = a >> 16;

  b_lsb = b & 65535;
  b_msb = b >> 16;
  temp_result = (UWORD32)a_lsb * (UWORD32)c1;
  temp_result = temp_result + (UWORD32)b_lsb * (UWORD32)c2;
  temp_result = ((UWORD32)temp_result) >> 16;
  result = temp_result + ((a_msb * (WORD32)c1)) + ((b_msb * (WORD32)c2));
  return (result);
}

static PLATFORM_INLINE WORD64 mac32x32in64_dual(WORD32 a, WORD32 b, WORD64 c) {
  WORD64 result;
  WORD64 temp_result;

  temp_result = (WORD64)a * (WORD64)b;
  result = c + (temp_result);
  return (result);
}

#endif
