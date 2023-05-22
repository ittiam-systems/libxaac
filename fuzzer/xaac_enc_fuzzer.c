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

#include "ixheaace_api.h"
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

int ia_enhaacplus_enc_fread(void *buf, int size, int bytes, FILE *fp) {
  return (int)fread(buf, size, bytes, fp);
}

int ia_enhaacplus_enc_fwrite(void *pb_buf, FILE *g_pf_out, WORD32 i_out_bytes) {
  fwrite(pb_buf, sizeof(char), i_out_bytes, g_pf_out);
  return 1;
}

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

IA_ERRORCODE ia_enhaacplus_enc_wav_header_decode(UWORD32 *n_channels, UWORD32 *i_channel_mask,
                                                 UWORD32 *sample_rate, UWORD32 *pcm_sz,
                                                 WORD32 *length, WORD32 *bytes_consumed,
                                                 pWORD8 data) {
  WORD8 *wav_hdr;
  WORD8 data_start[4];
  WORD16 num_ch;
  UWORD32 f_samp;
  WORD16 output_format;
  WORD32 check, count = 0;
  FLAG wav_format_pcm = 0, wav_format_extensible = 0;
  UWORD16 cbSize = 0;
  *i_channel_mask = 0;
  wav_hdr = data + *bytes_consumed;

  if (wav_hdr[0] != 'R' && wav_hdr[1] != 'I' && wav_hdr[2] != 'F' && wav_hdr[3] != 'F') {
    return 1;
  }

  if (wav_hdr[20] == 01 && wav_hdr[21] == 00) {
    wav_format_pcm = 1;
  } else if (wav_hdr[20] == ((WORD8)0xFE) && wav_hdr[21] == ((WORD8)0xFF)) {
    wav_format_extensible = 1;
  } else {
    return 1;
  }
  num_ch = (WORD16)((UWORD8)wav_hdr[23] * 256 + (UWORD8)wav_hdr[22]);
  f_samp = ((UWORD8)wav_hdr[27] * 256 * 256 * 256);
  f_samp += ((UWORD8)wav_hdr[26] * 256 * 256);
  f_samp += ((UWORD8)wav_hdr[25] * 256);
  f_samp += ((UWORD8)wav_hdr[24]);
  output_format = ((UWORD8)wav_hdr[35] * 256);
  output_format += ((UWORD8)wav_hdr[34]);
  *n_channels = num_ch;
  *sample_rate = f_samp;
  *pcm_sz = output_format;
  if (wav_format_pcm) {
    data_start[0] = wav_hdr[36];
    data_start[1] = wav_hdr[37];
    data_start[2] = wav_hdr[38];
    data_start[3] = wav_hdr[39];
  } else if (wav_format_extensible) {
    cbSize |= ((UWORD8)wav_hdr[37] << 8);
    cbSize |= ((UWORD8)wav_hdr[36]);

    if (cbSize > 25) return 1;

    *bytes_consumed = *bytes_consumed + 1;
    *i_channel_mask = 0;
    *i_channel_mask |= (UWORD8)wav_hdr[43] << 24;
    *i_channel_mask |= (UWORD8)wav_hdr[42] << 16;
    *i_channel_mask |= (UWORD8)wav_hdr[41] << 8;
    *i_channel_mask |= (UWORD8)wav_hdr[40];

    data_start[0] = wav_hdr[40 + cbSize - 2 + 0];
    data_start[1] = wav_hdr[40 + cbSize - 2 + 1];
    data_start[2] = wav_hdr[40 + cbSize - 2 + 2];
    data_start[3] = wav_hdr[40 + cbSize - 2 + 3];
  }
  check = 1;
  while (check) {
    if (data_start[0] == 'd' && data_start[1] == 'a' && data_start[2] == 't' &&
        data_start[3] == 'a') {
      *length = *(WORD32 *)(data + *bytes_consumed);
      check = 0;
      *bytes_consumed = *bytes_consumed + 4;
    } else {
      data_start[0] = data_start[1];
      data_start[1] = data_start[2];
      data_start[2] = data_start[3];
      *bytes_consumed = *bytes_consumed + 1;
    }
    count++;
    if (count > 40) {
      *length = 0xffffffff;
      return (1);
    }
  }
  return IA_NO_ERROR;
}
void ia_enhaacplus_enc_print_usage() {
  printf("\nUsage:\n");
  printf("\n<executable> -ifile:<inputfile> -ofile:<outputfile> [options]\n");
  printf("\nor\n");
  printf("\n<executable> -paramfile:<paramfile>\n");
  printf("\n[options] can be,");
  printf("\n[-br:<bitrate>]");
  printf("\n[-mps:<use_mps>]");
  printf("\n[-adts:<use_adts_flag (0/1)>]");
  printf("\n[-mp4:<use_mp4_flag (0/1)>]");
  printf("\n[-tns:<use_tns_flag>]");
  printf("\n[-pcmsz:<pcmwordsize>]");
  printf("\n[-chans:<num_chans>]");
  printf("\n[-fs:<sample_rate>]");
  printf("\n[-framesize:<framesize_to_be_used>]");
  printf("\n[-aot:<audio_object_type>]");
  printf("\n[-esbr:<esbr_flag (0/1)>]");
  printf("\n[-full_bandwidth:<Enable use of full bandwidth of input (0/1),1 to enable>]");
  printf("\n[-max_out_buffer_per_ch:<bitreservoir_size>]");
  printf("\n[-tree_cfg:<tree_config>]");
  printf("\n\nwhere, \n  <paramfile> is the parameter file with multiple commands");
  printf("\n  <inputfile> is the input 16-bit WAV or PCM file name");
  printf("\n  <outputfile> is the output ADTS/ADIF file name");
  printf("\n  <bitrate> is the bit-rate in bits per second. Valid values are ");
  printf("\n    Plain AAC: 8000-576000 bps per channel");
  printf("\n  <use_mps> When set to 1 MPS is enable. Default 0.");

  printf("\n  <use_adts_flag> when set to 1 ADTS header generated.");
  printf("\n                  Default is 0");
  printf("\n  <use_mp4_flag> when set to 1 MP4 file generated.");
  printf("\n                  Default is 1");
  printf("\n  <esbr_flag> when set to 1 enables eSBR in HEAACv1 encoding");
  printf("\n                  Default is 0");
  printf(
      "\n  <use_tns_flag> controls usage of TNS in encoding. Default 1 for AAC ELD / AAC "
      "ELDv2 and 0 for other profiles.");
  printf("\n  <input_is_pcm_flag> is set to 1 when input is a PCM file");
  printf("\n  <pcmwordsize> is the bits per sample info. Only 16 is valid");
  printf("\n  <num_chans> is the number of channels. Valid values are 1 & 2");
  printf("\n  <sample_rate> is the sample rate of input. Valid values are ");
  printf("\n    Plain AAC: 8000-96000 Hz");
  printf("\n  <framesize_to_be_used> is the framesize to be used.");
  printf(
      "\n    For AOT 23, 39 (LD core coder profiles) valid values are 480 and 512 .Default is "
      "512");
  printf(
      "\n    For AOT 2, 5, 29 (LC core coder profiles) valid values are 960 and 1024 .Default "
      "is 1024");
  printf("\n  <audio_object_type> is the Audio object type");
  printf("\n    2 for AAC LC");
  printf("\n    5 for HEAACv1(Legacy SBR)");
  printf("\n    23 for AAC LD");
  printf("\n    29 for HEAACv2");
  printf("\n    39 for AAC ELD");
  printf("\n    Default is 2 for AAC LC");
  printf("\n  <bitreservoir_size> is the maximum size of bit reservoir to be used.");
  printf("\n    Valid values are from -1 to 6144. -1 to omit use of bit reservoir.");
  printf("\n    Default is 384.");
  printf(
      "\n  <tree_config> MPS tree config"
      "0 for '212'"
      "1 for '5151'"
      "2 for '5152'"
      "3 for '525'"
      "Default '212' for stereo input '515' for 6ch input");
  exit(1);
}

