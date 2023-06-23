/******************************************************************************
 *                                                                            *
 * Copyright (C) 2023 The Android Open Source Project
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

#include <string.h>
#include <math.h>
#include <limits.h>

#include "ixheaac_type_def.h"
#include "ixheaac_constants.h"
#include "ixheaace_aac_constants.h"
#include "ixheaac_error_standards.h"
#include "ixheaace_error_codes.h"
#include "ixheaac_basic_ops32.h"
#include "ixheaac_basic_ops16.h"
#include "ixheaac_basic_ops40.h"
#include "ixheaac_basic_ops.h"

#include "ixheaace_sbr_header.h"
#include "ixheaace_sbr_def.h"
#include "ixheaace_resampler.h"

#include "ixheaace_sbr_rom.h"
#include "ixheaace_common_rom.h"
#include "ixheaace_sbr_qmf_enc.h"
#include "ixheaace_sbr_tran_det.h"
#include "ixheaace_sbr_frame_info_gen.h"
#include "ixheaace_sbr_env_est.h"
#include "ixheaace_sbr_code_envelope.h"
#include "ixheaace_sbr_hbe.h"
#include "ixheaace_sbr_main.h"
#include "ixheaace_sbr_missing_harmonics_det.h"
#include "ixheaace_sbr_inv_filtering_estimation.h"
#include "ixheaace_sbr_noise_floor_est.h"

#include "ixheaace_common_rom.h"
#include "ixheaace_sbr_ton_corr.h"
#include "ixheaace_sbr.h"

#include "ixheaace_bitbuffer.h"

#include "ixheaace_sbr_cmondata.h"
#include "ixheaace_sbr_write_bitstream.h"

#include "ixheaace_sbr_hybrid.h"
#include "ixheaace_sbr_ps_enc.h"

#include "ixheaace_common_utils.h"

#include "ixheaace_sbr_header.h"
#include "ixheaace_sbr_def.h"
#include "ixheaace_resampler.h"
#include "ixheaace_sbr_rom.h"
#include "ixheaace_common_rom.h"
#include "ixheaace_sbr_qmf_enc.h"
#include "ixheaace_sbr_tran_det.h"
#include "ixheaace_sbr_frame_info_gen.h"
#include "ixheaace_sbr_env_est.h"
#include "ixheaace_sbr_code_envelope.h"
#include "ixheaace_sbr_hbe.h"
#include "ixheaace_psy_const.h"
#include "ixheaace_tns.h"
#include "ixheaace_tns_params.h"
#include "ixheaace_rom.h"
#include "ixheaace_common_rom.h"
#include "ixheaace_bitbuffer.h"
#include "ixheaace_enc_main.h"
#include "ixheaace_sbr_main.h"
#include "ixheaace_common_rom.h"
#include "ixheaace_sbr_missing_harmonics_det.h"
#include "ixheaace_sbr_inv_filtering_estimation.h"
#include "ixheaace_sbr_noise_floor_est.h"
#include "ixheaace_sbr_ton_corr.h"
#include "ixheaace_sbr.h"

#include "ixheaace_sbr_freq_scaling.h"

#include "ixheaace_bitbuffer.h"

#include "ixheaace_sbr_hybrid.h"
#include "ixheaace_sbr_ps_enc.h"

#include "ixheaace_sbr_crc.h"
#include "ixheaace_sbr_cmondata.h"
#include "ixheaace_sbr_enc_struct.h"
#include "ixheaace_sbr_write_bitstream.h"

#include "ixheaace_common_utils.h"

static WORD32 ixheaace_map_panorama(WORD32 nrg_val, WORD32 amp_res, WORD32 *ptr_quant_error) {
  WORD32 i = 0;
  ;
  WORD32 min_val, val;
  WORD32 pan_tab[2][10] = {{0, 2, 4, 6, 8, 12, 16, 20, 24}, {0, 2, 4, 8, 12}};
  WORD32 max_index[2] = {9, 5};

  WORD32 pan_index;
  WORD32 sign;

  sign = nrg_val > 0 ? 1 : -1;

  nrg_val = sign * nrg_val;

  min_val = INT_MAX;
  pan_index = 0;

  while (i < max_index[amp_res]) {
    val = ixheaac_abs32(nrg_val - pan_tab[amp_res][i]);
    if (val < min_val) {
      min_val = val;
      pan_index = i;
    }
    i++;
  }

  *ptr_quant_error = min_val;

  return pan_tab[amp_res][max_index[amp_res] - 1] + sign * pan_tab[amp_res][pan_index];
}

static VOID ixheaace_sbr_noise_floor_levels_quantisation(WORD32 *ptr_noise_levels,
                                                         FLOAT32 *ptr_flt_noise_levels,
                                                         WORD32 coupling) {
  WORD32 i = 0;
  WORD32 dummy;

  while (i < MAXIMUM_NUM_NOISE_VALUES) {
    WORD32 tmp;

    tmp = ptr_flt_noise_levels[i] > 30.0f ? 30 : (WORD32)(ptr_flt_noise_levels[i] + 0.5f);

    if (coupling) {
      tmp = tmp < -30 ? -30 : tmp;
      tmp = ixheaace_map_panorama(tmp, 1, &dummy);
    }
    ptr_noise_levels[i] = tmp;

    i++;
  }
}

static VOID ixheaace_couple_noise_floor(FLOAT32 *ptr_noise_lvl_left,
                                        FLOAT32 *ptr_noise_lvl_right) {
  WORD32 i = 0;

  while (i < MAXIMUM_NUM_NOISE_VALUES) {
    FLOAT32 pow_left, pow_right;

    pow_left = (FLOAT32)pow(2.0f, (SBR_NOISE_FLOOR_OFFSET - ptr_noise_lvl_left[i]));
    pow_right = (FLOAT32)pow(2.0f, (SBR_NOISE_FLOOR_OFFSET - ptr_noise_lvl_right[i]));

    ptr_noise_lvl_right[i] -= ptr_noise_lvl_left[i];
    ptr_noise_lvl_left[i] =
        (FLOAT32)(SBR_NOISE_FLOOR_OFFSET - log((pow_left * pow_right) / 2) * SBR_INV_LOG_2);
    i++;
  }
}

static IA_ERRORCODE ixheaace_calculate_sbr_envelope(
    FLOAT32 **ptr_y_buf_left, FLOAT32 **ptr_y_buf_right,
    const ixheaace_str_frame_info_sbr *pstr_const_frame_info, WORD32 *ptr_sfb_ene_l,
    WORD32 *ptr_sfb_ene_r, ixheaace_pstr_sbr_config_data pstr_sbr_cfg,
    ixheaace_pstr_enc_channel pstr_sbr, ixheaace_sbr_stereo_mode stereo_mode,
    WORD32 *ptr_max_quant_err) {
  WORD32 i, j, k, l, count, m = 0;
  WORD32 num_bands, start_pos, stop_pos, li, ui;
  ixheaace_freq_res freq_res;

  WORD32 ca = 2 - pstr_sbr->enc_env_data.init_sbr_amp_res;
  WORD32 n_envelopes = pstr_const_frame_info->n_envelopes;
  WORD32 short_env = pstr_const_frame_info->short_env - 1;
  WORD32 time_step = pstr_sbr->str_sbr_extract_env.time_step;
  WORD32 missing_harmonic = 0;

  if ((ca != 1) && (ca != 2)) {
    return IA_EXHAACE_EXE_FATAL_SBR_INVALID_AMP_RES;
  }

  if (stereo_mode == SBR_COUPLING) {
    *ptr_max_quant_err = 0;
  }

  i = 0;
  while (i < n_envelopes) {
    start_pos = time_step * pstr_const_frame_info->borders[i];
    stop_pos = time_step * pstr_const_frame_info->borders[i + 1];
    freq_res = pstr_const_frame_info->freq_res[i];
    num_bands = pstr_sbr_cfg->num_scf[freq_res];

    if (i == short_env) {
      if (pstr_sbr_cfg->is_ld_sbr) {
        WORD32 temp = 2;
        if (temp < time_step) {
          temp = time_step;
        }
        if (stop_pos - start_pos > temp) {
          stop_pos = stop_pos - temp;
        }
      } else {
        stop_pos = stop_pos - time_step;
      }
    }
    for (j = 0; j < num_bands; j++) {
      FLOAT32 energy_left = 0, energy_right = 0, tmp_ene_l = 0, tmp_ene_r = 0;
      li = pstr_sbr_cfg->ptr_freq_band_tab[freq_res][j];
      ui = pstr_sbr_cfg->ptr_freq_band_tab[freq_res][j + 1];

      if ((freq_res == FREQ_RES_HIGH) && (j == 0 && ui - li > 1)) {
        li++;
      } else {
        if (j == 0 && ui - li > 2) {
          li++;
        }
      }

      missing_harmonic = 0;

      if (pstr_sbr->enc_env_data.add_harmonic_flag) {
        if (freq_res == FREQ_RES_HIGH) {
          if (pstr_sbr->enc_env_data.add_harmonic[j]) {
            missing_harmonic = 1;
          }
        } else {
          WORD32 band;
          WORD32 start_band_high = 0;
          WORD32 stop_band_high = 0;

          while (pstr_sbr_cfg->ptr_freq_band_tab[FREQ_RES_HIGH][start_band_high] <
                 pstr_sbr_cfg->ptr_freq_band_tab[FREQ_RES_LOW][j]) {
            start_band_high++;
          }

          while (pstr_sbr_cfg->ptr_freq_band_tab[FREQ_RES_HIGH][stop_band_high] <
                 pstr_sbr_cfg->ptr_freq_band_tab[FREQ_RES_LOW][j + 1]) {
            stop_band_high++;
          }

          for (band = start_band_high; band < stop_band_high; band++) {
            if (pstr_sbr->enc_env_data.add_harmonic[band]) {
              missing_harmonic = 1;
            }
          }
        }
      }

      if (missing_harmonic) {
        count = stop_pos - start_pos;
        for (l = start_pos; l < stop_pos; l++) {
          energy_left += ptr_y_buf_left[l / 2][li];
        }

        k = li + 1;
        while (k < ui) {
          tmp_ene_l = 0.0f;
          for (l = start_pos; l < stop_pos; l++) {
            tmp_ene_l += ptr_y_buf_left[l / 2][k];
          }

          if (tmp_ene_l > energy_left) {
            energy_left = tmp_ene_l;
          }
          k++;
        }

        if (ui - li > 2) {
          energy_left = energy_left * 0.398107267f;
        } else {
          if (ui - li > 1) {
            energy_left = energy_left * 0.5f;
          }
        }

        if (stereo_mode == SBR_COUPLING) {
          for (l = start_pos; l < stop_pos; l++) {
            energy_right += ptr_y_buf_right[l / 2][li];
          }

          k = li + 1;
          while (k < ui) {
            tmp_ene_r = 0.0f;
            for (l = start_pos; l < stop_pos; l++) {
              tmp_ene_r += ptr_y_buf_right[l / 2][k];
            }

            if (tmp_ene_r > energy_right) {
              energy_right = tmp_ene_r;
            }
            k++;
          }

          if (ui - li > 2) {
            energy_right = energy_right * 0.398107267f;
          } else {
            if (ui - li > 1) {
              energy_right = energy_right * 0.5f;
            }
          }

          tmp_ene_l = energy_left;
          energy_left = (energy_left + energy_right) * 0.5f;
          energy_right = (tmp_ene_l + 1) / (energy_right + 1);
        }
      } /* end missing_harmonic */
      else {
        count = (stop_pos - start_pos) * (ui - li);

        k = li;
        while (k < ui) {
          for (l = start_pos; l < stop_pos; l++) {
            if (pstr_sbr_cfg->is_ld_sbr) {
              energy_left += ptr_y_buf_left[l][k];
            } else {
              energy_left += ptr_y_buf_left[l / 2][k];
            }
          }
          k++;
        }

        if (stereo_mode == SBR_COUPLING) {
          k = li;
          while (k < ui) {
            for (l = start_pos; l < stop_pos; l++) {
              energy_right += ptr_y_buf_right[l / 2][k];
            }
            k++;
          }
          tmp_ene_l = energy_left;
          energy_left = (energy_left + energy_right) * 0.5f;
          energy_right = (tmp_ene_l + 1) / (energy_right + 1);
        }
      }

      energy_left = (FLOAT32)(log(energy_left / (count * 64) + EPS) * SBR_INV_LOG_2);

      if (energy_left < 0.0f) {
        energy_left = 0.0f;
      }

      ptr_sfb_ene_l[m] = (WORD32)(ca * energy_left + 0.5);

      if (stereo_mode == SBR_COUPLING) {
        WORD32 quant_err;
        energy_right = (FLOAT32)(log(energy_right) * SBR_INV_LOG_2);
        ptr_sfb_ene_r[m] =
            ixheaace_map_panorama((WORD32)((FLOAT32)ca * energy_right),
                                  pstr_sbr->enc_env_data.init_sbr_amp_res, &quant_err);
        if (quant_err > *ptr_max_quant_err) {
          *ptr_max_quant_err = quant_err;
        }
      }
      m++;
    } /*end j*/

    if (pstr_sbr_cfg->use_parametric_coding) {
      m -= num_bands;

      for (j = 0; j < num_bands; j++) {
        if (freq_res == FREQ_RES_HIGH && pstr_sbr->str_sbr_extract_env.envelope_compensation[j]) {
          ptr_sfb_ene_l[m] -= (WORD32)(
              ca * ixheaac_abs32(pstr_sbr->str_sbr_extract_env.envelope_compensation[j]));
        }

        if (ptr_sfb_ene_l[m] < 0) {
          ptr_sfb_ene_l[m] = 0;
        }
        m++;
      }
    }
    i++;

  } /*end i*/
  return IA_NO_ERROR;
}

