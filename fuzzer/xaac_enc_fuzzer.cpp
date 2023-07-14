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
#include "ixheaace_aac_constants.h"
#include "ixheaac_error_standards.h"
#include "impd_drc_common_enc.h"
#include "impd_drc_uni_drc.h"
#include "impd_drc_tables.h"
#include "impd_drc_api.h"
#include "ixheaace_api.h"
#include "ixheaace_memory_standards.h"
#include "ixheaace_config_params.h"
}

/*****************************************************************************/
/* Constant hash defines                                                     */
/*****************************************************************************/
#define MAX_MEM_ALLOCS 100
#define IA_MAX_CMD_LINE_LENGTH 300
#define DRC_CONFIG_FILE "impd_drc_config_params.txt"
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

WORD8 pb_drc_file_path[IA_MAX_CMD_LINE_LENGTH] = "";
FILE *g_drc_inp = NULL;

pVOID malloc_global(UWORD32 size, UWORD32 alignment) { return malloc(size + alignment); }

static FLOAT32 impd_drc_get_float_value(FILE *fp) {
  WORD32 i = 0;
  FLOAT32 result = 0.0f;
  CHAR8 line[1024];
  pCHAR8 retval;
  retval = fgets(line, sizeof(line), fp);
  if (retval) {
    pCHAR8 c = line;
    while ((line[0] == '#' || line[0] == '\n') && (retval != NULL)) {
      retval = fgets(line, sizeof(line), fp);
    }
    while (line[i] != ':') {
      c++;
      i++;
    }
    c++;
    result = (FLOAT32)atof(c);
  }
  return result;
}

static WORD32 impd_drc_get_integer_value(FILE *fp) {
  WORD32 i = 0;
  WORD32 result = 0;
  CHAR8 line[1024];
  pCHAR8 retval;
  retval = fgets(line, sizeof(line), fp);
  if (retval) {
    pCHAR8 c = line;
    while ((line[0] == '#' || line[0] == '\n') && (retval != NULL)) {
      retval = fgets(line, sizeof(line), fp);
    }
    while (line[i] != ':') {
      c++;
      i++;
    }
    c++;
    if (c[0] == '0' && c[1] == 'x') {
      result = (WORD32)strtol(c, NULL, 16);
    } else {
      result = atoi(c);
    }
  }
  return result;
}

