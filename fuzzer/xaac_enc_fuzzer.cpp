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
#include "ixheaac_type_def.h"
#include "ixheaace_aac_constants.h"
#include "ixheaac_error_standards.h"

extern "C" {
#include "ixheaace_api.h"
}
#include "ixheaace_memory_standards.h"
#include "ixheaace_config_params.h"

/*****************************************************************************/
/* Process select hash defines                                               */
/*****************************************************************************/
#define WAV_READER
#define DISPLAY_MESSAGE

/*****************************************************************************/
/* Constant hash defines                                                     */
/*****************************************************************************/
#define MAX_MEM_ALLOCS 100
#define IA_MAX_CMD_LINE_LENGTH 300
#define IA_MAX_ARGS 20
#define IA_SCREEN_WIDTH 80

WORD32 eld_ga_hdr;
WORD32 is_mp4;

/*****************************************************************************/
/* Global variables                                                          */
/*****************************************************************************/
#define HANDLE_ERROR(e, pv_output)             \
  if ((e) != IA_NO_ERROR) {                    \
    if ((e)&IA_FATAL_ERROR) {                  \
      IA_ERRORCODE err = IA_NO_ERROR;          \
      err = ixheaace_delete((pVOID)pv_output); \
      if (pstr_in_cfg_user) {                  \
        free(pstr_in_cfg_user);                \
      }                                        \
      if (pstr_enc_api) {                      \
        free(pstr_enc_api);                    \
      }                                        \
      if (ia_stsz_size != NULL) {              \
        free(ia_stsz_size);                    \
        ia_stsz_size = NULL;                   \
      }                                        \
      if ((err)&IA_FATAL_ERROR) {              \
        return (err);                          \
      } else {                                 \
        return (e);                            \
      }                                        \
    }                                          \
  }

pVOID malloc_global(UWORD32 size, UWORD32 alignment) { return malloc(size + alignment); }

static VOID iaace_aac_set_default_config(ixheaace_aac_enc_config *config) {
  /* make the pre initialization of the structs flexible */
  memset(config, 0, sizeof(*config));
  /* default configurations */
  config->bitrate = 48000;
  config->bandwidth = 0;
  config->inv_quant = 2;
  config->use_tns = 0;
  config->noise_filling = 0;
  config->bitreservoir_size = BITRESERVOIR_SIZE_CONFIG_PARAM_DEFAULT_VALUE_LC;
}

