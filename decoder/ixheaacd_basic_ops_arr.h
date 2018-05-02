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
#ifndef IXHEAACD_BASIC_OPS_ARR_H
#define IXHEAACD_BASIC_OPS_ARR_H

static PLATFORM_INLINE WORD16 norm32_arr(WORD32 *word32_arr, WORD32 n) {
  WORD32 i;
  WORD32 max_bits = 0;

  for (i = 0; i < n; i++) {
    max_bits = max_bits | ixheaacd_abs32_sat(word32_arr[i]);
  }

  return (ixheaacd_norm32(max_bits));
}

static PLATFORM_INLINE WORD16 norm16_arr(WORD16 *word16_arr, WORD32 n) {
  WORD32 i;
  WORD16 max_bits = 0;

  for (i = 0; i < n; i++) {
    max_bits = max_bits | ixheaacd_abs16_sat(word16_arr[i]);
  }

  return (norm16(max_bits));
}

static PLATFORM_INLINE VOID shl32_arr(WORD32 *word32_arr, WORD16 shift,
                                      WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word32_arr[i] = ixheaacd_shl32(word32_arr[i], shift);
  }

  return;
}

static PLATFORM_INLINE VOID ixheaacd_shr32_arr(WORD32 *word32_arr, WORD16 shift,
                                               WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    *word32_arr = ixheaacd_shr32(*word32_arr, shift);
    word32_arr++;
  }

  return;
}

static PLATFORM_INLINE VOID shl32_arr_dir(WORD32 *word32_arr, WORD16 shift,
                                          WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word32_arr[i] = ixheaacd_shl32_dir(word32_arr[i], shift);
  }

  return;
}

static PLATFORM_INLINE VOID shr32_arr_dir(WORD32 *word32_arr, WORD16 shift,
                                          WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word32_arr[i] = ixheaacd_shr32_dir(word32_arr[i], shift);
  }

  return;
}

static PLATFORM_INLINE VOID shl32_arr_dir_sat(WORD32 *word32_arr, WORD16 shift,
                                              WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word32_arr[i] = ixheaacd_shl32_dir_sat(word32_arr[i], shift);
  }

  return;
}

static PLATFORM_INLINE VOID shr32_arr_dir_sat(WORD32 *word32_arr, WORD16 shift,
                                              WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word32_arr[i] = shr32_dir_sat(word32_arr[i], shift);
  }

  return;
}

static PLATFORM_INLINE VOID ixheaacd_shl32_arr_sat(WORD32 *word32_arr,
                                                   WORD16 shift, WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    *word32_arr = ixheaacd_shl32_sat(*word32_arr, shift);
    word32_arr++;
  }

  return;
}

static PLATFORM_INLINE VOID shl16_arr(WORD16 *word16_arr, WORD16 shift,
                                      WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word16_arr[i] = ixheaacd_shl16(word16_arr[i], shift);
  }

  return;
}

static PLATFORM_INLINE VOID ixheaacd_shr16_arr(WORD16 *word16_arr, WORD16 shift,
                                               WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    *word16_arr = ixheaacd_shr16(*word16_arr, shift);
    word16_arr++;
  }

  return;
}

static PLATFORM_INLINE VOID shl16_arr_dir(WORD16 *word16_arr, WORD16 shift,
                                          WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word16_arr[i] = shl16_dir(word16_arr[i], shift);
  }

  return;
}

static PLATFORM_INLINE VOID shr16_arr_dir(WORD16 *word16_arr, WORD16 shift,
                                          WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word16_arr[i] = shr16_dir(word16_arr[i], shift);
  }

  return;
}

static PLATFORM_INLINE VOID shl16_arr_dir_sat(WORD16 *word16_arr, WORD16 shift,
                                              WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word16_arr[i] = shl16_dir_sat(word16_arr[i], shift);
  }

  return;
}

static PLATFORM_INLINE VOID shr16_arr_dir_sat(WORD16 *word16_arr, WORD16 shift,
                                              WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word16_arr[i] = ixheaacd_shr16_dir_sat(word16_arr[i], shift);
  }

  return;
}

static PLATFORM_INLINE VOID shl16_arr_sat(WORD16 *word16_arr, WORD16 shift,
                                          WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word16_arr[i] = ixheaacd_shl16_sat(word16_arr[i], shift);
  }

  return;
}

static PLATFORM_INLINE VOID shl3216_arr(WORD32 *word32_arr, WORD16 *word16_arr,
                                        WORD16 shift, WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word16_arr[i] = (WORD16)ixheaacd_shl32_dir(word32_arr[i], shift);
  }

  return;
}

