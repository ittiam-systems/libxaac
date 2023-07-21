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

#include "ixheaac_type_def.h"
#include "ixheaac_error_standards.h"
#include "ixheaace_error_codes.h"

#include "iusace_bitbuffer.h"
#include "iusace_cnst.h"
#include "impd_drc_common_enc.h"
#include "impd_drc_uni_drc.h"
#include "impd_drc_tables.h"
#include "impd_drc_api.h"
#include "impd_drc_uni_drc_eq.h"
#include "impd_drc_uni_drc_filter_bank.h"
#include "impd_drc_gain_enc.h"
#include "impd_drc_struct_def.h"
#include "impd_drc_enc.h"

static UWORD32 impd_drc_bound_check(WORD32 var, WORD32 lower_bound, WORD32 upper_bound) {
  var = MIN(var, upper_bound);
  var = MAX(var, lower_bound);
  return var;
}

static VOID impd_drc_validate_config_params(ia_drc_input_config *pstr_inp_config) {
  LOOPIDX i, j, k;
  ia_drc_uni_drc_config_struct *pstr_uni_drc_config = &pstr_inp_config->str_uni_drc_config;
  ia_drc_loudness_info_set_struct *pstr_enc_loudness_info_set =
      &pstr_inp_config->str_enc_loudness_info_set;

  pstr_uni_drc_config->drc_instructions_uni_drc_count = impd_drc_bound_check(
      pstr_uni_drc_config->drc_instructions_uni_drc_count, 0, MAX_DRC_INSTRUCTIONS_COUNT);
  for (i = 0; i < pstr_uni_drc_config->drc_instructions_uni_drc_count; i++) {
    pstr_uni_drc_config->str_drc_instructions_uni_drc[i].additional_downmix_id_count =
        impd_drc_bound_check(
            pstr_uni_drc_config->str_drc_instructions_uni_drc[i].additional_downmix_id_count, 0,
            ADDITIONAL_DOWNMIX_ID_COUNT_MAX);
    for (j = 0; j < MAX_CHANNEL_COUNT; j++) {
      pstr_uni_drc_config->str_drc_instructions_uni_drc[i].gain_set_index[j] =
          impd_drc_bound_check(
              pstr_uni_drc_config->str_drc_instructions_uni_drc[i].gain_set_index[j], 0,
              GAIN_SET_COUNT_MAX - 1);
    }
    pstr_uni_drc_config->str_drc_instructions_uni_drc[i].num_drc_channel_groups =
        impd_drc_bound_check(
            pstr_uni_drc_config->str_drc_instructions_uni_drc[i].num_drc_channel_groups, 0,
            MAX_CHANNEL_GROUP_COUNT);
  }
  pstr_uni_drc_config->drc_coefficients_uni_drc_count = impd_drc_bound_check(
      pstr_uni_drc_config->drc_coefficients_uni_drc_count, 0, MAX_DRC_COEFF_COUNT);
  for (i = 0; i < pstr_uni_drc_config->drc_coefficients_uni_drc_count; i++) {
    pstr_uni_drc_config->str_drc_coefficients_uni_drc[i].gain_set_count =
        impd_drc_bound_check(pstr_uni_drc_config->str_drc_coefficients_uni_drc[i].gain_set_count,
                             0, MAX_CHANNEL_GROUP_COUNT);
    for (j = 0; j < pstr_uni_drc_config->str_drc_coefficients_uni_drc[i].gain_set_count; j++) {
      pstr_uni_drc_config->str_drc_coefficients_uni_drc[i]
          .str_gain_set_params[j]
          .band_count = impd_drc_bound_check(
          pstr_uni_drc_config->str_drc_coefficients_uni_drc[i].str_gain_set_params[j].band_count,
          0, MAX_BAND_COUNT);
      for (k = 0;
           k <
           pstr_uni_drc_config->str_drc_coefficients_uni_drc[i].str_gain_set_params[j].band_count;
           k++) {
        pstr_uni_drc_config->str_drc_coefficients_uni_drc[i]
            .str_gain_set_params[j]
            .gain_params[k]
            .nb_points = impd_drc_bound_check(pstr_uni_drc_config->str_drc_coefficients_uni_drc[i]
                                                  .str_gain_set_params[j]
                                                  .gain_params[k]
                                                  .nb_points,
                                              0, MAX_GAIN_POINTS);
      }
    }
  }
  pstr_uni_drc_config->downmix_instructions_count = impd_drc_bound_check(
      pstr_uni_drc_config->downmix_instructions_count, 0, MAX_DOWNMIX_INSTRUCTION_COUNT);

  pstr_enc_loudness_info_set->loudness_info_count = impd_drc_bound_check(
      pstr_enc_loudness_info_set->loudness_info_count, 0, MAX_LOUDNESS_INFO_COUNT);
  for (i = 0; i < pstr_enc_loudness_info_set->loudness_info_count; i++) {
    pstr_enc_loudness_info_set->str_loudness_info[i].measurement_count =
        impd_drc_bound_check(pstr_enc_loudness_info_set->str_loudness_info[i].measurement_count,
                             0, MAX_MEASUREMENT_COUNT);
  }
  pstr_enc_loudness_info_set->loudness_info_album_count = impd_drc_bound_check(
      pstr_enc_loudness_info_set->loudness_info_album_count, 0, MAX_LOUDNESS_INFO_COUNT);
  for (i = 0; i < pstr_enc_loudness_info_set->loudness_info_album_count; i++) {
    pstr_enc_loudness_info_set->str_loudness_info_album[i].measurement_count =
        impd_drc_bound_check(
            pstr_enc_loudness_info_set->str_loudness_info_album[i].measurement_count, 0,
            MAX_MEASUREMENT_COUNT);
  }
}

