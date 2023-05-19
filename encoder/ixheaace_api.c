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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ixheaac_type_def.h"
#include "ixheaac_constants.h"
#include "ixheaace_aac_constants.h"
#include "ixheaac_basic_ops32.h"
#include "ixheaac_basic_ops16.h"
#include "ixheaac_basic_ops40.h"

/* standard */
#include "ixheaac_error_standards.h"
#include "ixheaace_apicmd_standards.h"

#include "ixheaace_config_params.h"

/* library */
#include "ixheaace_definitions.h"
#include "ixheaace_error_codes.h"

#include "iusace_bitbuffer.h"

#include "ixheaace_sbr_header.h"
#include "ixheaace_sbr_def.h"
#include "ixheaace_resampler.h"

#include "ixheaace_psy_const.h"
#include "ixheaace_tns.h"
#include "ixheaace_tns_params.h"
#include "ixheaace_rom.h"
#include "ixheaace_common_rom.h"
#include "ixheaace_bitbuffer.h"
#include "ixheaace_enc_main.h"

#include "ixheaace_sbr_rom.h"
#include "ixheaace_common_rom.h"
#include "ixheaace_sbr_main.h"
#include "ixheaace_definitions.h"
#include "ixheaace_memory_standards.h"
#include "iusace_cnst.h"
#include "iusace_config.h"
#include "ixheaace_version_number.h"

#include "ixheaace_adjust_threshold_data.h"
#include "ixheaace_dynamic_bits.h"
#include "ixheaace_qc_data.h"
#include "ixheaace_channel_map.h"
#include "ixheaace_block_switch.h"
#include "ixheaace_psy_data.h"
#include "ixheaace_interface.h"
#include "ixheaace_write_bitstream.h"
#include "ixheaace_psy_configuration.h"
#include "ixheaace_common_rom.h"
#include "ixheaace_psy_mod.h"
#include "ixheaace_qc_util.h"
#include "ixheaace_sbr_header.h"
#include "ixheaace_config.h"
#include "ixheaace_asc_write.h"
#include "ixheaace_stereo_preproc.h"
#include "ixheaace_api_struct_define.h"
#include "ixheaace_aac_api.h"

// MPS header
#include "ixheaace_mps_common_fix.h"
#include "ixheaace_mps_defines.h"
#include "ixheaace_mps_common_define.h"

#include "ixheaace_mps_struct_def.h"
#include "ixheaace_mps_sac_polyphase.h"
#include "ixheaace_mps_sac_hybfilter.h"
#include "ixheaace_mps_bitstream.h"
#include "ixheaace_mps_spatial_bitstream.h"

#include "ixheaace_mps_buf.h"
#include "ixheaace_mps_lib.h"
#include "ixheaace_mps_main_structure.h"
#include "ixheaace_mps_onset_detect.h"

#include "ixheaace_mps_param_extract.h"

#include "ixheaace_mps_static_gain.h"
#include "ixheaace_mps_filter.h"
#include "ixheaace_mps_delay.h"
#include "ixheaace_mps_dmx_tdom_enh.h"
#include "ixheaace_mps_main_structure.h"
#include "ixheaace_mps_tools_rom.h"
#include "ixheaace_mps_qmf.h"
#include "ixheaace_mps_tree.h"
#include "ixheaace_mps_frame_windowing.h"

#include "ixheaace_mps_structure.h"
#include "ixheaace_mps_memory.h"
#include "ixheaace_mps_enc.h"
#include "ixheaace_struct_def.h"
#include "ixheaace_api_defs.h"
#include "ixheaace_api.h"
#include "ixheaace_write_adts_adif.h"

/* Ensure all memory are aligned */
#define IXHEAACE_ALIGN_MEMORY(address, alignment) \
  ((WORD8 *)((address + (alignment - 1)) & ~(alignment - 1)))

static WORD32 ia_enhaacplus_enc_sizeof_delay_buffer(FLAG flag_framelength_small, WORD32 aot,
                                                    WORD32 resamp_idx, WORD32 delay_buf_size,
                                                    FLAG mps_enable) {
  WORD32 downsample_fac;
  if (resamp_idx > 1) {
    downsample_fac = resamp_idx / 2;
  } else {
    downsample_fac = 1;
  }
  // Set the downsampler delay
  WORD32 max_downsamp_delay, max_upsamp_delay;
  if (resamp_idx == 2)  // 8:3
  {
    max_downsamp_delay = MAXIMUM_DS_8_1_FILTER_DELAY;
    max_upsamp_delay = MAXIMUM_DS_1_3_FILTER_DELAY;
  } else if (resamp_idx == 3)  // 2:1
  {
    max_downsamp_delay = MAXIMUM_DS_2_1_FILTER_DELAY;
    max_upsamp_delay = 0;
  } else if (resamp_idx == 4)  // 4:1
  {
    max_downsamp_delay = MAXIMUM_DS_4_1_FILTER_DELAY;
    max_upsamp_delay = 0;
  } else {
    max_downsamp_delay = MAXIMUM_DS_2_1_FILTER_DELAY;
    max_upsamp_delay = 0;
  }

  if (aot == AOT_SBR || aot == AOT_PS) {
    if (flag_framelength_small)
      return (FRAME_LEN_960 * (1 << downsample_fac) + max_upsamp_delay + max_downsamp_delay +
              INPUT_DELAY_LC) *
             IXHEAACE_MAX_CH_IN_BS_ELE * delay_buf_size;
    else
      return (FRAME_LEN_1024 * (1 << downsample_fac) + max_upsamp_delay + max_downsamp_delay +
              INPUT_DELAY_LC) *
             IXHEAACE_MAX_CH_IN_BS_ELE * delay_buf_size;
  } else if (aot == AOT_AAC_LC) {
    if (flag_framelength_small)
      return (FRAME_LEN_960 * 2 + MAXIMUM_DS_2_1_FILTER_DELAY + INPUT_DELAY_LC) *
             IXHEAACE_MAX_CH_IN_BS_ELE * delay_buf_size;
    else
      return (FRAME_LEN_1024 * 2 + MAXIMUM_DS_2_1_FILTER_DELAY + INPUT_DELAY_LC) *
             IXHEAACE_MAX_CH_IN_BS_ELE * delay_buf_size;
  } else if (aot == AOT_USAC) {
    return (FRAME_LEN_1024 * (1 << downsample_fac) + max_upsamp_delay + max_downsamp_delay +
            INPUT_DELAY_LC) *
           IXHEAACE_MAX_CH_IN_BS_ELE * delay_buf_size;
  } else if (aot == AOT_AAC_LD) {
    if (flag_framelength_small)
      return (FRAME_LEN_480 * 2 + MAXIMUM_DS_2_1_FILTER_DELAY + INPUT_DELAY_LD_480) *
             IXHEAACE_MAX_CH_IN_BS_ELE * delay_buf_size;
    else
      return (FRAME_LEN_512 * 2 + MAXIMUM_DS_2_1_FILTER_DELAY + INPUT_DELAY_LD_512) *
             IXHEAACE_MAX_CH_IN_BS_ELE * delay_buf_size;
  } else if (aot == AOT_AAC_ELD) {
    if (flag_framelength_small) {
      if (mps_enable) {
        return (FRAME_LEN_480 * (1 << downsample_fac) + max_upsamp_delay + max_downsamp_delay +
                INPUT_DELAY_ELDV2_480) *
               IXHEAACE_MAX_CH_IN_BS_ELE * delay_buf_size;
      } else {
        return (FRAME_LEN_480 * (1 << downsample_fac) + max_upsamp_delay + max_downsamp_delay +
                INPUT_DELAY_ELD_480) *
               IXHEAACE_MAX_CH_IN_BS_ELE * delay_buf_size;
      }
    } else {
      if (mps_enable) {
        return (FRAME_LEN_512 * (1 << downsample_fac) + max_upsamp_delay + max_downsamp_delay +
                INPUT_DELAY_ELDV2_512) *
               IXHEAACE_MAX_CH_IN_BS_ELE * delay_buf_size;
      } else {
        return (FRAME_LEN_512 * (1 << downsample_fac) + max_upsamp_delay + max_downsamp_delay +
                INPUT_DELAY_ELD_512) *
               IXHEAACE_MAX_CH_IN_BS_ELE * delay_buf_size;
      }
    }
  } else {
    // return LC Delay buffer size by default
    return (FRAME_LEN_1024 * 2 + MAXIMUM_DS_2_1_FILTER_DELAY + INPUT_DELAY_LC) *
           IXHEAACE_MAX_CH_IN_BS_ELE * delay_buf_size;
  }
}

static VOID ia_enhaacplus_enc_find_slots_for_elements(WORD32 i_channel_mask,
                                                      WORD32 *slots_for_elements,
                                                      WORD32 i_num_coupling_chan) {
  WORD32 slot = 0, i;

  if ((i_channel_mask & 0x3)) {
    slots_for_elements[FRONT_LEFT_RIGHT] = slot;
    slot += 2;
  }

  if ((i_channel_mask & 0x4)) {
    slots_for_elements[FRONT_CENTER] = slot;
    slot += 1;
  }

  if ((i_channel_mask & 0x8)) {
    slots_for_elements[LFE_CHANNEL] = slot;
    slot += 1;
  }

  if ((i_channel_mask & 0x30)) {
    slots_for_elements[BACK_LEFT_RIGHT] = slot;
    slot += 2;
  }

  if ((i_channel_mask & 0xC0)) {
    slots_for_elements[BACK_LR_OF_CENTER] = slot;
    slot += 2;
  }

  if (i_num_coupling_chan != 0) {
    for (i = 0; i < i_num_coupling_chan; i++) {
      slots_for_elements[COUPLING_CH + i] = slot;
      slot += 1;
    }
  }

  return;
}

static VOID ia_enhaacplus_enc_find_channel_config(WORD32 *num_bs_elements, WORD32 *chan_config,
                                                  WORD32 *element_type, WORD32 *element_slot,
                                                  WORD32 *element_instance_tag,
                                                  WORD32 i_num_coupling_chan,
                                                  WORD32 i_channel_mask) {
  WORD32 slot = 0, i;
  WORD32 slots_for_elements[2 * MAXIMUM_BS_ELE];
  *num_bs_elements = 0;

  ia_enhaacplus_enc_find_slots_for_elements(i_channel_mask, slots_for_elements,
                                            i_num_coupling_chan);

  if ((i_channel_mask & 0x4)) {
    /*Front Center Present*/
    chan_config[*num_bs_elements] = 1;
    element_type[*num_bs_elements] = ID_SCE;
    element_slot[*num_bs_elements] = slots_for_elements[FRONT_CENTER];
    slot += chan_config[*num_bs_elements];
    element_instance_tag[*num_bs_elements] = 0;
    (*num_bs_elements)++;
  }

  if ((i_channel_mask & 0x3)) {
    /*Front Left and Right Present*/
    chan_config[*num_bs_elements] = 2;
    element_type[*num_bs_elements] = ID_CPE;
    element_slot[*num_bs_elements] = slots_for_elements[FRONT_LEFT_RIGHT];
    slot += chan_config[*num_bs_elements];
    element_instance_tag[*num_bs_elements] = 0;
    (*num_bs_elements)++;
  }

  if ((i_channel_mask & 0x30)) {
    /*Back Left and Right Present*/
    chan_config[*num_bs_elements] = 2;
    element_type[*num_bs_elements] = ID_CPE;
    element_slot[*num_bs_elements] = slots_for_elements[BACK_LEFT_RIGHT];
    slot += chan_config[*num_bs_elements];
    element_instance_tag[*num_bs_elements] = 1;
    (*num_bs_elements)++;
  }

  if ((i_channel_mask & 0xC0)) {
    /*Back Left and Right of center Present*/
    chan_config[*num_bs_elements] = 2;
    element_type[*num_bs_elements] = ID_CPE;
    element_slot[*num_bs_elements] = slots_for_elements[BACK_LR_OF_CENTER];
    ;
    slot += chan_config[*num_bs_elements];
    element_instance_tag[*num_bs_elements] = 2;
    (*num_bs_elements)++;
  }

  if ((i_channel_mask & 0x8)) {
    /*LFE channel Present*/
    chan_config[*num_bs_elements] = 1;
    element_type[*num_bs_elements] = ID_LFE;
    element_slot[*num_bs_elements] = slots_for_elements[LFE_CHANNEL];
    slot += chan_config[*num_bs_elements];
    element_instance_tag[*num_bs_elements] = 0;
    (*num_bs_elements)++;
  }

  if (i_num_coupling_chan != 0) {
    for (i = 0; i < i_num_coupling_chan; i++) {
      /*Coupling Channel Present*/
      chan_config[*num_bs_elements] = 1;
      element_type[*num_bs_elements] = ID_CCE;
      element_slot[*num_bs_elements] = slots_for_elements[COUPLING_CH + i];
      slot += chan_config[*num_bs_elements];
      // element_instance_tag[*num_bs_elements] = i;
      element_instance_tag[*num_bs_elements] = i_num_coupling_chan - i - 1;
      (*num_bs_elements)++;
    }
  }
}

static VOID ia_enhaacplus_enc_allocate_bitrate_between_channels(
    ixheaace_api_struct *pstr_api_struct, WORD32 *bitrate, WORD32 inp_bitrate) {
  WORD32 ele_idx;
  WORD32 num_lfe = 0, num_mono = 0, num_stereo = 0;
  WORD32 bitrate_per_stereo, bitrate_per_mono;
  for (ele_idx = 0; ele_idx < pstr_api_struct->config[0].num_bs_elements; ele_idx++) {
    switch (pstr_api_struct->config[ele_idx].element_type) {
      case ID_SCE:
      case ID_CCE:
        num_mono++;
        break;
      case ID_CPE:
        num_stereo++;
        break;
      case ID_LFE:
        num_lfe++;
        break;
      default:
        return;
    }
  }
  bitrate_per_stereo = (WORD32)((inp_bitrate - (num_lfe)*8000) / (num_mono * 0.625 + num_stereo));
  bitrate_per_mono = (WORD32)(0.625 * bitrate_per_stereo);
  for (ele_idx = 0; ele_idx < pstr_api_struct->config[0].num_bs_elements; ele_idx++) {
    switch (pstr_api_struct->config[ele_idx].element_type) {
      case ID_SCE:
      case ID_CCE:
        bitrate[ele_idx] = bitrate_per_mono;
        break;
      case ID_CPE:
        bitrate[ele_idx] = bitrate_per_stereo;
        break;
      case ID_LFE:
        bitrate[ele_idx] = 8000;
        break;
      default:
        return;
    }
  }
}

