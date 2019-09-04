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
#include "ixheaacd_sbr_common.h"
#include <ixheaacd_type_def.h>

#include "ixheaacd_constants.h"
#include <ixheaacd_basic_ops32.h>
#include <ixheaacd_basic_ops16.h>
#include <ixheaacd_basic_ops40.h>
#include "ixheaacd_basic_ops.h"

#include "ixheaacd_defines.h"
#include "ixheaacd_bitbuffer.h"

#include "ixheaacd_error_codes.h"
#include <ixheaacd_aac_rom.h>
#include "ixheaacd_pulsedata.h"

#include "ixheaacd_pns.h"
#include "ixheaacd_drc_data_struct.h"

#include "ixheaacd_lt_predict.h"

#include "ixheaacd_channelinfo.h"
#include "ixheaacd_drc_dec.h"
#include "ixheaacd_sbrdecoder.h"
#include "ixheaacd_tns.h"
#include "ixheaacd_intrinsics.h"

#include "ixheaacd_common_rom.h"
#include "ixheaacd_block.h"
#include "ixheaacd_channel.h"
#include "ixheaacd_audioobjtypes.h"
#include "ixheaacd_latmdemux.h"

#include "ixheaacd_aacdec.h"

static PLATFORM_INLINE WORD32 ixheaacd_mac32_tns_sat(WORD32 a, WORD32 b,
                                                     WORD32 c) {
  WORD32 result;
  WORD64 temp_result;

  temp_result = (WORD64)a * (WORD64)b;
  result = (WORD32)(temp_result >> 32);
  result = ixheaacd_add32_sat(c, result);
  return (result);
}

static PLATFORM_INLINE WORD64 mac32x32in64_dual(WORD32 a, WORD32 b, WORD64 c) {
  WORD64 result;
  WORD64 temp_result;

  temp_result = (WORD64)a * (WORD64)b;
  result = c + (temp_result);
  return (result);
}

VOID ixheaacd_tns_decode_coefficients(
    const ia_filter_info_struct *filter, WORD32 *a,
    ia_aac_dec_tables_struct *ptr_aac_tables) {
  WORD32 i;
  WORD32 tmp;
  WORD32 *aptr = a;
  WORD32 *tns_coeff_ptr;
  WORD8 ixheaacd_drc_offset;

  tmp = filter->resolution;
  if (tmp == 0) {
    tns_coeff_ptr = ptr_aac_tables->pstr_block_tables->tns_coeff3;
    ixheaacd_drc_offset = 4;

  } else {
    tns_coeff_ptr = ptr_aac_tables->pstr_block_tables->tns_coeff4;
    ixheaacd_drc_offset = 8;
  }

  for (i = 0; i < filter->order; i++) {
    *aptr++ = tns_coeff_ptr[filter->coef[i] + ixheaacd_drc_offset];
  }
}

VOID ixheaacd_tns_parcor_to_lpc(WORD32 *parcor, WORD32 *lpc, WORD16 *scale,
                                WORD32 order)

{
  WORD i, j, status;
  WORD32 z1;
  WORD32 z[MAX_ORDER + 1];
  WORD32 w[MAX_ORDER + 1];
  WORD32 accu1, accu2;

  status = 1;
  *scale = 1;

  while (status) {
    status = 0;

    for (i = MAX_ORDER; i >= 0; i--) {
      z[i] = 0;
      w[i] = 0;
    }

    accu1 = (0x40000000 >> (*scale - 1));

    for (i = 0; i <= order; i++) {
      z1 = accu1;

      for (j = 0; j < order; j++) {
        w[j] = (accu1);

        accu1 = ixheaacd_add32_sat(accu1,
                                   ixheaacd_mult32_shl_sat(parcor[j], (z[j])));
        if (ixheaacd_abs32_sat(accu1) == 0x7fffffff) status = 1;
      }
      for (j = (order - 1); j >= 0; j--) {
        accu2 = (z[j]);
        accu2 = ixheaacd_add32_sat(accu2,
                                   ixheaacd_mult32_shl_sat(parcor[j], (w[j])));
        z[j + 1] = (accu2);
        if (ixheaacd_abs32_sat(accu2) == 0x7fffffff) status = 1;
      }

      z[0] = (z1);
      lpc[i] = (accu1);
      accu1 = 0;
    }

    accu1 = (status - 1);

    if (accu1 == 0) {
      *scale = *scale + 1;
    }
  }
}