static IA_ERRORCODE impd_drc_validate_drc_instructions(
    ia_drc_uni_drc_config_struct *pstr_uni_drc_config) {
  LOOPIDX i, j;
  WORD32 profile_found = FALSE;

  for (i = 0; i < pstr_uni_drc_config->drc_instructions_uni_drc_count; i++) {
    profile_found = FALSE;
    for (j = 0; j < pstr_uni_drc_config->drc_coefficients_uni_drc_count; j++) {
      if (pstr_uni_drc_config->str_drc_coefficients_uni_drc[j].drc_location == 1) {
        profile_found = TRUE;
        break;
      }
    }
    if (pstr_uni_drc_config->uni_drc_config_ext_present &&
        pstr_uni_drc_config->str_uni_drc_config_ext.parametric_drc_present &&
        pstr_uni_drc_config->str_uni_drc_config_ext.str_drc_coeff_parametric_drc.drc_location ==
            1) {
      profile_found = TRUE;
    }
    if (profile_found == FALSE) {
      return IA_EXHEAACE_CONFIG_FATAL_DRC_INVALID_CONFIG;
    }
  }

  return IA_NO_ERROR;
}

IA_ERRORCODE impd_drc_enc_init(VOID *pstr_drc_state, VOID *ptr_drc_scratch,
                               ia_drc_input_config *pstr_inp_config) {
  IA_ERRORCODE err_code = IA_NO_ERROR;
  WORD32 bit_count = 0;
  ia_drc_enc_state *pstr_drc_state_local = pstr_drc_state;

#ifdef ENABLE_SET_JUMP
  jmp_buf drc_enc_init_jmp_buf;
  err_code = setjmp(drc_enc_init_jmp_buf);
  if (err_code != IA_NO_ERROR) {
    return IA_EXHEAACE_INIT_FATAL_DRC_INSUFFICIENT_WRITE_BUFFER_SIZE;
  }
#endif  // ENABLE_SET_JUMP

  pstr_drc_state_local->drc_scratch_mem = ptr_drc_scratch;
  pstr_drc_state_local->drc_scratch_used = 0;

  iusace_create_bit_buffer(&pstr_drc_state_local->str_bit_buf_cfg,
                           pstr_drc_state_local->bit_buf_base_cfg,
                           sizeof(pstr_drc_state_local->bit_buf_base_cfg), 1);

  iusace_create_bit_buffer(&pstr_drc_state_local->str_bit_buf_cfg_ext,
                           pstr_drc_state_local->bit_buf_base_cfg_ext,
                           sizeof(pstr_drc_state_local->bit_buf_base_cfg_ext), 1);

  iusace_create_bit_buffer(&pstr_drc_state_local->str_bit_buf_cfg_tmp,
                           pstr_drc_state_local->bit_buf_base_cfg_tmp,
                           sizeof(pstr_drc_state_local->bit_buf_base_cfg_tmp), 1);

  iusace_create_bit_buffer(&pstr_drc_state_local->str_bit_buf_out,
                           pstr_drc_state_local->bit_buf_base_out,
                           sizeof(pstr_drc_state_local->bit_buf_base_out), 1);

#ifdef ENABLE_SET_JUMP
  pstr_drc_state_local->str_bit_buf_cfg.impd_drc_jmp_buf = &drc_enc_init_jmp_buf;
  pstr_drc_state_local->str_bit_buf_cfg_ext.impd_drc_jmp_buf = &drc_enc_init_jmp_buf;
  pstr_drc_state_local->str_bit_buf_cfg_tmp.impd_drc_jmp_buf = &drc_enc_init_jmp_buf;
  pstr_drc_state_local->str_bit_buf_out.impd_drc_jmp_buf = &drc_enc_init_jmp_buf;
#endif  // ENABLE_SET_JUMP

  impd_drc_validate_config_params(pstr_inp_config);

  err_code = impd_drc_gain_enc_init(
      &pstr_drc_state_local->str_gain_enc, &pstr_inp_config->str_uni_drc_config,
      &pstr_inp_config->str_enc_loudness_info_set, pstr_inp_config->str_enc_params.frame_size,
      pstr_inp_config->str_enc_params.sample_rate, pstr_inp_config->str_enc_params.delay_mode,
      pstr_inp_config->str_enc_params.domain);
  if (err_code & IA_FATAL_ERROR) {
    return IA_EXHEAACE_CONFIG_FATAL_DRC_INVALID_CONFIG;
  }

  pstr_drc_state_local->str_enc_params = pstr_inp_config->str_enc_params;
  pstr_drc_state_local->str_uni_drc_config = pstr_inp_config->str_uni_drc_config;
  pstr_drc_state_local->str_enc_gain_extension = pstr_inp_config->str_enc_gain_extension;

  err_code = impd_drc_validate_drc_instructions(&pstr_inp_config->str_uni_drc_config);
  if (err_code & IA_FATAL_ERROR) {
    return IA_EXHEAACE_CONFIG_FATAL_DRC_INVALID_CONFIG;
  }

  err_code = impd_drc_write_uni_drc_config(pstr_drc_state_local, &bit_count);
  if (err_code & IA_FATAL_ERROR) {
    return err_code;
  }

  pstr_drc_state_local->drc_config_data_size_bit = bit_count;

  return err_code;
}