static VOID ixheaace_set_default_config(ixheaace_api_struct *pstr_api_struct,
                                        ixheaace_input_config *pstr_input_config) {
  WORD32 i;

  for (i = 0; i < MAXIMUM_BS_ELE; i++) {
    ia_enhaacplus_enc_aac_init_default_config(&pstr_api_struct->config[i].aac_config,
                                              pstr_input_config->aot);

    pstr_api_struct->config[i].i_channels = NUM_CHANNELS_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].sample_rate = AAC_SAMP_FREQ_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].native_sample_rate = AAC_SAMP_FREQ_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].i_n_memtabs = NUM_MEMTABS_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].aac_classic = AAC_CLASSIC_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].use_parametric_stereo = USE_PS_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].chmode_nchannels = CHMODE_NUM_CHANNELS_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].chmode = CHMODE_CONFIG_PARAM_DEFAULT_VALUE; /*stereo*/
    pstr_api_struct->config[i].adts_flag = ADTS_FLAG_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].num_bs_elements = NUM_BS_ELEMENTS_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].i_channels_mask = CHANNEL_MASK_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].i_num_coupling_chan =
        NUM_COUPLING_CHANNEL_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].element_type = ELEMENT_TYPE_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].element_slot = ELEMENT_SLOT_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].num_bs_elements = NUM_BS_ELEMENTS_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].element_instance_tag =
        ELEMENT_INSTANCE_TAG_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].aac_config.calc_crc = AAC_CFG_CALC_CRC_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].aac_config.full_bandwidth =
        AAC_CFG_FULL_BW_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].eldsbr_found = ELDSBR_FOUND_CONFIG_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].use_mps = USE_MPS_PARAM_DEFAULT_VALUE;
    pstr_api_struct->config[i].mps_tree_config = USE_MPS_TREE_CONFIG_PARAM_DEFAULT_VALUE;
  }

  /* Initialize table pointers */
  ia_enhaacplus_enc_init_aac_tabs(&(pstr_api_struct->pstr_aac_tabs));
  ia_enhaacplus_enc_init_sbr_tabs(&(pstr_api_struct->spectral_band_replication_tabs));
  pstr_api_struct->common_tabs.pstr_common_tab =
      (ixheaace_common_tables *)&ia_enhaacplus_enc_common_tab;
}

static VOID ixheaace_validate_config_params(ixheaace_input_config *pstr_input_config) {
  if (pstr_input_config->aot != AOT_AAC_ELD && pstr_input_config->aot != AOT_AAC_LC &&
      pstr_input_config->aot != AOT_AAC_LD && pstr_input_config->aot != AOT_PS &&
      pstr_input_config->aot != AOT_SBR && pstr_input_config->aot != AOT_USAC) {
    pstr_input_config->aot = AOT_AAC_LC;
  }
  pstr_input_config->i_native_samp_freq = pstr_input_config->i_samp_freq;
  if (pstr_input_config->aot == AOT_USAC) {
    if (pstr_input_config->i_samp_freq < 9391) {
      pstr_input_config->i_samp_freq = 8000;
    } else if ((pstr_input_config->i_samp_freq >= 9391) &&
               (pstr_input_config->i_samp_freq < 11502)) {
      pstr_input_config->i_samp_freq = 11025;
    } else if ((pstr_input_config->i_samp_freq >= 11502) &&
               (pstr_input_config->i_samp_freq < 13856)) {
      pstr_input_config->i_samp_freq = 12000;
    } else if ((pstr_input_config->i_samp_freq >= 13856) &&
               (pstr_input_config->i_samp_freq < 18783)) {
      pstr_input_config->i_samp_freq = 16000;
    } else if ((pstr_input_config->i_samp_freq >= 18783) &&
               (pstr_input_config->i_samp_freq < 23004)) {
      pstr_input_config->i_samp_freq = 22050;
    } else if ((pstr_input_config->i_samp_freq >= 23004) &&
               (pstr_input_config->i_samp_freq < 27713)) {
      pstr_input_config->i_samp_freq = 24000;
    } else if ((pstr_input_config->i_samp_freq >= 27713) &&
               (pstr_input_config->i_samp_freq < 37566)) {
      pstr_input_config->i_samp_freq = 32000;
    } else if ((pstr_input_config->i_samp_freq >= 37566) &&
               (pstr_input_config->i_samp_freq < 46009)) {
      pstr_input_config->i_samp_freq = 44100;
    } else if ((pstr_input_config->i_samp_freq >= 46009) &&
               (pstr_input_config->i_samp_freq < 55426)) {
      pstr_input_config->i_samp_freq = 48000;
    } else if ((pstr_input_config->i_samp_freq >= 55426) &&
               (pstr_input_config->i_samp_freq < 75132)) {
      pstr_input_config->i_samp_freq = 64000;
    } else if ((pstr_input_config->i_samp_freq >= 75132) &&
               (pstr_input_config->i_samp_freq < 92017)) {
      pstr_input_config->i_samp_freq = 88200;
    } else if (pstr_input_config->i_samp_freq >= 92017) {
      pstr_input_config->i_samp_freq = 96000;
    }
  }

  if (pstr_input_config->esbr_flag != 1 && pstr_input_config->esbr_flag != 0) {
    pstr_input_config->esbr_flag = 0;
  }
  if ((pstr_input_config->esbr_flag == 1) &&
      ((pstr_input_config->aot != AOT_SBR) && (pstr_input_config->aot != AOT_PS) &&
       (pstr_input_config->aot != AOT_USAC))) {
    pstr_input_config->esbr_flag = 0;
  }
  if (pstr_input_config->i_use_mps != 1 && pstr_input_config->i_use_mps != 0) {
    pstr_input_config->i_use_mps = 0;
  }
  if (pstr_input_config->i_channels == 1) {
    pstr_input_config->i_use_mps = 0;
  }
  if (pstr_input_config->aot != AOT_AAC_ELD && pstr_input_config->aot != AOT_USAC) {
    pstr_input_config->i_use_mps = 0;
  }
  if (pstr_input_config->i_use_mps == 1) {
    if (pstr_input_config->i_channels == 2) {
      if (pstr_input_config->i_mps_tree_config != TREE_212) {
        pstr_input_config->i_mps_tree_config = TREE_212;
      }
    } else {
      if (pstr_input_config->i_mps_tree_config != TREE_5151 &&
          pstr_input_config->i_mps_tree_config != TREE_5152 &&
          pstr_input_config->i_mps_tree_config != TREE_525) {
        pstr_input_config->i_mps_tree_config = TREE_5151;
      }
    }
  } else {
    pstr_input_config->i_mps_tree_config = INVALID_TREE_CONFIG;
  }
  if (pstr_input_config->aot == AOT_USAC || pstr_input_config->aot == AOT_AAC_ELD ||
      pstr_input_config->aot == AOT_AAC_LD) {
    pstr_input_config->i_use_adts = 0;
    pstr_input_config->i_use_es = 1;
  }

  {
    if (pstr_input_config->i_channels != 2 && pstr_input_config->aot == AOT_PS) {
      pstr_input_config->aot = AOT_SBR;
    }
    if (pstr_input_config->aac_config.use_tns != 1 &&
        pstr_input_config->aac_config.use_tns != 0) {
      pstr_input_config->aac_config.use_tns = 0;
    }
    if (pstr_input_config->aac_config.noise_filling != 1 &&
        pstr_input_config->aac_config.noise_filling != 0) {
      pstr_input_config->aac_config.noise_filling = 0;
    }
    if (pstr_input_config->i_use_adts != 1 && pstr_input_config->i_use_adts != 0) {
      pstr_input_config->i_use_adts = 0;
      pstr_input_config->i_use_es = 1;
    }
    if (pstr_input_config->aac_config.full_bandwidth != 1 &&
        pstr_input_config->aac_config.full_bandwidth != 0) {
      pstr_input_config->aac_config.full_bandwidth = 0;
    }
    {
      if (pstr_input_config->i_bitrate < MINIMUM_BITRATE * pstr_input_config->i_channels) {
        pstr_input_config->i_bitrate = MINIMUM_BITRATE * pstr_input_config->i_channels;
      }
      if (pstr_input_config->i_bitrate >
          (6 * pstr_input_config->i_samp_freq * pstr_input_config->i_channels)) {
        pstr_input_config->i_bitrate =
            (6 * pstr_input_config->i_samp_freq * pstr_input_config->i_channels);
      }
    }

    {
      if (pstr_input_config->aot == AOT_AAC_LC || pstr_input_config->aot == AOT_SBR ||
          pstr_input_config->aot == AOT_PS) {
        if (pstr_input_config->frame_length != LEN_SUPERFRAME &&
            pstr_input_config->frame_length != FRAME_LEN_960) {
          pstr_input_config->frame_length = LEN_SUPERFRAME;
        }
      } else if (pstr_input_config->aot == AOT_AAC_LD || pstr_input_config->aot == AOT_AAC_ELD) {
        if (pstr_input_config->frame_length != FRAME_LEN_512 &&
            pstr_input_config->frame_length != FRAME_LEN_480) {
          pstr_input_config->frame_length = FRAME_LEN_512;
        }
      } else {
        pstr_input_config->frame_length = LEN_SUPERFRAME;
      }
    }
  }
}