static WORD32 ixheaace_get_pitch_bin(FLOAT32 *fft_data, const WORD32 *ptr_sfb_tab,
                                     WORD32 is_4_1) {
  WORD32 i, j = 0, k = 0;
  WORD32 bin = -1;
  FLOAT32 tmp, prev_val = 0;

  while (ptr_sfb_tab[j] != -1) {
    WORD32 size = ptr_sfb_tab[j];
    tmp = 0;

    for (i = 0; i < size; i++) {
      tmp += fft_data[k] * fft_data[k];
      tmp += fft_data[k + 1] * fft_data[k + 1];
      k += 2;
    }

    tmp = (FLOAT32)log(MAX(MIN_FLT_VAL, (tmp / (FLOAT32)size)));
    if (j != 0) {
      if (fabs(tmp - prev_val) >= 3.0f) {
        if (1 == is_4_1) {
          bin = ((k - (ptr_sfb_tab[j] * 2)) * 3) / 8;
        } else {
          bin = ((k - (ptr_sfb_tab[j] * 2)) * 3) / 4;
        }
        if (bin > 127) {
          bin = -1;
        }
        break;
      }
    }
    prev_val = tmp;
    j++;
  }

  return bin;
}

static IA_ERRORCODE ixheaace_update_esbr_ext_data(
    FLOAT32 *ptr_time_in, ixheaace_pstr_sbr_config_data pstr_sbr_cfg, FLOAT32 *ptr_esbr_scr,
    ixheaace_str_esbr_bs_data *pstr_esbr, WORD32 transient_info[][3],
    ixheaace_str_sbr_tabs *ptr_sbr_tab, WORD32 coupling, WORD32 time_sn_stride,
    WORD32 num_samples) {
  WORD32 bin, bin1;
  const WORD32 *ptr_sbr_table = NULL;
  FLOAT32 *ptr_esbr_inp = ptr_esbr_scr;
  ptr_esbr_scr += num_samples * 2;
  FLOAT32 *ptr_scratch_fft = ptr_esbr_scr;
  WORD32 idx;
  switch (pstr_sbr_cfg->sample_freq) {
    case 16000:
      ptr_sbr_table = ptr_sbr_tab->ptr_esbr_sfb_tab->sfb_bins_8k;
      break;
    case 22050:
      ptr_sbr_table = ptr_sbr_tab->ptr_esbr_sfb_tab->sfb_bins_11k;
      break;
    case 24000:
      ptr_sbr_table = ptr_sbr_tab->ptr_esbr_sfb_tab->sfb_bins_12k;
      break;
    case 32000:
      ptr_sbr_table = ptr_sbr_tab->ptr_esbr_sfb_tab->sfb_bins_16k;
      break;
    case 44100:
      ptr_sbr_table = ptr_sbr_tab->ptr_esbr_sfb_tab->sfb_bins_22k;
      break;
    case 48000:
      ptr_sbr_table = ptr_sbr_tab->ptr_esbr_sfb_tab->sfb_bins_24k;
      break;
    default:
      return IA_EXHEAACE_EXE_FATAL_SBR_INVALID_SAMP_FREQ;
  }
  if (1 == pstr_sbr_cfg->num_ch) {
    for (idx = 0; idx < num_samples; idx += 2) {
      ptr_esbr_inp[idx] = ptr_time_in[time_sn_stride * idx];
      ptr_esbr_inp[idx + 1] = 0;
      ptr_esbr_inp[num_samples + idx] = ptr_time_in[time_sn_stride * (idx + 1)];
      ptr_esbr_inp[num_samples + idx + 1] = 0;
    }
    iusace_complex_fft_2048(ptr_esbr_inp, ptr_scratch_fft);
    bin = ixheaace_get_pitch_bin(ptr_esbr_inp, ptr_sbr_table, 0);
    pstr_esbr->sbr_num_chan = 1;
    if (transient_info[0][1] != 0) {
      pstr_esbr->sbr_preprocessing = 1;
    } else {
      pstr_esbr->sbr_preprocessing = 0;
    }

    if (transient_info[0][1] != 0 && bin != -1) {
      pstr_esbr->sbr_oversampling_flag[0] = 1;
      pstr_esbr->sbr_patching_mode[0] = 0;
      pstr_esbr->sbr_pitchin_flags[0] = 1;
      pstr_esbr->sbr_pitchin_bins[0] = (UWORD8)MIN(bin, 127);
    } else if (bin != -1) {
      pstr_esbr->sbr_oversampling_flag[0] = 0;
      pstr_esbr->sbr_patching_mode[0] = 0;
      pstr_esbr->sbr_pitchin_flags[0] = 1;
      pstr_esbr->sbr_pitchin_bins[0] = (UWORD8)MIN(bin, 127);
    } else if (transient_info[0][1] != 0) {
      pstr_esbr->sbr_oversampling_flag[0] = 1;
      pstr_esbr->sbr_patching_mode[0] = 0;
      pstr_esbr->sbr_pitchin_flags[0] = 0;
    } else {
      pstr_esbr->sbr_patching_mode[0] = 1;
    }
  } else {
    pstr_esbr->sbr_num_chan = 2;
    pstr_esbr->sbr_coupling = coupling;
    for (idx = 0; idx < num_samples; idx += 2) {
      ptr_esbr_inp[idx] = ptr_time_in[2 * idx];
      ptr_esbr_inp[idx + 1] = 0;
      ptr_esbr_inp[num_samples + idx] = ptr_time_in[2 * idx + 2];
      ptr_esbr_inp[num_samples + idx + 1] = 0;
    }
    iusace_complex_fft_2048(ptr_esbr_inp, ptr_scratch_fft);
    bin = ixheaace_get_pitch_bin(ptr_esbr_inp, ptr_sbr_table, 0);
    for (idx = 0; idx < num_samples; idx += 2) {
      ptr_esbr_inp[idx] = ptr_time_in[2 * idx + 1];
      ptr_esbr_inp[idx + 1] = 0;
      ptr_esbr_inp[num_samples + idx] = ptr_time_in[2 * idx + 3];
      ptr_esbr_inp[num_samples + idx + 1] = 0;
    }
    iusace_complex_fft_2048(ptr_esbr_inp, ptr_scratch_fft);
    bin1 = ixheaace_get_pitch_bin(ptr_esbr_inp, ptr_sbr_table, 0);

    if (coupling) {
      if ((transient_info[0][1] != 0 || transient_info[1][1] != 0)) {
        pstr_esbr->sbr_preprocessing = 1;
      } else {
        pstr_esbr->sbr_preprocessing = 0;
      }
      if ((transient_info[0][1] != 0 || transient_info[1][1] != 0) && bin != -1) {
        pstr_esbr->sbr_oversampling_flag[0] = 1;
        pstr_esbr->sbr_patching_mode[0] = 0;
        pstr_esbr->sbr_pitchin_flags[0] = 1;
        bin = MIN(bin, bin1);
        pstr_esbr->sbr_pitchin_bins[0] = (UWORD8)MIN(bin, 127);
      } else if (bin != -1) {
        pstr_esbr->sbr_oversampling_flag[0] = 0;
        pstr_esbr->sbr_patching_mode[0] = 0;
        pstr_esbr->sbr_pitchin_flags[0] = 1;
        bin = MIN(bin, bin1);
        pstr_esbr->sbr_pitchin_bins[0] = (UWORD8)MIN(bin, 127);
      } else if ((transient_info[0][1] != 0 || transient_info[1][1] != 0)) {
        pstr_esbr->sbr_oversampling_flag[0] = 1;
        pstr_esbr->sbr_patching_mode[0] = 0;
        pstr_esbr->sbr_pitchin_flags[0] = 0;
      } else {
        pstr_esbr->sbr_patching_mode[0] = 1;
      }
    } else {
      pstr_esbr->sbr_preprocessing = 0;
      if ((transient_info[0][1] != 0 || transient_info[1][1] != 0)) {
        pstr_esbr->sbr_preprocessing = 1;
      }

      if (transient_info[0][1] != 0 && bin != -1) {
        pstr_esbr->sbr_oversampling_flag[0] = 1;
        pstr_esbr->sbr_patching_mode[0] = 0;
        pstr_esbr->sbr_pitchin_flags[0] = 1;
        bin = MIN(bin, bin1);
        pstr_esbr->sbr_pitchin_bins[0] = (UWORD8)MIN(bin, 127);
      } else if (bin != -1) {
        pstr_esbr->sbr_oversampling_flag[0] = 0;
        pstr_esbr->sbr_patching_mode[0] = 0;
        pstr_esbr->sbr_pitchin_flags[0] = 1;
        pstr_esbr->sbr_pitchin_bins[0] = (UWORD8)MIN(bin, 127);
      } else if (transient_info[0][1] != 0) {
        pstr_esbr->sbr_oversampling_flag[0] = 1;
        pstr_esbr->sbr_patching_mode[0] = 0;
        pstr_esbr->sbr_pitchin_flags[0] = 0;
      } else {
        pstr_esbr->sbr_patching_mode[0] = 1;
      }

      if (transient_info[1][1] != 0 && bin1 != -1) {
        pstr_esbr->sbr_oversampling_flag[1] = 1;
        pstr_esbr->sbr_patching_mode[1] = 0;
        pstr_esbr->sbr_pitchin_flags[1] = 1;
        pstr_esbr->sbr_pitchin_bins[1] = (UWORD8)MIN(bin1, 127);
      } else if (bin1 != -1) {
        pstr_esbr->sbr_oversampling_flag[1] = 0;
        pstr_esbr->sbr_patching_mode[1] = 0;
        pstr_esbr->sbr_pitchin_flags[1] = 1;
        pstr_esbr->sbr_pitchin_bins[1] = (UWORD8)MIN(bin1, 127);
      } else if (transient_info[1][1] != 0) {
        pstr_esbr->sbr_oversampling_flag[1] = 1;
        pstr_esbr->sbr_patching_mode[1] = 0;
        pstr_esbr->sbr_pitchin_flags[1] = 0;
      } else {
        pstr_esbr->sbr_patching_mode[1] = 1;
      }
    }
  }
  return IA_NO_ERROR;
}