static VOID ixheaace_fuzzer_flag(ixheaace_input_config *pstr_in_cfg, WORD8 *data) {
  WORD32 bitrate_external;
  WORD32 mps_external;
  WORD32 adts_external;
  WORD32 mp4_external;
  WORD32 tns_external;
  WORD32 nf_external;
  WORD32 cmpx_pred_external;
  WORD32 pcm_external;
  WORD32 pcmsz_external;
  WORD32 chans_external;
  WORD32 fs_external;
  WORD32 framesize_external;
  WORD32 aot_external;
  WORD32 esbr_external;
  WORD32 full_bandwidth_external;
  WORD32 max_out_buffer_per_ch_external;
  WORD32 usac_external;
  WORD32 ccfl_idx_external;
  WORD32 tree_cfg_external;
  WORD32 pvc_enc_external;
  WORD32 harmonic_sbr_external;
  WORD32 esbr_hq_external;
  WORD32 drc_external;
  WORD32 inter_tes_enc_external;
  WORD32 channel_mask_external;

  bitrate_external = *(WORD32 *)(data);
  mps_external = *(WORD8 *)(data + 4);
  adts_external = *(WORD8 *)(data + 5);
  mp4_external = *(WORD8 *)(data + 6);
  tns_external = *(WORD8 *)(data + 7);
  nf_external = *(WORD8 *)(data + 8);
  cmpx_pred_external = *(WORD8 *)(data + 9);
  pcm_external = *(WORD8 *)(data + 10);
  pcmsz_external = *(WORD8 *)(data + 11);
  chans_external = *(WORD8 *)(data + 12);
  fs_external = *(WORD32 *)(data + 13);
  framesize_external = *(WORD16 *)(data + 17);
  aot_external = *(WORD8 *)(data + 19);
  esbr_external = *(WORD8 *)(data + 20);
  full_bandwidth_external = *(WORD8 *)(data + 21);
  max_out_buffer_per_ch_external = *(WORD16 *)(data + 22);
  usac_external = *(WORD8 *)(data + 24);
  ccfl_idx_external = *(WORD8 *)(data + 25);
  tree_cfg_external = *(WORD8 *)(data + 26);
  pvc_enc_external = *(WORD8 *)(data + 27);
  harmonic_sbr_external = *(WORD8 *)(data + 28);
  esbr_hq_external = *(WORD8 *)(data + 29);
  drc_external = *(WORD8 *)(data + 30);
  inter_tes_enc_external = *(WORD8 *)(data + 31);

  channel_mask_external = *(WORD8 *)(data + 32);

  pstr_in_cfg->aot = aot_external;
  pstr_in_cfg->esbr_flag = esbr_external;
  pstr_in_cfg->aac_config.use_tns = tns_external;
  pstr_in_cfg->aac_config.noise_filling = nf_external;
  pstr_in_cfg->i_use_adts = adts_external % 2;
  pstr_in_cfg->aac_config.full_bandwidth = full_bandwidth_external;
  pstr_in_cfg->i_bitrate = bitrate_external;
  pstr_in_cfg->i_use_mps = mps_external;
  pstr_in_cfg->i_mps_tree_config = tree_cfg_external;
  pstr_in_cfg->frame_length = framesize_external;
  pstr_in_cfg->i_samp_freq = fs_external;
  pstr_in_cfg->aac_config.bitreservoir_size = max_out_buffer_per_ch_external;
  pstr_in_cfg->i_channels = chans_external;
  pstr_in_cfg->ui_pcm_wd_sz = pcmsz_external;
  pstr_in_cfg->i_channels_mask = channel_mask_external;
}

VOID free_global(pVOID ptr) {
  free(ptr);
  ptr = NULL;
}
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  eld_ga_hdr = 1;
  is_mp4 = 1;
  if (size <= 0) {
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

  /* ******************************************************************/
  /* Parse input configuration parameters                             */
  /* ******************************************************************/

  iaace_aac_set_default_config(&pstr_in_cfg->aac_config);

  pstr_in_cfg->aot = AOT_AAC_LC;
  pstr_in_cfg->i_channels = 2;
  pstr_in_cfg->i_samp_freq = 44100;
  pstr_in_cfg->i_use_mps = 0;
  pstr_in_cfg->i_mps_tree_config = 0;
  pstr_in_cfg->i_use_adts = 0;
  pstr_in_cfg->esbr_flag = 0;
  pstr_in_cfg->i_use_es = 1;
  pstr_in_cfg->i_channels_mask = 0;
  pstr_in_cfg->i_num_coupling_chan = 0;
  pstr_in_cfg->aac_config.full_bandwidth = 0;
  pstr_out_cfg->malloc_xheaace = &malloc_global;
  pstr_out_cfg->free_xheaace = &free_global;
  pstr_in_cfg->frame_cmd_flag = 0;
  pstr_in_cfg->out_bytes_flag = 0;
  pstr_in_cfg->i_use_adts = !eld_ga_hdr;
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
  if (file_data_size < 300) {
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
  ixheaace_fuzzer_flag(&pstr_enc_api->input_config, (WORD8 *)data);
  bytes_consumed = 33;

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

  if (is_mp4 && pstr_in_cfg->i_use_adts) {
    // Give preference to MP4
    pstr_in_cfg->i_use_adts = 0;
    eld_ga_hdr = 1;
    pstr_in_cfg->i_use_es = 1;
  }

  data = data + bytes_consumed;
  data_size_left = data_size_left - bytes_consumed;

  err_code = ixheaace_create((pVOID)pstr_in_cfg, (pVOID)pstr_out_cfg);
  HANDLE_ERROR(err_code, pstr_out_cfg);

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
    HANDLE_ERROR(err_code, pstr_out_cfg);

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