static IA_ERRORCODE ixheaace_set_config_params(ixheaace_api_struct *pstr_api_struct,
                                               ixheaace_input_config *pstr_input_config) {
  WORD32 ele_idx;

  ixheaace_validate_config_params(pstr_input_config);

  if (pstr_input_config->ui_pcm_wd_sz != 16) {
    return (IA_EXHEAACE_CONFIG_FATAL_PCM_WDSZ);
  }
  if ((pstr_input_config->aac_config.inv_quant != 0) &&
      (pstr_input_config->aac_config.inv_quant != 1) &&
      (pstr_input_config->aac_config.inv_quant != 2)) {
    return (IA_EXHEAACE_CONFIG_FATAL_QUALITY_LEVEL);
  }
  if ((pstr_input_config->write_program_config_element != 0) &&
      (pstr_input_config->write_program_config_element != 1)) {
    return (IA_EXHEAACE_CONFIG_FATAL_WRITE_PCE);
  }
  if ((pstr_input_config->aac_config.f_no_stereo_preprocessing != 0) &&
      (pstr_input_config->aac_config.f_no_stereo_preprocessing != 1)) {
    return (IA_EXHEAACE_CONFIG_FATAL_USE_STEREO_PRE_PROC);
  }
  if ((pstr_input_config->aac_config.use_tns != 0) &&
      (pstr_input_config->aac_config.use_tns != 1)) {
    return (IA_EXHEAACE_CONFIG_FATAL_USE_TNS);
  }
  if ((pstr_input_config->aac_config.full_bandwidth != 0) &&
      (pstr_input_config->aac_config.full_bandwidth != 1)) {
    return (IA_EXHEAACE_CONFIG_FATAL_USE_FULL_BANDWIDTH);
  }

  for (ele_idx = 0; ele_idx < MAXIMUM_BS_ELE; ele_idx++) {
    pstr_api_struct->config[ele_idx].aac_classic = 1;
    pstr_api_struct->config[ele_idx].firstframe = 1;
    pstr_api_struct->config[ele_idx].aot = pstr_input_config->aot;
    pstr_api_struct->config[ele_idx].adts_flag = pstr_input_config->i_use_adts;
    pstr_api_struct->config[ele_idx].sample_rate = pstr_input_config->i_samp_freq;
    pstr_api_struct->config[ele_idx].native_sample_rate = pstr_input_config->i_native_samp_freq;

    pstr_api_struct->config[ele_idx].i_channels = pstr_input_config->i_channels;
    pstr_api_struct->config[ele_idx].i_native_channels = pstr_input_config->i_channels;
    pstr_api_struct->config[ele_idx].i_channels_mode = pstr_input_config->i_channels;
    pstr_api_struct->config[ele_idx].aac_config.bit_rate = pstr_input_config->i_bitrate;
    pstr_api_struct->config[ele_idx].esbr_flag = pstr_input_config->esbr_flag;
    pstr_api_struct->config[ele_idx].use_mps = pstr_input_config->i_use_mps;
    pstr_api_struct->config[ele_idx].mps_tree_config = pstr_input_config->i_mps_tree_config;
    pstr_api_struct->config[ele_idx].i_num_coupling_chan = pstr_input_config->i_num_coupling_chan;
    pstr_api_struct->config[ele_idx].ui_pcm_wd_sz = pstr_input_config->ui_pcm_wd_sz;
    pstr_api_struct->config[ele_idx].write_program_config_element =
        pstr_input_config->write_program_config_element;
    pstr_api_struct->config[ele_idx].frame_length = pstr_input_config->frame_length;
    pstr_api_struct->config[ele_idx].aac_config.num_stereo_preprocessing =
        pstr_input_config->aac_config.f_no_stereo_preprocessing;

    pstr_api_struct->config[ele_idx].aac_config.use_tns = pstr_input_config->aac_config.use_tns;
    if (pstr_input_config->aot == AOT_AAC_LD || pstr_input_config->aot == AOT_AAC_ELD) {
      pstr_api_struct->config[ele_idx].aac_config.inv_quant =
          pstr_input_config->aac_config.inv_quant;
      if (pstr_input_config->frame_length == FRAME_LEN_512) {
        pstr_api_struct->config[ele_idx].aac_config.flag_framelength_small = 0;
      } else if (pstr_input_config->frame_length == FRAME_LEN_480) {
        pstr_api_struct->config[ele_idx].aac_config.flag_framelength_small = 1;
      }
    } else if (pstr_input_config->aot == AOT_AAC_LC || pstr_input_config->aot == AOT_SBR ||
               pstr_input_config->aot == AOT_PS) {
      pstr_api_struct->config[ele_idx].aac_config.inv_quant = 0;
      if (pstr_input_config->frame_length == FRAME_LEN_1024) {
        pstr_api_struct->config[ele_idx].aac_config.flag_framelength_small = 0;
      } else if (pstr_input_config->frame_length == FRAME_LEN_960) {
        pstr_api_struct->config[ele_idx].aac_config.flag_framelength_small = 1;
      }
    }
    if ((AOT_SBR == pstr_input_config->aot) || (AOT_PS == pstr_input_config->aot) ||
        (AOT_AAC_ELD == pstr_input_config->aot)) {
      pstr_api_struct->config[ele_idx].aac_classic = 0;
    }
    if (pstr_api_struct->config[ele_idx].sample_rate < 32000) {
      if (pstr_api_struct->config[ele_idx].aac_classic == 0) {
        pstr_api_struct->config[ele_idx].aac_classic = 1;
      }
      if (pstr_input_config->aot == AOT_SBR || pstr_input_config->aot == AOT_PS) {
        pstr_input_config->aot = AOT_AAC_LC;
      }
      if (pstr_input_config->aot == AOT_AAC_ELD) {
        pstr_input_config->aot = AOT_AAC_LD;
      }
    }
    pstr_api_struct->config[ele_idx].eldsbr_found =
        !(pstr_api_struct->config[ele_idx].aac_classic);
  }
  if (pstr_input_config->aot == AOT_USAC) {
    if (pstr_input_config->i_channels > 2) {
      return IA_EXHEAACE_CONFIG_FATAL_NUM_CHANNELS;
    }
    if ((pstr_input_config->i_samp_freq < 6000) || (pstr_input_config->i_samp_freq > 96000)) {
      return (IA_EXHEAACE_CONFIG_FATAL_SAMP_FREQ);
    }

    pstr_api_struct->config[0].chmode_nchannels = pstr_api_struct->config[0].i_channels;
  } else {
    if ((pstr_input_config->i_channels > 10)) {
      return (IA_EXHEAACE_CONFIG_FATAL_NUM_CHANNELS);
    }
    if (!((pstr_input_config->i_native_samp_freq == 7350) ||
          (pstr_input_config->i_native_samp_freq == 11025) ||
          (pstr_input_config->i_native_samp_freq == 12000) ||
          (pstr_input_config->i_native_samp_freq == 16000) ||
          (pstr_input_config->i_native_samp_freq == 22050) ||
          (pstr_input_config->i_native_samp_freq == 24000) ||
          (pstr_input_config->i_native_samp_freq == 32000) ||
          (pstr_input_config->i_native_samp_freq == 44100) ||
          (pstr_input_config->i_native_samp_freq == 48000) ||
          (pstr_input_config->i_native_samp_freq == 64000) ||
          (pstr_input_config->i_native_samp_freq == 88200) ||
          (pstr_input_config->i_native_samp_freq == 96000))) {
      return (IA_EXHEAACE_CONFIG_FATAL_SAMP_FREQ);
    }

    if ((pstr_input_config->aot == AOT_AAC_ELD) && (pstr_input_config->i_use_mps == 1) &&
        (pstr_input_config->i_channels > 2)) {
      pstr_api_struct->config[0].num_bs_elements = 1;
      pstr_api_struct->config[0].i_channels = pstr_input_config->i_channels;
      pstr_api_struct->config[0].i_native_channels = pstr_input_config->i_channels;
      pstr_api_struct->config[0].chmode_nchannels = pstr_input_config->i_channels;
      pstr_api_struct->config[0].element_type = ID_SCE;
      if (pstr_api_struct->config[0].mps_tree_config == TREE_525) {
        pstr_api_struct->config[0].element_type = ID_CPE;
      }
      pstr_api_struct->config[0].element_slot = 0;
      pstr_api_struct->config[0].element_instance_tag = 0;
      pstr_api_struct->config[0].aac_config.bit_rate = pstr_input_config->i_bitrate;
      pstr_api_struct->config[0].use_parametric_stereo = 0;
    }
    if (pstr_input_config->i_channels_mask != 0) {
      if (pstr_input_config->aot != AOT_AAC_ELD || (pstr_input_config->i_use_mps != 1)) {
        WORD32 num_bs_elements, chan_config[MAXIMUM_BS_ELE], element_type[MAXIMUM_BS_ELE],
            element_slot[MAXIMUM_BS_ELE], element_instance_tag[MAXIMUM_BS_ELE],
            bitrate[MAXIMUM_BS_ELE];
        if ((pstr_input_config->i_channels_mask > 0x3FFFF)) {
          return (IA_EXHEAACE_CONFIG_FATAL_CHANNELS_MASK);
        }
        for (ele_idx = 0; ele_idx < MAXIMUM_BS_ELE; ele_idx++) {
          pstr_api_struct->config[ele_idx].i_channels_mask = pstr_input_config->i_channels_mask;
        }
        ia_enhaacplus_enc_find_channel_config(
            &num_bs_elements, chan_config, element_type, element_slot, element_instance_tag,
            pstr_api_struct->config[0].i_num_coupling_chan, pstr_input_config->i_channels_mask);
        for (ele_idx = 0; ele_idx < num_bs_elements; ele_idx++) {
          pstr_api_struct->config[ele_idx].i_channels = chan_config[ele_idx];
          pstr_api_struct->config[ele_idx].i_native_channels = chan_config[ele_idx];
          pstr_api_struct->config[ele_idx].chmode_nchannels = chan_config[ele_idx];
          pstr_api_struct->config[ele_idx].element_type = element_type[ele_idx];
          pstr_api_struct->config[ele_idx].element_slot = element_slot[ele_idx];
          pstr_api_struct->config[ele_idx].num_bs_elements = num_bs_elements;
          pstr_api_struct->config[ele_idx].element_instance_tag = element_instance_tag[ele_idx];
        }

        if (pstr_api_struct->config[0].num_bs_elements > 1) {
          ia_enhaacplus_enc_allocate_bitrate_between_channels(pstr_api_struct, bitrate,
                                                              pstr_input_config->i_bitrate);

          for (ele_idx = 0; ele_idx < pstr_api_struct->config[0].num_bs_elements; ele_idx++) {
            pstr_api_struct->config[ele_idx].aac_config.bit_rate = bitrate[ele_idx];
          }

          for (ele_idx = 0; ele_idx < MAXIMUM_BS_ELE; ele_idx++) {
            pstr_api_struct->config[ele_idx].use_parametric_stereo = 0;
          }
        }
      }
    } else if (pstr_api_struct->config[0].aac_config.dual_mono) {
      WORD32 num_bs_elements;
      WORD32 bitrate[MAXIMUM_BS_ELE];

      num_bs_elements = 2;

      pstr_api_struct->config[0].i_channels = 1;
      pstr_api_struct->config[0].chmode_nchannels = 1;
      pstr_api_struct->config[0].element_type = ID_SCE;
      pstr_api_struct->config[0].element_slot = 0;
      pstr_api_struct->config[0].num_bs_elements = num_bs_elements;
      pstr_api_struct->config[0].element_instance_tag = 0;

      pstr_api_struct->config[1].i_channels = 1;
      pstr_api_struct->config[1].chmode_nchannels = 1;
      pstr_api_struct->config[1].element_type = ID_SCE;
      pstr_api_struct->config[1].element_slot = 1;
      pstr_api_struct->config[1].num_bs_elements = num_bs_elements;
      pstr_api_struct->config[1].element_instance_tag = 1;

      if (pstr_api_struct->config[0].num_bs_elements > 1) {
        ia_enhaacplus_enc_allocate_bitrate_between_channels(
            pstr_api_struct, bitrate, pstr_api_struct->config[0].aac_config.bit_rate);

        for (ele_idx = 0; ele_idx < pstr_api_struct->config[0].num_bs_elements; ele_idx++) {
          pstr_api_struct->config[ele_idx].aac_config.bit_rate = bitrate[ele_idx];
        }

        for (ele_idx = 0; ele_idx < MAXIMUM_BS_ELE; ele_idx++) {
          pstr_api_struct->config[ele_idx].use_parametric_stereo = 0;
        }
      }
    }

    if (pstr_input_config->aot == AOT_PS && pstr_input_config->i_channels == 2) {
      pstr_api_struct->config[0].use_parametric_stereo = 1;
      pstr_api_struct->config[0].chmode_nchannels = 2;
    } else {
      for (ele_idx = 0; ele_idx < pstr_api_struct->config[0].num_bs_elements; ele_idx++) {
        pstr_api_struct->config[ele_idx].chmode_nchannels =
            pstr_api_struct->config[ele_idx].i_channels;
      }
    }

    if (pstr_input_config->aot == AOT_AAC_LD || pstr_input_config->aot == AOT_AAC_ELD) {
      WORD32 max_channel_bits = (pstr_api_struct->config[0].aac_config.flag_framelength_small
                                     ? MAXIMUM_CHANNEL_BITS_480
                                     : MAXIMUM_CHANNEL_BITS_512);
      if ((pstr_input_config->aac_config.bitreservoir_size > max_channel_bits / 8) ||
          (pstr_input_config->aac_config.bitreservoir_size < -1)) {
        pstr_input_config->aac_config.bitreservoir_size =
            BITRESERVOIR_SIZE_CONFIG_PARAM_DEFAULT_VALUE_LD;
      }
      pstr_api_struct->config[0].aac_config.bitreservoir_size =
          pstr_input_config->aac_config.bitreservoir_size;
    }
    if (pstr_input_config->aot == AOT_AAC_LC || pstr_input_config->aot == AOT_SBR ||
        pstr_input_config->aot == AOT_PS) {
      WORD32 max_channel_bits = (pstr_api_struct->config[0].aac_config.flag_framelength_small
                                     ? MAXIMUM_CHANNEL_BITS_960
                                     : MAXIMUM_CHANNEL_BITS_1024);

      if ((pstr_input_config->aac_config.bitreservoir_size > max_channel_bits / 8) ||
          (pstr_input_config->aac_config.bitreservoir_size < -1)) {
        pstr_input_config->aac_config.bitreservoir_size =
            BITRESERVOIR_SIZE_CONFIG_PARAM_DEFAULT_VALUE_LC;
      }
      pstr_api_struct->config[0].aac_config.bitreservoir_size =
          pstr_input_config->aac_config.bitreservoir_size;
    }
    pstr_api_struct->config[0].aac_config.full_bandwidth =
        pstr_input_config->aac_config.full_bandwidth;
  }

  return IA_NO_ERROR;
}

