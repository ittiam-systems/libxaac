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
/*****************************************************************************/
/* File includes                                                             */
/*****************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

extern "C" {
#include "ixheaac_type_def.h"
#include "impd_drc_common_enc.h"
#include "impd_drc_uni_drc.h"
#include "impd_drc_api.h"
#include "ixheaace_api.h"
}

pVOID malloc_global(UWORD32 size, UWORD32 alignment) { return malloc(size + alignment); }

VOID free_global(pVOID ptr) { free(ptr); }

static UWORD32 ixheaace_bound_check(WORD32 var, WORD32 lower_bound, WORD32 upper_bound) {
  var = MIN(var, upper_bound);
  var = MAX(var, lower_bound);
  return var;
}

static VOID ixheaace_read_drc_config_params(
    ia_drc_enc_params_struct *pstr_enc_params, ia_drc_uni_drc_config_struct *pstr_uni_drc_config,
    ia_drc_loudness_info_set_struct *pstr_enc_loudness_info_set,
    ia_drc_uni_drc_gain_ext_struct *pstr_enc_gain_extension, WORD8 *data) {
  WORD32 n, g, s, m, ch, p;
  WORD32 gain_set_channels;
  WORD8 *ptr_lcl = (WORD8 *)(data);
  FLOAT32 *flt_ptr;
  pstr_enc_params->delay_mode = *ptr_lcl++;
  pstr_uni_drc_config->sample_rate_present = *ptr_lcl++;
  pstr_uni_drc_config->str_drc_coefficients_uni_drc->drc_frame_size_present = *ptr_lcl++;
  pstr_uni_drc_config->loudness_info_set_present = *ptr_lcl++;

  pstr_uni_drc_config->drc_instructions_uni_drc_count =
      ixheaace_bound_check(*ptr_lcl++, 0, MAX_DRC_INSTRUCTIONS_COUNT);
  for (n = 0; n < pstr_uni_drc_config->drc_instructions_uni_drc_count; n++) {
    ia_drc_instructions_uni_drc *pstr_drc_instructions_uni_drc =
        &pstr_uni_drc_config->str_drc_instructions_uni_drc[n];
    pstr_drc_instructions_uni_drc->drc_set_id = *ptr_lcl++;
    pstr_drc_instructions_uni_drc->downmix_id = *ptr_lcl++;
    pstr_drc_instructions_uni_drc->additional_downmix_id_present = *ptr_lcl++;
    pstr_drc_instructions_uni_drc->additional_downmix_id_count =
        ixheaace_bound_check(*ptr_lcl++, 0, ADDITIONAL_DOWNMIX_ID_COUNT_MAX);
    pstr_drc_instructions_uni_drc->drc_location = *ptr_lcl++;
    pstr_drc_instructions_uni_drc->depends_on_drc_set_present = *ptr_lcl++;
    pstr_drc_instructions_uni_drc->depends_on_drc_set = *ptr_lcl++;
    pstr_drc_instructions_uni_drc->no_independent_use = *ptr_lcl++;
    pstr_drc_instructions_uni_drc->drc_set_effect = *ptr_lcl++;
    pstr_drc_instructions_uni_drc->drc_set_target_loudness_present = *ptr_lcl++;
    pstr_drc_instructions_uni_drc->drc_set_target_loudness_value_upper = *ptr_lcl++;
    pstr_drc_instructions_uni_drc->drc_set_target_loudness_value_lower_present = *ptr_lcl++;
    pstr_drc_instructions_uni_drc->drc_set_target_loudness_value_lower = *ptr_lcl++;

    gain_set_channels = ixheaace_bound_check(*ptr_lcl++, 0, 32);
    for (ch = 0; ch < gain_set_channels; ch++) {
      pstr_drc_instructions_uni_drc->gain_set_index[ch] =
          ixheaace_bound_check(*ptr_lcl++, 0, GAIN_SET_COUNT_MAX - 1);
    }
    for (; ch < MAX_CHANNEL_COUNT; ch++) {
      if (gain_set_channels > 0) {
        pstr_drc_instructions_uni_drc->gain_set_index[ch] =
            pstr_drc_instructions_uni_drc->gain_set_index[gain_set_channels - 1];
      } else {
        pstr_drc_instructions_uni_drc->gain_set_index[ch] = 0;
      }
    }

    pstr_drc_instructions_uni_drc->num_drc_channel_groups =
        ixheaace_bound_check(*ptr_lcl++, 0, MAX_CHANNEL_GROUP_COUNT);
    for (g = 0; g < pstr_drc_instructions_uni_drc->num_drc_channel_groups; g++) {
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].gain_scaling_present[0] = *ptr_lcl++;
      flt_ptr = (FLOAT32 *)ptr_lcl;
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].attenuation_scaling[0] = *flt_ptr;
      ptr_lcl += 4;
      flt_ptr = (FLOAT32 *)ptr_lcl;
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].amplification_scaling[0] = *flt_ptr;
      ptr_lcl += 4;
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].gain_offset_present[0] = *ptr_lcl++;
      flt_ptr = (FLOAT32 *)ptr_lcl;
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].gain_offset[0] = *flt_ptr;
      ptr_lcl += 4;
    }

    pstr_drc_instructions_uni_drc->limiter_peak_target_present = *ptr_lcl++;
    flt_ptr = (FLOAT32 *)ptr_lcl;
    pstr_drc_instructions_uni_drc->limiter_peak_target = *flt_ptr;
    ptr_lcl += 4;
    pstr_drc_instructions_uni_drc->drc_instructions_type = *ptr_lcl++;
    pstr_drc_instructions_uni_drc->mae_group_id = *ptr_lcl++;
    pstr_drc_instructions_uni_drc->mae_group_preset_id = *ptr_lcl++;
  }

  pstr_uni_drc_config->drc_coefficients_uni_drc_count =
      ixheaace_bound_check(*ptr_lcl++, 0, MAX_DRC_COEFF_COUNT);
  for (n = 0; n < pstr_uni_drc_config->drc_coefficients_uni_drc_count; n++) {
    ia_drc_coefficients_uni_drc_struct *pstr_drc_coefficients_uni_drc =
        &pstr_uni_drc_config->str_drc_coefficients_uni_drc[n];
    pstr_drc_coefficients_uni_drc->drc_location = *ptr_lcl++;
    pstr_drc_coefficients_uni_drc->gain_set_count =
        ixheaace_bound_check(*ptr_lcl++, 0, GAIN_SET_COUNT_MAX);
    for (s = 0; s < pstr_drc_coefficients_uni_drc->gain_set_count; s++) {
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_coding_profile = *ptr_lcl++;
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_interpolation_type = *ptr_lcl++;
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].full_frame = *ptr_lcl++;
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].time_alignment = *ptr_lcl++;
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].time_delta_min_present = *ptr_lcl++;
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].band_count =
          ixheaace_bound_check(*ptr_lcl++, 0, MAX_BAND_COUNT);
      if (pstr_drc_coefficients_uni_drc->str_gain_set_params[s].band_count == 1) {
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].nb_points =
            ixheaace_bound_check(*ptr_lcl++, 0, 5);
        for (p = 0;
             p < pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].nb_points;
             p++) {
          flt_ptr = (FLOAT32 *)ptr_lcl;
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].gain_points[p].x =
              *flt_ptr;
          ptr_lcl += 4;
          flt_ptr = (FLOAT32 *)ptr_lcl;
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].gain_points[p].y =
              *flt_ptr;
          ptr_lcl += 4;
        }
        flt_ptr = (FLOAT32 *)ptr_lcl;
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].width = *flt_ptr;
        ptr_lcl += 4;
        flt_ptr = (FLOAT32 *)ptr_lcl;
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].attack = *flt_ptr;
        ptr_lcl += 4;
        flt_ptr = (FLOAT32 *)ptr_lcl;
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].decay = *flt_ptr;
        ptr_lcl += 4;
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].drc_characteristic =
            *ptr_lcl++;
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
            .gain_params[0]
            .crossover_freq_index = *ptr_lcl++;
      } else {
        for (m = 0; m < pstr_drc_coefficients_uni_drc->str_gain_set_params[s].band_count; m++) {
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].nb_points =
              ixheaace_bound_check(*ptr_lcl++, 0, 5);
          for (p = 0;
               p < pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].nb_points;
               p++) {
            flt_ptr = (FLOAT32 *)ptr_lcl;
            pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
                .gain_params[m]
                .gain_points[p]
                .x = *flt_ptr;
            ptr_lcl += 4;
            flt_ptr = (FLOAT32 *)ptr_lcl;
            pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
                .gain_params[m]
                .gain_points[p]
                .y = *flt_ptr;
            ptr_lcl += 4;
          }
          flt_ptr = (FLOAT32 *)ptr_lcl;
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].width = *flt_ptr;
          ptr_lcl += 4;
          flt_ptr = (FLOAT32 *)ptr_lcl;
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].attack = *flt_ptr;
          ptr_lcl += 4;
          flt_ptr = (FLOAT32 *)ptr_lcl;
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].decay = *flt_ptr;
          ptr_lcl += 4;
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].drc_band_type = *ptr_lcl++;
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
              .gain_params[m]
              .start_sub_band_index = *ptr_lcl++;
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
              .gain_params[m]
              .drc_characteristic = *ptr_lcl++;
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
              .gain_params[m]
              .crossover_freq_index = *ptr_lcl++;
        }
      }
    }
  }

  pstr_enc_loudness_info_set->loudness_info_count =
      ixheaace_bound_check(*ptr_lcl++, 0, MAX_LOUDNESS_INFO_COUNT);
  for (n = 0; n < pstr_enc_loudness_info_set->loudness_info_count; n++) {
    pstr_enc_loudness_info_set->str_loudness_info[n].drc_set_id = *ptr_lcl++;
    pstr_enc_loudness_info_set->str_loudness_info[n].downmix_id = *ptr_lcl++;
    pstr_enc_loudness_info_set->str_loudness_info[n].sample_peak_level_present = *ptr_lcl++;
    if (1 == pstr_enc_loudness_info_set->str_loudness_info[n].sample_peak_level_present) {
      flt_ptr = (FLOAT32 *)ptr_lcl;
      pstr_enc_loudness_info_set->str_loudness_info[n].sample_peak_level = *flt_ptr;
      ptr_lcl += 4;
    }
    pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level_present = *ptr_lcl++;
    if (1 == pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level_present) {
      flt_ptr = (FLOAT32 *)ptr_lcl;
      pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level = *flt_ptr;
      ptr_lcl += 4;
      pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level_measurement_system =
          *ptr_lcl++;
      pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level_reliability = *ptr_lcl++;
    }
    pstr_enc_loudness_info_set->str_loudness_info[n].measurement_count =
        ixheaace_bound_check(*ptr_lcl++, 0, MAX_MEASUREMENT_COUNT);
    for (m = 0; m < pstr_enc_loudness_info_set->str_loudness_info[n].measurement_count; m++) {
      pstr_enc_loudness_info_set->str_loudness_info[n].str_loudness_measure[m].method_definition =
          *ptr_lcl++;
      flt_ptr = (FLOAT32 *)ptr_lcl;
      pstr_enc_loudness_info_set->str_loudness_info[n].str_loudness_measure[m].method_value =
          *flt_ptr;
      ptr_lcl += 4;
      pstr_enc_loudness_info_set->str_loudness_info[n]
          .str_loudness_measure[m]
          .measurement_system = *ptr_lcl++;
      pstr_enc_loudness_info_set->str_loudness_info[n].str_loudness_measure[m].reliability =
          *ptr_lcl++;
    }
  }

  pstr_enc_loudness_info_set->loudness_info_album_count =
      ixheaace_bound_check(*ptr_lcl++, 0, MAX_LOUDNESS_INFO_COUNT);
  for (n = 0; n < pstr_enc_loudness_info_set->loudness_info_album_count; n++) {
    pstr_enc_loudness_info_set->str_loudness_info_album[n].drc_set_id = *ptr_lcl++;
    pstr_enc_loudness_info_set->str_loudness_info_album[n].downmix_id = *ptr_lcl++;
    pstr_enc_loudness_info_set->str_loudness_info_album[n].sample_peak_level_present = *ptr_lcl++;
    if (1 == pstr_enc_loudness_info_set->str_loudness_info_album[n].sample_peak_level_present) {
      flt_ptr = (FLOAT32 *)ptr_lcl;
      pstr_enc_loudness_info_set->str_loudness_info_album[n].sample_peak_level = *flt_ptr;
      ptr_lcl += 4;
    }
    pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level_present = *ptr_lcl++;
    if (1 == pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level_present) {
      flt_ptr = (FLOAT32 *)ptr_lcl;
      pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level = *flt_ptr;
      ptr_lcl += 4;
      pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level_measurement_system =
          *ptr_lcl++;
      pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level_reliability =
          *ptr_lcl++;
    }
    pstr_enc_loudness_info_set->str_loudness_info_album[n].measurement_count =
        ixheaace_bound_check(*ptr_lcl++, 0, MAX_MEASUREMENT_COUNT);
    for (m = 0; m < pstr_enc_loudness_info_set->str_loudness_info_album[n].measurement_count;
         m++) {
      pstr_enc_loudness_info_set->str_loudness_info_album[n]
          .str_loudness_measure[m]
          .method_definition = *ptr_lcl++;
      flt_ptr = (FLOAT32 *)ptr_lcl;
      pstr_enc_loudness_info_set->str_loudness_info_album[n]
          .str_loudness_measure[m]
          .method_value = *flt_ptr;
      ptr_lcl += 4;
      pstr_enc_loudness_info_set->str_loudness_info_album[n]
          .str_loudness_measure[m]
          .measurement_system = *ptr_lcl++;
      pstr_enc_loudness_info_set->str_loudness_info_album[n].str_loudness_measure[m].reliability =
          *ptr_lcl++;
    }
  }

  pstr_uni_drc_config->str_channel_layout.layout_signaling_present = *ptr_lcl++;
  pstr_uni_drc_config->str_channel_layout.defined_layout = *ptr_lcl++;
  pstr_uni_drc_config->str_channel_layout.speaker_position[0] = *ptr_lcl++;

  pstr_uni_drc_config->downmix_instructions_count =
      ixheaace_bound_check(*ptr_lcl++, 0, MAX_DOWNMIX_INSTRUCTION_COUNT);
  for (n = 0; n < pstr_uni_drc_config->downmix_instructions_count; n++) {
    pstr_uni_drc_config->str_downmix_instructions[n].target_layout = *ptr_lcl++;
    ;
    pstr_uni_drc_config->str_downmix_instructions[n].downmix_coefficients_present = *ptr_lcl++;
  }

  pstr_uni_drc_config->drc_description_basic_present = *ptr_lcl++;
  pstr_uni_drc_config->uni_drc_config_ext_present = *ptr_lcl++;
  pstr_enc_loudness_info_set->loudness_info_set_ext_present = *ptr_lcl++;
  pstr_enc_gain_extension->uni_drc_gain_ext_present = *ptr_lcl++;
}

static VOID ixheaace_fuzzer_flag(ixheaace_input_config *pstr_in_cfg, WORD8 *data, WORD32 size) {
  // Set Default value for AAC config structure
  memset(pstr_in_cfg, 0, sizeof(*pstr_in_cfg));

  pstr_in_cfg->i_bitrate = *(WORD32 *)(data);
  pstr_in_cfg->i_use_mps = *(WORD8 *)(data + 4);
  pstr_in_cfg->i_use_adts = *(WORD8 *)(data + 5);
  pstr_in_cfg->i_use_es = *(WORD8 *)(data + 6);
  pstr_in_cfg->aac_config.use_tns = *(WORD8 *)(data + 7);
  pstr_in_cfg->aac_config.noise_filling = *(WORD8 *)(data + 8);
  pstr_in_cfg->ui_pcm_wd_sz = *(WORD8 *)(data + 9);
  pstr_in_cfg->i_channels = *(WORD8 *)(data + 10);
  pstr_in_cfg->i_samp_freq = *(WORD32 *)(data + 11);
  pstr_in_cfg->frame_length = *(WORD16 *)(data + 15);
  pstr_in_cfg->aot = *(WORD8 *)(data + 16);
  pstr_in_cfg->esbr_flag = *(WORD8 *)(data + 17);
  pstr_in_cfg->aac_config.full_bandwidth = *(WORD8 *)(data + 18);
  pstr_in_cfg->aac_config.bitreservoir_size = *(WORD16 *)(data + 19);
  pstr_in_cfg->i_mps_tree_config = *(WORD8 *)(data + 20);
  pstr_in_cfg->i_channels_mask = *(WORD8 *)(data + 21);
  pstr_in_cfg->cplx_pred = *(WORD8 *)(data + 22);
  pstr_in_cfg->usac_en = *(WORD8 *)(data + 23);
  pstr_in_cfg->ccfl_idx = *(WORD8 *)(data + 24);
  pstr_in_cfg->pvc_active = *(WORD8 *)(data + 25);
  pstr_in_cfg->harmonic_sbr = *(WORD8 *)(data + 26);
  pstr_in_cfg->hq_esbr = *(WORD8 *)(data + 27);
  pstr_in_cfg->use_drc_element = *(WORD8 *)(data + 28);
  pstr_in_cfg->inter_tes_active = *(WORD8 *)(data + 39);
  pstr_in_cfg->codec_mode = *(WORD8 *)(data + 30);

  /* DRC */
  if (pstr_in_cfg->use_drc_element == 1) {
    LOOPIDX k;
    if (size <= 17600)
      pstr_in_cfg->use_drc_element = 0;
    else {
      memset(&pstr_in_cfg->str_drc_cfg, 0, sizeof(ia_drc_input_config));
      ixheaace_read_drc_config_params(
          &pstr_in_cfg->str_drc_cfg.str_enc_params, &pstr_in_cfg->str_drc_cfg.str_uni_drc_config,
          &pstr_in_cfg->str_drc_cfg.str_enc_loudness_info_set,
          &pstr_in_cfg->str_drc_cfg.str_enc_gain_extension, (WORD8 *)(data + 31));

      pstr_in_cfg->str_drc_cfg.str_enc_params.gain_sequence_present = 0;
      for (k = 0; k < pstr_in_cfg->str_drc_cfg.str_uni_drc_config.drc_coefficients_uni_drc_count;
           k++) {
        if (pstr_in_cfg->str_drc_cfg.str_uni_drc_config.str_drc_coefficients_uni_drc[k]
                .drc_location == 1) {
          if (pstr_in_cfg->str_drc_cfg.str_uni_drc_config.str_drc_coefficients_uni_drc[k]
                  .gain_set_count > 0) {
            pstr_in_cfg->str_drc_cfg.str_enc_params.gain_sequence_present = 1;
            break;
          }
        }
      }

      if (pstr_in_cfg->str_drc_cfg.str_enc_params.gain_sequence_present == 0) {
        for (k = 0; k < pstr_in_cfg->str_drc_cfg.str_uni_drc_config.str_uni_drc_config_ext
                            .drc_coefficients_uni_drc_v1_count;
             k++) {
          if (pstr_in_cfg->str_drc_cfg.str_uni_drc_config.str_uni_drc_config_ext
                  .str_drc_coefficients_uni_drc_v1[k]
                  .drc_location == 1) {
            if (pstr_in_cfg->str_drc_cfg.str_uni_drc_config.str_uni_drc_config_ext
                    .str_drc_coefficients_uni_drc_v1[k]
                    .gain_sequence_count > 0) {
              pstr_in_cfg->str_drc_cfg.str_enc_params.gain_sequence_present = 1;
              break;
            }
          }
        }
      }
    }
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // Need atleast 300 bytes for processing
  if (size <= 300) {
    return 0;
  }

  /* Error code */
  IA_ERRORCODE err_code = 0;

  /* API obj */
  pVOID pv_ia_process_api_obj;

  pWORD8 pb_inp_buf = NULL;
  WORD32 i_bytes_read = 0;
  WORD32 input_size = 0;

  /* ******************************************************************/
  /* The API config structure                                         */
  /* ******************************************************************/

  ixheaace_user_config_struct *pstr_enc_api =
      (ixheaace_user_config_struct *)malloc_global(sizeof(ixheaace_user_config_struct), 0);
  memset(pstr_enc_api, 0, sizeof(ixheaace_user_config_struct));
  ixheaace_input_config *pstr_in_cfg = &pstr_enc_api->input_config;
  ixheaace_output_config *pstr_out_cfg = &pstr_enc_api->output_config;
  ixheaace_input_config *pstr_in_cfg_user =
      (ixheaace_input_config *)malloc_global(sizeof(ixheaace_input_config), 0);

  pstr_out_cfg->malloc_xheaace = &malloc_global;
  pstr_out_cfg->free_xheaace = &free_global;

  /* ******************************************************************/
  /* Parse input configuration parameters                             */
  /* ******************************************************************/
  //////FUZZER////
  WORD32 file_data_size;
  WORD32 bytes_consumed;
  WORD32 data_size_left = 0;
  file_data_size = size;
  data_size_left = file_data_size;
  bytes_consumed = 0;

  ixheaace_fuzzer_flag(&pstr_enc_api->input_config, (WORD8 *)data, size);
  bytes_consumed = 31;

  {
    if (pstr_in_cfg->aot == AOT_AAC_LC || pstr_in_cfg->aot == AOT_SBR ||
        pstr_in_cfg->aot == AOT_PS) {
      if (pstr_in_cfg->frame_cmd_flag == 0) {
        pstr_in_cfg->frame_length = 1024;
      }
      if (pstr_in_cfg->out_bytes_flag == 0) {
        pstr_in_cfg->aac_config.bitreservoir_size = 768;
      }
    } else if (pstr_in_cfg->aot == AOT_AAC_LD || pstr_in_cfg->aot == AOT_AAC_ELD) {
      if (pstr_in_cfg->frame_cmd_flag == 0) {
        pstr_in_cfg->frame_length = 512;
      }
      if (pstr_in_cfg->out_bytes_flag == 0) {
        pstr_in_cfg->aac_config.bitreservoir_size = 384;
      }
    }
  }

  if (!(pstr_in_cfg->i_use_adts)) {
    pstr_in_cfg->i_use_es = 1;
  } else {
    pstr_in_cfg->i_use_es = 0;
  }

  if (pstr_in_cfg->i_use_es && pstr_in_cfg->i_use_adts) {
    // Give preference to MP4
    pstr_in_cfg->i_use_adts = 0;
    pstr_in_cfg->i_use_es = 1;
  }

  data = data + bytes_consumed;
  data_size_left = data_size_left - bytes_consumed;

  err_code = ixheaace_create((pVOID)pstr_in_cfg, (pVOID)pstr_out_cfg);
  if (err_code) {
    if (pstr_in_cfg_user) {
      free(pstr_in_cfg_user);
    }
    if (pstr_enc_api) {
      free(pstr_enc_api);
    }
    return err_code;
  }

  pv_ia_process_api_obj = pstr_out_cfg->pv_ia_process_api_obj;
  pb_inp_buf = (pWORD8)pstr_out_cfg->mem_info_table[IA_MEMTYPE_INPUT].mem_ptr;

  /* End first part */

  /* Second part        */
  /* Initialize process */
  /* Get config params  */

  input_size = pstr_out_cfg->input_size;

  if (data) {
    if (data_size_left > input_size) {
      i_bytes_read = input_size;
    } else if (data_size_left <= 0) {
      i_bytes_read = 0;
    } else {
      i_bytes_read = data_size_left;
      input_size = data_size_left;
    }

    if (i_bytes_read > 0) {
      memcpy(pb_inp_buf, data, input_size);
      bytes_consumed = bytes_consumed + input_size;
      data_size_left = data_size_left - input_size;
      data = data + input_size;
    }
  }

  while (i_bytes_read) {
    if (i_bytes_read != input_size) {
      memset((pb_inp_buf + i_bytes_read), 0, (input_size - i_bytes_read));
    }

    ixheaace_process(pv_ia_process_api_obj, (pVOID)pstr_in_cfg, (pVOID)pstr_out_cfg);

    if (data) {
      if (data_size_left > input_size) {
        i_bytes_read = input_size;
      } else if (data_size_left <= 0) {
        i_bytes_read = 0;
      } else {
        i_bytes_read = data_size_left;
        input_size = data_size_left;
      }

      if (i_bytes_read > 0) {
        memcpy(pb_inp_buf, data, input_size);
        bytes_consumed = bytes_consumed + input_size;
        data_size_left = data_size_left - input_size;
        data = data + input_size;
      }
    }
  }

  ixheaace_delete((pVOID)pstr_out_cfg);

  if (pstr_in_cfg_user) {
    free(pstr_in_cfg_user);
  }
  if (pstr_enc_api) {
    free(pstr_enc_api);
  }
  return 0;
}

/* End ia_main_process() */