static PLATFORM_INLINE WORD32 max32_arr(WORD32 *word32_arr, WORD32 n) {
  WORD32 i;

  WORD32 max_value;

  max_value = word32_arr[0];

  for (i = 1; i < n; i++) {
    max_value = ixheaacd_max32(max_value, word32_arr[i]);
  }

  return max_value;
}

static PLATFORM_INLINE WORD32 min32_arr(WORD32 *word32_arr, WORD32 n) {
  WORD32 i;

  WORD32 min_value;

  min_value = word32_arr[0];

  for (i = 1; i < n; i++) {
    min_value = ixheaacd_min32(min_value, word32_arr[i]);
  }

  return min_value;
}

static PLATFORM_INLINE WORD16 max16_arr(WORD16 *word16_arr, WORD32 n) {
  WORD32 i;

  WORD16 max_value;

  max_value = word16_arr[0];

  for (i = 1; i < n; i++) {
    max_value = ixheaacd_max16(max_value, word16_arr[i]);
  }

  return max_value;
}

static PLATFORM_INLINE WORD16 min16_arr(WORD16 *word16_arr, WORD32 n) {
  WORD32 i;

  WORD16 min_value;

  min_value = word16_arr[0];

  for (i = 1; i < n; i++) {
    min_value = ixheaacd_min16(min_value, word16_arr[i]);
  }

  return min_value;
}

static PLATFORM_INLINE VOID copy8(WORD8 *src, WORD8 *dst, WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    dst[i] = src[i];
  }

  return;
}

static PLATFORM_INLINE VOID copy16(WORD16 *src, WORD16 *dst, WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    dst[i] = src[i];
  }

  return;
}

static PLATFORM_INLINE VOID copy32(WORD32 *src, WORD32 *dst, WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    dst[i] = src[i];
  }

  return;
}

static PLATFORM_INLINE VOID delay8(WORD8 *word8_arr, WORD32 delay, WORD32 n) {
  WORD32 source_index;
  WORD32 destination_index;

  source_index = (n - 1) - delay;
  destination_index = n - 1;

  for (; source_index >= 0; source_index--, destination_index--) {
    word8_arr[destination_index] = word8_arr[source_index];
  }

  return;
}

static PLATFORM_INLINE VOID delay16(WORD16 *word16_arr, WORD32 delay,
                                    WORD32 n) {
  WORD32 source_index;
  WORD32 destination_index;

  source_index = (n - 1) - delay;
  destination_index = n - 1;

  for (; source_index >= 0; source_index--, destination_index--) {
    word16_arr[destination_index] = word16_arr[source_index];
  }

  return;
}

static PLATFORM_INLINE VOID delay32(WORD32 *word32_arr, WORD32 delay,
                                    WORD32 n) {
  WORD32 source_index;
  WORD32 destination_index;

  source_index = (n - 1) - delay;
  destination_index = n - 1;

  for (; source_index >= 0; source_index--, destination_index--) {
    word32_arr[destination_index] = word32_arr[source_index];
  }

  return;
}

static PLATFORM_INLINE VOID copy_reverse16(WORD16 *src, WORD16 *dst, WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    *dst++ = *src--;
  }

  return;
}

static PLATFORM_INLINE VOID copy_reverse32(WORD32 *src, WORD32 *dst, WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    *dst++ = *src--;
  }

  return;
}

static PLATFORM_INLINE VOID set_val8(WORD8 *word8_arr, WORD8 set_val,
                                     WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word8_arr[i] = set_val;
  }

  return;
}

static PLATFORM_INLINE VOID set_val16(WORD16 *word16_arr, WORD16 set_val,
                                      WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word16_arr[i] = set_val;
  }

  return;
}

static PLATFORM_INLINE VOID set_val32(WORD32 *word32_arr, WORD32 set_val,
                                      WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word32_arr[i] = set_val;
  }

  return;
}

static PLATFORM_INLINE VOID set_zero8(WORD8 *word8_arr, WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word8_arr[i] = 0;
  }

  return;
}

static PLATFORM_INLINE VOID set_zero16(WORD16 *word16_arr, WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word16_arr[i] = 0;
  }

  return;
}

static PLATFORM_INLINE VOID set_zero32(WORD32 *word32_arr, WORD32 n) {
  WORD32 i;

  for (i = 0; i < n; i++) {
    word32_arr[i] = 0;
  }

  return;
}
#endif