static VOID ixheaace_fill_mem_tabs(ixheaace_api_struct *pstr_api_struct, WORD32 aot) {
  WORD32 ele_idx;
  WORD32 num_channel;
  WORD32 frame_length = LEN_SUPERFRAME;
  ixheaace_mem_info_struct *pstr_mem_info;
  frame_length = pstr_api_struct->config[0].frame_length;

  {
    /* persistant */
    {
      pstr_mem_info = &pstr_api_struct->pstr_mem_info[IA_ENHAACPLUSENC_PERSIST_IDX];
      {
        if (pstr_api_struct->config[0].num_bs_elements == 1) {
          num_channel = pstr_api_struct->config[0].aac_classic
                            ? pstr_api_struct->config[0].chmode_nchannels
                            : (pstr_api_struct->config[0].use_parametric_stereo
                                   ? 1
                                   : pstr_api_struct->config[0].chmode_nchannels);
          pstr_mem_info->ui_size = sizeof(ixheaace_state_struct) +
                                   ia_enhaacplus_enc_aac_enc_pers_size(num_channel, aot) + 32;
          if (pstr_api_struct->config[0].aot != AOT_AAC_LC &&
              pstr_api_struct->config[0].aot != AOT_AAC_LD) {
            pstr_mem_info->ui_size += ixheaace_sbr_enc_pers_size(
                num_channel, pstr_api_struct->config[0].use_parametric_stereo);
          }
          pstr_mem_info->ui_size +=
              ia_enhaacplus_enc_sizeof_delay_buffer(
                  pstr_api_struct->config[0].aac_config.flag_framelength_small, aot, 3,
                  sizeof(pstr_api_struct->pstr_state->inp_delay[0]),
                  pstr_api_struct->config[0].use_mps) *
              pstr_api_struct->config[0].num_bs_elements;
        }
        if (pstr_api_struct->config[0].num_bs_elements > 1) {
          pstr_mem_info->ui_size =
              sizeof(ixheaace_state_struct) +
              ia_enhaacplus_enc_sizeof_delay_buffer(
                  pstr_api_struct->config[0].aac_config.flag_framelength_small, aot, 3,
                  sizeof(pstr_api_struct->pstr_state->inp_delay[0]),
                  pstr_api_struct->config[0].use_mps) *
                  pstr_api_struct->config[0].num_bs_elements;
          for (ele_idx = 0; ele_idx < pstr_api_struct->config[0].num_bs_elements; ele_idx++) {
            num_channel = pstr_api_struct->config[ele_idx].i_channels;
            if (pstr_api_struct->config[ele_idx].element_type != ID_LFE)
              pstr_mem_info->ui_size += ixheaace_sbr_enc_pers_size(num_channel, 0);
            pstr_mem_info->ui_size += ia_enhaacplus_enc_aac_enc_pers_size(num_channel, aot) + 32;
          }
        }

        if (pstr_api_struct->config[0].use_mps) {
          if (pstr_api_struct->config[0].aac_config.flag_framelength_small) {
            pstr_mem_info->ui_size +=
                (MAX_INPUT_SAMPLES + (INPUT_DELAY_ELDV2_480 * IXHEAACE_MAX_CH_IN_BS_ELE)) *
                sizeof(pstr_api_struct->pstr_state->time_signal_mps[0]);
          } else {
            pstr_mem_info->ui_size +=
                (MAX_INPUT_SAMPLES + (INPUT_DELAY_ELDV2_512 * IXHEAACE_MAX_CH_IN_BS_ELE)) *
                sizeof(pstr_api_struct->pstr_state->time_signal_mps[0]);
          }
          if (pstr_api_struct->config[0].mps_tree_config == TREE_212) {
            pstr_mem_info->ui_size += sizeof(ixheaace_mps_212_memory_struct) + 7;
          } else {
            pstr_mem_info->ui_size += sizeof(ixheaace_mps_515_memory_struct) + 7;
          }
          pstr_mem_info->ui_size +=
              (MAX_MPS_BS_PAYLOAD_SIZE) * sizeof(pstr_api_struct->pstr_state->mps_bs[0]);
        }

        pstr_mem_info->ui_size +=
            IXHEAACE_MAX_PAYLOAD_SIZE * pstr_api_struct->config[0].num_bs_elements;
        pstr_mem_info->ui_size += (MAX_FRAME_LEN * 2 + MAX_DS_2_1_FILTER_DELAY + INPUT_DELAY) *
                                  MAX_INPUT_CHAN * sizeof(pstr_mem_info->ui_size);
      }

      pstr_mem_info->ui_alignment = 8;
      pstr_mem_info->ui_type = IA_MEMTYPE_PERSIST;
      pstr_mem_info->ui_placement[0] = 0;
      pstr_mem_info->ui_placement[1] = 0;
      pstr_mem_info->ui_priority = IA_MEMPRIORITY_ANYWHERE;
      pstr_mem_info->ui_placed[0] = 0;
      pstr_mem_info->ui_placed[1] = 0;
    }

    /* scratch */
    {
      pstr_mem_info = &pstr_api_struct->pstr_mem_info[IA_ENHAACPLUSENC_SCRATCH_IDX];
      {
        pstr_mem_info->ui_size = ia_enhaacplus_enc_aac_enc_scr_size();
        UWORD32 sbr_scr_size = ixheaace_sbr_enc_scr_size() + ixheaace_resampler_scr_size() + 32;
        UWORD32 mps_scr_size = 0;
        if (pstr_api_struct->config[0].use_mps) {
          if (pstr_api_struct->config[0].mps_tree_config != TREE_212) {
            mps_scr_size = ixheaace_mps_515_scratch_size();
          }
        }
        pstr_mem_info->ui_size += max(sbr_scr_size, mps_scr_size);
      }
      pstr_mem_info->ui_alignment = 8;
      pstr_mem_info->ui_type = IA_MEMTYPE_SCRATCH;
      pstr_mem_info->ui_placement[0] = 0;
      pstr_mem_info->ui_placement[1] = 0;
      pstr_mem_info->ui_priority = IA_MEMPRIORITY_ANYWHERE;
      pstr_mem_info->ui_placed[0] = 0;
      pstr_mem_info->ui_placed[1] = 0;
    }

    /* input */
    {
      pstr_mem_info = &pstr_api_struct->pstr_mem_info[IA_ENHAACPLUSENC_INPUT_IDX];

      WORD32 pcm_wd_sz;
      pcm_wd_sz = pstr_api_struct->config[0].ui_pcm_wd_sz;
      num_channel = 0;
      for (ele_idx = 0; ele_idx < pstr_api_struct->config[0].num_bs_elements; ele_idx++) {
        num_channel += pstr_api_struct->config[ele_idx].i_channels;
      }
      {
        if (pstr_api_struct->config[0].aac_classic) {
          pstr_mem_info->ui_size = (frame_length * (pcm_wd_sz >> 3)) * num_channel;
        } else {
          pstr_mem_info->ui_size = (frame_length * (pcm_wd_sz >> 3)) * 2 * num_channel;
        }
      }
      pstr_mem_info->ui_alignment = 4; /* As input is used as scratch memory internally */
      pstr_mem_info->ui_type = IA_MEMTYPE_INPUT;
      pstr_mem_info->ui_placement[0] = 0;
      pstr_mem_info->ui_placement[1] = 0;
      pstr_mem_info->ui_priority = IA_MEMPRIORITY_ANYWHERE;
      pstr_mem_info->ui_placed[0] = 0;
      pstr_mem_info->ui_placed[1] = 0;
    }

    /* output */
    {
      pstr_mem_info = &pstr_api_struct->pstr_mem_info[IA_ENHAACPLUSENC_OUTPUT_IDX];
      if (aot == AOT_AAC_LC || aot == AOT_SBR || aot == AOT_PS) {
        pstr_mem_info->ui_size = (6 * frame_length / 8) * num_channel;
        pstr_mem_info->ui_size += (7) * IXHEAACE_MAX_CH_IN_BS_ELE * MAXIMUM_BS_ELE;
      } else if (aot == AOT_AAC_LD || aot == AOT_AAC_ELD) {
        pstr_mem_info->ui_size = (6 * frame_length / 8) * num_channel;
      }
      pstr_mem_info->ui_alignment = 1;
      pstr_mem_info->ui_type = IA_MEMTYPE_OUTPUT;
      pstr_mem_info->ui_placement[0] = 0;
      pstr_mem_info->ui_placement[1] = 0;
      pstr_mem_info->ui_priority = IA_MEMPRIORITY_ANYWHERE;
      pstr_mem_info->ui_placed[0] = 0;
      pstr_mem_info->ui_placed[1] = 0;
    }
  }

  return;
}

static IA_ERRORCODE ixheaace_alloc_and_assign_mem(ixheaace_api_struct *pstr_api_struct,
                                                  ixheaace_output_config *ptr_out_cfg) {
  IA_ERRORCODE err_code = IA_NO_ERROR;
  UWORD32 i_idx;
  pVOID pv_value;
  for (i_idx = 0; i_idx < 4; i_idx++) {
    UWORD32 *meminfo = (UWORD32 *)(pstr_api_struct->pstr_mem_info + i_idx);

    ptr_out_cfg->mem_info_table[i_idx].ui_size =
        *(meminfo + (IA_API_CMD_GET_MEM_INFO_SIZE - IA_API_CMD_GET_MEM_INFO_SIZE));
    ptr_out_cfg->mem_info_table[i_idx].ui_alignment =
        *(meminfo + (IA_API_CMD_GET_MEM_INFO_ALIGNMENT - IA_API_CMD_GET_MEM_INFO_SIZE));
    ptr_out_cfg->mem_info_table[i_idx].ui_type =
        *(meminfo + (IA_API_CMD_GET_MEM_INFO_TYPE - IA_API_CMD_GET_MEM_INFO_SIZE));

    ptr_out_cfg->arr_alloc_memory[ptr_out_cfg->malloc_count] =
        ptr_out_cfg->malloc_xheaace(ptr_out_cfg->mem_info_table[i_idx].ui_size,
                                    ptr_out_cfg->mem_info_table[i_idx].ui_alignment);

    if (NULL == ptr_out_cfg->arr_alloc_memory[ptr_out_cfg->malloc_count]) {
      return IA_EXHEAACE_API_FATAL_MEM_ALLOC;
    }
    memset(ptr_out_cfg->arr_alloc_memory[ptr_out_cfg->malloc_count], 0,
           ptr_out_cfg->mem_info_table[i_idx].ui_size);

#ifndef BUILD_ARM64
    ptr_out_cfg->ui_rem =
        (UWORD32)((SIZE_T)ptr_out_cfg->arr_alloc_memory[ptr_out_cfg->malloc_count] %
                  ptr_out_cfg->mem_info_table[i_idx].ui_alignment);
#else
    ptr_out_cfg->ui_rem =
        (UWORD64)((SIZE_T)ptr_out_cfg->arr_alloc_memory[ptr_out_cfg->malloc_count] %
                  ptr_out_cfg->mem_info_table[i_idx].ui_alignment);
#endif

    pv_value = ptr_out_cfg->mem_info_table[i_idx].mem_ptr =
        (pVOID)((WORD8 *)ptr_out_cfg->arr_alloc_memory[ptr_out_cfg->malloc_count] +
                ptr_out_cfg->mem_info_table[i_idx].ui_alignment - ptr_out_cfg->ui_rem);

    pstr_api_struct->pp_mem[i_idx] = ptr_out_cfg->mem_info_table[i_idx].mem_ptr;
    memset(pstr_api_struct->pp_mem[i_idx], 0, pstr_api_struct->pstr_mem_info[i_idx].ui_size);

    pstr_api_struct->pp_mem[i_idx] = pv_value;

    if (i_idx == IA_ENHAACPLUSENC_PERSIST_IDX) {
      WORD32 offset_size = sizeof(ixheaace_state_struct);
      WORD8 *p_offset = NULL;

      /* Set persistent memory pointer in api obj */
      pstr_api_struct->pstr_state = (ixheaace_state_struct *)pv_value;

      {
        WORD32 num_aac_chan;
        ixheaace_state_struct *pstr_state = pstr_api_struct->pstr_state;
        memset(pstr_api_struct->pstr_state, 0, sizeof(*(pstr_api_struct->pstr_state)));

        pstr_api_struct->pstr_state->inp_delay = (FLOAT32 *)((WORD8 *)pstr_state + offset_size);
        p_offset = (WORD8 *)pstr_api_struct->pstr_state->inp_delay +
                   ia_enhaacplus_enc_sizeof_delay_buffer(
                       pstr_api_struct->config[0].aac_config.flag_framelength_small,
                       pstr_api_struct->config[0].aot, 3,
                       sizeof(pstr_api_struct->pstr_state->inp_delay[0]),
                       pstr_api_struct->config[0].use_mps) *
                       pstr_api_struct->config[0].num_bs_elements;
#ifdef BUILD_ARM64
        p_offset = (WORD8 *)(((UWORD64)p_offset + 7) & 0xFFFFFFFFFFFFFFF8);
#else
        p_offset = (WORD8 *)(((SIZE_T)p_offset + 7) & 0xFFFFFFF8);
#endif

        if (pstr_api_struct->config[0].use_mps) {
          pstr_api_struct->pstr_state->time_signal_mps = (FLOAT32 *)(p_offset);

          memset(pstr_api_struct->pstr_state->time_signal_mps, 0,
                 (MAX_INPUT_SAMPLES + (INPUT_DELAY_ELDV2_512 * IXHEAACE_MAX_CH_IN_BS_ELE)) *
                     sizeof(pstr_api_struct->pstr_state->time_signal_mps[0]));
          p_offset += (MAX_INPUT_SAMPLES + (INPUT_DELAY_ELDV2_512 * IXHEAACE_MAX_CH_IN_BS_ELE)) *
                      sizeof(pstr_api_struct->pstr_state->time_signal_mps[0]);
          p_offset = IXHEAACE_ALIGN_MEMORY((SIZE_T)p_offset, 8);

          pstr_api_struct->pstr_state->mps_bs = (UWORD8 *)(p_offset);
          memset(pstr_api_struct->pstr_state->mps_bs, 0,
                 (MAX_MPS_BS_PAYLOAD_SIZE) * sizeof(pstr_api_struct->pstr_state->mps_bs[0]));
          p_offset += (MAX_MPS_BS_PAYLOAD_SIZE) * sizeof(pstr_api_struct->pstr_state->mps_bs[0]);
          p_offset = IXHEAACE_ALIGN_MEMORY((SIZE_T)p_offset, 8);

          if (pstr_api_struct->config[0].mps_tree_config == TREE_212) {
            pstr_api_struct->pstr_state->mps_pers_mem =
                (ixheaace_mps_212_memory_struct *)p_offset;
            p_offset += sizeof(ixheaace_mps_212_memory_struct);

            p_offset = IXHEAACE_ALIGN_MEMORY((SIZE_T)p_offset, 8);
          } else {
            pstr_api_struct->pstr_state->mps_515_pers_mem =
                (ixheaace_mps_515_memory_struct *)p_offset;
            p_offset += sizeof(ixheaace_mps_515_memory_struct);

            p_offset = IXHEAACE_ALIGN_MEMORY((SIZE_T)p_offset, 8);
          }
        } else {
          pstr_api_struct->pstr_state->mps_bs = NULL;
        }

        for (WORD32 ele_idx = 0; ele_idx < pstr_api_struct->config[0].num_bs_elements;
             ele_idx++) {
          num_aac_chan = pstr_api_struct->config[ele_idx].aac_classic
                             ? pstr_api_struct->config[ele_idx].chmode_nchannels
                             : (pstr_api_struct->config[ele_idx].use_parametric_stereo
                                    ? 1
                                    : pstr_api_struct->config[ele_idx].chmode_nchannels);

          /* Set aac enc persistent memory pointer in api obj */
          pstr_api_struct->pstr_state->aac_enc_pers_mem[ele_idx] =
              (iexheaac_encoder_str *)p_offset;
          p_offset = p_offset + ia_enhaacplus_enc_aac_enc_pers_size(
                                    num_aac_chan, pstr_api_struct->config[ele_idx].aot);

          p_offset = IXHEAACE_ALIGN_MEMORY((SIZE_T)p_offset, 8);
          if (pstr_api_struct->config[ele_idx].aot != AOT_AAC_LC &&
              pstr_api_struct->config[ele_idx].aot != AOT_AAC_LD) {
            if (pstr_api_struct->config[ele_idx].element_type != ID_LFE) {
              /* Set spectral_band_replication_ enc persistent memory pointer in api obj */
              pstr_api_struct->pstr_state->spectral_band_replication_enc_pers_mem[ele_idx] =
                  (struct ixheaace_str_sbr_enc *)p_offset;
              p_offset = p_offset + ixheaace_sbr_enc_pers_size(
                                        num_aac_chan,
                                        pstr_api_struct->config[ele_idx].use_parametric_stereo);
              p_offset = IXHEAACE_ALIGN_MEMORY((SIZE_T)p_offset, 8);
            }
          }
        }
      }
    }

    if (i_idx == IA_ENHAACPLUSENC_INPUT_IDX) {
      ptr_out_cfg->ui_inp_buf_size = ptr_out_cfg->mem_info_table[i_idx].ui_size;
    }
    ptr_out_cfg->malloc_count++;
  }
  return err_code;
}

