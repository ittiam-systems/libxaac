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

#include <ixheaacd_basic_op.h>
#include "ixheaacd_intrinsics.h"
#include "ixheaacd_common_rom.h"
#include "ixheaacd_basic_funcs.h"
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_sbrdecsettings.h"
#include "ixheaacd_sbr_scale.h"
#include "ixheaacd_lpp_tran.h"
#include "ixheaacd_env_extr_part.h"
#include <ixheaacd_sbr_rom.h>
#include "ixheaacd_hybrid.h"
#include "ixheaacd_ps_dec.h"
#include "ixheaacd_env_extr.h"

#include "ixheaacd_sbr_const.h"
#include "ixheaacd_env_extr.h"
#include "ixheaacd_freq_sca.h"
#include "ixheaacd_intrinsics.h"

WORD32 ixheaacd_samp_rate_table[12] = {92017, 75132, 55426, 46009,
                                       37566, 27713, 23004, 18783,
                                       13856, 11502, 9391,  16428320};

WORD32 ixheaacd_v_offset_40[16] = {3 + 1, 2 + 1, 2 + 1, 2 + 1, 2 + 1, 2 + 1,
                                   2 + 1, 2 + 1, 2 + 1, 2 + 1, 2 + 1, 2 + 1,
                                   2 + 1, 2 + 1, 1 + 1, 0};
static WORD32 ixheaacd_get_sr_idx_2(WORD32 samplerate) {
  WORD32 i;
  for (i = 0; i < 12; i++) {
    if (ixheaacd_samp_rate_table[i] <= samplerate) break;
  }
  return i;
}

static WORD32 ixheaacd_get_sr_idx_4(WORD32 fs) {
  if (92017 <= fs) return 0;
  if (75132 <= fs) return 1;
  if (55426 <= fs) return 2;
  if (46009 <= fs) return 3;
  if (42000 <= fs) return 4;
  if (35777 <= fs) return 5;
  if (27713 <= fs) return 6;
  if (23004 <= fs) return 7;
  if (18783 <= fs) return 8;
  if (00000 <= fs) return 9;
  return 0;
}

static PLATFORM_INLINE WORD16 ixheaacd_get_k0(
    WORD32 sr_idx, WORD16 start_freq, ia_sbr_tables_struct *ptr_sbr_tables) {
  WORD32 start_min = ptr_sbr_tables->env_extr_tables_ptr->start_min[sr_idx];
  WORD32 ixheaacd_drc_offset =
      ptr_sbr_tables->env_extr_tables_ptr->offset_idx[sr_idx];
  return start_min +
         ptr_sbr_tables->env_extr_tables_ptr
             ->ixheaacd_drc_offset[ixheaacd_drc_offset][start_freq];
}