VOID ixheaacd_tns_parcor_lpc_convert_dec(WORD16 *parcor, WORD16 *lpc,
                                         WORD16 *scale, WORD order)

{
  WORD i, j, status;
  WORD32 accu;
  WORD16 temp_buf1[MAX_ORDER + 1];
  WORD16 temp_buf2[MAX_ORDER + 1];
  WORD32 accu1, accu2;

  status = 1;
  *scale = 0;

  while (status) {
    status = 0;

    for (i = MAX_ORDER; i >= 0; i--) {
      temp_buf1[i] = 0;
      temp_buf2[i] = 0;
    }

    accu1 = (0x7fffffff >> *scale);

    for (i = 0; i <= order; i++) {
      accu = accu1;

      for (j = 0; j < order; j++) {
        temp_buf2[j] = ixheaacd_round16(accu1);
        accu1 = ixheaacd_mac16x16in32_shl_sat(accu1, parcor[j], temp_buf1[j]);

        if (ixheaacd_abs32_sat(accu1) == 0x7fffffff) {
          status = 1;
        }
      }

      for (j = (order - 1); j >= 0; j--) {
        accu2 = ixheaacd_deposit16h_in32(temp_buf1[j]);
        accu2 = ixheaacd_mac16x16in32_shl_sat(accu2, parcor[j], temp_buf2[j]);
        temp_buf1[j + 1] = ixheaacd_round16(accu2);
        if (ixheaacd_abs32_sat(accu2) == 0x7fffffff) {
          status = 1;
        }
      }

      temp_buf1[0] = ixheaacd_round16(accu);
      lpc[i] = ixheaacd_round16(accu1);
      accu1 = 0;
    }

    accu1 = (status - 1);

    if (accu1 == 0) {
      *scale = *scale + 1;
    }
  }
}

VOID ixheaacd_tns_ar_filter_fixed_dec(WORD32 *spectrum, WORD32 size, WORD32 inc,
                                      WORD32 *lpc, WORD32 order,
                                      WORD32 shift_value, WORD scale_spec)

{
  WORD32 i, j;
  WORD32 y, state[MAX_ORDER + 1];
  WORD32 acc;

  if ((order & 3) != 0) {
    for (i = order + 1; i < ((WORD32)(order & 0xfffffffc) + 4); i++) {
      lpc[i] = 0;
    }
    lpc[i] = 0;
    order = ((order & 0xfffffffc) + 4);
  }
  {
    WORD32 temp_lo = 0;
    for (i = 0; i < order; i++) {
      y = ixheaacd_shl32_sat((*spectrum), scale_spec);
      acc = 0;

      for (j = i; j > 0; j--) {
        acc = ixheaacd_mac32_tns_sat(state[j - 1], lpc[j], acc);
        state[j] = state[j - 1];
      }
      y = ixheaacd_sub32_sat(y, ixheaacd_shl32_sat(acc, 1));
      state[0] = ixheaacd_shl32_sat(y, shift_value);

      *spectrum = y >> scale_spec;
      spectrum += inc;
    }
    temp_lo = 0;
    for (i = order; i < size; i++) {
      y = ixheaacd_shl32_sat((*spectrum), scale_spec);
      acc = 0;
      for (j = order; j > 0; j--) {
        acc = ixheaacd_mac32_tns_sat(state[j - 1], lpc[j], acc);
        state[j] = state[j - 1];
      }
      y = ixheaacd_sub32_sat(y, ixheaacd_shl32_sat(acc, 1));
      state[0] = ixheaacd_shl32_sat(y, shift_value);

      *spectrum = y >> scale_spec;
      spectrum += inc;
    }
  }
}