static IA_ERRORCODE ia_enhaacplus_enc_init(ixheaace_api_struct *pstr_api_struct, WORD32 ele_idx) {
  IA_ERRORCODE error = IA_NO_ERROR;
  WORD32 anc_flag = 0;
  WORD32 anc_rate = -1;
  WORD32 anc_mode = 0;
  WORD32 core_ch;
  WORD32 asc_channels;
  WORD8 *ptr_resampler_scratch;
  iaace_scratch *pstr_aac_scratch;
  WORD8 *ptr_spectral_band_replication_scratch;
  UWORD32 core_sample_rate;
  ixheaace_state_struct *pstr_enc_state = pstr_api_struct->pstr_state;
  ixheaace_audio_specific_config_struct *pstr_asc = &pstr_enc_state->audio_specific_config;

  iaace_config *pstr_aac_config = &(pstr_api_struct->config[ele_idx].aac_config);

  ixheaace_config_ancillary *pstr_ancillary = &(pstr_api_struct->config[ele_idx].pstr_ancillary);
  pstr_aac_scratch = (iaace_scratch *)pstr_api_struct->pp_mem[IA_ENHAACPLUSENC_SCRATCH_IDX];
  ptr_spectral_band_replication_scratch =
      ((pWORD8)pstr_aac_scratch) + ia_enhaacplus_enc_aac_enc_scr_size();
  ptr_resampler_scratch = ((pWORD8)pstr_aac_scratch) + ia_enhaacplus_enc_aac_enc_scr_size() +
                          ixheaace_sbr_enc_scr_size();

  /* fill pstr_ancillary data */
  pstr_ancillary->anc_flag = anc_flag;
  pstr_ancillary->anc_mode = anc_mode;
  pstr_ancillary->anc_rate = anc_rate;
  pstr_api_struct->pstr_state->temp_buff_aac =
      (void *)pstr_api_struct->pp_mem[IA_ENHAACPLUSENC_SCRATCH_IDX];
  pstr_api_struct->pstr_state->temp_buff_sbr =
      (void *)((WORD8 *)pstr_api_struct->pp_mem[IA_ENHAACPLUSENC_SCRATCH_IDX] +
               ia_enhaacplus_enc_aac_enc_scr_size());

  if (pstr_api_struct->config[ele_idx].use_mps &&
      pstr_api_struct->config[ele_idx].mps_tree_config != TREE_212) {
    pstr_api_struct->pstr_state->mps_scratch =
        (void *)((WORD8 *)pstr_api_struct->pp_mem[IA_ENHAACPLUSENC_SCRATCH_IDX] +
                 ia_enhaacplus_enc_aac_enc_scr_size());

    pstr_api_struct->pstr_state->ptr_temp_buff_resamp =
        (void *)((WORD8 *)pstr_api_struct->pp_mem[IA_ENHAACPLUSENC_SCRATCH_IDX] +
                 ia_enhaacplus_enc_aac_enc_scr_size() + ixheaace_sbr_enc_scr_size());
  } else {
    pstr_api_struct->pstr_state->ptr_temp_buff_resamp =
        (void *)((WORD8 *)pstr_api_struct->pp_mem[IA_ENHAACPLUSENC_SCRATCH_IDX] +
                 ia_enhaacplus_enc_aac_enc_scr_size() + ixheaace_sbr_enc_scr_size());
  }

  if (pstr_api_struct->config[ele_idx].chmode_nchannels == 2) {
    /* When the chmode option is not used,
      the number of channels depends on the input file */
    pstr_api_struct->config[ele_idx].chmode_nchannels =
        pstr_api_struct->config[ele_idx].i_channels;
  }

  switch (pstr_api_struct->pstr_state->aot) {
    case AOT_AAC_LC:
    case AOT_SBR:
    case AOT_PS:
      pstr_api_struct->pstr_state->buffer_offset = INPUT_DELAY_LC;
      break;

    case AOT_AAC_LD:
      if (pstr_aac_config->flag_framelength_small == 1) {
        pstr_api_struct->pstr_state->buffer_offset = INPUT_DELAY_LD_480;
      } else {
        pstr_api_struct->pstr_state->buffer_offset = INPUT_DELAY_LD_512;
      }
      break;

    case AOT_AAC_ELD:
      if (pstr_aac_config->flag_framelength_small == 1) {
        pstr_api_struct->pstr_state->buffer_offset = INPUT_DELAY_ELD_480;
      } else {
        pstr_api_struct->pstr_state->buffer_offset = INPUT_DELAY_ELD_512;
      }
      break;
  }
  pstr_api_struct->pstr_state->downsample[ele_idx] = 0;
  pstr_api_struct->config->frame_count = 0;
  core_ch = pstr_api_struct->config[ele_idx].chmode_nchannels;
  pstr_aac_config->sample_rate = pstr_api_struct->config[ele_idx].sample_rate;
  pstr_aac_config->core_sample_rate = pstr_api_struct->config[ele_idx].sample_rate;
  pstr_aac_config->native_sample_rate = pstr_api_struct->config[ele_idx].native_sample_rate;
  pstr_aac_config->num_in_channels = pstr_api_struct->config[ele_idx].i_channels;
  pstr_aac_config->num_out_channels = pstr_api_struct->config[ele_idx].chmode_nchannels;
  if (pstr_api_struct->config[ele_idx].aac_classic == 0) {
    pstr_aac_config->core_sample_rate = pstr_aac_config->core_sample_rate / 2;
  }
  if (pstr_api_struct->pstr_state->mps_enable) {
    switch (pstr_api_struct->pstr_state->mps_tree_config) {
      case TREE_212:
      case TREE_5151:
      case TREE_5152:
        pstr_aac_config->num_out_channels = 1;
        core_ch = 1;
        break;

      case TREE_525:
        pstr_aac_config->num_out_channels = 2;
        core_ch = 2;
        break;
    }
  }

  if (pstr_api_struct->config[ele_idx].use_parametric_stereo) {
    pstr_api_struct->config[ele_idx].chmode_nchannels = 2;
    pstr_aac_config->num_out_channels = 1;
  }
  if ((pstr_api_struct->config[ele_idx].i_channels == 2) &&
      (pstr_api_struct->config[ele_idx].chmode == 0)) {
    pstr_api_struct->config[ele_idx].chmode_nchannels = 1;
    pstr_aac_config->num_out_channels = 1;
  }
  if ((pstr_api_struct->config[ele_idx].use_parametric_stereo) &&
      (pstr_api_struct->config[ele_idx].aac_classic)) {
    return (IA_EXHEAACE_CONFIG_FATAL_AAC_CLASSIC_WITH_PS);
  }

  if (!(pstr_api_struct->config[ele_idx].adts_flag) &&
      (pstr_api_struct->config[0].aac_config.calc_crc)) {
    // set here default for crc as 0
    pstr_api_struct->config[0].aac_config.calc_crc = 0;
    return (IA_EXHEAACE_CONFIG_NONFATAL_INVALID_CONFIG);
  }

  if ((pstr_api_struct->config[0].chmode == 3) &&
      (pstr_api_struct->config[0].aac_config.calc_crc == 1)) {
    // dual mono case so crc if enabled must be disabled
    pstr_api_struct->config[0].aac_config.calc_crc = 0;
    return (IA_EXHEAACE_CONFIG_NONFATAL_INVALID_CONFIG);
  }

  pstr_aac_config->bit_rate = ixheaac_min32(360000 * core_ch, pstr_aac_config->bit_rate);
  pstr_aac_config->bit_rate = ixheaac_max32(8000 * core_ch, pstr_aac_config->bit_rate);
  switch (pstr_api_struct->pstr_state->aot) {
    case AOT_AAC_LC:
    case AOT_SBR:
    case AOT_PS:
      pstr_aac_config->bit_rate = ia_enhaacplus_aac_limitbitrate(
          pstr_aac_config->core_sample_rate,
          (pstr_aac_config->flag_framelength_small ? FRAME_LEN_960 : FRAME_LEN_1024),
          pstr_api_struct->config[ele_idx].chmode_nchannels, pstr_aac_config->bit_rate);
      break;

    case AOT_AAC_LD:
    case AOT_AAC_ELD:
      pstr_aac_config->bit_rate = ia_enhaacplus_aac_limitbitrate(
          pstr_aac_config->core_sample_rate,
          (pstr_aac_config->flag_framelength_small ? FRAME_LEN_480 : FRAME_LEN_512), core_ch,
          pstr_aac_config->bit_rate);
      break;
  }
  if (pstr_api_struct->config[ele_idx].eldsbr_found == 1) {
    WORD32 num_iter = 0;
    WORD32 initial_bitrate, adjusted_bitrate;
    adjusted_bitrate = pstr_aac_config->bit_rate;

    /* Find total bitrate which provides valid configuration for each SBR element. */
    do {
      WORD32 e;
      WORD32 q_format;
      initial_bitrate = adjusted_bitrate;

      for (e = 0; e < pstr_api_struct->config[ele_idx].num_bs_elements; e++) {
        WORD32 sbr_element_bitate_in, sbr_bitrate_out;

        sbr_element_bitate_in = pstr_aac_config->bit_rate;
        sbr_bitrate_out = ixheaace_sbr_limit_bitrate(
            sbr_element_bitate_in, core_ch, pstr_aac_config->core_sample_rate,
            pstr_api_struct->spectral_band_replication_tabs.ptr_qmf_tab,
            pstr_api_struct->pstr_state->aot);

        if (sbr_bitrate_out == 0) {
          pstr_aac_config->bit_rate = 0;
        }

        /* If bitrates don't match, distribution and limiting needs to be determined again.
        Abort element loop and restart with adapted bitrate. */
        if (sbr_element_bitate_in != sbr_bitrate_out) {
          if (sbr_element_bitate_in < sbr_bitrate_out) {
            adjusted_bitrate = ixheaac_max32(
                initial_bitrate,
                (WORD32)ixheaac_div32((WORD32)(sbr_bitrate_out + 8), MAX_32, &q_format));
            adjusted_bitrate = adjusted_bitrate >> (q_format - 31);
            break;
          }

          if (sbr_element_bitate_in > sbr_bitrate_out) {
            adjusted_bitrate = ixheaac_min32(
                initial_bitrate,
                (WORD32)ixheaac_div32((WORD32)(sbr_bitrate_out - 8), MAX_32, &q_format));
            break;
          }

        } /* sbr_element_bitate_in != sbr_bitrate_out */

      } /* elements */

      num_iter++; /* restrict iteration to worst case of num elements */

    } while ((initial_bitrate != adjusted_bitrate) &&
             (num_iter <= pstr_api_struct->config[ele_idx].num_bs_elements));

    /* Unequal bitrates mean that no reasonable bitrate configuration found. */
    pstr_aac_config->bit_rate = (initial_bitrate == adjusted_bitrate) ? adjusted_bitrate : 0;
  }

  pstr_asc->str_aac_config.frame_length_flag = pstr_aac_config->flag_framelength_small;
  pstr_asc->sampling_frequency = pstr_api_struct->config[0].native_sample_rate;
  asc_channels = pstr_api_struct->config[ele_idx].i_channels_mode;
  pstr_asc->channel_configuration =
      (pstr_api_struct->pstr_state->mps_enable ? core_ch : asc_channels);
  pstr_asc->channel_configuration =
      pstr_api_struct->config[ele_idx].use_parametric_stereo
          ? 1
          : (pstr_api_struct->pstr_state->mps_enable
                 ? core_ch
                 : pstr_api_struct->config[ele_idx].i_channels_mode);

  if (!(pstr_api_struct->config[ele_idx].aac_classic) &&
      ixheaace_is_sbr_setting_available(
          pstr_aac_config->bit_rate,
          (pstr_api_struct->pstr_state->mps_enable
               ? 1
               : pstr_api_struct->config[ele_idx].chmode_nchannels),
          pstr_aac_config->sample_rate, &core_sample_rate,
          pstr_api_struct->spectral_band_replication_tabs.ptr_qmf_tab,
          pstr_api_struct->pstr_state->aot)) {
    ixheaace_str_sbr_cfg spectral_band_replication_config;
    ixheaace_initialize_sbr_defaults(&spectral_band_replication_config);

    spectral_band_replication_config.use_ps =
        pstr_api_struct->config[ele_idx].use_parametric_stereo;
    spectral_band_replication_config.crc_sbr = 0;
    spectral_band_replication_config.parametric_coding = 1;
    spectral_band_replication_config.is_esbr = pstr_api_struct->config[0].esbr_flag;
    if (pstr_api_struct->pstr_state->aot == AOT_AAC_ELD) {
      spectral_band_replication_config.is_ld_sbr = 1;
      spectral_band_replication_config.sbr_codec = ELD_SBR;
      spectral_band_replication_config.frame_flag_480 = pstr_aac_config->flag_framelength_small;
    } else if (pstr_api_struct->pstr_state->aot == AOT_SBR ||
               pstr_api_struct->pstr_state->aot == AOT_PS) {
      spectral_band_replication_config.frame_flag_960 = pstr_aac_config->flag_framelength_small;
      spectral_band_replication_config.sbr_codec = HEAAC_SBR;
    }

    if (pstr_api_struct->config[ele_idx].aac_classic == 0) {
      pstr_api_struct->pstr_state->downsample[ele_idx] = 1;
    }

    if (pstr_api_struct->pstr_state->mps_enable) {
      if (pstr_api_struct->pstr_state->mps_tree_config == TREE_212) {
        WORD32 delay =
            ((pstr_aac_config->flag_framelength_small ? FRAME_LEN_480 / 2 : FRAME_LEN_512 / 2) *
                 2 +
             4);
        ixheaace_mps_212_memory_struct *pstr_mps_memory;
        pstr_mps_memory = pstr_api_struct->pstr_state->mps_pers_mem;
        ixheaace_mps_212_open(&pstr_api_struct->pstr_mps_212_enc, pstr_mps_memory);

        pstr_asc->str_aac_config.num_sac_cfg_bits = 0;

        error = ixheaace_mps_212_initialise(
            pstr_api_struct->pstr_mps_212_enc, AOT_AAC_ELD, pstr_aac_config->sample_rate,
            &pstr_aac_config->bit_rate,
            pstr_api_struct->config[ele_idx].eldsbr_found ? 2 /*hAacConfig->sbrRatio*/ : 0,
            (pstr_aac_config->flag_framelength_small ? FRAME_LEN_480 * 2 : FRAME_LEN_512 * 2),
            /* for dual rate sbr this value is already multiplied by 2 */
            (pstr_aac_config->flag_framelength_small
                 ? FRAME_LEN_480 * 2
                 : FRAME_LEN_512 * 2) /* samples read per ch*/,
            delay, (WORD8 *)pstr_api_struct->pstr_state->ptr_temp_buff_resamp);
        if (error) {
          return error;
        }

        pstr_asc->str_aac_config.num_sac_cfg_bits = ixheaace_mps_212_get_spatial_specific_config(
            pstr_api_struct->pstr_mps_212_enc, (WORD8 *)pstr_asc->str_aac_config.sac_cfg_data,
            sizeof(pstr_asc->str_aac_config.sac_cfg_data), AOT_AAC_ELD);
        pstr_asc->str_aac_config.eld_ext_type[0] = IAAC_ELDEXT_LDSAC;
        pstr_asc->str_aac_config.eld_ext_len[0] =
            (pstr_asc->str_aac_config.num_sac_cfg_bits + 7) >> 3;
      } else {
        WORD32 tree_config;
        WORD32 bits_written;
        ixheaace_bit_buf bit_buf;
        ixheaace_bit_buf_handle ptr_bit_buf;
        ixheaace_mps_515_memory_struct *pstr_mps_memory;
        if (pstr_api_struct->pstr_state->mps_tree_config == TREE_525) {
          tree_config = 525;
        } else if (pstr_api_struct->pstr_state->mps_tree_config == TREE_5152) {
          tree_config = 5152;
        } else {
          tree_config = 5151;
        }
        pstr_mps_memory = pstr_api_struct->pstr_state->mps_515_pers_mem;
        ptr_bit_buf = ia_enhaacplus_enc_create_bitbuffer(
            &bit_buf, (UWORD8 *)pstr_asc->str_aac_config.sac_cfg_data,
            sizeof(pstr_asc->str_aac_config.sac_cfg_data));

        error = ixheaace_mps_515_open(
            &pstr_api_struct->pstr_mps_515_enc, pstr_aac_config->sample_rate, tree_config,
            ptr_bit_buf, &bits_written, pstr_mps_memory, pstr_aac_config->flag_framelength_small);
        pstr_asc->str_aac_config.num_sac_cfg_bits = bits_written;
        pstr_asc->str_aac_config.eld_ext_type[0] = IAAC_ELDEXT_LDSAC;
        pstr_asc->str_aac_config.eld_ext_len[0] = (bits_written + 7) >> 3;
        if (error) {
          return error;
        }
      }
    }

    ixheaace_adjust_sbr_settings(&spectral_band_replication_config, pstr_aac_config->bit_rate,
                                 pstr_aac_config->num_out_channels, core_sample_rate,
                                 AACENC_TRANS_FAC, 24000,
                                 pstr_api_struct->spectral_band_replication_tabs.ptr_qmf_tab,
                                 pstr_api_struct->pstr_state->aot);

    if (pstr_api_struct->config[ele_idx].element_type != ID_LFE) {
      /* open SBR PART, set core bandwidth */
      error = ixheaace_env_open(
          &pstr_api_struct->pstr_state->spectral_band_replication_enc_pers_mem[ele_idx],
          &spectral_band_replication_config, &pstr_aac_config->band_width,
          ptr_spectral_band_replication_scratch,
          &(pstr_api_struct->spectral_band_replication_tabs),
          &pstr_asc->str_aac_config.sbr_config);
      pstr_asc->str_aac_config.ld_sbr_present_flag =
          pstr_api_struct->config[ele_idx].eldsbr_found;
      if (error) {
        return error;
      }
      pstr_asc->str_aac_config.ld_sbr_sample_rate =
          (spectral_band_replication_config.codec_settings.sample_freq !=
           pstr_aac_config->sample_rate)
              ? 1
              : 0;
      pstr_asc->str_aac_config.ld_sbr_crc_flag = spectral_band_replication_config.crc_sbr;
    }

    if (!pstr_api_struct->config[ele_idx].use_parametric_stereo) {
      IA_ERRORCODE resamp_error = IA_NO_ERROR;
      WORD32 ch_idx = 0;

      resamp_error = ia_enhaacplus_enc_init_iir_resampler(
          &(pstr_api_struct->pstr_state->down_sampler[ele_idx][ch_idx]), 2,
          pstr_api_struct->spectral_band_replication_tabs.ptr_resamp_tab);
      if (resamp_error) {
        return resamp_error;
      }
      if (pstr_api_struct->config[ele_idx].i_channels > 1) {
        resamp_error = ia_enhaacplus_enc_init_iir_resampler(
            &(pstr_api_struct->pstr_state->down_sampler[ele_idx][ch_idx + 1]), 2,
            pstr_api_struct->spectral_band_replication_tabs.ptr_resamp_tab);
        if (resamp_error) {
          return resamp_error;
        }
      }
    }
  } else {
    if (!(pstr_api_struct->config[ele_idx].aac_classic &&
          !(pstr_api_struct->config[ele_idx].eldsbr_found))) {
      if (pstr_api_struct->config[ele_idx].aac_classic == 0) {
        error = IA_EXHEAACE_INIT_FATAL_AACPLUS_NOT_AVAIL;
      } else {
        error = IA_EXHEAACE_INIT_FATAL_BITRATE_NOT_SUPPORTED;
      }
      return error;
    }
    pstr_api_struct->pstr_state->buffer_offset = 0;
    pstr_api_struct->config[ele_idx].aac_classic = 1;
  }
  {
    WORD32 *shared_buf1, *shared_buf2;
    WORD32 *shared_buf3;
    WORD8 *shared_buf4;
    switch (pstr_api_struct->pstr_state->aot) {
      case AOT_AAC_LC:
      case AOT_SBR:
      case AOT_PS:
        ia_enhaacplus_enc_get_shared_bufs(
            ptr_spectral_band_replication_scratch, &shared_buf1, &shared_buf2, &shared_buf3,
            &shared_buf4,
            (pstr_aac_config->flag_framelength_small == 1) ? FRAME_LEN_960 : FRAME_LEN_1024);
        break;

      case AOT_AAC_LD:
      case AOT_AAC_ELD:
        ia_enhaacplus_enc_get_shared_bufs(
            ptr_spectral_band_replication_scratch, &shared_buf1, &shared_buf2, &shared_buf3,
            &shared_buf4,
            (pstr_aac_config->flag_framelength_small == 1) ? FRAME_LEN_480 : FRAME_LEN_512);
        break;
    }

    ia_enhaacplus_enc_set_shared_bufs(pstr_aac_scratch, &shared_buf1, &shared_buf2, &shared_buf3,
                                      &ptr_resampler_scratch);

    error = ia_enhaacplus_enc_aac_enc_open(
        &(pstr_api_struct->pstr_state->aac_enc_pers_mem[ele_idx]),
        *pstr_aac_config /*, *pstr_ancillary*/, pstr_aac_scratch,
        &(pstr_api_struct->pstr_aac_tabs), pstr_api_struct->config[ele_idx].element_type,
        pstr_api_struct->config[ele_idx].element_instance_tag, 1,
        pstr_api_struct->pstr_state->aot);
    if (error != IA_NO_ERROR) {
      return error;
    }
    if (error) {
      if (pstr_api_struct->pstr_mps_212_enc && pstr_api_struct->pstr_state->mps_enable) {
        ixheaace_mps_212_close(&(pstr_api_struct->pstr_mps_212_enc));
      }
      if (pstr_api_struct->pstr_mps_515_enc && pstr_api_struct->pstr_state->mps_enable) {
        ixheaace_mps_515_close(pstr_api_struct->pstr_mps_212_enc);
      }

      return (IA_EXHEAACE_INIT_FATAL_AAC_INIT_FAILED);
    }
  }

  return error;
}