WORD16 ixheaacd_calc_master_frq_bnd_tbl(
    ia_freq_band_data_struct *pstr_freq_band_data,
    ia_sbr_header_data_struct *ptr_header_data,
    ia_sbr_tables_struct *ptr_sbr_tables,
    ixheaacd_misc_tables *pstr_common_tables) {
  WORD32 k;
  WORD32 fs = ptr_header_data->out_sampling_freq;
  WORD16 bands;
  WORD16 k0, k2, k1;
  WORD32 k2_achived;
  WORD32 k2_diff;
  WORD32 incr;
  WORD32 dk, sr_idx;
  WORD16 vec_dk[MAX_OCTAVE + MAX_SECOND_REGION];
  WORD16 *vec_dk0 = &vec_dk[0];
  WORD16 *vec_dk1 = &vec_dk[MAX_OCTAVE];
  WORD16 upsamp_fac = ptr_header_data->upsamp_fac;
  WORD16 *f_master_tbl = pstr_freq_band_data->f_master_tbl;
  WORD16 num_mf_bands;
  WORD32 usac_flag = ptr_header_data->usac_flag;

  k1 = 0;
  incr = 0;
  dk = 0;

  if (upsamp_fac == 4) {
    fs = fs / 2;
    sr_idx = ixheaacd_get_sr_idx_4(fs);
    k0 = pstr_common_tables->start_band[sr_idx][ptr_header_data->start_freq];
  } else {
    sr_idx = ixheaacd_get_sr_idx_2(fs);
    k0 = ixheaacd_get_k0(sr_idx, ptr_header_data->start_freq, ptr_sbr_tables);
    if (usac_flag && fs == 40000) {
      k0 = k0 + ixheaacd_v_offset_40[ptr_header_data->start_freq];
    }
  }

  if (ptr_header_data->stop_freq < 14) {
    if (upsamp_fac == 4) {
      k2 = pstr_common_tables->stop_band[sr_idx][ptr_header_data->stop_freq];
      k2 = (WORD16)ixheaacd_min32(64, k2);
      if (fs == 20000) {
        k2 = pstr_common_tables
                 ->stop_freq_table_fs40k_4[ptr_header_data->stop_freq];
      }
    } else {
      WORD32 stop_minv = ptr_sbr_tables->env_extr_tables_ptr->stop_min[sr_idx];
      k2 = (WORD16)ixheaacd_min32(
          64, stop_minv +
                  ptr_sbr_tables->env_extr_tables_ptr
                      ->stop_off[sr_idx][ptr_header_data->stop_freq]);
      if (usac_flag && fs == 40000) {
        k2 = pstr_common_tables
                 ->stop_freq_table_fs40k_2[ptr_header_data->stop_freq];
      }
    }

  } else {
    if (ptr_header_data->stop_freq == 14)
      k2 = ixheaacd_shl16(k0, 1);
    else
      k2 = add_d(ixheaacd_shl16(k0, 1), k0);
  }
  if (k2 > NO_SYNTHESIS_CHANNELS) {
    k2 = NO_SYNTHESIS_CHANNELS;
  }
  if (upsamp_fac == 4) {
    if ((sub_d(k2, k0) > MAX_FREQ_COEFFS) || (k2 <= k0)) {
      return -1;
    }
    if ((2 * fs == 44100) && (sub_d(k2, k0) > MAX_FREQ_COEFFS)) {
      return -1;
    }
    if ((2 * fs >= 48000) && (sub_d(k2, k0) > MAX_FREQ_COEFFS)) {
      return -1;
    }
  } else {
    if ((sub_d(k2, k0) > MAX_FREQ_COEFFS_SBR) || (k2 <= k0)) {
      return -1;
    }
    if ((fs == 44100) && (sub_d(k2, k0) > MAX_FREQ_COEFFS_FS44100)) {
      return -1;
    }
    if ((fs >= 48000) && (sub_d(k2, k0) > MAX_FREQ_COEFFS_FS48000)) {
      return -1;
    }
  }

  if (ptr_header_data->freq_scale == 0) {
    WORD16 num_bands;
    if (ptr_header_data->alter_scale == 0) {
      dk = 1;
      num_bands = (WORD16)(k2 - k0);
      num_bands = num_bands - (num_bands & 0x1);
    } else {
      dk = 2;
      num_bands = (WORD16)((k2 - k0) + 2) >> 2;
      num_bands = num_bands << 1;
    }
    if (num_bands < 1) {
      return -1;
    }
    k2_achived = k0 + (num_bands << (dk - 1));

    k2_diff = k2 - k2_achived;

    for (k = 0; k < num_bands; k++) {
      vec_dk[k] = dk;
    }

    if (k2_diff < 0) {
      incr = 1;
      k = 0;
    }
    if (k2_diff > 0) {
      incr = -1;
      k = sub_d(num_bands, 1);
    }
    while (k2_diff != 0) {
      vec_dk[k] = vec_dk[k] - incr;
      k = (WORD16)(k + incr);
      k2_diff = k2_diff + incr;
    }
    f_master_tbl[0] = k0;
    for (k = 1; k <= num_bands; k++)
      f_master_tbl[k] = f_master_tbl[k - 1] + vec_dk[k - 1];
    num_mf_bands = num_bands;
  } else {
    WORD32 num_bands0;
    WORD32 num_bands1;

    switch (ptr_header_data->freq_scale) {
      case 1:
        bands = 12;
        break;
      case 2:
        bands = 10;
        break;
      case 3:
        bands = 8;
        break;
      default:
        bands = 8;
    };

    if ((upsamp_fac == 4) && (k0 < bands)) {
      bands = ((WORD32)(k0 - (k0 & 1)));
    }

    if ((WORD32)(10000 * k2) > (WORD32)(22449 * k0)) {
      k1 = k0 << 1;

      num_bands0 = bands;

      num_bands1 = pstr_common_tables->log_dual_is_table[k2] -
                   pstr_common_tables->log_dual_is_table[k1];
      num_bands1 = bands * num_bands1;

      if (ptr_header_data->alter_scale) {
        num_bands1 = num_bands1 * (0x6276);
        num_bands1 = num_bands1 >> 15;
      }
      num_bands1 = num_bands1 + 0x1000;

      num_bands1 = num_bands1 >> 13;
      num_bands1 = num_bands1 << 1;

      if (num_bands0 < 1) {
        return -1;
      }

      if (num_bands1 < 1) {
        return -1;
      }

      ixheaacd_calc_bands(vec_dk0, k0, k1, (WORD16)num_bands0);

      ixheaacd_aac_shellsort(vec_dk0, num_bands0);

      if (vec_dk0[0] == 0) {
        return -1;
      }

      f_master_tbl[0] = k0;

      for (k = 1; k <= num_bands0; k++)
        f_master_tbl[k] = f_master_tbl[k - 1] + vec_dk0[k - 1];

      ixheaacd_calc_bands(vec_dk1, k1, k2, (WORD16)num_bands1);
      ixheaacd_aac_shellsort(vec_dk1, num_bands1);

      if (vec_dk1[0] < vec_dk0[num_bands0 - 1]) {
        WORD16 change = vec_dk0[num_bands0 - 1] - vec_dk1[0];
        WORD16 temp = vec_dk1[num_bands1 - 1] - vec_dk1[0];
        temp = temp >> 1;
        if (change > temp) {
          change = temp;
        }
        vec_dk1[0] = vec_dk1[0] + change;
        vec_dk1[num_bands1 - 1] = vec_dk1[num_bands1 - 1] - change;
        ixheaacd_aac_shellsort(vec_dk1, num_bands1);
      }

      f_master_tbl[num_bands0] = k1;
      for (k = 1; k <= num_bands1; k++)
        f_master_tbl[num_bands0 + k] =
            f_master_tbl[num_bands0 + k - 1] + vec_dk1[k - 1];
      num_mf_bands = add_d(num_bands0, num_bands1);

    } else {
      k1 = k2;

      num_bands0 = pstr_common_tables->log_dual_is_table[k1] -
                   pstr_common_tables->log_dual_is_table[k0];

      num_bands0 = bands * num_bands0;

      num_bands0 = num_bands0 + 0x1000;

      num_bands0 = num_bands0 >> 13;
      num_bands0 = num_bands0 << 1;

      if (num_bands0 < 1) {
        return -1;
      }
      ixheaacd_calc_bands(vec_dk0, k0, k1, (WORD16)num_bands0);
      ixheaacd_aac_shellsort(vec_dk0, num_bands0);

      if (vec_dk0[0] == 0) {
        return -1;
      }

      f_master_tbl[0] = k0;
      for (k = 1; k <= num_bands0; k++)
        f_master_tbl[k] = f_master_tbl[k - 1] + vec_dk0[k - 1];

      num_mf_bands = num_bands0;
    }
  }
  if (num_mf_bands < 1) {
    return -1;
  }
  pstr_freq_band_data->num_mf_bands = num_mf_bands;
  return 0;
}

