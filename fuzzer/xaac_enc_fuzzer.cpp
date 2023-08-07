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
#include <fuzzer/FuzzedDataProvider.h>

extern "C" {
#include "ixheaac_type_def.h"
#include "impd_drc_common_enc.h"
#include "impd_drc_uni_drc.h"
#include "impd_drc_api.h"
#include "ixheaace_api.h"
}

static constexpr WORD32 kSampleRates[] = {7350,  8000,  11025, 12000, 16000, 22050, 24000,
                                          32000, 44100, 48000, 64000, 88200, 96000};
pVOID malloc_global(UWORD32 size, UWORD32 alignment) {
  pVOID ptr = NULL;
  if (posix_memalign((VOID **)&ptr, alignment, size)) {
    ptr = NULL;
  }
  return ptr;
}

VOID free_global(pVOID ptr) { free(ptr); }

static VOID ixheaace_read_drc_config_params(
    ia_drc_enc_params_struct *pstr_enc_params, ia_drc_uni_drc_config_struct *pstr_uni_drc_config,
    ia_drc_loudness_info_set_struct *pstr_enc_loudness_info_set,
    ia_drc_uni_drc_gain_ext_struct *pstr_enc_gain_extension, FuzzedDataProvider *fuzzed_data) {
  WORD32 n, g, s, m, ch, p;
  WORD32 gain_set_channels;

  pstr_enc_params->gain_sequence_present = fuzzed_data->ConsumeBool();
  pstr_enc_params->delay_mode = fuzzed_data->ConsumeBool();
  pstr_uni_drc_config->sample_rate_present = fuzzed_data->ConsumeBool();
  pstr_uni_drc_config->str_drc_coefficients_uni_drc->drc_frame_size_present =
      fuzzed_data->ConsumeBool();
  pstr_uni_drc_config->loudness_info_set_present = fuzzed_data->ConsumeBool();

  pstr_uni_drc_config->drc_instructions_uni_drc_count =
      fuzzed_data->ConsumeIntegralInRange<WORD8>(0, MAX_DRC_INSTRUCTIONS_COUNT);
  for (n = 0; n < pstr_uni_drc_config->drc_instructions_uni_drc_count; n++) {
    ia_drc_instructions_uni_drc *pstr_drc_instructions_uni_drc =
        &pstr_uni_drc_config->str_drc_instructions_uni_drc[n];
    pstr_drc_instructions_uni_drc->drc_set_id = fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_drc_instructions_uni_drc->downmix_id = fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_drc_instructions_uni_drc->additional_downmix_id_present = fuzzed_data->ConsumeBool();
    pstr_drc_instructions_uni_drc->additional_downmix_id_count =
        fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_drc_instructions_uni_drc->drc_location = fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_drc_instructions_uni_drc->depends_on_drc_set_present = fuzzed_data->ConsumeBool();
    pstr_drc_instructions_uni_drc->depends_on_drc_set = fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_drc_instructions_uni_drc->no_independent_use = fuzzed_data->ConsumeBool();
    pstr_drc_instructions_uni_drc->drc_set_effect = fuzzed_data->ConsumeIntegral<WORD16>();
    pstr_drc_instructions_uni_drc->drc_set_target_loudness_present = fuzzed_data->ConsumeBool();
    pstr_drc_instructions_uni_drc->drc_set_target_loudness_value_upper =
        fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_drc_instructions_uni_drc->drc_set_target_loudness_value_lower_present =
        fuzzed_data->ConsumeBool();
    pstr_drc_instructions_uni_drc->drc_set_target_loudness_value_lower =
        fuzzed_data->ConsumeIntegral<WORD8>();

    gain_set_channels = fuzzed_data->ConsumeIntegralInRange<WORD8>(0, MAX_CHANNEL_COUNT);
    for (ch = 0; ch < gain_set_channels; ch++) {
      pstr_drc_instructions_uni_drc->gain_set_index[ch] = fuzzed_data->ConsumeIntegral<WORD8>();
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
        fuzzed_data->ConsumeIntegralInRange<WORD8>(0, MAX_CHANNEL_GROUP_COUNT);
    for (g = 0; g < pstr_drc_instructions_uni_drc->num_drc_channel_groups; g++) {
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].gain_scaling_present[0] =
          fuzzed_data->ConsumeBool();
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].attenuation_scaling[0] =
          fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].amplification_scaling[0] =
          fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].gain_offset_present[0] =
          fuzzed_data->ConsumeBool();
      pstr_drc_instructions_uni_drc->str_gain_modifiers[g].gain_offset[0] =
          fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
    }

    pstr_drc_instructions_uni_drc->limiter_peak_target_present = fuzzed_data->ConsumeBool();
    pstr_drc_instructions_uni_drc->limiter_peak_target =
        fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
    pstr_drc_instructions_uni_drc->drc_instructions_type = fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_drc_instructions_uni_drc->mae_group_id = fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_drc_instructions_uni_drc->mae_group_preset_id = fuzzed_data->ConsumeIntegral<WORD8>();
  }

  pstr_uni_drc_config->drc_coefficients_uni_drc_count =
      fuzzed_data->ConsumeIntegralInRange<WORD8>(0, MAX_DRC_COEFF_COUNT);
  for (n = 0; n < pstr_uni_drc_config->drc_coefficients_uni_drc_count; n++) {
    ia_drc_coefficients_uni_drc_struct *pstr_drc_coefficients_uni_drc =
        &pstr_uni_drc_config->str_drc_coefficients_uni_drc[n];
    pstr_drc_coefficients_uni_drc->drc_location = fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_drc_coefficients_uni_drc->gain_set_count =
        fuzzed_data->ConsumeIntegralInRange<WORD8>(0, GAIN_SET_COUNT_MAX);
    for (s = 0; s < pstr_drc_coefficients_uni_drc->gain_set_count; s++) {
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_coding_profile =
          fuzzed_data->ConsumeIntegral<WORD8>();
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_interpolation_type =
          fuzzed_data->ConsumeBool();
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].full_frame =
          fuzzed_data->ConsumeBool();
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].time_alignment =
          fuzzed_data->ConsumeBool();
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].time_delta_min_present =
          fuzzed_data->ConsumeBool();
      pstr_drc_coefficients_uni_drc->str_gain_set_params[s].band_count =
          fuzzed_data->ConsumeIntegralInRange<WORD8>(0, MAX_BAND_COUNT);
      if (pstr_drc_coefficients_uni_drc->str_gain_set_params[s].band_count == 1) {
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].nb_points =
            fuzzed_data->ConsumeIntegralInRange<WORD16>(0, MAX_GAIN_POINTS);
        for (p = 0;
             p < pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].nb_points;
             p++) {
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].gain_points[p].x =
              fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].gain_points[p].y =
              fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
        }
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].width =
            fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].attack =
            fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].decay =
            fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[0].drc_characteristic =
            fuzzed_data->ConsumeIntegral<WORD8>();
        pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
            .gain_params[0]
            .crossover_freq_index = fuzzed_data->ConsumeIntegral<WORD8>();
      } else {
        for (m = 0; m < pstr_drc_coefficients_uni_drc->str_gain_set_params[s].band_count; m++) {
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].nb_points =
              fuzzed_data->ConsumeIntegralInRange<WORD16>(0, MAX_GAIN_POINTS);
          for (p = 0;
               p < pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].nb_points;
               p++) {
            pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
                .gain_params[m]
                .gain_points[p]
                .x = fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
            pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
                .gain_params[m]
                .gain_points[p]
                .y = fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
          }
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].width =
              fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].attack =
              fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].gain_params[m].decay =
              fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s].drc_band_type =
              fuzzed_data->ConsumeBool();
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
              .gain_params[m]
              .start_sub_band_index = fuzzed_data->ConsumeIntegral<WORD16>();
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
              .gain_params[m]
              .drc_characteristic = fuzzed_data->ConsumeIntegral<WORD8>();
          pstr_drc_coefficients_uni_drc->str_gain_set_params[s]
              .gain_params[m]
              .crossover_freq_index = fuzzed_data->ConsumeIntegral<WORD8>();
        }
      }
    }
  }

  pstr_enc_loudness_info_set->loudness_info_count =
      fuzzed_data->ConsumeIntegralInRange<WORD8>(0, MAX_LOUDNESS_INFO_COUNT);
  for (n = 0; n < pstr_enc_loudness_info_set->loudness_info_count; n++) {
    pstr_enc_loudness_info_set->str_loudness_info[n].drc_set_id =
        fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_enc_loudness_info_set->str_loudness_info[n].downmix_id =
        fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_enc_loudness_info_set->str_loudness_info[n].sample_peak_level_present =
        fuzzed_data->ConsumeBool();
    if (1 == pstr_enc_loudness_info_set->str_loudness_info[n].sample_peak_level_present) {
      pstr_enc_loudness_info_set->str_loudness_info[n].sample_peak_level =
          fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
    }
    pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level_present =
        fuzzed_data->ConsumeBool();
    if (1 == pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level_present) {
      pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level =
          fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
      pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level_measurement_system =
          fuzzed_data->ConsumeIntegral<WORD8>();
      pstr_enc_loudness_info_set->str_loudness_info[n].true_peak_level_reliability =
          fuzzed_data->ConsumeIntegral<WORD8>();
    }

    pstr_enc_loudness_info_set->str_loudness_info[n].measurement_count =
        fuzzed_data->ConsumeIntegralInRange<WORD8>(0, MAX_MEASUREMENT_COUNT);
    for (m = 0; m < pstr_enc_loudness_info_set->str_loudness_info[n].measurement_count; m++) {
      pstr_enc_loudness_info_set->str_loudness_info[n].str_loudness_measure[m].method_definition =
          fuzzed_data->ConsumeIntegral<WORD8>();
      pstr_enc_loudness_info_set->str_loudness_info[n].str_loudness_measure[m].method_value =
          fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
      pstr_enc_loudness_info_set->str_loudness_info[n]
          .str_loudness_measure[m]
          .measurement_system = fuzzed_data->ConsumeIntegral<WORD8>();
      pstr_enc_loudness_info_set->str_loudness_info[n].str_loudness_measure[m].reliability =
          fuzzed_data->ConsumeIntegral<WORD8>();
    }
  }

  pstr_enc_loudness_info_set->loudness_info_album_count =
      fuzzed_data->ConsumeIntegralInRange<WORD8>(0, MAX_LOUDNESS_INFO_COUNT);
  for (n = 0; n < pstr_enc_loudness_info_set->loudness_info_album_count; n++) {
    pstr_enc_loudness_info_set->str_loudness_info_album[n].drc_set_id =
        fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_enc_loudness_info_set->str_loudness_info_album[n].downmix_id =
        fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_enc_loudness_info_set->str_loudness_info_album[n].sample_peak_level_present =
        fuzzed_data->ConsumeBool();
    if (1 == pstr_enc_loudness_info_set->str_loudness_info_album[n].sample_peak_level_present) {
      pstr_enc_loudness_info_set->str_loudness_info_album[n].sample_peak_level =
          fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
    }
    pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level_present =
        fuzzed_data->ConsumeBool();
    if (1 == pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level_present) {
      pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level =
          fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
      pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level_measurement_system =
          fuzzed_data->ConsumeIntegral<WORD8>();
      pstr_enc_loudness_info_set->str_loudness_info_album[n].true_peak_level_reliability =
          fuzzed_data->ConsumeIntegral<WORD8>();
    }

    pstr_enc_loudness_info_set->str_loudness_info_album[n].measurement_count =
        fuzzed_data->ConsumeIntegralInRange<WORD8>(0, MAX_MEASUREMENT_COUNT);
    for (m = 0; m < pstr_enc_loudness_info_set->str_loudness_info_album[n].measurement_count;
         m++) {
      pstr_enc_loudness_info_set->str_loudness_info_album[n]
          .str_loudness_measure[m]
          .method_definition = fuzzed_data->ConsumeIntegral<WORD8>();
      pstr_enc_loudness_info_set->str_loudness_info_album[n]
          .str_loudness_measure[m]
          .method_value = fuzzed_data->ConsumeFloatingPoint<FLOAT32>();
      pstr_enc_loudness_info_set->str_loudness_info_album[n]
          .str_loudness_measure[m]
          .measurement_system = fuzzed_data->ConsumeIntegral<WORD8>();
      pstr_enc_loudness_info_set->str_loudness_info_album[n].str_loudness_measure[m].reliability =
          fuzzed_data->ConsumeIntegral<WORD8>();
    }
  }

  pstr_uni_drc_config->str_channel_layout.layout_signaling_present = fuzzed_data->ConsumeBool();
  pstr_uni_drc_config->str_channel_layout.defined_layout = fuzzed_data->ConsumeIntegral<WORD8>();
  pstr_uni_drc_config->str_channel_layout.speaker_position[0] =
      fuzzed_data->ConsumeIntegral<WORD8>();

  pstr_uni_drc_config->downmix_instructions_count =
      fuzzed_data->ConsumeIntegralInRange<WORD8>(0, MAX_DOWNMIX_INSTRUCTION_COUNT);
  for (n = 0; n < pstr_uni_drc_config->downmix_instructions_count; n++) {
    pstr_uni_drc_config->str_downmix_instructions[n].target_layout =
        fuzzed_data->ConsumeIntegral<WORD8>();
    pstr_uni_drc_config->str_downmix_instructions[n].downmix_coefficients_present =
        fuzzed_data->ConsumeBool();
  }

  pstr_uni_drc_config->drc_description_basic_present = fuzzed_data->ConsumeBool();
  pstr_uni_drc_config->uni_drc_config_ext_present = fuzzed_data->ConsumeBool();
  pstr_enc_loudness_info_set->loudness_info_set_ext_present = fuzzed_data->ConsumeBool();
  pstr_enc_gain_extension->uni_drc_gain_ext_present = fuzzed_data->ConsumeBool();
}