static IA_ERRORCODE ia_enhaacplus_enc_execute(ixheaace_api_struct *pstr_api_struct,
                                              WORD32 ele_idx) {
  IA_ERRORCODE error = IA_NO_ERROR;
  WORD32 downsample;
  WORD32 aot;
  WORD32 idx;
  WORD32 header_bytes = 0;
  WORD32 num_samples_read;
  WORD32 env_read_offset = 0;
  WORD32 write_off_set = 0;
  WORD32 aacenc_blocksize;
  WORD32 ch, out_samples;
  WORD32 total_channels = 0, ele, slot;
  WORD32 time_in_stride;
  WORD32 num_bs_elements;
  FLAG flag_last_element;
  WORD32 i_num_coup_channels;
  WORD32 i_channels_mask;
  ixheaace_pstr_sbr_enc pstr_sbr_encoder = NULL;
  iexheaac_encoder_str **pstr_aac_enc;
  iaace_config *pstr_aac_config;
  pWORD16 pw_inp_buf = NULL;
  pUWORD8 pub_out_buf = NULL;
  FLOAT32 *ptr_input_buffer = NULL;
  FLOAT32 *ptr_input_buffer_mps = NULL;
  FLOAT32 *shared_buf1_ring, *shared_buf2_ring;
  WORD32 out_stride = IXHEAACE_MAX_CH_IN_BS_ELE;
  WORD32 *pstr_aac_scratch = (pWORD32)pstr_api_struct->pp_mem[IA_ENHAACPLUSENC_SCRATCH_IDX];
  WORD8 *ptr_spectral_band_replication_scratch =
      ((pWORD8)pstr_aac_scratch) + ia_enhaacplus_enc_aac_enc_scr_size();
  WORD32 *total_fill_bits = &(pstr_api_struct->pstr_state->total_fill_bits);
  WORD32 *write_program_config_element =
      &(pstr_api_struct->config[0].write_program_config_element);

  aot = pstr_api_struct->pstr_state->aot;
  num_bs_elements = pstr_api_struct->config[0].num_bs_elements;
  flag_last_element = (ele_idx == (num_bs_elements - 1));
  i_num_coup_channels = pstr_api_struct->config[0].i_num_coupling_chan;
  i_channels_mask = pstr_api_struct->config[0].i_channels_mask;
  aacenc_blocksize = pstr_api_struct->config[0].frame_length;
  downsample = pstr_api_struct->pstr_state->downsample[ele_idx];
  num_samples_read =
      aacenc_blocksize * pstr_api_struct->config[ele_idx].i_native_channels * (1 << downsample);
  for (ele = 0; ele < num_bs_elements; ele++) {
    total_channels += pstr_api_struct->config[ele].i_native_channels;
  }
  pstr_aac_config = &(pstr_api_struct->config[ele_idx].aac_config);
  if ((pstr_api_struct->config[ele_idx].aac_classic == 0) &&
      pstr_api_struct->config[ele_idx].use_parametric_stereo) {
    time_in_stride = 1;
  } else if ((pstr_api_struct->config[ele_idx].aac_classic == 1) &&
             (pstr_api_struct->config[ele_idx].num_bs_elements == 1)) {
    time_in_stride = pstr_aac_config->num_out_channels;
  } else {
    time_in_stride = IXHEAACE_MAX_CH_IN_BS_ELE;
  }
  pstr_aac_enc = pstr_api_struct->pstr_state->aac_enc_pers_mem;

  pw_inp_buf = (pWORD16)pstr_api_struct->pp_mem[IA_ENHAACPLUSENC_INPUT_IDX];
  if (ele_idx == 0) {
    pstr_api_struct->pstr_state->i_out_bytes = 0;
    pub_out_buf = ((pUWORD8)pstr_api_struct->pp_mem[IA_ENHAACPLUSENC_OUTPUT_IDX]);
    *total_fill_bits = 0;
  }
#ifndef BUILD_ARM64
  if (pstr_api_struct->config->adts_flag) {
    pub_out_buf = (pUWORD8)((SIZE_T)pub_out_buf + 7);
  }
#else
  if (pstr_api_struct->config->adts_flag) {
    pub_out_buf = (pUWORD8)((UWORD64)pub_out_buf + 7);
  }
#endif
  for (ch = 0; ch < IXHEAACE_MAX_CH_IN_BS_ELE; ch++)
    pstr_api_struct->pstr_state->num_anc_data_bytes[ele_idx][ch] = 0;

  if (aot == AOT_SBR || aot == AOT_PS) {
    write_off_set = INPUT_DELAY_LC * IXHEAACE_MAX_CH_IN_BS_ELE;
  } else if (aot == AOT_AAC_ELD && pstr_api_struct->pstr_state->mps_enable != 1) {
    if (pstr_api_struct->config[0].aac_config.flag_framelength_small)
      write_off_set = INPUT_DELAY_ELD_480 * IXHEAACE_MAX_CH_IN_BS_ELE;
    else
      write_off_set = INPUT_DELAY_ELD_512 * IXHEAACE_MAX_CH_IN_BS_ELE;
  } else if (aot == AOT_AAC_ELD && pstr_api_struct->pstr_state->mps_enable == 1 &&
             pstr_api_struct->pstr_state->mps_tree_config == TREE_212) {
    if (pstr_api_struct->config[0].aac_config.flag_framelength_small)
      write_off_set = INPUT_DELAY_ELDV2_480 * IXHEAACE_MAX_CH_IN_BS_ELE;
    else
      write_off_set = INPUT_DELAY_ELDV2_512 * IXHEAACE_MAX_CH_IN_BS_ELE;
  } else if (aot == AOT_AAC_ELD && pstr_api_struct->pstr_state->mps_enable == 1 &&
             pstr_api_struct->pstr_state->mps_tree_config != TREE_212) {
    write_off_set = INPUT_DELAY_ELD_512_MPS * IXHEAACE_MAX_CH_IN_BS_ELE;
  }
  if (pstr_api_struct->config[ele_idx].aac_classic == 1) {
    write_off_set = 0;
  }
  if (aot == AOT_AAC_ELD && pstr_api_struct->pstr_state->mps_enable == 1) {
    if (pstr_api_struct->pstr_state->mps_tree_config == TREE_212) {
      env_read_offset = INPUT_DELAY_ELD_512_MPS;
    } else {
      env_read_offset = write_off_set;
    }
  }
  if (pstr_api_struct->pstr_state->downsample[ele_idx]) {
    if (!pstr_api_struct->config[ele_idx].use_parametric_stereo) {
      if (aot == AOT_AAC_ELD && pstr_api_struct->pstr_state->mps_enable != 1) {
        write_off_set +=
            4 * IXHEAACE_MAX_CH_IN_BS_ELE;  // Downsampler delay = 4 samples per channel @input SR
      } else {
        write_off_set += (pstr_api_struct->pstr_state->down_sampler[ele_idx][0].delay) *
                         IXHEAACE_MAX_CH_IN_BS_ELE;
      }
    }
    if (pstr_api_struct->config[ele_idx].use_parametric_stereo) {
      env_read_offset = (MAXIMUM_DS_2_1_FILTER_DELAY + INPUT_DELAY) * IXHEAACE_MAX_CH_IN_BS_ELE;
      write_off_set = env_read_offset;
    }
  }

  if (aot == AOT_AAC_LC || aot == AOT_SBR || aot == AOT_PS) {
    ptr_input_buffer = pstr_api_struct->pstr_state->inp_delay +
                       (aacenc_blocksize * 2 + MAXIMUM_DS_2_1_FILTER_DELAY + INPUT_DELAY_LC) *
                           IXHEAACE_MAX_CH_IN_BS_ELE * ele_idx;
  } else if (aot == AOT_AAC_LD) {
    if (pstr_api_struct->config[0].aac_config.flag_framelength_small) {
      ptr_input_buffer = pstr_api_struct->pstr_state->inp_delay +
                         (FRAME_LEN_480 * 2 + MAXIMUM_DS_2_1_FILTER_DELAY + INPUT_DELAY_LD_480) *
                             IXHEAACE_MAX_CH_IN_BS_ELE * ele_idx;
    } else {
      ptr_input_buffer = pstr_api_struct->pstr_state->inp_delay +
                         (FRAME_LEN_512 * 2 + MAXIMUM_DS_2_1_FILTER_DELAY + INPUT_DELAY_LD_512) *
                             IXHEAACE_MAX_CH_IN_BS_ELE * ele_idx;
    }
  } else if (aot == AOT_AAC_ELD) {
    if (pstr_api_struct->config[0].aac_config.flag_framelength_small) {
      if (pstr_api_struct->pstr_state->mps_enable == 1) {
        ptr_input_buffer =
            pstr_api_struct->pstr_state->inp_delay +
            (FRAME_LEN_480 * 2 + MAXIMUM_DS_2_1_FILTER_DELAY + INPUT_DELAY_ELDV2_480) *
                IXHEAACE_MAX_CH_IN_BS_ELE * ele_idx;
      } else {
        ptr_input_buffer =
            pstr_api_struct->pstr_state->inp_delay +
            (FRAME_LEN_480 * 2 + MAXIMUM_DS_2_1_FILTER_DELAY + INPUT_DELAY_ELD_480) *
                IXHEAACE_MAX_CH_IN_BS_ELE * ele_idx;
      }
    } else {
      if (pstr_api_struct->pstr_state->mps_enable == 1) {
        ptr_input_buffer =
            pstr_api_struct->pstr_state->inp_delay +
            (FRAME_LEN_512 * 2 + MAXIMUM_DS_2_1_FILTER_DELAY + INPUT_DELAY_ELDV2_512) *
                IXHEAACE_MAX_CH_IN_BS_ELE * ele_idx;
      } else {
        ptr_input_buffer =
            pstr_api_struct->pstr_state->inp_delay +
            (FRAME_LEN_512 * 2 + MAXIMUM_DS_2_1_FILTER_DELAY + INPUT_DELAY_ELD_512) *
                IXHEAACE_MAX_CH_IN_BS_ELE * ele_idx;
      }
    }
  } else {
    return IA_EXHEAACE_EXE_FATAL_UNSUPPORTED_AOT;
  }

  if (aot != AOT_AAC_LD && aot != AOT_AAC_LC) {
    pstr_sbr_encoder =
        pstr_api_struct->pstr_state->spectral_band_replication_enc_pers_mem[ele_idx];
    if (pstr_api_struct->config[ele_idx].element_type != ID_LFE) {
      ixheaace_sbr_set_scratch_ptr(pstr_sbr_encoder, ptr_spectral_band_replication_scratch);
    }
  }

  {
    ixheaace_mps_enc_ext_payload mps_extension_payload;
    UWORD8 *mps_bs = pstr_api_struct->pstr_state->mps_bs;
    memset(&mps_extension_payload, 0, sizeof(ixheaace_mps_enc_ext_payload));
    mps_extension_payload.p_data = mps_bs;

    if ((pstr_api_struct->config[ele_idx].num_bs_elements == 1) &&
        (pstr_api_struct->config[ele_idx].i_channels <= 2)) {
      if (pstr_api_struct->config[ele_idx].aac_classic != 1) {
        if ((pstr_api_struct->config[ele_idx].i_channels == 2 &&
             pstr_api_struct->config[ele_idx].chmode_nchannels == 2) &&
            (!((pstr_api_struct->pstr_mps_212_enc != NULL) &&
               pstr_api_struct->pstr_state->mps_enable))) {
          for (idx = 0; idx < (num_samples_read); idx++) {
            ptr_input_buffer[write_off_set + idx] = (FLOAT32)pw_inp_buf[idx];
          }
        } else if (pstr_api_struct->config[ele_idx].i_channels == 1) {
          for (idx = 0; idx < num_samples_read; idx++) {
            ptr_input_buffer[write_off_set + (IXHEAACE_MAX_CH_IN_BS_ELE * idx)] =
                (FLOAT32)pw_inp_buf[idx];
          }
        } else if ((pstr_api_struct->pstr_mps_212_enc != NULL) &&
                   pstr_api_struct->pstr_state->mps_enable) {
          ptr_input_buffer_mps = pstr_api_struct->pstr_state->time_signal_mps;
          for (idx = 0; idx < (num_samples_read / 2); idx++) {
            ptr_input_buffer_mps[idx] = (FLOAT32)pw_inp_buf[2 * idx];
            ptr_input_buffer_mps[(num_samples_read / 2) + idx] =
                (FLOAT32)pw_inp_buf[(2 * idx) + 1];
          }
        }
      } else {
        for (idx = 0; idx < (num_samples_read + write_off_set); idx++) {
          ptr_input_buffer[idx] = (FLOAT32)pw_inp_buf[idx];
        }
      }
    } else {
      if (pstr_api_struct->config[ele_idx].i_channels == 2) {
        slot = pstr_api_struct->config[ele_idx].element_slot;
        for (idx = 0; idx < num_samples_read / 2; idx++) {
          ptr_input_buffer[2 * idx + write_off_set] =
              (FLOAT32)pw_inp_buf[total_channels * idx + slot];
          ptr_input_buffer[2 * idx + write_off_set + 1] =
              (FLOAT32)pw_inp_buf[total_channels * idx + slot + 1];
        }
      }

      if (pstr_api_struct->config[ele_idx].i_channels == 1) {
        slot = pstr_api_struct->config[ele_idx].element_slot;
        for (idx = 0; idx < num_samples_read; idx++) {
          ptr_input_buffer[write_off_set + (IXHEAACE_MAX_CH_IN_BS_ELE * idx)] =
              (FLOAT32)pw_inp_buf[total_channels * idx + slot];
        }
      }

      if (pstr_api_struct->config[ele_idx].i_channels == 6) {
        ptr_input_buffer_mps = pstr_api_struct->pstr_state->time_signal_mps;
        for (idx = 0; idx < num_samples_read; idx++) {
          ptr_input_buffer_mps[idx] = (FLOAT32)pw_inp_buf[idx];
        }
      }
    }

    if ((pstr_api_struct->pstr_mps_212_enc != NULL) && pstr_api_struct->pstr_state->mps_enable) {
      ptr_input_buffer_mps = pstr_api_struct->pstr_state->time_signal_mps;
      error = ixheaace_mps_212_process(pstr_api_struct->pstr_mps_212_enc, ptr_input_buffer_mps,
                                       num_samples_read, &mps_extension_payload);
      if (error) {
        return error;
      }
      num_samples_read /= 2;
      for (idx = 0; idx < num_samples_read; idx++) {
        ptr_input_buffer[2 * idx + write_off_set] = (FLOAT32)ptr_input_buffer_mps[idx];
      }
      env_read_offset = write_off_set;
    }
    if ((pstr_api_struct->pstr_mps_515_enc != NULL) && pstr_api_struct->pstr_state->mps_enable) {
      ixheaace_bit_buf bit_buf;
      ixheaace_bit_buf_handle ptr_bit_buf = NULL;
      FLOAT32 *ptr_downmix_buffer_mps = pstr_api_struct->pstr_state->mps_scratch;
      VOID *ptr_scratch_515_mps = (VOID *)(pstr_api_struct->pstr_state->mps_scratch +
                                           (MAX_INPUT_CHANNELS * MAX_BUFFER_SIZE) +
                                           (MAX_OUTPUT_CHANNELS * MAX_BUFFER_SIZE));
      ptr_bit_buf = ia_enhaacplus_enc_create_bitbuffer(&bit_buf, mps_bs, MAX_MPS_BS_PAYLOAD_SIZE);

      error =
          ixheaace_mps_515_apply(pstr_api_struct->pstr_mps_515_enc, &ptr_input_buffer_mps[0],
                                 &ptr_downmix_buffer_mps[0], ptr_bit_buf, ptr_scratch_515_mps);
      if (error) {
        return error;
      }
      mps_extension_payload.data_size = ptr_bit_buf->cnt_bits;
      mps_extension_payload.data_type = IXHEAACE_MPS_EXT_LDSAC_DATA;
      mps_extension_payload.associated_ch_element = -1;

      if (pstr_api_struct->pstr_state->mps_tree_config == TREE_5151 ||
          pstr_api_struct->pstr_state->mps_tree_config == TREE_5152) {
        num_samples_read /= 6;
        for (idx = 0; idx < num_samples_read; idx++) {
          ptr_input_buffer[2 * idx + write_off_set] = (FLOAT32)ptr_downmix_buffer_mps[idx];
        }
      } else {
        num_samples_read /= 3;
        for (idx = 0; idx < num_samples_read; idx++) {
          ptr_input_buffer[idx + write_off_set] = (FLOAT32)ptr_downmix_buffer_mps[idx];
        }
      }
      env_read_offset = write_off_set;
    }

    if (pstr_api_struct->pstr_state->downsample[ele_idx]) {
      ixheaace_resampler_scratch *pstr_scratch_resampler =
          (ixheaace_resampler_scratch *)pstr_api_struct->pstr_state->ptr_temp_buff_resamp;

      if (pstr_api_struct->config[ele_idx].element_type != ID_LFE) {
        error = ixheaace_env_encode_frame(
            pstr_sbr_encoder, ptr_input_buffer + env_read_offset, ptr_input_buffer,
            IXHEAACE_MAX_CH_IN_BS_ELE,
            &(pstr_api_struct->pstr_state->num_anc_data_bytes[ele_idx][0]),
            pstr_api_struct->pstr_state->anc_data_bytes[ele_idx],
            &(pstr_api_struct->spectral_band_replication_tabs), &(pstr_api_struct->common_tabs),
            &(mps_extension_payload.p_data[0]), mps_extension_payload.data_size,
            pstr_api_struct->config[0].aac_config.flag_framelength_small);

        if (error != IA_NO_ERROR) {
          return error;
        }
      }

      if (!pstr_api_struct->config[ele_idx].use_parametric_stereo) {
        for (ch = 0; ch < pstr_aac_config->num_out_channels; ch++) {
          ia_enhaacplus_enc_get_scratch_bufs(pstr_api_struct->pstr_state->temp_buff_sbr,
                                             &shared_buf1_ring, &shared_buf2_ring);
          {
            ia_enhaacplus_enc_iir_downsampler(
                &(pstr_api_struct->pstr_state->down_sampler[ele_idx][ch]),
                ptr_input_buffer + write_off_set + ch,
                num_samples_read / pstr_aac_config->num_out_channels, IXHEAACE_MAX_CH_IN_BS_ELE,
                ptr_input_buffer + ch, &out_samples, out_stride, shared_buf1_ring,
                shared_buf2_ring, pstr_scratch_resampler);
          }
        }
      }
    }

    error = ia_enhaacplus_enc_aac_core_encode(
        pstr_aac_enc, ptr_input_buffer, time_in_stride,
        pstr_api_struct->pstr_state->anc_data_bytes[ele_idx],
        pstr_api_struct->pstr_state->num_anc_data_bytes[ele_idx], pub_out_buf,
        &(pstr_api_struct->pstr_state->i_out_bytes), &(pstr_api_struct->pstr_aac_tabs),
        pstr_api_struct->pstr_state->pstr_bit_stream_handle,
        &(pstr_api_struct->pstr_state->bit_stream), flag_last_element,
        write_program_config_element, i_num_coup_channels, i_channels_mask, ele_idx,
        total_fill_bits, total_channels, aot, pstr_api_struct->config->adts_flag,
        num_bs_elements);
    if (error != IA_NO_ERROR) {
      return error;
    }
    if (pstr_sbr_encoder && !(pstr_api_struct->config[ele_idx].use_parametric_stereo)) {
      if (pstr_sbr_encoder && (ptr_input_buffer != NULL)) {
        memmove(ptr_input_buffer,
                ptr_input_buffer + aacenc_blocksize * 2 * IXHEAACE_MAX_CH_IN_BS_ELE,
                write_off_set * sizeof(ptr_input_buffer[0]));
      }
    }
  }
  {}
  /*ADTS Header Write*/
  if (pstr_api_struct->config->adts_flag) {
    pub_out_buf = ((pUWORD8)pstr_api_struct->pp_mem[IA_ENHAACPLUSENC_OUTPUT_IDX]);
    {
      WORD32 bit_rate = 0;
      WORD32 num_channels = 0;

      for (ele = 0; ele < pstr_api_struct->config[0].num_bs_elements; ele++) {
        bit_rate += pstr_api_struct->config[ele].aac_config.bit_rate;
        num_channels += pstr_api_struct->config[ele].i_channels;
      }
      {
        header_bytes = ia_enhaacplus_enc_write_ADTS_header(
            pub_out_buf, pstr_api_struct->pstr_state->i_out_bytes,
            pstr_api_struct->config->aac_config.core_sample_rate,
            (pstr_api_struct->config[0].num_bs_elements > 1)
                ? num_channels
                : pstr_api_struct->config[0].aac_config.num_out_channels);
      }
    }

    pstr_api_struct->pstr_state->i_out_bytes += header_bytes;
  }

  return IA_NO_ERROR;
}