static WORD16 ixheaacd_calc_freq_ratio(WORD16 k_start, WORD16 k_stop,
                                       WORD16 num_bands) {
  WORD32 bandfactor;
  WORD32 step;
  WORD32 direction;
  WORD32 start;
  WORD32 stop;
  WORD32 temp;
  WORD32 j, i;

  bandfactor = 0x3f000000L;
  step = 0x20000000L;
  direction = 1;
  start = ixheaacd_shl32(ixheaacd_deposit16l_in32(k_start), INT_BITS - 8);
  stop = ixheaacd_shl32(ixheaacd_deposit16l_in32(k_stop), INT_BITS - 8);

  i = 0;

  do {
    i = i + 1;
    temp = stop;

    for (j = 0; j < num_bands; j++)
      temp = ixheaacd_mult16x16in32_shl(ixheaacd_extract16h(temp),
                                        ixheaacd_extract16h(bandfactor));

    if (temp < start) {
      if (direction == 0) step = ixheaacd_shr32(step, 1);
      direction = 1;
      bandfactor = ixheaacd_add32_sat(bandfactor, step);

    } else {
      if (direction == 1) step = ixheaacd_shr32(step, 1);
      direction = 0;
      bandfactor = ixheaacd_sub32_sat(bandfactor, step);
    }

    if (i > 100) {
      step = 0;
    }
  } while (step > 0);

  return ixheaacd_extract16h(bandfactor);
}