static VOID ixheaace_fuzzer_flag(ixheaace_input_config *pstr_in_cfg,
                                 ia_drc_input_config *pstr_drc_cfg,
                                 FuzzedDataProvider *fuzzed_data) {
  // Set Default value for AAC config structure

  pstr_in_cfg->i_bitrate = fuzzed_data->ConsumeIntegral<WORD32>();
  pstr_in_cfg->i_use_mps = fuzzed_data->ConsumeBool();
  pstr_in_cfg->i_use_adts = fuzzed_data->ConsumeBool();
  pstr_in_cfg->i_use_es = fuzzed_data->ConsumeBool();
  pstr_in_cfg->aac_config.use_tns = fuzzed_data->ConsumeBool();
  pstr_in_cfg->aac_config.noise_filling = fuzzed_data->ConsumeBool();
  pstr_in_cfg->ui_pcm_wd_sz = fuzzed_data->ConsumeIntegral<WORD8>();
  pstr_in_cfg->i_channels = fuzzed_data->ConsumeIntegral<WORD8>();
  if (fuzzed_data->ConsumeBool()) {
    pstr_in_cfg->i_samp_freq = fuzzed_data->PickValueInArray(kSampleRates);
  } else {
    pstr_in_cfg->i_samp_freq = fuzzed_data->ConsumeIntegral<WORD32>();
  }
  pstr_in_cfg->frame_length = fuzzed_data->ConsumeIntegral<WORD8>();
  pstr_in_cfg->aot = fuzzed_data->ConsumeIntegral<WORD8>();
  pstr_in_cfg->esbr_flag = fuzzed_data->ConsumeBool();
  pstr_in_cfg->aac_config.full_bandwidth = fuzzed_data->ConsumeBool();
  pstr_in_cfg->aac_config.bitreservoir_size = fuzzed_data->ConsumeIntegral<WORD16>();
  pstr_in_cfg->i_mps_tree_config = fuzzed_data->ConsumeIntegral<WORD8>();
  pstr_in_cfg->i_channels_mask = fuzzed_data->ConsumeIntegral<WORD8>();
  pstr_in_cfg->cplx_pred = fuzzed_data->ConsumeBool();
  pstr_in_cfg->usac_en = fuzzed_data->ConsumeBool();
  pstr_in_cfg->ccfl_idx = fuzzed_data->ConsumeIntegral<WORD8>();
  pstr_in_cfg->pvc_active = fuzzed_data->ConsumeBool();
  pstr_in_cfg->harmonic_sbr = fuzzed_data->ConsumeBool();
  pstr_in_cfg->hq_esbr = fuzzed_data->ConsumeBool();
  pstr_in_cfg->use_drc_element = fuzzed_data->ConsumeBool();
  pstr_in_cfg->inter_tes_active = fuzzed_data->ConsumeBool();
  pstr_in_cfg->codec_mode = fuzzed_data->ConsumeIntegral<WORD8>();

  /* DRC */
  if (pstr_in_cfg->use_drc_element == 1) {
    ixheaace_read_drc_config_params(&pstr_drc_cfg->str_enc_params,
                                    &pstr_drc_cfg->str_uni_drc_config,
                                    &pstr_drc_cfg->str_enc_loudness_info_set,
                                    &pstr_drc_cfg->str_enc_gain_extension, fuzzed_data);
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzedDataProvider fuzzed_data(data, size);

  /* Error code */
  IA_ERRORCODE err_code = 0;

  /* API obj */
  pVOID pv_ia_process_api_obj;

  pWORD8 pb_inp_buf = NULL;
  WORD32 input_size = 0;

  /* ******************************************************************/
  /* The API config structure                                         */
  /* ******************************************************************/

  ixheaace_user_config_struct str_enc_api{};

  ixheaace_input_config *pstr_in_cfg = &str_enc_api.input_config;
  ixheaace_output_config *pstr_out_cfg = &str_enc_api.output_config;
  pstr_in_cfg->pv_drc_cfg = malloc_global(sizeof(ia_drc_input_config), DEFAULT_MEM_ALIGN_8);
  ia_drc_input_config *pstr_drc_cfg = (ia_drc_input_config *)pstr_in_cfg->pv_drc_cfg;
  memset(pstr_drc_cfg, 0, sizeof(ia_drc_input_config));

  pstr_out_cfg->malloc_xheaace = &malloc_global;
  pstr_out_cfg->free_xheaace = &free_global;

  /* ******************************************************************/
  /* Parse input configuration parameters                             */
  /* ******************************************************************/
  ixheaace_fuzzer_flag(pstr_in_cfg, pstr_drc_cfg, &fuzzed_data);

  err_code = ixheaace_create((pVOID)pstr_in_cfg, (pVOID)pstr_out_cfg);
  if (err_code) {
    if (pstr_drc_cfg) {
      free(pstr_drc_cfg);
    }
    return 0;
  }

  pv_ia_process_api_obj = pstr_out_cfg->pv_ia_process_api_obj;
  pb_inp_buf = (pWORD8)pstr_out_cfg->mem_info_table[IA_MEMTYPE_INPUT].mem_ptr;

  /* End first part */

  /* Second part        */
  /* Initialize process */
  /* Get config params  */

  input_size = pstr_out_cfg->input_size;
  memset(pb_inp_buf, 0, input_size);

  while (fuzzed_data.remaining_bytes()) {
    if (fuzzed_data.ConsumeBool()) {
      std::vector<WORD8> inputVec = fuzzed_data.ConsumeBytes<WORD8>(input_size);
      memcpy(pb_inp_buf, inputVec.data(), inputVec.size());
    } else {
      memset(pb_inp_buf, fuzzed_data.ConsumeIntegral<WORD8>(), input_size);
    }
    ixheaace_process(pv_ia_process_api_obj, (pVOID)pstr_in_cfg, (pVOID)pstr_out_cfg);
  }

  ixheaace_delete((pVOID)pstr_out_cfg);

  if (pstr_drc_cfg) {
    free(pstr_drc_cfg);
  }
  return 0;
}

/* End ia_main_process() */