static VOID ixheaace_read_drc_config_params(
    FILE *fp, ia_drc_enc_params_struct *pstr_enc_params,
    ia_drc_uni_drc_config_struct *pstr_uni_drc_config,
    ia_drc_loudness_info_set_struct *pstr_enc_loudness_info_set,
    ia_drc_uni_drc_gain_ext_struct *pstr_enc_gain_extension) {
  WORD32 n, g, s, m, ch, p;
  WORD32 gain_set_channels;

  pstr_enc_params->delay_mode = DELAY_MODE_REGULAR_DELAY;
  pstr_uni_drc_config->sample_rate_present = 1;
  pstr_uni_drc_config->str_drc_coefficients_uni_drc->drc_frame_size_present = 0;
  pstr_uni_drc_config->loudness_info_set_present = 1;

  /***********  str_drc_instructions_uni_drc  *************/

  pstr_uni_drc_config->drc_instructions_uni_drc_count = impd_drc_get_integer_value(fp);
  for (n = 0; n < pstr_uni_drc_config->drc_instructions_uni_drc_count; n++) {
    ia_drc_instructions_uni_drc *pstr_drc_instructions_uni_drc =
        &pstr_uni_drc_config->str_drc_instructions_uni_drc[n];
    pstr_drc_instructions_uni_drc->drc_set_id = n + 1;
    pstr_drc_instructions_uni_drc->downmix_id = impd_drc_get_integer_value(fp);
    pstr_drc_instructions_uni_drc->additional_downmix_id_present = 0;
    pstr_drc_instructions_uni_drc->additional_downmix_id_count = 0;
    pstr_drc_instructions_uni_drc->drc_location = 1;
    pstr_drc_instructions_uni_drc->depends_on_drc_set_present = 0;
    pstr_drc_instructions_uni_drc->depends_on_drc_set = 0;
    pstr_drc_instructions_uni_drc->no_independent_use = 0;
    pstr_drc_instructions_uni_drc->drc_set_effect = impd_drc_get_integer_value(fp);
    pstr_drc_instructions_uni_drc->drc_set_target_loudness_present = 0;
    pstr_drc_instructions_uni_drc->drc_set_target_loudness_value_upper = 0;
    pstr_drc_instructions_uni_drc->drc_set_target_loudness_value_lower_present = 0;
    pstr_drc_instructions_uni_drc->drc_set_target_loudness_value_lower = 0;

    gain_set_channels = impd_drc_get_integer_value(fp);
    for (ch = 0; ch < gain_set_channels; ch++) {
      pstr_drc_instructions_uni_drc->gain_set_index[ch] = impd_drc_get_integer_value(fp);
    }
    for (; ch < MAX_CHANNEL_COUNT; ch++) {
      pstr_drc_instructions_uni_drc->gain_set_index[ch] =
          pstr_drc_instructions_uni_drc->gain_set_index[gain_set_channels - 1];
    }

    pstr_drc_instructions_uni_drc->num_drc_channel_groups = impd_drc_get_integer_value(fp);

    for (g = 0; g < pstr_drc_instructions_uni_drc->num_drc_channel_groups; g++) {
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].gain_scaling_present[0] = 0;
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].attenuation_scaling[0] = 1.5f;
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].amplification_scaling[0] = 1.5f;
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].gain_offset_present[0] = 0;
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].gain_offset[0] = 16.0f;
    }

    pstr_drc_instructions_uni_drc->limiter_peak_target_present = 0;
    pstr_drc_instructions_uni_drc->limiter_peak_target = 0.0f;
    pstr_drc_instructions_uni_drc->drc_instructions_type = 0;
    pstr_drc_instructions_uni_drc->mae_group_id = 0;
    pstr_drc_instructions_uni_drc->mae_group_preset_id = 0;
  }

  /***********  str_drc_coefficients_uni_drc  *************/

  pstr_uni_drc_config->drc_coefficients_uni_drc_count = impd_drc_get_integer_value(fp);
  for (n = 0; n < pstr_uni_drc_config->drc_coefficients_uni_drc_count; n++) {
    ia_drc_coefficients_uni_drc_struct *pstr_drc_coefficients_uni_drc =
        &pstr_uni_drc_config->str_drc_coefficients_uni_drc[n];
    pstr_drc_coefficients_uni_drc->drc_location = 1;
    pstr_drc_coefficients_uni_drc->gain_set_count = impd_drc_get_integer_value(fp);

    for (s = 0; s < pstr_drc_coefficients_uni_drc->gain_set_count; s++) {
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_coding_profile = 0;
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_interpolation_type = 1;
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].full_frame = 0;
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].time_alignment = 0;
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].time_delta_min_present = 0;
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].band_count =
          impd_drc_get_integer_value(fp);
      if (pstr_drc_coefficients_uni_drc->str_gain_set_params[s].band_count == 1) {
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].nb_points =
            impd_drc_get_integer_value(fp);
        for (p = 0;
             p < pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].nb_points;
             p++) {
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].gain_points[p].x =
              impd_drc_get_float_value(fp);
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].gain_points[p].y =
              impd_drc_get_float_value(fp);
        }
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].width =
            impd_drc_get_float_value(fp);
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].attack =
            impd_drc_get_float_value(fp);
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].decay =
            impd_drc_get_float_value(fp);
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].drc_characteristic =
            0;
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
            .gain_params[0]
            .crossover_freq_index = 0;
      } else {
        for (m = 0; m < pstr_drc_coefficients_uni_drc->str_gain_set_params[s].band_count; m++) {
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].nb_points =
              impd_drc_get_integer_value(fp);
          for (p = 0;
               p < pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].nb_points;
               p++) {
            pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
                .gain_params[m]
                .gain_points[p]
                .x = impd_drc_get_float_value(fp);
            pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
                .gain_params[m]
                .gain_points[p]
                .y = impd_drc_get_float_value(fp);
          }
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].width =
              impd_drc_get_float_value(fp);
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].attack =
              impd_drc_get_float_value(fp);
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].decay =
              impd_drc_get_float_value(fp);
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].drc_band_type = 0;
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
              .gain_params[m]
              .start_sub_band_index = impd_drc_get_integer_value(fp);
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
              .gain_params[m]
              .drc_characteristic = 0;
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
              .gain_params[m]
              .crossover_freq_index = 0;
        }
      }
    }
  }

  pstr_enc_loudness_info_set->loudness_info_count = impd_drc_get_integer_value(fp);
  for (n = 0; n < pstr_enc_loudness_info_set->loudness_info_count; n++) {
    pstr_enc_loudness_info_set->str_loudness_info[n].drc_set_id = impd_drc_get_integer_value(fp);
    pstr_enc_loudness_info_set->str_loudness_info[n].downmix_id = impd_drc_get_integer_value(fp);
    pstr_enc_loudness_info_set->str_loudness_info[n].sample_peak_level_present =
        impd_drc_get_integer_value(fp);
    if (1 == pstr_enc_loudness_info_set->str_loudness_info[n].sample_peak_level_present) {
      pstr_enc_loudness_info_set->str_loudness_info[n].sample_peak_level =
          impd_drc_get_float_value(fp);
    }
    pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level_present =
        impd_drc_get_integer_value(fp);
    if (1 == pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level_present) {
      pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level =
          impd_drc_get_float_value(fp);
      pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level_measurement_system =
          impd_drc_get_integer_value(fp);
      pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level_reliability =
          impd_drc_get_integer_value(fp);
    }
    pstr_enc_loudness_info_set->str_loudness_info[n].measurement_count =
        impd_drc_get_integer_value(fp);

    for (m = 0; m < pstr_enc_loudness_info_set->str_loudness_info[n].measurement_count; m++) {
      pstr_enc_loudness_info_set->str_loudness_info[n].str_loudness_measure[m].method_definition =
          impd_drc_get_integer_value(fp);
      pstr_enc_loudness_info_set->str_loudness_info[n].str_loudness_measure[m].method_value =
          impd_drc_get_float_value(fp);
      pstr_enc_loudness_info_set->str_loudness_info[n]
          .str_loudness_measure[m]
          .measurement_system = impd_drc_get_integer_value(fp);
      pstr_enc_loudness_info_set->str_loudness_info[n].str_loudness_measure[m].reliability =
          impd_drc_get_integer_value(fp);
    }
  }

  pstr_enc_loudness_info_set->loudness_info_album_count = impd_drc_get_integer_value(fp);
  for (n = 0; n < pstr_enc_loudness_info_set->loudness_info_album_count; n++) {
    pstr_enc_loudness_info_set->str_loudness_info_album[n].drc_set_id =
        impd_drc_get_integer_value(fp);
    pstr_enc_loudness_info_set->str_loudness_info_album[n].downmix_id =
        impd_drc_get_integer_value(fp);
    pstr_enc_loudness_info_set->str_loudness_info_album[n].sample_peak_level_present =
        impd_drc_get_integer_value(fp);
    if (1 == pstr_enc_loudness_info_set->str_loudness_info_album[n].sample_peak_level_present) {
      pstr_enc_loudness_info_set->str_loudness_info_album[n].sample_peak_level =
          impd_drc_get_float_value(fp);
    }
    pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level_present =
        impd_drc_get_integer_value(fp);
    if (1 == pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level_present) {
      pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level =
          impd_drc_get_float_value(fp);
      pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level_measurement_system =
          impd_drc_get_integer_value(fp);
      pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level_reliability =
          impd_drc_get_integer_value(fp);
    }
    pstr_enc_loudness_info_set->str_loudness_info_album[n].measurement_count =
        impd_drc_get_integer_value(fp);

    for (m = 0; m < pstr_enc_loudness_info_set->str_loudness_info_album[n].measurement_count;
         m++) {
      pstr_enc_loudness_info_set->str_loudness_info_album[n]
          .str_loudness_measure[m]
          .method_definition = impd_drc_get_integer_value(fp); /* 0 = program loudness */
      pstr_enc_loudness_info_set->str_loudness_info_album[n]
          .str_loudness_measure[m]
          .method_value = impd_drc_get_float_value(fp);
      pstr_enc_loudness_info_set->str_loudness_info_album[n]
          .str_loudness_measure[m]
          .measurement_system = impd_drc_get_integer_value(fp); /* 2 = ITU-R BS.1770-3 */
      pstr_enc_loudness_info_set->str_loudness_info_album[n].str_loudness_measure[m].reliability =
          impd_drc_get_integer_value(fp); /* 0 = unknown */
    }
  }

  /***********  str_channel_layout  *************/

  pstr_uni_drc_config->str_channel_layout.layout_signaling_present = 0;
  pstr_uni_drc_config->str_channel_layout.defined_layout = 0;
  pstr_uni_drc_config->str_channel_layout.speaker_position[0] = 0;

  /***********  str_downmix_instructions  *************/

  pstr_uni_drc_config->downmix_instructions_count =
      0;  // pstr_ext_cfg_downmix_input->downmix_id_count;
  for (n = 0; n < pstr_uni_drc_config->downmix_instructions_count; n++) {
    pstr_uni_drc_config->str_downmix_instructions[n].target_layout =
        impd_drc_get_integer_value(fp);
    pstr_uni_drc_config->str_downmix_instructions[n].downmix_coefficients_present =
        impd_drc_get_integer_value(fp);
  }

  pstr_uni_drc_config->drc_description_basic_present = 0;
  pstr_uni_drc_config->uni_drc_config_ext_present = 0;
  pstr_enc_loudness_info_set->loudness_info_set_ext_present = 0;
  pstr_enc_gain_extension->uni_drc_gain_ext_present = 0;
}