VOID ixheaacd_calc_bands(WORD16 *diff, WORD16 start, WORD16 stop,
                         WORD16 num_bands) {
  WORD32 i;
  WORD32 previous;
  WORD32 current;
  WORD32 temp, exact;
  WORD16 bandfactor = ixheaacd_calc_freq_ratio(start, stop, num_bands);

  previous = stop;
  exact = ixheaacd_shl32_sat(ixheaacd_deposit16l_in32(stop), INT_BITS - 8);

  for (i = num_bands - 1; i >= 0; i--) {
    exact = ixheaacd_mult16x16in32(ixheaacd_extract16h(exact), bandfactor);

    temp = ixheaacd_add32_sat(exact, 0x00400000);
    exact = exact << 1;

    current = ixheaacd_extract16l(ixheaacd_shr32(temp, (INT_BITS - 9)));

    diff[i] = sub_d(previous, current);
    previous = current;
  }
}

static VOID ixheaacd_derive_hi_lo_freq_bnd_tbls(
    ia_freq_band_data_struct *pstr_freq_band_data,
    ia_sbr_header_data_struct *ptr_header_data) {
  WORD16 k;
  WORD16 xover_band = ptr_header_data->xover_band;
  WORD16 *f_master_tbl = pstr_freq_band_data->f_master_tbl + xover_band;
  WORD16 *f_low_tbl = pstr_freq_band_data->freq_band_table[LOW];
  WORD16 *f_high_tbl = pstr_freq_band_data->freq_band_table[HIGH];
  WORD16 num_mf_bands = pstr_freq_band_data->num_mf_bands;
  WORD16 num_lf_bands, num_hf_bands;
  num_hf_bands = num_mf_bands - xover_band;
  k = 0;
  *f_low_tbl = *f_high_tbl = *f_master_tbl;
  f_low_tbl++;
  f_high_tbl++;
  f_master_tbl++;
  k++;
  if ((num_hf_bands & 1)) {
    *f_low_tbl = *f_high_tbl = *f_master_tbl;
    f_high_tbl++;
    f_master_tbl++;
    f_low_tbl++;
    k++;
  }
  for (; k <= num_hf_bands; k++) {
    *f_high_tbl = *f_master_tbl;
    f_high_tbl++;
    f_master_tbl++;
    k++;

    *f_low_tbl = *f_high_tbl = *f_master_tbl;
    f_high_tbl++;
    f_master_tbl++;
    f_low_tbl++;
  }
  num_lf_bands = ((num_hf_bands + 1) >> 1);

  pstr_freq_band_data->num_sf_bands[LOW] = num_lf_bands;
  pstr_freq_band_data->num_sf_bands[HIGH] = num_hf_bands;
}

static WORD32 ixheaacd_int_div(WORD32 num, WORD32 den) {
  if (den != 0) {
    WORD32 result = 0;
    WORD32 temp = 0;
    while (den <= num) {
      temp = 0;
      while (num >= (den << (temp + 1))) {
        temp++;
      }
      result = result + (1 << temp);
      num = num - (den * (1 << temp));
    }
    return result;
  } else {
    return 0;
  }
}

VOID ixheaacd_aac_shellsort(WORD16 *in, WORD32 n) {
  WORD32 i, j;
  WORD32 inc;
  WORD32 v, w;

  inc = 1;

  do {
    inc = (((inc << 1) + inc) + 1);
  } while (inc <= n);

  do {
    inc = (ixheaacd_int_div(inc, 3));
    for (i = inc; i < n; i++) {
      v = in[i];
      j = i;

      while ((w = in[(j - inc)]) > v) {
        in[j] = w;
        j = (j - inc);

        if (j < inc) break;
      }
      in[j] = v;
    }

  } while (inc > 1);
}