IA_ERRORCODE ixheaace_get_lib_id_strings(pVOID pv_output) {
  IA_ERRORCODE err_code = IA_NO_ERROR;
  ixheaace_version *pstr_output_config = (ixheaace_version *)pv_output;
  pstr_output_config->p_lib_name = (WORD8 *)LIB_NAME;
  pstr_output_config->p_version_num = (WORD8 *)ITTIAM_VER;

  return err_code;
}

IA_ERRORCODE ixheaace_allocate(pVOID pv_input, pVOID pv_output) {
  IA_ERRORCODE err_code = IA_NO_ERROR;
  WORD32 ui_api_size;
  pVOID pv_value;
  ixheaace_input_config *pstr_input_config = (ixheaace_input_config *)pv_input;
  ixheaace_output_config *pstr_output_config = (ixheaace_output_config *)pv_output;
  ixheaace_api_struct *pstr_api_struct;

  if (pstr_input_config->aot != AOT_AAC_ELD && pstr_input_config->aot != AOT_AAC_LC &&
      pstr_input_config->aot != AOT_AAC_LD && pstr_input_config->aot != AOT_SBR &&
      pstr_input_config->aot != AOT_PS) {
    return IA_EXHEAACE_API_FATAL_UNSUPPORTED_AOT;
  }
  ui_api_size = sizeof(ixheaace_api_struct);
  pstr_output_config->arr_alloc_memory[pstr_output_config->malloc_count] =
      pstr_output_config->malloc_xheaace(ui_api_size, 4);
  if (NULL == pstr_output_config->arr_alloc_memory[pstr_output_config->malloc_count]) {
    return IA_EXHEAACE_API_FATAL_MEM_ALLOC;
  }
  memset(pstr_output_config->arr_alloc_memory[pstr_output_config->malloc_count], 0, ui_api_size);

#ifndef BUILD_ARM64
  pstr_output_config->ui_rem =
      (SIZE_T)((SIZE_T)pstr_output_config->arr_alloc_memory[pstr_output_config->malloc_count] &
               3);
#else
  pstr_output_config->ui_rem =
      (UWORD64)((SIZE_T)pstr_output_config->arr_alloc_memory[pstr_output_config->malloc_count] &
                3);
#endif
  pstr_output_config->pv_ia_process_api_obj =
      (pVOID)((WORD8 *)pstr_output_config->arr_alloc_memory[pstr_output_config->malloc_count] +
              4 - pstr_output_config->ui_rem);
  pstr_output_config->malloc_count++;

  pstr_api_struct = (ixheaace_api_struct *)pstr_output_config->pv_ia_process_api_obj;
  memset(pstr_api_struct, 0, sizeof(*pstr_api_struct));
  ixheaace_set_default_config(pstr_api_struct, pstr_input_config);

  err_code = ixheaace_set_config_params(pstr_api_struct, pstr_input_config);
  if (err_code) {
    return err_code;
  }

  pstr_output_config->ui_proc_mem_tabs_size =
      (sizeof(ixheaace_mem_info_struct) + sizeof(pVOID *)) * 4;
  pstr_output_config->arr_alloc_memory[pstr_output_config->malloc_count] =
      pstr_output_config->malloc_xheaace(pstr_output_config->ui_proc_mem_tabs_size, 4);
  if (NULL == pstr_output_config->arr_alloc_memory[pstr_output_config->malloc_count]) {
    return IA_EXHEAACE_API_FATAL_MEM_ALLOC;
  }
  memset(pstr_output_config->arr_alloc_memory[pstr_output_config->malloc_count], 0,
         pstr_output_config->ui_proc_mem_tabs_size);

#ifndef BUILD_ARM64
  pstr_output_config->ui_rem =
      (UWORD32)((SIZE_T)pstr_output_config->arr_alloc_memory[pstr_output_config->malloc_count] &
                3);
#else
  pstr_output_config->ui_rem =
      (UWORD32)((SIZE_T)pstr_output_config->arr_alloc_memory[pstr_output_config->malloc_count] &
                3);
#endif

  pv_value =
      (pVOID)((WORD8 *)pstr_output_config->arr_alloc_memory[pstr_output_config->malloc_count] +
              4 - pstr_output_config->ui_rem);
  if (pv_value == NULL) {
    return IA_EXHEAACE_API_FATAL_MEM_ALLOC;
  }
  memset(pv_value, 0, (sizeof(ixheaace_mem_info_struct) + sizeof(pVOID *)) * 4);

  pstr_api_struct->pstr_mem_info = pv_value;
  pstr_api_struct->pp_mem = (pVOID *)((WORD8 *)pv_value + sizeof(ixheaace_mem_info_struct) * 4);

  pstr_output_config->malloc_count++;

  ixheaace_fill_mem_tabs(pstr_api_struct, pstr_input_config->aot);

  err_code = ixheaace_alloc_and_assign_mem(pstr_api_struct, pstr_output_config);
  if (err_code) {
    return err_code;
  }

  return err_code;
}