VOID ia_enhaacplus_enc_display_id_message(WORD8 lib_name[], WORD8 lib_version[]) {
  WORD8 str[4][IA_SCREEN_WIDTH] = {"ITTIAM SYSTEMS PVT LTD, BANGALORE\n",
                                   "http:\\\\www.ittiam.com\n", "", ""};
  WORD8 spaces[IA_SCREEN_WIDTH / 2 + 1];
  WORD32 i, spclen;

  strcpy((pCHAR8)str[2], (pCHAR8)lib_name);
  strcat((pCHAR8)str[2], (pCHAR8)lib_version);
  strcat((pCHAR8)str[2], "\n");
  strcat((pCHAR8)str[4 - 1], "\n");

  for (i = 0; i < IA_SCREEN_WIDTH / 2 + 1; i++) {
    spaces[i] = ' ';
  }

  for (i = 0; i < 4; i++) {
    spclen = IA_SCREEN_WIDTH / 2 - (WORD32)(strlen((const char *)str[i]) / 2);
    spaces[spclen] = '\0';
    printf("%s", (const char *)spaces);
    spaces[spclen] = ' ';
    printf("%s", (const char *)str[i]);
  }
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
}

VOID free_global(pVOID ptr) {
  free(ptr);
  ptr = NULL;
}
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  eld_ga_hdr = 1;
  is_mp4 = 1;
  if (size <= 0) {
    return 0;
  }

  LOOPIDX frame_count = 0;
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
  WORD32 i_total_length = 0;
  WORD32 start_offset_samples = 0;
  UWORD32 *ia_stsz_size = NULL;
  UWORD32 ui_samp_freq, ui_num_chan, ui_pcm_wd_sz, ui_pcm = 0, ui_channel_mask,
                                                   ui_num_coupling_chans = 0;
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
  ixheaace_input_config *pstr_in_cfg_user = malloc_global(sizeof(ixheaace_input_config), 0);

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
    goto clean_return;
  }
  ixheaace_fuzzer_flag(&pstr_enc_api->input_config, (WORD8 *)data);
  bytes_consumed = 32;

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
  if (!ui_pcm) {
    /* Decode WAV header */
    if (ia_enhaacplus_enc_wav_header_decode(&ui_num_chan, &ui_channel_mask, &ui_samp_freq,
                                            &ui_pcm_wd_sz, &i_total_length, &bytes_consumed,
                                            (WORD8 *)data) == 1) {
      fprintf(stdout, "Unable to Read Input WAV File\n");
      goto clean_return;
    }

    /* PCM Word Size (For single input file) */
    pstr_in_cfg->ui_pcm_wd_sz = ui_pcm_wd_sz;
    /* Sampling Frequency */
    pstr_in_cfg->i_samp_freq = ui_samp_freq;
    /* Total Number of Channels */
    pstr_in_cfg->i_channels = ui_num_chan;
    /* Number of coupling channels*/
    pstr_in_cfg->i_num_coupling_chan = ui_num_coupling_chans;
    /* Channels Mask */
    pstr_in_cfg->i_channels_mask = ui_channel_mask;

    pstr_in_cfg->aac_config.length = i_total_length;
  }

  /* Get library id and version number and display it */
  /*
  ixheaace_get_lib_id_strings((pVOID)&pstr_out_cfg->version);
#ifdef DISPLAY_MESSAGE
  ia_enhaacplus_enc_display_id_message(pstr_out_cfg->version.p_lib_name,
                                       pstr_out_cfg->version.p_version_num);
#endif
*/
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
    ia_stsz_size = malloc_global((expected_frame_count + 2) * sizeof(*ia_stsz_size), 0);
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
      goto clean_return;
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
        goto clean_return;
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

clean_return:
  err_code = ixheaace_delete((pVOID)pstr_out_cfg);
  fprintf(stderr, "Frames Processed :%d\r", frame_count);

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