VOID impd_drc_enc(VOID *pstr_drc_state, FLOAT32 **pptr_input, UWORD32 inp_offset,
                  WORD32 *ptr_bits_written, VOID *pstr_scratch) {
  LOOPIDX i, j, k;
  WORD32 band_count = 0;
  WORD32 stop_sub_band_index;
  WORD32 num_bits_payload = 0;
  UWORD8 is_fft_ready[MAX_NUM_CHANNELS] = {0};
  ia_drc_enc_state *pstr_drc_state_local = pstr_drc_state;
  ia_drc_gain_enc_struct *pstr_gain_enc = &pstr_drc_state_local->str_gain_enc;
  ia_drc_uni_drc_config_struct *pstr_uni_drc_config = &pstr_drc_state_local->str_uni_drc_config;
  ia_drc_compand_struct *pstr_drc_compand;
  ia_drc_stft_gain_calc_struct *pstr_drc_stft_gain_calc;

  if (pstr_drc_state_local->str_enc_params.gain_sequence_present) {
    for (i = 0; i < MAX_DRC_COEFF_COUNT; i++) {
      for (j = 0; j < GAIN_SET_COUNT_MAX; j++) {
        pstr_drc_stft_gain_calc = &pstr_gain_enc->str_drc_stft_gain_handle[i][j][0];
        pstr_drc_compand = &pstr_gain_enc->str_drc_compand[i][j];
        if ((pstr_drc_compand->is_valid == 0) && (pstr_drc_stft_gain_calc->is_valid == 0)) {
          break;
        }

        if (pstr_drc_compand->is_valid == 0) {
          if (is_fft_ready[pstr_drc_stft_gain_calc->ch_idx] == 0) {
            impd_drc_stft_drc_convert_to_fd(
                pstr_gain_enc, &pptr_input[pstr_drc_stft_gain_calc->ch_idx][inp_offset],
                pstr_drc_stft_gain_calc->ch_idx, pstr_drc_state_local->str_enc_params.frame_size,
                pstr_gain_enc->complex_fft_ptr[pstr_drc_stft_gain_calc->ch_idx], pstr_scratch);
            is_fft_ready[pstr_drc_stft_gain_calc->ch_idx] = 1;
          }

          for (k = 0; k < pstr_uni_drc_config->str_drc_coefficients_uni_drc[i]
                              .str_gain_set_params[j]
                              .band_count;
               k++) {
            if (k == pstr_uni_drc_config->str_drc_coefficients_uni_drc[i]
                             .str_gain_set_params[j]
                             .band_count -
                         1) {
              stop_sub_band_index = STFT256_HOP_SIZE - 1;
            } else {
              stop_sub_band_index = pstr_uni_drc_config->str_drc_coefficients_uni_drc[i]
                                        .str_gain_set_params[j]
                                        .gain_params[k + 1]
                                        .start_sub_band_index -
                                    1;
            }

            impd_drc_stft_drc_gain_calc_process(
                pstr_gain_enc, i, j, k,
                pstr_uni_drc_config->str_drc_coefficients_uni_drc[i]
                    .str_gain_set_params[j]
                    .gain_params[k]
                    .start_sub_band_index,
                stop_sub_band_index, pstr_drc_state_local->str_enc_params.frame_size,
                pstr_gain_enc->complex_fft_ptr[pstr_drc_stft_gain_calc->ch_idx],
                pstr_drc_state_local->gain_buffer[band_count + k]);
          }
        } else {
          impd_drc_td_drc_gain_calc_process(pstr_gain_enc, i, j,
                                            pstr_drc_state_local->str_enc_params.frame_size,
                                            &pptr_input[pstr_drc_compand->ch_idx][inp_offset],
                                            pstr_drc_state_local->gain_buffer[band_count]);
        }

        band_count += pstr_uni_drc_config->str_drc_coefficients_uni_drc[i]
                          .str_gain_set_params[j]
                          .band_count;
      }
    }
  }
  impd_drc_encode_uni_drc_gain(pstr_gain_enc, pstr_drc_state_local->gain_buffer[0], pstr_scratch);

  if (pstr_drc_state_local->is_first_drc_process_complete == 1) {
    impd_drc_write_uni_drc_gain(pstr_drc_state_local, &num_bits_payload);
  }

  *ptr_bits_written = num_bits_payload;
}
