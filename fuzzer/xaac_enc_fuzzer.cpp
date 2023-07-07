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
/* Constant hash defines                                                     */
/*****************************************************************************/
#define MAX_MEM_ALLOCS 100

pVOID malloc_global(UWORD32 size, UWORD32 alignment) { return malloc(size + alignment); }

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
  bytes_consumed = 22;

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