VOID ixheaace_esbr_qmfanal32_winadd(FLOAT32 *ptr_inp1, FLOAT32 *ptr_inp2, FLOAT32 *ptr_qmf1,
                                    FLOAT32 *ptr_qmf2, FLOAT32 *ptr_out,
                                    WORD32 num_band_anal_qmf) {
  WORD32 n;
  FLOAT32 accu;

  switch (num_band_anal_qmf) {
    case 32: {
      n = 0;
      while (n < num_band_anal_qmf) {
        accu = (ptr_inp1[n + 0] * ptr_qmf1[2 * (n + 0)]);
        accu += (ptr_inp1[n + 2 * num_band_anal_qmf] * ptr_qmf1[2 * (n + 2 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 4 * num_band_anal_qmf] * ptr_qmf1[2 * (n + 4 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 6 * num_band_anal_qmf] * ptr_qmf1[2 * (n + 6 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 8 * num_band_anal_qmf] * ptr_qmf1[2 * (n + 8 * num_band_anal_qmf)]);
        ptr_out[n] = 2 * accu;

        accu = (ptr_inp1[n + 1 + 0] * ptr_qmf1[2 * (n + 1 + 0)]);
        accu += (ptr_inp1[n + 1 + 2 * num_band_anal_qmf] *
                 ptr_qmf1[2 * (n + 1 + 2 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 1 + 4 * num_band_anal_qmf] *
                 ptr_qmf1[2 * (n + 1 + 4 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 1 + 6 * num_band_anal_qmf] *
                 ptr_qmf1[2 * (n + 1 + 6 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 1 + 8 * num_band_anal_qmf] *
                 ptr_qmf1[2 * (n + 1 + 8 * num_band_anal_qmf)]);
        ptr_out[n + 1] = 2 * accu;

        accu = (ptr_inp2[n + 0] * ptr_qmf2[2 * (n + 0)]);
        accu += (ptr_inp2[n + 2 * num_band_anal_qmf] * ptr_qmf2[2 * (n + 2 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 4 * num_band_anal_qmf] * ptr_qmf2[2 * (n + 4 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 6 * num_band_anal_qmf] * ptr_qmf2[2 * (n + 6 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 8 * num_band_anal_qmf] * ptr_qmf2[2 * (n + 8 * num_band_anal_qmf)]);
        ptr_out[n + num_band_anal_qmf] = 2 * accu;

        accu = (ptr_inp2[n + 1 + 0] * ptr_qmf2[2 * (n + 1 + 0)]);
        accu += (ptr_inp2[n + 1 + 2 * num_band_anal_qmf] *
                 ptr_qmf2[2 * (n + 1 + 2 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 1 + 4 * num_band_anal_qmf] *
                 ptr_qmf2[2 * (n + 1 + 4 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 1 + 6 * num_band_anal_qmf] *
                 ptr_qmf2[2 * (n + 1 + 6 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 1 + 8 * num_band_anal_qmf] *
                 ptr_qmf2[2 * (n + 1 + 8 * num_band_anal_qmf)]);
        ptr_out[n + 1 + num_band_anal_qmf] = 2 * accu;

        n += 2;
      }
      break;
    }
    case 24: {
      n = 0;
      while (n < num_band_anal_qmf) {
        accu = (ptr_inp1[n + 0] * ptr_qmf1[(n + 0)]);
        accu += (ptr_inp1[n + 2 * num_band_anal_qmf] * ptr_qmf1[(n + 2 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 4 * num_band_anal_qmf] * ptr_qmf1[(n + 4 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 6 * num_band_anal_qmf] * ptr_qmf1[(n + 6 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 8 * num_band_anal_qmf] * ptr_qmf1[(n + 8 * num_band_anal_qmf)]);
        ptr_out[n] = 2 * accu;

        accu = (ptr_inp1[n + 1 + 0] * ptr_qmf1[(n + 1 + 0)]);
        accu +=
            (ptr_inp1[n + 1 + 2 * num_band_anal_qmf] * ptr_qmf1[(n + 1 + 2 * num_band_anal_qmf)]);
        accu +=
            (ptr_inp1[n + 1 + 4 * num_band_anal_qmf] * ptr_qmf1[(n + 1 + 4 * num_band_anal_qmf)]);
        accu +=
            (ptr_inp1[n + 1 + 6 * num_band_anal_qmf] * ptr_qmf1[(n + 1 + 6 * num_band_anal_qmf)]);
        accu +=
            (ptr_inp1[n + 1 + 8 * num_band_anal_qmf] * ptr_qmf1[(n + 1 + 8 * num_band_anal_qmf)]);
        ptr_out[n + 1] = 2 * accu;

        accu = (ptr_inp2[n + 0] * ptr_qmf2[(n + 0)]);
        accu += (ptr_inp2[n + 2 * num_band_anal_qmf] * ptr_qmf2[(n + 2 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 4 * num_band_anal_qmf] * ptr_qmf2[(n + 4 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 6 * num_band_anal_qmf] * ptr_qmf2[(n + 6 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 8 * num_band_anal_qmf] * ptr_qmf2[(n + 8 * num_band_anal_qmf)]);
        ptr_out[n + num_band_anal_qmf] = 2 * accu;

        accu = (ptr_inp2[n + 1 + 0] * ptr_qmf2[(n + 1 + 0)]);
        accu +=
            (ptr_inp2[n + 1 + 2 * num_band_anal_qmf] * ptr_qmf2[(n + 1 + 2 * num_band_anal_qmf)]);
        accu +=
            (ptr_inp2[n + 1 + 4 * num_band_anal_qmf] * ptr_qmf2[(n + 1 + 4 * num_band_anal_qmf)]);
        accu +=
            (ptr_inp2[n + 1 + 6 * num_band_anal_qmf] * ptr_qmf2[(n + 1 + 6 * num_band_anal_qmf)]);
        accu +=
            (ptr_inp2[n + 1 + 8 * num_band_anal_qmf] * ptr_qmf2[(n + 1 + 8 * num_band_anal_qmf)]);
        ptr_out[n + 1 + num_band_anal_qmf] = 2 * accu;
        n += 2;
      }
      break;
    }
    default: {
      n = 0;
      while (n < num_band_anal_qmf) {
        accu = (ptr_inp1[n + 0] * ptr_qmf1[4 * (n + 0)]);
        accu += (ptr_inp1[n + 2 * num_band_anal_qmf] * ptr_qmf1[4 * (n + 2 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 4 * num_band_anal_qmf] * ptr_qmf1[4 * (n + 4 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 6 * num_band_anal_qmf] * ptr_qmf1[4 * (n + 6 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 8 * num_band_anal_qmf] * ptr_qmf1[4 * (n + 8 * num_band_anal_qmf)]);
        ptr_out[n] = 2 * accu;

        accu = (ptr_inp1[n + 1 + 0] * ptr_qmf1[4 * (n + 1 + 0)]);
        accu += (ptr_inp1[n + 1 + 2 * num_band_anal_qmf] *
                 ptr_qmf1[4 * (n + 1 + 2 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 1 + 4 * num_band_anal_qmf] *
                 ptr_qmf1[4 * (n + 1 + 4 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 1 + 6 * num_band_anal_qmf] *
                 ptr_qmf1[4 * (n + 1 + 6 * num_band_anal_qmf)]);
        accu += (ptr_inp1[n + 1 + 8 * num_band_anal_qmf] *
                 ptr_qmf1[4 * (n + 1 + 8 * num_band_anal_qmf)]);
        ptr_out[n + 1] = 2 * accu;

        accu = (ptr_inp2[n + 0] * ptr_qmf2[4 * (n + 0)]);
        accu += (ptr_inp2[n + 2 * num_band_anal_qmf] * ptr_qmf2[4 * (n + 2 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 4 * num_band_anal_qmf] * ptr_qmf2[4 * (n + 4 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 6 * num_band_anal_qmf] * ptr_qmf2[4 * (n + 6 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 8 * num_band_anal_qmf] * ptr_qmf2[4 * (n + 8 * num_band_anal_qmf)]);
        ptr_out[n + num_band_anal_qmf] = 2 * accu;

        accu = (ptr_inp2[n + 1 + 0] * ptr_qmf2[4 * (n + 1 + 0)]);
        accu += (ptr_inp2[n + 1 + 2 * num_band_anal_qmf] *
                 ptr_qmf2[4 * (n + 1 + 2 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 1 + 4 * num_band_anal_qmf] *
                 ptr_qmf2[4 * (n + 1 + 4 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 1 + 6 * num_band_anal_qmf] *
                 ptr_qmf2[4 * (n + 1 + 6 * num_band_anal_qmf)]);
        accu += (ptr_inp2[n + 1 + 8 * num_band_anal_qmf] *
                 ptr_qmf2[4 * (n + 1 + 8 * num_band_anal_qmf)]);
        ptr_out[n + 1 + num_band_anal_qmf] = 2 * accu;
        n += 2;
      }
      break;
    }
  }
}

VOID ixheaace_esbr_radix4bfly(const FLOAT32 *ptr_w_in, FLOAT32 *ptr_x, WORD32 index1,
                              WORD32 index) {
  int i;
  WORD32 l1, l2, h2, fft_jmp;
  FLOAT32 xt0_0, yt0_0, xt1_0, yt1_0, xt2_0, yt2_0;
  FLOAT32 xh0_0, xh1_0, xh20_0, xh21_0, xl0_0, xl1_0, xl20_0, xl21_0;
  FLOAT32 x_0, x_1, x_l1_0, x_l1_1, x_l2_0, x_l2_1;
  FLOAT32 x_h2_0, x_h2_1;
  FLOAT32 si10, si20, si30, co10, co20, co30;

  FLOAT32 mul_1, mul_2, mul_3, mul_4, mul_5, mul_6;
  FLOAT32 mul_7, mul_8, mul_9, mul_10, mul_11, mul_12;

  const FLOAT32 *ptr_w = ptr_w_in;
  WORD32 i1;

  h2 = index << 1;
  l1 = index << 2;
  l2 = (index << 2) + (index << 1);

  fft_jmp = 6 * (index);

  i1 = 0;
  while (i1 < index1) {
    for (i = 0; i < index; i++) {
      si10 = (*ptr_w++);
      co10 = (*ptr_w++);
      si20 = (*ptr_w++);
      co20 = (*ptr_w++);
      si30 = (*ptr_w++);
      co30 = (*ptr_w++);

      x_0 = ptr_x[0];
      x_h2_0 = ptr_x[h2];
      x_l1_0 = ptr_x[l1];
      x_l2_0 = ptr_x[l2];

      xh0_0 = (x_0 + x_l1_0);
      xl0_0 = (x_0 - x_l1_0);

      xh20_0 = (x_h2_0 + x_l2_0);
      xl20_0 = (x_h2_0 - x_l2_0);

      ptr_x[0] = (xh0_0 + xh20_0);
      xt0_0 = (xh0_0 - xh20_0);

      x_1 = ptr_x[1];
      x_h2_1 = ptr_x[h2 + 1];
      x_l1_1 = ptr_x[l1 + 1];
      x_l2_1 = ptr_x[l2 + 1];

      xh1_0 = (x_1 + x_l1_1);
      xl1_0 = (x_1 - x_l1_1);

      xh21_0 = (x_h2_1 + x_l2_1);
      xl21_0 = (x_h2_1 - x_l2_1);

      ptr_x[1] = (xh1_0 + xh21_0);
      yt0_0 = (xh1_0 - xh21_0);

      xt1_0 = (xl0_0 + xl21_0);
      xt2_0 = (xl0_0 - xl21_0);

      yt2_0 = (xl1_0 + xl20_0);
      yt1_0 = (xl1_0 - xl20_0);

      mul_11 = (xt2_0 * co30);
      mul_3 = (yt2_0 * si30);
      ptr_x[l2] = 2 * (mul_3 + mul_11);

      mul_5 = (xt2_0 * si30);
      mul_9 = (yt2_0 * co30);
      ptr_x[l2 + 1] = 2 * (mul_9 - mul_5);

      mul_12 = (xt0_0 * co20);
      mul_2 = (yt0_0 * si20);
      ptr_x[l1] = 2 * (mul_2 + mul_12);

      mul_6 = (xt0_0 * si20);
      mul_8 = (yt0_0 * co20);
      ptr_x[l1 + 1] = 2 * (mul_8 - mul_6);

      mul_4 = (xt1_0 * co10);
      mul_1 = (yt1_0 * si10);
      ptr_x[h2] = 2 * (mul_1 + mul_4);

      mul_10 = (xt1_0 * si10);
      mul_7 = (yt1_0 * co10);
      ptr_x[h2 + 1] = 2 * (mul_7 - mul_10);

      ptr_x += 2;
    }
    ptr_x += fft_jmp;
    ptr_w = ptr_w - fft_jmp;
    i1++;
  }
}

VOID ixheaace_esbr_postradixcompute2(FLOAT32 *ptr_y, FLOAT32 *ptr_x,
                                     const FLOAT32 *ptr_dig_rev_tbl, WORD32 npoints) {
  WORD32 i, k;
  WORD32 h2;
  FLOAT32 x_0, x_1, x_2, x_3;
  FLOAT32 x_4, x_5, x_6, x_7;
  FLOAT32 x_8, x_9, x_a, x_b, x_c, x_d, x_e, x_f;
  FLOAT32 n00, n10, n20, n30, n01, n11, n21, n31;
  FLOAT32 n02, n12, n22, n32, n03, n13, n23, n33;

  FLOAT32 *ptr_x2, *ptr_x0;
  FLOAT32 *ptr_y0, *ptr_y1, *ptr_y2, *ptr_y3;

  ptr_y0 = ptr_y;
  ptr_y2 = ptr_y + (WORD32)npoints;
  ptr_x0 = ptr_x;
  ptr_x2 = ptr_x + (WORD32)(npoints >> 1);

  ptr_y1 = ptr_y0 + (WORD32)(npoints >> 2);
  ptr_y3 = ptr_y2 + (WORD32)(npoints >> 2);

  for (k = 0; k < 2; k++) {
    for (i = 0; i<npoints>> 1; i += 8) {
      h2 = (WORD32)*ptr_dig_rev_tbl++ / 4;

      x_0 = *ptr_x0++;
      x_1 = *ptr_x0++;
      x_2 = *ptr_x0++;
      x_3 = *ptr_x0++;
      x_4 = *ptr_x0++;
      x_5 = *ptr_x0++;
      x_6 = *ptr_x0++;
      x_7 = *ptr_x0++;

      n00 = (x_0 + x_2);
      n01 = (x_1 + x_3);

      n20 = (x_0 - x_2);
      n21 = (x_1 - x_3);

      n10 = (x_4 + x_6);
      n11 = (x_5 + x_7);

      n30 = (x_4 - x_6);
      n31 = (x_5 - x_7);

      ptr_y0[h2] = n00;
      ptr_y0[h2 + 1] = n01;
      ptr_y1[h2] = n10;
      ptr_y1[h2 + 1] = n11;
      ptr_y2[h2] = n20;
      ptr_y2[h2 + 1] = n21;
      ptr_y3[h2] = n30;
      ptr_y3[h2 + 1] = n31;

      x_8 = *ptr_x2++;
      x_9 = *ptr_x2++;
      x_a = *ptr_x2++;
      x_b = *ptr_x2++;
      x_c = *ptr_x2++;
      x_d = *ptr_x2++;
      x_e = *ptr_x2++;
      x_f = *ptr_x2++;

      n02 = (x_8 + x_a);
      n03 = (x_9 + x_b);

      n22 = (x_8 - x_a);
      n23 = (x_9 - x_b);

      n12 = (x_c + x_e);
      n13 = (x_d + x_f);

      n32 = (x_c - x_e);
      n33 = (x_d - x_f);

      ptr_y0[h2 + 2] = n02;
      ptr_y0[h2 + 3] = n03;
      ptr_y1[h2 + 2] = n12;
      ptr_y1[h2 + 3] = n13;
      ptr_y2[h2 + 2] = n22;
      ptr_y2[h2 + 3] = n23;
      ptr_y3[h2 + 2] = n32;
      ptr_y3[h2 + 3] = n33;
    }
    ptr_x0 += (WORD32)npoints >> 1;
    ptr_x2 += (WORD32)npoints >> 1;
  }
}

VOID ixheaace_esbr_postradixcompute4(FLOAT32 *ptr_y, FLOAT32 *ptr_x,
                                     const FLOAT32 *ptr_dig_rev_tbl, WORD32 npoints) {
  WORD32 i, k;
  WORD32 h2;
  FLOAT32 xh0_0, xh1_0, xl0_0, xl1_0;
  FLOAT32 xh0_1, xh1_1, xl0_1, xl1_1;
  FLOAT32 x_0, x_1, x_2, x_3;
  FLOAT32 xh0_2, xh1_2, xl0_2, xl1_2, xh0_3, xh1_3, xl0_3, xl1_3;
  FLOAT32 x_4, x_5, x_6, x_7;
  FLOAT32 x_8, x_9, x_a, x_b, x_c, x_d, x_e, x_f;
  FLOAT32 n00, n10, n20, n30, n01, n11, n21, n31;
  FLOAT32 n02, n12, n22, n32, n03, n13, n23, n33;

  FLOAT32 *ptr_x2, *ptr_x0;
  FLOAT32 *ptr_y0, *ptr_y1, *ptr_y2, *ptr_y3;

  ptr_y0 = ptr_y;
  ptr_y2 = ptr_y + npoints;
  ptr_x0 = ptr_x;
  ptr_x2 = ptr_x + (WORD32)(npoints >> 1);

  ptr_y1 = ptr_y0 + (WORD32)(npoints >> 1);
  ptr_y3 = ptr_y2 + (WORD32)(npoints >> 1);

  for (k = 0; k < 2; k++) {
    for (i = 0; i<npoints>> 1; i += 8) {
      h2 = (WORD32)*ptr_dig_rev_tbl++ / 4;
      x_0 = *ptr_x0++;
      x_1 = *ptr_x0++;
      x_2 = *ptr_x0++;
      x_3 = *ptr_x0++;
      x_4 = *ptr_x0++;
      x_5 = *ptr_x0++;
      x_6 = *ptr_x0++;
      x_7 = *ptr_x0++;

      xh0_0 = (x_0 + x_4);
      xh1_0 = (x_1 + x_5);

      xl0_0 = (x_0 - x_4);
      xl1_0 = (x_1 - x_5);

      xh0_1 = (x_2 + x_6);
      xh1_1 = (x_3 + x_7);

      xl0_1 = (x_2 - x_6);
      xl1_1 = (x_3 - x_7);

      n00 = (xh0_0 + xh0_1);
      n01 = (xh1_0 + xh1_1);
      n10 = (xl0_0 + xl1_1);

      n11 = (xl1_0 - xl0_1);
      n20 = (xh0_0 - xh0_1);
      n21 = (xh1_0 - xh1_1);
      n30 = (xl0_0 - xl1_1);

      n31 = (xl1_0 + xl0_1);

      ptr_y0[h2] = n00;
      ptr_y0[h2 + 1] = n01;
      ptr_y1[h2] = n10;
      ptr_y1[h2 + 1] = n11;
      ptr_y2[h2] = n20;
      ptr_y2[h2 + 1] = n21;
      ptr_y3[h2] = n30;
      ptr_y3[h2 + 1] = n31;

      x_8 = *ptr_x2++;
      x_9 = *ptr_x2++;
      x_a = *ptr_x2++;
      x_b = *ptr_x2++;
      x_c = *ptr_x2++;
      x_d = *ptr_x2++;
      x_e = *ptr_x2++;
      x_f = *ptr_x2++;

      xh0_2 = (x_8 + x_c);
      xh1_2 = (x_9 + x_d);

      xl0_2 = (x_8 - x_c);
      xl1_2 = (x_9 - x_d);

      xh0_3 = (x_a + x_e);
      xh1_3 = (x_b + x_f);

      xl0_3 = (x_a - x_e);
      xl1_3 = (x_b - x_f);

      n02 = (xh0_2 + xh0_3);
      n03 = (xh1_2 + xh1_3);
      n12 = (xl0_2 + xl1_3);

      n13 = (xl1_2 - xl0_3);
      n22 = (xh0_2 - xh0_3);
      n23 = (xh1_2 - xh1_3);
      n32 = (xl0_2 - xl1_3);

      n33 = (xl1_2 + xl0_3);

      ptr_y0[h2 + 2] = n02;
      ptr_y0[h2 + 3] = n03;
      ptr_y1[h2 + 2] = n12;
      ptr_y1[h2 + 3] = n13;
      ptr_y2[h2 + 2] = n22;
      ptr_y2[h2 + 3] = n23;
      ptr_y3[h2 + 2] = n32;
      ptr_y3[h2 + 3] = n33;
    }
    ptr_x0 += (WORD32)npoints >> 1;
    ptr_x2 += (WORD32)npoints >> 1;
  }
}

IA_ERRORCODE ixheaace_extract_sbr_envelope(FLOAT32 *ptr_in_time, FLOAT32 *ptr_core_buf,
                                           UWORD32 time_sn_stride,
                                           ixheaace_pstr_sbr_enc pstr_env_enc,
                                           ixheaace_str_sbr_tabs *ptr_sbr_tab,
                                           ixheaace_comm_tables *pstr_com_tab,
                                           WORD32 flag_framelength_small) {
  IA_ERRORCODE err_code = IA_NO_ERROR;
  WORD32 ch, i, j, c;
  WORD32 n_envelopes[IXHEAACE_MAX_CH_IN_BS_ELE];
  WORD32 transient_info[IXHEAACE_MAX_CH_IN_BS_ELE][3];
  const ixheaace_str_frame_info_sbr *pstr_const_frame_info[IXHEAACE_MAX_CH_IN_BS_ELE];
  ixheaace_str_frame_info_sbr *pstr_frame_info = NULL;

  ixheaace_pstr_sbr_config_data pstr_sbr_cfg = &pstr_env_enc->str_sbr_cfg;
  ixheaace_pstr_sbr_hdr_data pstr_sbr_hdr = &pstr_env_enc->str_sbr_hdr;
  ixheaace_pstr_sbr_bitstream_data pstr_sbr_bs = &pstr_env_enc->str_sbr_bs;
  struct ixheaace_ps_enc *pstr_ps_enc = pstr_env_enc->pstr_ps_enc;
  ixheaace_pstr_sbr_qmf_filter_bank pstr_synthesis_qmf_bank =
      pstr_env_enc->pstr_synthesis_qmf_bank;
  ixheaace_pstr_common_data pstr_com_data = &pstr_env_enc->str_cmon_data;
  WORD8 *ptr_sbr_scratch = pstr_env_enc->ptr_sbr_enc_scr->sbr_scratch;
  ixheaace_pstr_enc_channel pstr_env_ch[IXHEAACE_MAX_CH_IN_BS_ELE];
  pstr_env_ch[0] = pstr_env_enc->pstr_env_channel[0];
  pstr_env_ch[1] = pstr_env_enc->pstr_env_channel[1];

  WORD32 num_channels = pstr_sbr_cfg->num_ch;
  WORD32 n_in_channels = (pstr_ps_enc) ? 2 : num_channels;

  ixheaace_sbr_stereo_mode stereo_mode = pstr_sbr_cfg->stereo_mode;
  struct ixheaace_str_sbr_env_data *pstr_env_0 = &(pstr_env_ch[0]->enc_env_data);
  struct ixheaace_str_sbr_env_data *pstr_env_1 = &(pstr_env_ch[1]->enc_env_data);

  ixheaace_freq_res res[MAXIMUM_NUM_NOISE_VALUES];
  WORD32 *ptr_v_tuning;
  WORD32 v_tuning_lc_sbr[6] = {0, 2, 4, 0, 0, 0};
  WORD32 v_tuning_ld_sbr[6] = {0, 2, 3, 0, 0, 0};
  if (pstr_sbr_cfg->is_ld_sbr) {
    ptr_v_tuning = v_tuning_ld_sbr;
  } else {
    ptr_v_tuning = v_tuning_lc_sbr;
  }
  FLOAT32 *ptr_noise_floor[IXHEAACE_MAX_CH_IN_BS_ELE] = {NULL};
  WORD32 *ptr_scale_factor_band_nrg[IXHEAACE_MAX_CH_IN_BS_ELE] = {NULL};
  WORD32 *ptr_noise_level[IXHEAACE_MAX_CH_IN_BS_ELE] = {NULL};

  WORD32 *ptr_sfb_nrg_coupling[IXHEAACE_MAX_CH_IN_BS_ELE];
  WORD32 *ptr_noise_lvl_coupling[IXHEAACE_MAX_CH_IN_BS_ELE];
  WORD32 *ptr_frame_splitter_scratch =
      (WORD32 *)pstr_env_ch[0]->str_sbr_extract_env.ptr_r_buffer[0];

  WORD32 max_quant_error;
  ixheaace_str_esbr_bs_data str_esbr;
  WORD32 samp_ratio_fac = DOWNSAMPLE_FAC_2_1;

  if ((n_in_channels > IXHEAACE_MAX_CH_IN_BS_ELE) || (n_in_channels < num_channels) ||
      (n_in_channels <= 0) || (num_channels <= 0)) {
    return IA_EXHEAACE_EXE_FATAL_SBR_INVALID_IN_CHANNELS;
  }
  ch = 0;
  while (ch < n_in_channels) {
    ptr_sfb_nrg_coupling[ch] = (WORD32 *)pstr_env_ch[ch]->str_sbr_extract_env.ptr_r_buffer[0];
    ptr_noise_lvl_coupling[ch] = (WORD32 *)pstr_env_ch[ch]->str_sbr_extract_env.ptr_i_buffer[0];
    ptr_scale_factor_band_nrg[ch] =
        (WORD32 *)pstr_env_ch[ch]->str_sbr_extract_env.ptr_r_buffer[0] +
        IXHEAACE_MAX_CH_IN_BS_ELE * MAXIMUM_NUM_ENVELOPE_VALUES;
    ptr_noise_level[ch] = (WORD32 *)pstr_env_ch[ch]->str_sbr_extract_env.ptr_i_buffer[0] +
                          IXHEAACE_MAX_CH_IN_BS_ELE * MAXIMUM_NUM_ENVELOPE_VALUES;
    ptr_noise_floor[ch] = pstr_env_ch[ch]->str_sbr_extract_env.ptr_i_buffer[0] +
                          IXHEAACE_MAX_CH_IN_BS_ELE * MAXIMUM_NUM_ENVELOPE_VALUES * 2;
    ch++;
  }

  i = 0;
  while (i < MAXIMUM_NUM_NOISE_VALUES) {
    res[i] = FREQ_RES_HIGH;
    i++;
  }

  memset(transient_info, 0, sizeof(transient_info));

  ch = 0;
  while (ch < n_in_channels) {
    ixheaace_str_sbr_extr_env *pstr_sbr_extract_env = &(pstr_env_ch[ch]->str_sbr_extract_env);

    ixheaace_sbr_analysis_filtering(
        ptr_in_time ? ptr_in_time + ch : NULL, time_sn_stride, pstr_sbr_extract_env->ptr_r_buffer,
        pstr_sbr_extract_env->ptr_i_buffer, &pstr_env_ch[ch]->str_sbr_qmf,
        ptr_sbr_tab->ptr_qmf_tab,
        pstr_env_ch[ch]->str_sbr_qmf.num_time_slots * pstr_env_ch[ch]->str_sbr_qmf.rate,
        pstr_sbr_cfg->is_ld_sbr, (FLOAT32 *)ptr_sbr_scratch);

    ch++;
  } /*end ch */

  if (pstr_ps_enc && pstr_synthesis_qmf_bank) {
    err_code = ixheaace_encode_ps_frame(
        pstr_ps_enc, pstr_env_ch[0]->str_sbr_extract_env.ptr_i_buffer,
        pstr_env_ch[0]->str_sbr_extract_env.ptr_r_buffer,
        pstr_env_ch[1]->str_sbr_extract_env.ptr_i_buffer,
        pstr_env_ch[1]->str_sbr_extract_env.ptr_r_buffer, ptr_sbr_tab->ptr_ps_tab, pstr_com_tab);
    if (err_code) {
      return err_code;
    }
    ixheaace_enc_synthesis_qmf_filtering(
        pstr_env_ch[0]->str_sbr_extract_env.ptr_r_buffer,
        pstr_env_ch[0]->str_sbr_extract_env.ptr_i_buffer, ptr_core_buf,
        (ixheaace_pstr_sbr_qmf_filter_bank)pstr_synthesis_qmf_bank);
  }

  ch = 0;
  while (ch < num_channels) {
    ixheaace_str_sbr_extr_env *pstr_sbr_extract_env = &(pstr_env_ch[ch]->str_sbr_extract_env);

    ixheaace_get_energy_from_cplx_qmf(
        pstr_sbr_extract_env->ptr_y_buffer + pstr_sbr_extract_env->y_buffer_write_offset,
        pstr_sbr_extract_env->ptr_r_buffer, pstr_sbr_extract_env->ptr_i_buffer,
        pstr_sbr_cfg->is_ld_sbr, pstr_env_ch[ch]->str_sbr_qmf.num_time_slots, samp_ratio_fac);

    ixheaace_calculate_tonality_quotas(
        &pstr_env_ch[ch]->str_ton_corr, pstr_sbr_extract_env->ptr_r_buffer,
        pstr_sbr_extract_env->ptr_i_buffer,
        pstr_sbr_cfg->ptr_freq_band_tab[HI][pstr_sbr_cfg->num_scf[HI]],
        pstr_env_ch[ch]->str_sbr_qmf.num_time_slots, pstr_sbr_cfg->is_ld_sbr);
    if (pstr_sbr_cfg->is_ld_sbr) {
      ixheaace_detect_transient_eld(pstr_sbr_extract_env->ptr_y_buffer,
                                    &pstr_env_ch[ch]->str_sbr_trans_detector, transient_info[ch]);
    } else {
      ixheaace_detect_transient(pstr_sbr_extract_env->ptr_y_buffer,
                                &pstr_env_ch[ch]->str_sbr_trans_detector, transient_info[ch],
                                pstr_sbr_extract_env->time_step, pstr_sbr_cfg->sbr_codec);
    }
    if (transient_info[ch][1] == 0) {
      if (pstr_sbr_cfg->is_ld_sbr) {
        err_code = ixheaace_frame_splitter(
            pstr_sbr_extract_env->ptr_y_buffer, &pstr_env_ch[ch]->str_sbr_trans_detector,
            pstr_sbr_cfg->ptr_freq_band_tab[1], pstr_sbr_cfg->num_scf[1],
            pstr_sbr_extract_env->time_step, pstr_sbr_extract_env->time_slots, transient_info[ch],
            (FLOAT32 *)ptr_frame_splitter_scratch, pstr_sbr_cfg->is_ld_sbr);
      } else {
        err_code = ixheaace_frame_splitter(
            pstr_sbr_extract_env->ptr_y_buffer, &pstr_env_ch[ch]->str_sbr_trans_detector,
            pstr_sbr_cfg->ptr_freq_band_tab[1], pstr_sbr_cfg->num_scf[1],
            pstr_sbr_extract_env->time_step, pstr_sbr_extract_env->no_cols, transient_info[ch],
            (FLOAT32 *)ptr_frame_splitter_scratch, pstr_sbr_cfg->is_ld_sbr);
      }
      if (err_code) {
        return err_code;
      }
    }
    ch++;
  } /*end ch */

  if (stereo_mode == SBR_COUPLING) {
    if (transient_info[0][1] && transient_info[1][1]) {
      transient_info[0][0] = ixheaac_min32(transient_info[1][0], transient_info[0][0]);

      transient_info[1][0] = transient_info[0][0];
    } else if (transient_info[0][1] && !transient_info[1][1]) {
      transient_info[1][0] = transient_info[0][0];
    } else if (!transient_info[0][1] && transient_info[1][1]) {
      transient_info[0][0] = transient_info[1][0];
    } else {
      transient_info[0][0] = ixheaac_max32(transient_info[1][0], transient_info[0][0]);

      transient_info[1][0] = transient_info[0][0];
    }
  }

  err_code = ixheaace_frame_info_generator(
      &pstr_env_ch[0]->str_sbr_env_frame, pstr_env_ch[0]->str_sbr_extract_env.pre_transient_info,
      transient_info[0], ptr_v_tuning, ptr_sbr_tab->ptr_qmf_tab,
      pstr_env_ch[0]->str_sbr_qmf.num_time_slots, pstr_sbr_cfg->is_ld_sbr, &pstr_frame_info,
      flag_framelength_small);
  if (pstr_sbr_cfg->is_ld_sbr && transient_info[0][2]) {
    pstr_frame_info->short_env = pstr_frame_info->n_envelopes;
  }
  pstr_const_frame_info[0] = pstr_frame_info;
  if (err_code) {
    return err_code;
  }

  pstr_env_0->pstr_sbr_bs_grid = &pstr_env_ch[0]->str_sbr_env_frame.sbr_grid;

  for (ch = 0; ch < num_channels; ch++) {
    memset(
        ptr_noise_floor[ch], 0,
        IXHEAACE_MAX_CH_IN_BS_ELE * MAXIMUM_NUM_ENVELOPE_VALUES * sizeof(ptr_noise_floor[0][0]));
  }

  switch (stereo_mode) {
    case IXHEAACE_SBR_MODE_LEFT_RIGHT:
    case IXHEAACE_SBR_MODE_SWITCH_LRC:

      err_code = ixheaace_frame_info_generator(
          &pstr_env_ch[1]->str_sbr_env_frame,
          pstr_env_ch[1]->str_sbr_extract_env.pre_transient_info, transient_info[1], ptr_v_tuning,
          ptr_sbr_tab->ptr_qmf_tab, pstr_env_ch[1]->str_sbr_qmf.num_time_slots,
          pstr_sbr_cfg->is_ld_sbr, &pstr_frame_info, flag_framelength_small);

      if (pstr_sbr_cfg->is_ld_sbr && transient_info[1][2]) {
        pstr_frame_info->short_env = pstr_frame_info->n_envelopes;
      }
      pstr_const_frame_info[1] = pstr_frame_info;
      if (err_code) {
        return err_code;
      }

      pstr_env_1->pstr_sbr_bs_grid = &pstr_env_ch[1]->str_sbr_env_frame.sbr_grid;

      if (pstr_const_frame_info[0]->n_envelopes != pstr_const_frame_info[1]->n_envelopes) {
        stereo_mode = IXHEAACE_SBR_MODE_LEFT_RIGHT;
      } else {
        for (i = 0; i < pstr_const_frame_info[0]->n_envelopes + 1; i++) {
          if (pstr_const_frame_info[0]->borders[i] != pstr_const_frame_info[1]->borders[i]) {
            stereo_mode = IXHEAACE_SBR_MODE_LEFT_RIGHT;
            break;
          }
        }

        for (i = 0; i < pstr_const_frame_info[0]->n_envelopes; i++) {
          if (pstr_const_frame_info[0]->freq_res[i] != pstr_const_frame_info[1]->freq_res[i]) {
            stereo_mode = IXHEAACE_SBR_MODE_LEFT_RIGHT;
            break;
          }
        }

        if (pstr_const_frame_info[0]->short_env != pstr_const_frame_info[1]->short_env) {
          stereo_mode = IXHEAACE_SBR_MODE_LEFT_RIGHT;
        }
      }
      break;
    case SBR_COUPLING:

      pstr_const_frame_info[1] = pstr_const_frame_info[0];

      pstr_env_1->pstr_sbr_bs_grid = &pstr_env_ch[0]->str_sbr_env_frame.sbr_grid;
      break;
    case IXHEAACE_SBR_MODE_MONO:
      break;
  }

  for (ch = 0; ch < num_channels; ch++) {
    ixheaace_str_sbr_extr_env *pstr_sbr_extract_env = &(pstr_env_ch[ch]->str_sbr_extract_env);

    pstr_sbr_extract_env->pre_transient_info[0] = transient_info[ch][0];
    pstr_sbr_extract_env->pre_transient_info[1] = transient_info[ch][1];
    pstr_env_ch[ch]->enc_env_data.no_of_envelopes = n_envelopes[ch] =
        pstr_const_frame_info[ch]->n_envelopes;

    for (i = 0; i < n_envelopes[ch]; i++) {
      pstr_env_ch[ch]->enc_env_data.no_scf_bands[i] =
          (pstr_const_frame_info[ch]->freq_res[i] == FREQ_RES_HIGH
               ? pstr_sbr_cfg->num_scf[FREQ_RES_HIGH]
               : pstr_sbr_cfg->num_scf[FREQ_RES_LOW]);
    }

    if ((pstr_env_ch[ch]->enc_env_data.pstr_sbr_bs_grid->frame_type == IXHEAACE_FIXFIX) &&
        (n_envelopes[ch] == 1)) {
      if (pstr_sbr_cfg->is_ld_sbr) {
        pstr_env_ch[ch]->enc_env_data.curr_sbr_amp_res = IXHEAACE_SBR_AMP_RES_3_0;
      } else {
        pstr_env_ch[ch]->enc_env_data.curr_sbr_amp_res = IXHEAACE_SBR_AMP_RES_1_5;
      }
      if (pstr_env_ch[ch]->enc_env_data.init_sbr_amp_res !=
          pstr_env_ch[ch]->enc_env_data.curr_sbr_amp_res) {
        err_code = ixheaace_init_sbr_huffman_tabs(
            &pstr_env_ch[ch]->enc_env_data, &pstr_env_ch[ch]->str_sbr_code_env,
            &pstr_env_ch[ch]->str_sbr_code_noise_floor, IXHEAACE_SBR_AMP_RES_1_5,
            ptr_sbr_tab->ptr_sbr_huff_tab);
        if (err_code) {
          return err_code;
        }
        pstr_env_ch[ch]->sbr_amp_res_init = IXHEAACE_SBR_AMP_RES_1_5;
      }
    } else {
      if (pstr_sbr_hdr->sbr_amp_res != pstr_env_ch[ch]->enc_env_data.init_sbr_amp_res) {
        err_code = ixheaace_init_sbr_huffman_tabs(
            &pstr_env_ch[ch]->enc_env_data, &pstr_env_ch[ch]->str_sbr_code_env,
            &pstr_env_ch[ch]->str_sbr_code_noise_floor, pstr_sbr_hdr->sbr_amp_res,
            ptr_sbr_tab->ptr_sbr_huff_tab);
        if (err_code) {
          return err_code;
        }
        pstr_env_ch[ch]->sbr_amp_res_init = pstr_sbr_hdr->sbr_amp_res;
      }
    }

    ixheaace_ton_corr_param_extr(
        &pstr_env_ch[ch]->str_ton_corr, pstr_env_ch[ch]->enc_env_data.sbr_invf_mode_vec,
        ptr_noise_floor[ch], &pstr_env_ch[ch]->enc_env_data.add_harmonic_flag,
        pstr_env_ch[ch]->enc_env_data.add_harmonic, pstr_sbr_extract_env->envelope_compensation,
        pstr_const_frame_info[ch], transient_info[ch], pstr_sbr_cfg->ptr_freq_band_tab[HI],
        pstr_sbr_cfg->num_scf[HI], pstr_env_ch[ch]->enc_env_data.sbr_xpos_mode, ptr_sbr_scratch,
        pstr_sbr_cfg->is_ld_sbr);

    pstr_env_ch[ch]->enc_env_data.sbr_invf_mode =
        pstr_env_ch[ch]->enc_env_data.sbr_invf_mode_vec[0];
    pstr_env_ch[ch]->enc_env_data.noise_band_count =
        pstr_env_ch[ch]->str_ton_corr.sbr_noise_floor_est.num_of_noise_bands;
  } /*end ch */

  switch (stereo_mode) {
    case IXHEAACE_SBR_MODE_MONO:
      err_code = ixheaace_calculate_sbr_envelope(pstr_env_ch[0]->str_sbr_extract_env.ptr_y_buffer,
                                                 NULL, pstr_const_frame_info[0],
                                                 ptr_scale_factor_band_nrg[0], NULL, pstr_sbr_cfg,
                                                 pstr_env_ch[0], IXHEAACE_SBR_MODE_MONO, NULL);

      if (err_code) {
        return err_code;
      }
      break;

    case IXHEAACE_SBR_MODE_LEFT_RIGHT:

      err_code = ixheaace_calculate_sbr_envelope(pstr_env_ch[0]->str_sbr_extract_env.ptr_y_buffer,
                                                 NULL, pstr_const_frame_info[0],
                                                 ptr_scale_factor_band_nrg[0], NULL, pstr_sbr_cfg,
                                                 pstr_env_ch[0], IXHEAACE_SBR_MODE_MONO, NULL);
      if (err_code) {
        return err_code;
      }

      err_code = ixheaace_calculate_sbr_envelope(pstr_env_ch[1]->str_sbr_extract_env.ptr_y_buffer,
                                                 NULL, pstr_const_frame_info[1],
                                                 ptr_scale_factor_band_nrg[1], NULL, pstr_sbr_cfg,
                                                 pstr_env_ch[1], IXHEAACE_SBR_MODE_MONO, NULL);

      if (err_code) {
        return err_code;
      }

      break;

    case SBR_COUPLING:

      err_code = ixheaace_calculate_sbr_envelope(
          pstr_env_ch[0]->str_sbr_extract_env.ptr_y_buffer,
          pstr_env_ch[1]->str_sbr_extract_env.ptr_y_buffer, pstr_const_frame_info[0],
          ptr_scale_factor_band_nrg[0], ptr_scale_factor_band_nrg[1], pstr_sbr_cfg,
          pstr_env_ch[0], SBR_COUPLING, &max_quant_error);
      if (err_code) {
        return err_code;
      }
      break;

    case IXHEAACE_SBR_MODE_SWITCH_LRC:
      err_code = ixheaace_calculate_sbr_envelope(pstr_env_ch[0]->str_sbr_extract_env.ptr_y_buffer,
                                                 NULL, pstr_const_frame_info[0],
                                                 ptr_scale_factor_band_nrg[0], NULL, pstr_sbr_cfg,
                                                 pstr_env_ch[0], IXHEAACE_SBR_MODE_MONO, NULL);
      if (err_code) {
        return err_code;
      }

      err_code = ixheaace_calculate_sbr_envelope(pstr_env_ch[1]->str_sbr_extract_env.ptr_y_buffer,
                                                 NULL, pstr_const_frame_info[1],
                                                 ptr_scale_factor_band_nrg[1], NULL, pstr_sbr_cfg,
                                                 pstr_env_ch[1], IXHEAACE_SBR_MODE_MONO, NULL);
      if (err_code) {
        return err_code;
      }

      err_code = ixheaace_calculate_sbr_envelope(
          pstr_env_ch[0]->str_sbr_extract_env.ptr_y_buffer,
          pstr_env_ch[1]->str_sbr_extract_env.ptr_y_buffer, pstr_const_frame_info[0],
          ptr_sfb_nrg_coupling[0], ptr_sfb_nrg_coupling[1], pstr_sbr_cfg, pstr_env_ch[0],
          SBR_COUPLING, &max_quant_error);
      if (err_code) {
        return err_code;
      }
      break;
  }

  switch (stereo_mode) {
    case IXHEAACE_SBR_MODE_MONO:

      ixheaace_sbr_noise_floor_levels_quantisation(ptr_noise_level[0], ptr_noise_floor[0], 0);

      err_code = ixheaace_code_envelope(
          ptr_noise_level[0], res, &pstr_env_ch[0]->str_sbr_code_noise_floor,
          pstr_env_0->domain_vec_noise, 0, (pstr_const_frame_info[0]->n_envelopes > 1 ? 2 : 1), 0,
          pstr_sbr_bs->header_active, pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      break;

    case IXHEAACE_SBR_MODE_LEFT_RIGHT:
      // We have a error checks for Number of channels to ensure memory is assigned to
      // ptr_noise_floor[]. However, MSVS static analysis is marking this as a potential error.
      // So, suppressed this in source
      ixheaace_sbr_noise_floor_levels_quantisation(ptr_noise_level[0], ptr_noise_floor[0], 0);

      err_code = ixheaace_code_envelope(
          ptr_noise_level[0], res, &pstr_env_ch[0]->str_sbr_code_noise_floor,
          pstr_env_0->domain_vec_noise, 0, (pstr_const_frame_info[0]->n_envelopes > 1 ? 2 : 1), 0,
          pstr_sbr_bs->header_active, pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      ixheaace_sbr_noise_floor_levels_quantisation(ptr_noise_level[1], ptr_noise_floor[1], 0);

      err_code = ixheaace_code_envelope(
          ptr_noise_level[1], res, &pstr_env_ch[1]->str_sbr_code_noise_floor,
          pstr_env_1->domain_vec_noise, 0, (pstr_const_frame_info[1]->n_envelopes > 1 ? 2 : 1), 0,
          pstr_sbr_bs->header_active, pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      break;

    case SBR_COUPLING:
      ixheaace_couple_noise_floor(ptr_noise_floor[0], ptr_noise_floor[1]);

      ixheaace_sbr_noise_floor_levels_quantisation(ptr_noise_level[0], ptr_noise_floor[0], 0);

      err_code = ixheaace_code_envelope(
          ptr_noise_level[0], res, &pstr_env_ch[0]->str_sbr_code_noise_floor,
          pstr_env_0->domain_vec_noise, 1, (pstr_const_frame_info[0]->n_envelopes > 1 ? 2 : 1), 0,
          pstr_sbr_bs->header_active, pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      ixheaace_sbr_noise_floor_levels_quantisation(ptr_noise_level[1], ptr_noise_floor[1], 1);

      err_code = ixheaace_code_envelope(
          ptr_noise_level[1], res, &pstr_env_ch[1]->str_sbr_code_noise_floor,
          pstr_env_1->domain_vec_noise, 1, (pstr_const_frame_info[1]->n_envelopes > 1 ? 2 : 1), 1,
          pstr_sbr_bs->header_active, pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      break;

    case IXHEAACE_SBR_MODE_SWITCH_LRC:

      ixheaace_sbr_noise_floor_levels_quantisation(ptr_noise_level[0], ptr_noise_floor[0], 0);

      ixheaace_sbr_noise_floor_levels_quantisation(ptr_noise_level[1], ptr_noise_floor[1], 0);

      ixheaace_couple_noise_floor(ptr_noise_floor[0], ptr_noise_floor[1]);

      ixheaace_sbr_noise_floor_levels_quantisation(ptr_noise_lvl_coupling[0], ptr_noise_floor[0],
                                                   0);

      ixheaace_sbr_noise_floor_levels_quantisation(ptr_noise_lvl_coupling[1], ptr_noise_floor[1],
                                                   1);
      break;
  }

  switch (stereo_mode) {
    case IXHEAACE_SBR_MODE_MONO:

      pstr_sbr_hdr->coupling = 0;
      pstr_env_0->balance = 0;

      err_code = ixheaace_code_envelope(
          ptr_scale_factor_band_nrg[0], pstr_const_frame_info[0]->freq_res,
          &pstr_env_ch[0]->str_sbr_code_env, pstr_env_0->domain_vec, pstr_sbr_hdr->coupling,
          pstr_const_frame_info[0]->n_envelopes, 0, pstr_sbr_bs->header_active,
          pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }
      break;

    case IXHEAACE_SBR_MODE_LEFT_RIGHT:

      pstr_sbr_hdr->coupling = 0;

      pstr_env_0->balance = 0;
      pstr_env_1->balance = 0;

      err_code = ixheaace_code_envelope(
          ptr_scale_factor_band_nrg[0], pstr_const_frame_info[0]->freq_res,
          &pstr_env_ch[0]->str_sbr_code_env, pstr_env_0->domain_vec, pstr_sbr_hdr->coupling,
          pstr_const_frame_info[0]->n_envelopes, 0, pstr_sbr_bs->header_active,
          pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      err_code = ixheaace_code_envelope(
          ptr_scale_factor_band_nrg[1], pstr_const_frame_info[1]->freq_res,
          &pstr_env_ch[1]->str_sbr_code_env, pstr_env_1->domain_vec, pstr_sbr_hdr->coupling,
          pstr_const_frame_info[1]->n_envelopes, 0, pstr_sbr_bs->header_active,
          pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }
      break;

    case SBR_COUPLING:

      pstr_sbr_hdr->coupling = 1;
      pstr_env_0->balance = 0;
      pstr_env_1->balance = 1;

      err_code = ixheaace_code_envelope(
          ptr_scale_factor_band_nrg[0], pstr_const_frame_info[0]->freq_res,
          &pstr_env_ch[0]->str_sbr_code_env, pstr_env_0->domain_vec, pstr_sbr_hdr->coupling,
          pstr_const_frame_info[0]->n_envelopes, 0, pstr_sbr_bs->header_active,
          pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      err_code = ixheaace_code_envelope(
          ptr_scale_factor_band_nrg[1], pstr_const_frame_info[1]->freq_res,
          &pstr_env_ch[1]->str_sbr_code_env, pstr_env_1->domain_vec, pstr_sbr_hdr->coupling,
          pstr_const_frame_info[1]->n_envelopes, 1, pstr_sbr_bs->header_active,
          pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }
      break;

    case IXHEAACE_SBR_MODE_SWITCH_LRC: {
      WORD32 payloadbits_lr;
      WORD32 payloadbits_coupling;

      WORD32 scale_factor_band_nrg_prev_temp[IXHEAACE_MAX_CH_IN_BS_ELE][MAXIMUM_FREQ_COEFFS];
      WORD32 noise_prev_temp[IXHEAACE_MAX_CH_IN_BS_ELE][MAXIMUM_NUM_NOISE_COEFFS];
      WORD32 up_date_nrg_temp[IXHEAACE_MAX_CH_IN_BS_ELE];
      WORD32 up_date_noise_temp[IXHEAACE_MAX_CH_IN_BS_ELE];
      WORD32 domain_vec_temp[IXHEAACE_MAX_CH_IN_BS_ELE][IXHEAACE_MAX_ENV];
      WORD32 domain_vec_noise_temp[IXHEAACE_MAX_CH_IN_BS_ELE][IXHEAACE_MAX_ENV];

      WORD32 temp_flag_right = 0;
      WORD32 temp_flag_left = 0;

      ch = 0;
      while (ch < num_channels) {
        memcpy(scale_factor_band_nrg_prev_temp[ch],
               pstr_env_ch[ch]->str_sbr_code_env.sfb_nrg_prev,
               MAXIMUM_FREQ_COEFFS * sizeof(scale_factor_band_nrg_prev_temp[0][0]));

        memcpy(noise_prev_temp[ch], pstr_env_ch[ch]->str_sbr_code_noise_floor.sfb_nrg_prev,
               MAXIMUM_NUM_NOISE_COEFFS * sizeof(noise_prev_temp[0][0]));

        up_date_nrg_temp[ch] = pstr_env_ch[ch]->str_sbr_code_env.update;
        up_date_noise_temp[ch] = pstr_env_ch[ch]->str_sbr_code_noise_floor.update;

        if (pstr_sbr_hdr->prev_coupling) {
          pstr_env_ch[ch]->str_sbr_code_env.update = 0;
          pstr_env_ch[ch]->str_sbr_code_noise_floor.update = 0;
        }
        ch++;
      } /*end ch */

      err_code = ixheaace_code_envelope(
          ptr_scale_factor_band_nrg[0], pstr_const_frame_info[0]->freq_res,
          &pstr_env_ch[0]->str_sbr_code_env, pstr_env_0->domain_vec, 0,
          pstr_const_frame_info[0]->n_envelopes, 0, pstr_sbr_bs->header_active,
          pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      err_code = ixheaace_code_envelope(
          ptr_scale_factor_band_nrg[1], pstr_const_frame_info[1]->freq_res,
          &pstr_env_ch[1]->str_sbr_code_env, pstr_env_1->domain_vec, 0,
          pstr_const_frame_info[1]->n_envelopes, 0, pstr_sbr_bs->header_active,
          pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      c = 0;
      i = 0;
      while (i < n_envelopes[0]) {
        for (j = 0; j < pstr_env_0->no_scf_bands[i]; j++) {
          pstr_env_0->ienvelope[i][j] = ptr_scale_factor_band_nrg[0][c];
          pstr_env_1->ienvelope[i][j] = ptr_scale_factor_band_nrg[1][c];

          c++;
        }
        i++;
      }

      err_code = ixheaace_code_envelope(
          ptr_noise_level[0], res, &pstr_env_ch[0]->str_sbr_code_noise_floor,
          pstr_env_0->domain_vec_noise, 0, (pstr_const_frame_info[0]->n_envelopes > 1 ? 2 : 1), 0,
          pstr_sbr_bs->header_active, pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      i = 0;
      while (i < MAXIMUM_NUM_NOISE_VALUES) {
        pstr_env_0->noise_level[i] = ptr_noise_level[0][i];
        i++;
      }

      err_code = ixheaace_code_envelope(
          ptr_noise_level[1], res, &pstr_env_ch[1]->str_sbr_code_noise_floor,
          pstr_env_1->domain_vec_noise, 0, (pstr_const_frame_info[1]->n_envelopes > 1 ? 2 : 1), 0,
          pstr_sbr_bs->header_active, pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      i = 0;
      while (i < MAXIMUM_NUM_NOISE_VALUES) {
        pstr_env_1->noise_level[i] = ptr_noise_level[1][i];
        i++;
      }

      pstr_sbr_hdr->coupling = 0;
      pstr_env_0->balance = 0;
      pstr_env_1->balance = 0;

      err_code = ixheaace_count_sbr_channel_pair_element(
          pstr_sbr_hdr, pstr_sbr_bs, &pstr_env_ch[0]->enc_env_data, &pstr_env_ch[1]->enc_env_data,
          pstr_com_data, ptr_sbr_tab, pstr_sbr_cfg->sbr_codec, pstr_sbr_cfg->is_esbr, &str_esbr,
          &payloadbits_lr);
      if (err_code) {
        return err_code;
      }

      for (ch = 0; ch < num_channels; ch++) {
        WORD32 itmp;

        for (i = 0; i < MAXIMUM_FREQ_COEFFS; i++) {
          itmp = pstr_env_ch[ch]->str_sbr_code_env.sfb_nrg_prev[i];
          pstr_env_ch[ch]->str_sbr_code_env.sfb_nrg_prev[i] =
              scale_factor_band_nrg_prev_temp[ch][i];
          scale_factor_band_nrg_prev_temp[ch][i] = itmp;
        }

        for (i = 0; i < MAXIMUM_NUM_NOISE_COEFFS; i++) {
          itmp = pstr_env_ch[ch]->str_sbr_code_noise_floor.sfb_nrg_prev[i];
          pstr_env_ch[ch]->str_sbr_code_noise_floor.sfb_nrg_prev[i] = noise_prev_temp[ch][i];
          noise_prev_temp[ch][i] = itmp;
        }

        itmp = pstr_env_ch[ch]->str_sbr_code_env.update;
        pstr_env_ch[ch]->str_sbr_code_env.update = up_date_nrg_temp[ch];
        up_date_nrg_temp[ch] = itmp;

        itmp = pstr_env_ch[ch]->str_sbr_code_noise_floor.update;
        pstr_env_ch[ch]->str_sbr_code_noise_floor.update = up_date_noise_temp[ch];
        up_date_noise_temp[ch] = itmp;

        memcpy(domain_vec_temp[ch], pstr_env_ch[ch]->enc_env_data.domain_vec,
               sizeof(domain_vec_temp[0][0]) * IXHEAACE_MAX_ENV);

        memcpy(domain_vec_noise_temp[ch], pstr_env_ch[ch]->enc_env_data.domain_vec_noise,
               sizeof(domain_vec_noise_temp[0][0]) * IXHEAACE_MAX_ENV);

        if (!pstr_sbr_hdr->prev_coupling) {
          pstr_env_ch[ch]->str_sbr_code_env.update = 0;
          pstr_env_ch[ch]->str_sbr_code_noise_floor.update = 0;
        }
      } /*end ch */

      err_code = ixheaace_code_envelope(
          ptr_sfb_nrg_coupling[0], pstr_const_frame_info[0]->freq_res,
          &pstr_env_ch[0]->str_sbr_code_env, pstr_env_0->domain_vec, 1,
          pstr_const_frame_info[0]->n_envelopes, 0, pstr_sbr_bs->header_active,
          pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      err_code = ixheaace_code_envelope(
          ptr_sfb_nrg_coupling[1], pstr_const_frame_info[1]->freq_res,
          &pstr_env_ch[1]->str_sbr_code_env, pstr_env_1->domain_vec, 1,
          pstr_const_frame_info[1]->n_envelopes, 1, pstr_sbr_bs->header_active,
          pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      c = 0;
      i = 0;
      while (i < n_envelopes[0]) {
        for (j = 0; j < pstr_env_0->no_scf_bands[i]; j++) {
          pstr_env_0->ienvelope[i][j] = ptr_sfb_nrg_coupling[0][c];
          pstr_env_1->ienvelope[i][j] = ptr_sfb_nrg_coupling[1][c];
          c++;
        }
        i++;
      }

      err_code = ixheaace_code_envelope(
          ptr_noise_lvl_coupling[0], res, &pstr_env_ch[0]->str_sbr_code_noise_floor,
          pstr_env_0->domain_vec_noise, 1, (pstr_const_frame_info[0]->n_envelopes > 1 ? 2 : 1), 0,
          pstr_sbr_bs->header_active, pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      for (i = 0; i < MAXIMUM_NUM_NOISE_VALUES; i++) {
        pstr_env_0->noise_level[i] = ptr_noise_lvl_coupling[0][i];
      }

      err_code = ixheaace_code_envelope(
          ptr_noise_lvl_coupling[1], res, &pstr_env_ch[1]->str_sbr_code_noise_floor,
          pstr_env_1->domain_vec_noise, 1, ((pstr_const_frame_info[1]->n_envelopes > 1) ? 2 : 1),
          1, pstr_sbr_bs->header_active, pstr_sbr_bs->usac_indep_flag, pstr_sbr_cfg->is_ld_sbr);
      if (err_code) {
        return err_code;
      }

      for (i = 0; i < MAXIMUM_NUM_NOISE_VALUES; i++) {
        pstr_env_1->noise_level[i] = ptr_noise_lvl_coupling[1][i];
      }

      pstr_sbr_hdr->coupling = 1;

      pstr_env_0->balance = 0;
      pstr_env_1->balance = 1;

      temp_flag_left = pstr_env_0->add_harmonic_flag;
      temp_flag_right = pstr_env_1->add_harmonic_flag;

      err_code = ixheaace_count_sbr_channel_pair_element(
          pstr_sbr_hdr, pstr_sbr_bs, &pstr_env_ch[0]->enc_env_data, &pstr_env_ch[1]->enc_env_data,
          pstr_com_data, ptr_sbr_tab, pstr_sbr_cfg->sbr_codec, pstr_sbr_cfg->is_esbr, &str_esbr,
          &payloadbits_coupling);
      if (err_code) {
        return err_code;
      }

      pstr_env_0->add_harmonic_flag = temp_flag_left;
      pstr_env_1->add_harmonic_flag = temp_flag_right;

      if (payloadbits_coupling < payloadbits_lr) {
        ch = 0;
        while (ch < num_channels) {
          memcpy(ptr_scale_factor_band_nrg[ch], ptr_sfb_nrg_coupling[ch],
                 MAXIMUM_NUM_ENVELOPE_VALUES * sizeof(ptr_scale_factor_band_nrg[0][0]));

          memcpy(ptr_noise_level[ch], ptr_noise_lvl_coupling[ch],
                 MAXIMUM_NUM_NOISE_VALUES * sizeof(ptr_noise_level[0][0]));
          ch++;
        }

        pstr_sbr_hdr->coupling = 1;
        pstr_env_0->balance = 0;
        pstr_env_1->balance = 1;
      } else {
        ch = 0;
        while (ch < num_channels) {
          memcpy(pstr_env_ch[ch]->str_sbr_code_env.sfb_nrg_prev,
                 scale_factor_band_nrg_prev_temp[ch],
                 MAXIMUM_FREQ_COEFFS * sizeof(scale_factor_band_nrg_prev_temp[0][0]));

          pstr_env_ch[ch]->str_sbr_code_env.update = up_date_nrg_temp[ch];

          memcpy(pstr_env_ch[ch]->str_sbr_code_noise_floor.sfb_nrg_prev, noise_prev_temp[ch],
                 MAXIMUM_NUM_NOISE_COEFFS * sizeof(noise_prev_temp[0][0]));

          memcpy(pstr_env_ch[ch]->enc_env_data.domain_vec, domain_vec_temp[ch],
                 sizeof(domain_vec_temp[0][0]) * IXHEAACE_MAX_ENV);

          memcpy(pstr_env_ch[ch]->enc_env_data.domain_vec_noise, domain_vec_noise_temp[ch],
                 sizeof(domain_vec_noise_temp[0][0]) * IXHEAACE_MAX_ENV);

          pstr_env_ch[ch]->str_sbr_code_noise_floor.update = up_date_noise_temp[ch];
          ch++;
        }

        pstr_sbr_hdr->coupling = 0;
        pstr_env_0->balance = 0;
        pstr_env_1->balance = 0;
      }
    } break;

  } /* end switch(stereo_mode) */

  if (num_channels == 1) {
    if (pstr_env_0->domain_vec[0] == TIME) {
      pstr_env_ch[0]->str_sbr_code_env.df_edge_incr_fac++;
    } else {
      pstr_env_ch[0]->str_sbr_code_env.df_edge_incr_fac = 0;
    }
  } else {
    if (pstr_env_0->domain_vec[0] == TIME || pstr_env_1->domain_vec[0] == TIME) {
      pstr_env_ch[0]->str_sbr_code_env.df_edge_incr_fac++;
      pstr_env_ch[1]->str_sbr_code_env.df_edge_incr_fac++;
    } else {
      pstr_env_ch[0]->str_sbr_code_env.df_edge_incr_fac = 0;
      pstr_env_ch[1]->str_sbr_code_env.df_edge_incr_fac = 0;
    }
  }

  for (ch = 0; ch < num_channels; ch++) {
    c = 0;
    i = 0;

    while (i < n_envelopes[ch]) {
      for (j = 0; j < pstr_env_ch[ch]->enc_env_data.no_scf_bands[i]; j++) {
        pstr_env_ch[ch]->enc_env_data.ienvelope[i][j] = ptr_scale_factor_band_nrg[ch][c];
        c++;
      }
      i++;
    }

    i = 0;
    while (i < MAXIMUM_NUM_NOISE_VALUES) {
      pstr_env_ch[ch]->enc_env_data.noise_level[i] = ptr_noise_level[ch][i];
      i++;
    }

  } /*end ch */

  // Pitch detection, pre-processing detection and oversampling decision making
  if ((1 == pstr_sbr_cfg->is_esbr) && (pstr_sbr_cfg->sbr_codec == HEAAC_SBR)) {
    err_code = ixheaace_update_esbr_ext_data(
        ptr_in_time, pstr_sbr_cfg, pstr_env_ch[0]->str_sbr_extract_env.ptr_r_buffer[0], &str_esbr,
        transient_info, ptr_sbr_tab, pstr_sbr_hdr->coupling, time_sn_stride, 2048);
    if (err_code) return err_code;
  }

  if (num_channels == 2) {
    WORD32 num_bits;
    pstr_env_0->usac_indep_flag = pstr_sbr_bs->usac_indep_flag;
    pstr_env_1->usac_indep_flag = pstr_sbr_bs->usac_indep_flag;
    err_code = ixheaace_write_env_channel_pair_element(
        pstr_sbr_hdr, pstr_sbr_bs, &pstr_env_ch[0]->enc_env_data, &pstr_env_ch[1]->enc_env_data,
        pstr_com_data, ptr_sbr_tab, pstr_sbr_cfg->sbr_codec, pstr_sbr_cfg->is_esbr, &str_esbr,
        &num_bits);
    if (err_code) {
      return err_code;
    }
  } else {
    WORD32 num_bits;
    pstr_env_0->sbr_sinusoidal_pos_flag = 0;
    pstr_env_0->usac_indep_flag = pstr_sbr_bs->usac_indep_flag;

    err_code = ixheaace_write_env_single_channel_element(
        pstr_sbr_hdr, pstr_sbr_bs, &pstr_env_ch[0]->enc_env_data, pstr_ps_enc, pstr_com_data,
        ptr_sbr_tab, pstr_sbr_cfg->sbr_codec, pstr_sbr_cfg->is_esbr, &str_esbr, &num_bits);
    if (err_code) {
      return err_code;
    }
  }

  ch = 0;
  while (ch < num_channels) {
    ixheaace_str_sbr_extr_env *pstr_sbr_extract_env = &(pstr_env_ch[ch]->str_sbr_extract_env);
    for (i = 0; i < pstr_sbr_extract_env->y_buffer_write_offset; i++) {
      FLOAT32 *ptr_tmp;
      ptr_tmp = pstr_sbr_extract_env->ptr_y_buffer[i];
      pstr_sbr_extract_env->ptr_y_buffer[i] =
          pstr_sbr_extract_env->ptr_y_buffer[i + (pstr_sbr_extract_env->no_cols >> 1)];
      pstr_sbr_extract_env->ptr_y_buffer[i + (pstr_sbr_extract_env->no_cols >> 1)] = ptr_tmp;
    }

    pstr_sbr_extract_env->buffer_flag ^= 1;
    ch++;
  } /* ch */

  pstr_sbr_hdr->prev_coupling = pstr_sbr_hdr->coupling;

  return err_code;
}
