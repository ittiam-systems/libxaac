/******************************************************************************
 *
 * Copyright (C) 2015 The Android Open Source Project
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
#include <stdio.h>
#include <math.h>

#include "impd_type_def.h"
#include "impd_drc_uni_tables.h"
#include "impd_drc_uni_common.h"
#include "impd_drc_struct.h"
#include "impd_drc_filter_bank.h"
#include "impd_drc_rom.h"

WORD32 impd_init_tbls(const WORD32 num_gain_max_values,
                      ia_tables_struct* str_tables) {
  impd_gen_delta_time_code_tbl(num_gain_max_values,
                               str_tables->delta_time_code_table);
  return (0);
}

void impd_get_slope_code_tbl_and_size(
    ia_slope_code_table_struct const** slope_code_tbl_entry,
    WORD32* num_slope_code_tbl_entries) {
  *slope_code_tbl_entry = &(slopeCodeTableEntryBySize[0]);
  *num_slope_code_tbl_entries = kNumSlopeValuesTable;
}

void impd_get_delta_gain_code_tbl(
    const WORD32 gain_coding_profile,
    ia_delta_gain_code_table_struct const** delta_time_code_tbl,
    WORD32* num_entries) {
  if (gain_coding_profile == GAIN_CODING_PROFILE_CLIPPING) {
    *delta_time_code_tbl = deltaGainCodeTableProfile2BySize;
    *num_entries = kNumDeltaGainValuesTableProfile2;
  } else {
    *delta_time_code_tbl = deltaGainCodeTableBySize;
    *num_entries = kNumDeltaGainValuesTable;
  }
}

void impd_gen_delta_time_code_tbl(
    const WORD32 num_gain_max_values,
    ia_delta_time_code_table_entry_struct* delta_time_code_tbl_item) {
  WORD32 n, k;

  WORD32 Z = 1;
  while ((1 << Z) < 2 * num_gain_max_values) {
    Z++;
  }

  delta_time_code_tbl_item[0].size = -1;
  delta_time_code_tbl_item[0].code = -1;
  delta_time_code_tbl_item[0].value = -1;

  delta_time_code_tbl_item[1].size = 2;
  delta_time_code_tbl_item[1].code = 0x0;
  delta_time_code_tbl_item[1].value = 1;
  for (n = 0; n < 4; n++) {
    delta_time_code_tbl_item[n + 2].size = 4;
    delta_time_code_tbl_item[n + 2].code = 0x4 + n;
    delta_time_code_tbl_item[n + 2].value = n + 2;
  }
  for (n = 0; n < 8; n++) {
    delta_time_code_tbl_item[n + 6].size = 5;
    delta_time_code_tbl_item[n + 6].code = 0x10 + n;
    delta_time_code_tbl_item[n + 6].value = n + 6;
  }

  k = 2 * num_gain_max_values - 14 + 1;
  for (n = 0; n < k; n++) {
    delta_time_code_tbl_item[n + 14].size = 2 + Z;
    delta_time_code_tbl_item[n + 14].code = (0x3 << Z) + n;
    delta_time_code_tbl_item[n + 14].value = n + 14;
  }
}

WORD32
impd_get_delta_tmin(const WORD32 sampling_rate) {
  WORD32 lowerBound = (WORD32)(0.5f + 0.0005f * sampling_rate);
  WORD32 result = 1;
  if (sampling_rate < 1000) {
    return (UNEXPECTED_ERROR);
  }
  while (result <= lowerBound) result = result << 1;
  return result;
}
