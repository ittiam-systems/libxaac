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
#include <stdlib.h>
#include <string.h>
#include <ixheaacd_type_def.h>
#include <ixheaacd_type_def.h>
#include "ixheaacd_memory_standards.h"
#include "ixheaacd_sbrdecsettings.h"
#include "ixheaacd_env_extr_part.h"
#include "ixheaacd_defines.h"
#include <ixheaacd_aac_rom.h>
#include "ixheaacd_common_rom.h"
#include <ixheaacd_sbr_rom.h>
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_pulsedata.h"
#include "ixheaacd_pns.h"

#include "ixheaacd_lt_predict.h"

#include "ixheaacd_channelinfo.h"
#include "ixheaacd_sbr_common.h"
#include "ixheaacd_drc_data_struct.h"
#include "ixheaacd_drc_dec.h"
#include "ixheaacd_channel.h"
#include "ixheaacd_channelinfo.h"
#include "ixheaacd_sbrdecoder.h"
#include "ixheaacd_audioobjtypes.h"
#include "ixheaacd_latmdemux.h"
#include "ixheaacd_aacdec.h"
#include "ixheaacd_sbr_common.h"

#include "ixheaacd_mps_polyphase.h"
#include "ixheaacd_config.h"
#include "ixheaacd_mps_dec.h"

#include "ixheaacd_struct_def.h"

#include <ixheaacd_type_def.h>
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_interface.h"

#include "ixheaacd_tns_usac.h"
#include "ixheaacd_cnst.h"

#include "ixheaacd_acelp_info.h"

#include "ixheaacd_sbrdecsettings.h"
#include "ixheaacd_info.h"
#include "ixheaacd_sbrdecoder.h"
#include "ixheaacd_mps_polyphase.h"
#include "ixheaacd_sbr_const.h"
#include "ixheaacd_main.h"

#include "ixheaacd_arith_dec.h"

#include "ixheaacd_config.h"
#include "ixheaacd_struct.h"

#include "ixheaacd_create.h"

#include "ixheaacd_dec_main.h"

VOID ixheaacd_samples_sat(WORD8 *outbuffer, WORD32 num_samples_out,
                          WORD32 pcmsize, FLOAT32 (*out_samples)[4096],
                          WORD32 *out_bytes, WORD32 num_channel_out) {
  WORD32 num;
  WORD32 i;
  WORD64 write_local;

  WORD16 *out_buf = (WORD16 *)outbuffer;

  num = num_channel_out * num_samples_out;

  if (pcmsize == 16) {
    for (i = 0; i < num; i++) {
      write_local =
          ((WORD64)(out_samples[i % num_channel_out][i / num_channel_out]));

      if (write_local > 32767) {
        write_local = 32767;
      }
      if (write_local < -32768) {
        write_local = -32768;
      }
      out_buf[i] = (WORD16)write_local;
    }

    *out_bytes = num * sizeof(WORD16);
  } else {
    WORD8 *out_24bit = (WORD8 *)out_buf;
    for (i = 0; i < num; i++) {
      write_local = ((WORD64)(
          out_samples[i % num_channel_out][i / num_channel_out] * 256));

      if (write_local > 8388607) {
        write_local = 8388607;
      }
      if (write_local < -8388608) {
        write_local = -8388608;
      }
      *out_24bit++ = (WORD32)write_local & 0xff;
      *out_24bit++ = ((WORD32)write_local >> 8) & 0xff;
      *out_24bit++ = ((WORD32)write_local >> 16) & 0xff;
    }

    *out_bytes = num * 3 * sizeof(WORD8);
  }
}

WORD32 ixheaacd_dec_main(VOID *temp_handle, WORD8 *inbuffer, WORD8 *outbuffer,
                         WORD32 *out_bytes, WORD32 frames_done, WORD32 pcmsize,
                         WORD32 *num_channel_out) {
  WORD32 err = 0;
  ia_exhaacplus_dec_api_struct *handle =
      (ia_exhaacplus_dec_api_struct *)temp_handle;
  ia_aac_dec_state_struct *aac_dec_handle = handle->p_state_aac;

  WORD32 tmp;
  ia_audio_specific_config_struct *pstr_audio_specific_config =
      (ia_audio_specific_config_struct *)
          aac_dec_handle->ia_audio_specific_config;
  WORD32 suitable_tracks = 1;
  WORD32 num_samples_out;
  ia_dec_data_struct *pstr_dec_data;

  if (frames_done == 0) {
    if ((pstr_audio_specific_config->channel_configuration > 2) ||
        (pstr_audio_specific_config->channel_configuration == 0)) {
      return -1;
    }

    pstr_dec_data = (ia_dec_data_struct *)aac_dec_handle->pstr_dec_data;

    tmp = pstr_audio_specific_config->channel_configuration;

    suitable_tracks =
        ixheaacd_frm_data_init(pstr_audio_specific_config, pstr_dec_data);

    pstr_audio_specific_config->channel_configuration = tmp;

    if (suitable_tracks <= 0) {
      return -1;
    }
  }

  {
    pstr_dec_data = (ia_dec_data_struct *)aac_dec_handle->pstr_dec_data;

    if (frames_done == 0) {
      WORD32 delay;
      delay = ixheaacd_decode_create(
          handle, pstr_dec_data,
          pstr_dec_data->pstr_frame_data->scal_out_select + 1);
      if (delay == -1) return -1;
      *num_channel_out = pstr_dec_data->pstr_frame_data->scal_out_num_channels;
      return 0;
    }

    pstr_dec_data->dec_bit_buf.ptr_bit_buf_base = (UWORD8 *)inbuffer;
    pstr_dec_data->dec_bit_buf.size = aac_dec_handle->ui_in_bytes << 3;
    pstr_dec_data->dec_bit_buf.ptr_bit_buf_end =
        (UWORD8 *)inbuffer + aac_dec_handle->ui_in_bytes;
    pstr_dec_data->dec_bit_buf.ptr_read_next = (UWORD8 *)inbuffer;
    pstr_dec_data->dec_bit_buf.bit_pos = 7;
    pstr_dec_data->dec_bit_buf.cnt_bits = pstr_dec_data->dec_bit_buf.size;

    pstr_dec_data->pstr_usac_data->usac_flag = aac_dec_handle->usac_flag;

    err = ixheaacd_usac_process(pstr_dec_data, num_channel_out, aac_dec_handle);
    if (err == -1) return err;

    num_samples_out = pstr_dec_data->pstr_usac_data->output_samples;

    ixheaacd_samples_sat(outbuffer, num_samples_out, pcmsize,
                         pstr_dec_data->pstr_usac_data->time_sample_vector,
                         out_bytes, *num_channel_out);
#ifdef ENABLE_DRC
    pstr_audio_specific_config->str_usac_config.str_usac_dec_config
        .usac_ext_gain_payload_len =
        pstr_dec_data->pstr_frame_data->str_audio_specific_config
            .str_usac_config.str_usac_dec_config.usac_ext_gain_payload_len;
    memcpy(
        pstr_audio_specific_config->str_usac_config.str_usac_dec_config
            .usac_ext_gain_payload_buf,
        pstr_dec_data->pstr_frame_data->str_audio_specific_config
            .str_usac_config.str_usac_dec_config.usac_ext_gain_payload_buf,
        pstr_dec_data->pstr_frame_data->str_audio_specific_config
                .str_usac_config.str_usac_dec_config.usac_ext_gain_payload_len *
            sizeof(WORD8));
#endif
  }

  return err;
}