WORD32 ixheaacd_derive_noise_freq_bnd_tbl(
    ia_sbr_header_data_struct *ptr_header_data,
    ixheaacd_misc_tables *pstr_common_tables,
    ia_freq_band_data_struct *pstr_freq_band_data) {
  WORD16 k2, kx;
  WORD32 temp;
  WORD32 num_lf_bands = pstr_freq_band_data->num_sf_bands[LOW];
  WORD32 num_hf_bands = pstr_freq_band_data->num_sf_bands[HIGH];
  k2 = pstr_freq_band_data->freq_band_table[HIGH][num_hf_bands];
  kx = pstr_freq_band_data->freq_band_table[HIGH][0];

  if (ptr_header_data->noise_bands == 0) {
    pstr_freq_band_data->num_nf_bands = 1;
  } else {
    temp = pstr_common_tables->log_dual_is_table[k2] -
           pstr_common_tables->log_dual_is_table[kx];
    temp = temp * ptr_header_data->noise_bands;
    temp = temp + 0x800;
    temp = temp >> 12;
    if (temp == 0) {
      temp = 1;
    }
    pstr_freq_band_data->num_nf_bands = temp;
  }
  pstr_freq_band_data->num_if_bands = pstr_freq_band_data->num_nf_bands;

  if (pstr_freq_band_data->num_nf_bands > MAX_NOISE_COEFFS) {
    return -1;
  }
  {
    WORD16 i_k, k;
    WORD16 num, den;
    WORD16 *f_noise_tbl = pstr_freq_band_data->freq_band_tbl_noise;
    WORD16 *f_low_tbl = pstr_freq_band_data->freq_band_table[LOW];
    WORD32 num_nf_bands = pstr_freq_band_data->num_nf_bands;

    num = num_lf_bands;
    den = num_nf_bands;

    k = 0;
    *f_noise_tbl = f_low_tbl[0];
    f_noise_tbl++;
    k++;
    i_k = 0;

    for (; k <= num_nf_bands; k++) {
      i_k = i_k + (WORD16)ixheaacd_int_div(num, den);
      *f_noise_tbl = f_low_tbl[i_k];
      num = num_lf_bands - i_k;
      den = den - 1;
      f_noise_tbl++;
    }
  }
  return 0;
}

WORD32 ixheaacd_calc_frq_bnd_tbls(ia_sbr_header_data_struct *ptr_header_data,
                                  ia_sbr_tables_struct *ptr_sbr_tables,
                                  ixheaacd_misc_tables *pstr_common_tables) {
  WORD32 err;
  WORD16 num_lf_bands, num_hf_bands, lsb, usb;
  ia_freq_band_data_struct *pstr_freq_band_data =
      ptr_header_data->pstr_freq_band_data;

  err = ixheaacd_calc_master_frq_bnd_tbl(pstr_freq_band_data, ptr_header_data,
                                         ptr_sbr_tables, pstr_common_tables);

  if (err ||
      (ptr_header_data->xover_band > pstr_freq_band_data->num_mf_bands)) {
    return -1;
  }

  ixheaacd_derive_hi_lo_freq_bnd_tbls(pstr_freq_band_data, ptr_header_data);

  num_lf_bands = pstr_freq_band_data->num_sf_bands[LOW];
  num_hf_bands = pstr_freq_band_data->num_sf_bands[HIGH];

  if ((num_lf_bands <= 0) ||
      (num_lf_bands > ixheaacd_shr16(MAX_FREQ_COEFFS, 1))) {
    return -1;
  }

  lsb = pstr_freq_band_data->freq_band_table[LOW][0];
  usb = pstr_freq_band_data->freq_band_table[LOW][num_lf_bands];

  pstr_freq_band_data->sub_band_start = lsb;

  ptr_header_data->status = 1;

  if ((lsb > NO_ANALYSIS_CHANNELS) || (lsb >= usb)) {
    return -1;
  }

  if (ixheaacd_derive_noise_freq_bnd_tbl(ptr_header_data, pstr_common_tables,
                                         pstr_freq_band_data)) {
    return -1;
  }

  pstr_freq_band_data->sub_band_start = lsb;
  pstr_freq_band_data->sub_band_end = usb;

  return 0;
}