static VOID ixheaace_fuzzer_flag(ixheaace_input_config *pstr_in_cfg, WORD8 *data) {
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
  if (pstr_in_cfg->use_drc_element == 1 /*&& pstr_in_cfg->aot == AOT_USAC*/) {
    LOOPIDX k;
    CHAR8 drc_config_file_name[IA_MAX_CMD_LINE_LENGTH];
    strcpy(drc_config_file_name, DRC_CONFIG_FILE);
    g_drc_inp = fopen(drc_config_file_name, "rt");

    if (!g_drc_inp) {
      // Disable DRC
      pstr_in_cfg->use_drc_element = 0;
    }

    if (g_drc_inp != 0) {
      memset(&pstr_in_cfg->str_drc_cfg, 0, sizeof(ia_drc_input_config));
      ixheaace_read_drc_config_params(g_drc_inp, &pstr_in_cfg->str_drc_cfg.str_enc_params,
                                      &pstr_in_cfg->str_drc_cfg.str_uni_drc_config,
                                      &pstr_in_cfg->str_drc_cfg.str_enc_loudness_info_set,
                                      &pstr_in_cfg->str_drc_cfg.str_enc_gain_extension);

      pstr_in_cfg->str_drc_cfg.str_enc_params.gain_sequence_present = FALSE;
      for (k = 0; k < pstr_in_cfg->str_drc_cfg.str_uni_drc_config.drc_coefficients_uni_drc_count;
           k++) {
        if (pstr_in_cfg->str_drc_cfg.str_uni_drc_config.str_drc_coefficients_uni_drc[k]
                .drc_location == 1) {
          if (pstr_in_cfg->str_drc_cfg.str_uni_drc_config.str_drc_coefficients_uni_drc[k]
                  .gain_set_count > 0) {
            pstr_in_cfg->str_drc_cfg.str_enc_params.gain_sequence_present = TRUE;
            break;
          }
        }
      }

      if (pstr_in_cfg->str_drc_cfg.str_enc_params.gain_sequence_present == FALSE) {
        for (k = 0; k < pstr_in_cfg->str_drc_cfg.str_uni_drc_config.str_uni_drc_config_ext
                            .drc_coefficients_uni_drc_v1_count;
             k++) {
          if (pstr_in_cfg->str_drc_cfg.str_uni_drc_config.str_uni_drc_config_ext
                  .str_drc_coefficients_uni_drc_v1[k]
                  .drc_location == 1) {
            if (pstr_in_cfg->str_drc_cfg.str_uni_drc_config.str_uni_drc_config_ext
                    .str_drc_coefficients_uni_drc_v1[k]
                    .gain_sequence_count > 0) {
              pstr_in_cfg->str_drc_cfg.str_enc_params.gain_sequence_present = TRUE;
              break;
            }
          }
        }
      }
    }
  }
}