IA_ERRORCODE ixheaace_init(pVOID pstr_obj_ixheaace, pVOID pv_input, pVOID pv_output) {
  IA_ERRORCODE error = IA_NO_ERROR;
  WORD32 frame_length;
  WORD32 channels, ele_idx;
  ixheaace_api_struct *pstr_api_struct = (ixheaace_api_struct *)pstr_obj_ixheaace;
  ixheaace_input_config *pstr_input_config = (ixheaace_input_config *)pv_input;
  ixheaace_output_config *pstr_output_config = (ixheaace_output_config *)pv_output;
  frame_length = pstr_input_config->frame_length;
  channels = 0;
  for (ele_idx = 0; ele_idx < pstr_api_struct->config[0].num_bs_elements; ele_idx++) {
    channels += pstr_api_struct->config[ele_idx].i_channels;
  }
  pstr_api_struct->pstr_state->aot = pstr_input_config->aot;

  if ((pstr_api_struct->config[0].use_mps == 1) &&
      (0 == pstr_api_struct->config->aac_classic ||
       pstr_api_struct->pstr_state->aot == AOT_USAC)) {
    pstr_api_struct->pstr_state->mps_enable = pstr_api_struct->config[0].use_mps;
    pstr_api_struct->pstr_state->mps_tree_config = pstr_api_struct->config[0].mps_tree_config;
  }
  if (pstr_api_struct->config[0].num_bs_elements == 1) {
    pstr_api_struct->config[ele_idx].write_program_config_element = 0;
  }

  if (pstr_api_struct->pstr_state->aot != AOT_USAC) {
    for (ele_idx = 0; ele_idx < pstr_api_struct->config[0].num_bs_elements; ele_idx++) {
      /* Set config pointer in api obj */
      pstr_api_struct->pstr_state->pstr_config[ele_idx] = &pstr_api_struct->config[ele_idx];

      error = ia_enhaacplus_enc_init(pstr_api_struct, ele_idx);
      if (error) {
        return error;
      }

      pstr_api_struct->pstr_state->ui_init_done = 1;
      if (pstr_input_config->i_bitrate != pstr_api_struct->config[0].aac_config.bit_rate) {
        if (pstr_input_config->i_bitrate < pstr_api_struct->config[0].aac_config.bit_rate) {
          pstr_input_config->i_bitrate = pstr_api_struct->config[0].aac_config.bit_rate - 8;
        } else {
          pstr_input_config->i_bitrate = pstr_api_struct->config[0].aac_config.bit_rate + 8;
        }
      }
    }
    if (pstr_api_struct->config[0].aac_config.bitreservoir_size != -1) {
      WORD32 avg_bytes_per_frame_per_ch = pstr_api_struct->config[0].aac_config.bitreservoir_size;
      if (pstr_api_struct->config[0].aac_config.flag_framelength_small) {
        if (pstr_api_struct->config[0].aot == AOT_AAC_LC ||
            pstr_api_struct->config[0].aot == AOT_PS ||
            pstr_api_struct->config[0].aot == AOT_SBR) {
          avg_bytes_per_frame_per_ch = (pstr_api_struct->config[0].aac_config.bit_rate) *
                                       FRAME_LEN_960 /
                                       (pstr_api_struct->config[0].aac_config.core_sample_rate *
                                        pstr_api_struct->config[0].i_channels * 8);
        }
        if (pstr_input_config->aot == AOT_AAC_LD || pstr_input_config->aot == AOT_AAC_ELD) {
          avg_bytes_per_frame_per_ch = (pstr_api_struct->config[0].aac_config.bit_rate) *
                                       FRAME_LEN_480 /
                                       (pstr_api_struct->config[0].aac_config.core_sample_rate *
                                        pstr_api_struct->config[0].i_channels * 8);
        }
      } else {
        if (pstr_api_struct->config[0].aot == AOT_AAC_LC ||
            pstr_api_struct->config[0].aot == AOT_PS ||
            pstr_api_struct->config[0].aot == AOT_SBR) {
          avg_bytes_per_frame_per_ch = (pstr_api_struct->config[0].aac_config.bit_rate) *
                                       FRAME_LEN_1024 /
                                       (pstr_api_struct->config[0].aac_config.core_sample_rate *
                                        pstr_api_struct->config[0].i_channels * 8);
        }
        if (pstr_input_config->aot == AOT_AAC_LD || pstr_input_config->aot == AOT_AAC_ELD) {
          avg_bytes_per_frame_per_ch = (pstr_api_struct->config[0].aac_config.bit_rate) *
                                       FRAME_LEN_512 /
                                       (pstr_api_struct->config[0].aac_config.core_sample_rate *
                                        pstr_api_struct->config[0].i_channels * 8);
        }
      }

      if (pstr_api_struct->config[0].aac_config.bitreservoir_size < avg_bytes_per_frame_per_ch) {
        return IA_EXHEAACE_CONFIG_NONFATAL_BITRES_SIZE_TOO_SMALL;
      }
    }
    if (pstr_input_config->i_use_es) {
      // Write GA header
      ia_bit_buf_struct *pstr_ia_asc_bit_buf;
      pstr_ia_asc_bit_buf = iusace_create_bit_buffer(
          &(pstr_api_struct->pstr_state->str_bit_buf), pstr_api_struct->pp_mem[IA_MEMTYPE_OUTPUT],
          pstr_api_struct->pstr_mem_info[IA_MEMTYPE_OUTPUT].ui_size, 1);

      ixheaace_get_audiospecific_config_bytes(pstr_ia_asc_bit_buf,
                                              &pstr_api_struct->pstr_state->audio_specific_config,
                                              pstr_api_struct->pstr_state->aot);

      pstr_api_struct->pstr_state->i_out_bytes = (pstr_ia_asc_bit_buf->cnt_bits + 7) >> 3;
    }
    if (pstr_api_struct->config->aac_classic) {
      pstr_output_config->input_size =
          frame_length * channels * pstr_api_struct->config[0].ui_pcm_wd_sz / 8;
    } else {
      pstr_output_config->input_size =
          2 * frame_length * channels * pstr_api_struct->config[0].ui_pcm_wd_sz / 8;
    }
    pstr_output_config->samp_freq = pstr_api_struct->config[0].native_sample_rate;
    pstr_output_config->header_samp_freq = pstr_api_struct->config[0].aac_config.core_sample_rate;
    pstr_output_config->down_sampling_ratio =
        pstr_api_struct->config->aac_classic == 0 ? 2.0f : 1.0f;
    switch (pstr_api_struct->config->aot) {
      case AOT_AAC_LC:
        pstr_output_config->audio_profile = AUDIO_PROFILE_AAC_LC_L5;
        break;
      case AOT_SBR:
        pstr_output_config->audio_profile = AUDIO_PROFILE_HEAAC_L5;
        break;
      case AOT_PS:
        pstr_output_config->audio_profile = AUDIO_PROFILE_HEAAC_V2_L5;
        break;
      case AOT_AAC_LD:
        pstr_output_config->audio_profile = AUDIO_PROFILE_AAC_LD_L4;
        break;
      case AOT_AAC_ELD:
        if (pstr_api_struct->config[0].use_mps) {
          if (pstr_api_struct->config[0].mps_tree_config == TREE_212) {
            pstr_output_config->audio_profile = AUDIO_PROFILE_AAC_ELD_L2;
          } else {
            pstr_output_config->audio_profile = AUDIO_PROFILE_AAC_ELD_L4;
          }
        } else {
          pstr_output_config->audio_profile = AUDIO_PROFILE_AAC_ELD_L1;
        }
        break;
      default:
        pstr_output_config->audio_profile = AUDIO_PROFILE_NOT_SPECIFIED;
        break;
    }
  }

  pstr_api_struct->pstr_state->ui_init_done = 1;
  pstr_output_config->i_out_bytes = pstr_api_struct->pstr_state->i_out_bytes;

  return error;
}

IA_ERRORCODE ixheaace_create(pVOID pv_input, pVOID pv_output) {
  IA_ERRORCODE err_code = IA_NO_ERROR;
  ixheaace_output_config *pstr_out_cfg = (ixheaace_output_config *)pv_output;
  err_code = ixheaace_allocate(pv_input, pv_output);
  if (err_code) {
    return err_code;
  } else {
    return ixheaace_init(pstr_out_cfg->pv_ia_process_api_obj, pv_input, pv_output);
  }
}

IA_ERRORCODE ixheaace_process(pVOID pstr_obj_ixheaace, pVOID pv_input, pVOID pv_output) {
  IA_ERRORCODE error = IA_NO_ERROR;
  WORD32 ele_idx;
  (VOID) pv_input;
  ixheaace_api_struct *pstr_api_struct = (ixheaace_api_struct *)pstr_obj_ixheaace;
  ixheaace_output_config *pstr_output_config = (ixheaace_output_config *)pv_output;

  for (ele_idx = 0; ele_idx < pstr_api_struct->config[0].num_bs_elements; ele_idx++) {
    error = ia_enhaacplus_enc_execute(pstr_api_struct, ele_idx);
  }

  pstr_output_config->i_out_bytes = pstr_api_struct->pstr_state->i_out_bytes;

  return error;
}

IA_ERRORCODE ixheaace_delete(pVOID pv_output) {
  WORD32 idx;
  ixheaace_output_config *pstr_output_config = (ixheaace_output_config *)pv_output;

  for (idx = pstr_output_config->malloc_count - 1; idx >= 0; idx--) {
    if (pstr_output_config->arr_alloc_memory[idx]) {
      pstr_output_config->free_xheaace(pstr_output_config->arr_alloc_memory[idx]);
    }
  }
  return IA_NO_ERROR;
}