VOID ixheaacd_tns_ar_filter_fixed_non_neon_armv7(WORD32 *spectrum, WORD32 size,
                                                 WORD32 inc, WORD32 *lpc,
                                                 WORD32 order,
                                                 WORD32 shift_value,
                                                 WORD scale_spec) {
  WORD32 i, j;
  WORD32 y, state[MAX_ORDER + 1];
  WORD32 acc;

  if ((order & 3) != 0) {
    for (i = order + 1; i < ((WORD32)(order & 0xfffffffc) + 4); i++) {
      lpc[i] = 0;
    }
    lpc[i] = 0;
    order = ((order & 0xfffffffc) + 4);
  }
  {
    WORD32 temp_lo = 0;
    for (i = 0; i < order; i++) {
      y = ixheaacd_shl32_sat((*spectrum), scale_spec);
      acc = 0;

      for (j = i; j > 0; j--) {
        acc = ixheaacd_mac32_tns_sat(state[j - 1], lpc[j], acc);
        state[j] = state[j - 1];
      }
      y = ixheaacd_sub32_sat(y, ixheaacd_shl32_sat(acc, 1));
      state[0] = ixheaacd_shl32_sat(y, shift_value);

      *spectrum = y >> scale_spec;
      spectrum += inc;
    }
    temp_lo = 0;
    for (i = order; i < size; i++) {
      WORD64 acc = 0;
      WORD32 acc1;
      y = ixheaacd_shl32_sat((*spectrum), scale_spec);
      for (j = order; j > 0; j--) {
        acc = mac32x32in64_dual(state[j - 1], lpc[j], acc);
        state[j] = state[j - 1];
      }
      acc1 = (WORD32)(acc >> 32);

      y = ixheaacd_sub32_sat(y, ixheaacd_shl32_sat(acc1, 1));
      state[0] = ixheaacd_shl32_sat(y, shift_value);

      *spectrum = y >> scale_spec;
      spectrum += inc;
    }
  }
}

VOID ixheaacd_tns_ar_filter_fixed_armv8(WORD32 *spectrum, WORD32 size,
                                        WORD32 inc, WORD32 *lpc, WORD32 order,
                                        WORD32 shift_value, WORD scale_spec) {
  WORD32 i, j;
  WORD32 y, state[MAX_ORDER + 1];
  WORD32 acc;

  if ((order & 3) != 0) {
    for (i = order + 1; i < ((WORD32)(order & 0xfffffffc) + 4); i++) {
      lpc[i] = 0;
    }
    lpc[i] = 0;
    order = ((order & 0xfffffffc) + 4);
  }
  {
    WORD32 temp_lo = 0;
    for (i = 0; i < order; i++) {
      y = ixheaacd_shl32_sat((*spectrum), scale_spec);
      acc = 0;

      for (j = i; j > 0; j--) {
        acc = ixheaacd_mac32_tns_sat(state[j - 1], lpc[j], acc);
        state[j] = state[j - 1];
      }
      y = ixheaacd_sub32_sat(y, ixheaacd_shl32_sat(acc, 1));
      state[0] = ixheaacd_shl32_sat(y, shift_value);

      *spectrum = y >> scale_spec;
      spectrum += inc;
    }
    temp_lo = 0;
    for (i = order; i < size; i++) {
      WORD64 acc = 0;
      WORD32 acc1;
      y = ixheaacd_shl32_sat((*spectrum), scale_spec);
      for (j = order; j > 0; j--) {
        acc = ixheaacd_mac32x32in64_dual(state[j - 1], lpc[j], acc);
        state[j] = state[j - 1];
      }
      acc1 = (WORD32)(acc >> 32);

      y = ixheaacd_sub32(y, ixheaacd_shl32_sat(acc1, 1));
      state[0] = ixheaacd_shl32_sat(y, shift_value);

      *spectrum = y >> scale_spec;
      spectrum += inc;
    }
  }
}