VOID free_global(pVOID ptr) {
  free(ptr);
  ptr = NULL;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  // Need atleast 300 bytes for processing
  if (size <= 300) {
    return 0;
  }

  /* Error code */
  IA_ERRORCODE err_code = IA_NO_ERROR;

  /* API obj */
  pVOID pv_ia_process_api_obj;

  pWORD8 pb_inp_buf = NULL, pb_out_buf = NULL;
  WORD32 i_bytes_read = 0;
  WORD32 input_size = 0;
  WORD32 samp_freq;
  WORD32 audio_profile;
  WORD32 header_samp_freq;
  FLOAT32 down_sampling_ratio = 1;
  WORD32 i_out_bytes = 0;
  WORD32 start_offset_samples = 0;
  UWORD32 *ia_stsz_size = NULL;
  UWORD32 ui_samp_freq, ui_num_chan, ui_pcm_wd_sz, ui_channel_mask, ui_num_coupling_chans = 0;
  WORD32 frame_length;
  WORD32 max_frame_size = 0;
  WORD32 expected_frame_count = 0;

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

  ixheaace_fuzzer_flag(&pstr_enc_api->input_config, (WORD8 *)data);
  bytes_consumed = 31;

  {
    if (pstr_in_cfg->aot == AOT_AAC_LC || pstr_in_cfg->aot == AOT_SBR ||
        pstr_in_cfg->aot == AOT_PS) {
      if (pstr_in_cfg->frame_cmd_flag == 0) {
        pstr_in_cfg->frame_length = 1024;
      }
      if (pstr_in_cfg->out_bytes_flag == 0) {
        pstr_in_cfg->aac_config.bitreservoir_size =
            BITRESERVOIR_SIZE_CONFIG_PARAM_DEFAULT_VALUE_LC;
      }
    } else if (pstr_in_cfg->aot == AOT_AAC_LD || pstr_in_cfg->aot == AOT_AAC_ELD) {
      if (pstr_in_cfg->frame_cmd_flag == 0) {
        pstr_in_cfg->frame_length = 512;
      }
      if (pstr_in_cfg->out_bytes_flag == 0) {
        pstr_in_cfg->aac_config.bitreservoir_size =
            BITRESERVOIR_SIZE_CONFIG_PARAM_DEFAULT_VALUE_LD;
      }
    }
  }
  ui_samp_freq = pstr_in_cfg->i_samp_freq;
  ui_num_chan = pstr_in_cfg->i_channels;
  ui_channel_mask = pstr_in_cfg->i_channels_mask;
  ui_num_coupling_chans = pstr_in_cfg->i_num_coupling_chan;
  ui_pcm_wd_sz = pstr_in_cfg->ui_pcm_wd_sz;

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
  if (err_code != IA_NO_ERROR) {
    err_code = ixheaace_delete((pVOID)pstr_out_cfg);
    if (pstr_in_cfg_user) {
      free(pstr_in_cfg_user);
    }
    if (pstr_enc_api) {
      free(pstr_enc_api);
    }
    if (ia_stsz_size != NULL) {
      free(ia_stsz_size);
      ia_stsz_size = NULL;
    }
    return err_code;
  }

  pv_ia_process_api_obj = pstr_out_cfg->pv_ia_process_api_obj;
  pb_inp_buf = (pWORD8)pstr_out_cfg->mem_info_table[IA_MEMTYPE_INPUT].mem_ptr;
  pb_out_buf = (pWORD8)pstr_out_cfg->mem_info_table[IA_MEMTYPE_OUTPUT].mem_ptr;

  /* End first part */

  /* Second part        */
  /* Initialize process */
  /* Get config params  */

  frame_length = pstr_in_cfg->frame_length;
  start_offset_samples = 0;
  input_size = pstr_out_cfg->input_size;

  if (input_size) {
    expected_frame_count = (pstr_in_cfg->aac_config.length + (input_size - 1)) / input_size;
  }

  if (NULL == ia_stsz_size) {
    ia_stsz_size =
        (UWORD32 *)malloc_global((expected_frame_count + 2) * sizeof(*ia_stsz_size), 0);
    memset(ia_stsz_size, 0, (expected_frame_count + 2) * sizeof(*ia_stsz_size));
  }
  down_sampling_ratio = pstr_out_cfg->down_sampling_ratio;
  samp_freq = (WORD32)(pstr_out_cfg->samp_freq / down_sampling_ratio);
  header_samp_freq = pstr_out_cfg->header_samp_freq;
  audio_profile = pstr_out_cfg->audio_profile;

  if (data) {
    if (data_size_left > input_size) {
      i_bytes_read = input_size;
    } else if (data_size_left <= 0) {
      i_bytes_read = 0;
      err_code = ixheaace_delete((pVOID)pstr_out_cfg);

      if (pstr_in_cfg_user) {
        free(pstr_in_cfg_user);
      }
      if (pstr_enc_api) {
        free(pstr_enc_api);
      }
      if (ia_stsz_size != NULL) {
        free(ia_stsz_size);
        ia_stsz_size = NULL;
      }
      return IA_NO_ERROR;
    } else {
      i_bytes_read = data_size_left;
      input_size = data_size_left;
    }
    memcpy(pb_inp_buf, data, input_size);
    bytes_consumed = bytes_consumed + input_size;
    data_size_left = data_size_left - input_size;
    data = data + input_size;
  }

  while (i_bytes_read) {
    if (i_bytes_read != input_size) {
      memset((pb_inp_buf + i_bytes_read), 0, (input_size - i_bytes_read));
    }

    err_code = ixheaace_process(pv_ia_process_api_obj, (pVOID)pstr_in_cfg, (pVOID)pstr_out_cfg);
    // Ignore error from process call

    /* Get the output bytes */
    i_out_bytes = pstr_out_cfg->i_out_bytes;

    if (max_frame_size < i_out_bytes) max_frame_size = i_out_bytes;

    if (data) {
      if (data_size_left > input_size) {
        i_bytes_read = input_size;
      } else if (data_size_left <= 0) {
        i_bytes_read = 0;
        err_code = ixheaace_delete((pVOID)pstr_out_cfg);

        if (pstr_in_cfg_user) {
          free(pstr_in_cfg_user);
        }
        if (pstr_enc_api) {
          free(pstr_enc_api);
        }
        if (ia_stsz_size != NULL) {
          free(ia_stsz_size);
          ia_stsz_size = NULL;
        }
        return IA_NO_ERROR;
      } else {
        i_bytes_read = data_size_left;
        input_size = data_size_left;
      }
      memcpy(pb_inp_buf, data, input_size);
      bytes_consumed = bytes_consumed + input_size;
      data_size_left = data_size_left - input_size;
      data = data + input_size;
    }
  }

  err_code = ixheaace_delete((pVOID)pstr_out_cfg);

  if (pstr_in_cfg_user) {
    free(pstr_in_cfg_user);
  }
  if (pstr_enc_api) {
    free(pstr_enc_api);
  }
  if (ia_stsz_size != NULL) {
    free(ia_stsz_size);
    ia_stsz_size = NULL;
  }
  return IA_NO_ERROR;
}

/* End ia_main_process() */