void ixheaacd_tns_ma_filter_fixed_ld(WORD32 *spectrum, WORD32 size, WORD32 inc,
                                     WORD32 *lpc, WORD32 order,
                                     WORD16 shift_value) {
  WORD32 i, j;
  WORD32 y, state[MAX_ORDER];

  for (i = 0; i < order; i++) state[i] = 0;

  for (i = 0; i < size; i++) {
    y = *spectrum;

    for (j = 0; j < order; j++) y += ixheaacd_mult32_shl(state[j], lpc[j + 1]);

    for (j = order - 1; j > 0; j--) state[j] = state[j - 1];

    state[0] = ixheaacd_shl32_dir_sat(*spectrum, shift_value);
    *spectrum = y;
    spectrum += inc;
  }
}

VOID ixheaacd_tns_ar_filter_dec(WORD32 *spectrum, WORD32 size, WORD32 inc,
                                WORD16 *lpc, WORD32 order, WORD32 shift_value,
                                WORD scale_spec, WORD32 *ptr_filter_state) {
  WORD32 i, j;
  WORD32 y;
  WORD32 acc;

  if ((order & 3) != 0) {
    for (i = order + 1; i < ((WORD32)(order & 0xfffffffc) + 4); i++) {
      lpc[i] = 0;
    }
    lpc[i] = 0;
    order = ((order & 0xfffffffc) + 4);
  }

  for (i = 0; i < order; i++) {
    y = ixheaacd_shl32_sat((*spectrum), scale_spec);
    acc = 0;

    for (j = i; j > 0; j--) {
      acc = ixheaacd_add32_sat(
          acc, ixheaacd_mult32x16in32(ptr_filter_state[j - 1], lpc[j]));
      ptr_filter_state[j] = ptr_filter_state[j - 1];
    }

    y = ixheaacd_sub32_sat(y, ixheaacd_shl32_sat(acc, 1));
    ptr_filter_state[0] = ixheaacd_shl32_sat(y, shift_value);
    *spectrum = y >> scale_spec;
    spectrum += inc;
  }

  for (i = order; i < size; i++) {
    y = ixheaacd_shl32_sat((*spectrum), scale_spec);
    acc = 0;
    for (j = order; j > 0; j--) {
      acc = ixheaacd_add32_sat(
          acc, ixheaacd_mult32x16in32(ptr_filter_state[j - 1], lpc[j]));
      ptr_filter_state[j] = ptr_filter_state[j - 1];
    }

    y = ixheaacd_sub32_sat(y, ixheaacd_shl32_sat(acc, 1));
    ptr_filter_state[0] = ixheaacd_shl32_sat(y, shift_value);
    *spectrum = y >> scale_spec;
    spectrum += inc;
  }
}

WORD32 ixheaacd_calc_max_spectral_line_dec(WORD32 *ptr_tmp, WORD32 size) {
  WORD32 max_spec_line = 0, i;
  WORD unroll_cnt, rem;

  unroll_cnt = size >> 3;
  for (i = unroll_cnt; i--;) {
    max_spec_line = ixheaacd_abs32_nrm(*ptr_tmp++) | max_spec_line;
    max_spec_line = ixheaacd_abs32_nrm(*ptr_tmp++) | max_spec_line;
    max_spec_line = ixheaacd_abs32_nrm(*ptr_tmp++) | max_spec_line;
    max_spec_line = ixheaacd_abs32_nrm(*ptr_tmp++) | max_spec_line;

    max_spec_line = ixheaacd_abs32_nrm(*ptr_tmp++) | max_spec_line;
    max_spec_line = ixheaacd_abs32_nrm(*ptr_tmp++) | max_spec_line;
    max_spec_line = ixheaacd_abs32_nrm(*ptr_tmp++) | max_spec_line;
    max_spec_line = ixheaacd_abs32_nrm(*ptr_tmp++) | max_spec_line;
  }

  rem = size - (unroll_cnt << 3);

  if (rem) {
    for (i = rem; i--;) {
      max_spec_line = ixheaacd_abs32_nrm(*ptr_tmp++) | max_spec_line;
    }
  }

  return ixheaacd_norm32(max_spec_line);
}