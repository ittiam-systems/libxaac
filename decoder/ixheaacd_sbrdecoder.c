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
#include <string.h>
#include "ixheaacd_sbr_common.h"
#include <ixheaacd_type_def.h>

#include "ixheaacd_constants.h"
#include <ixheaacd_basic_ops32.h>
#include <ixheaacd_basic_ops16.h>
#include <ixheaacd_basic_ops40.h>
#include "ixheaacd_basic_ops.h"
#include "ixheaacd_defines.h"

#include "ixheaacd_intrinsics.h"
#include "ixheaacd_sbr_const.h"
#include <ixheaacd_basic_op.h>
#include "ixheaacd_defines.h"
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_pns.h"

#include <ixheaacd_aac_rom.h>
#include "ixheaacd_pulsedata.h"

#include "ixheaacd_drc_data_struct.h"
#include "ixheaacd_lt_predict.h"
#include "ixheaacd_channelinfo.h"
#include "ixheaacd_drc_dec.h"

#include "ixheaacd_sbrdecoder.h"

#include "ixheaacd_sbrdecsettings.h"
#include "ixheaacd_sbr_scale.h"
#include "ixheaacd_lpp_tran.h"
#include "ixheaacd_env_extr_part.h"
#include <ixheaacd_sbr_rom.h>
#include "ixheaacd_hybrid.h"
#include "ixheaacd_ps_dec.h"
#include "ixheaacd_ps_bitdec.h"
#include "ixheaacd_env_extr.h"
#include "ixheaacd_common_rom.h"
#include "ixheaacd_freq_sca.h"

#include "ixheaacd_qmf_dec.h"

#include "ixheaacd_env_calc.h"

#include "ixheaacd_pvc_dec.h"
#include "ixheaacd_sbr_dec.h"
#include "ixheaacd_env_dec.h"
#include "ixheaacd_basic_funcs.h"
#include "ixheaacd_sbr_crc.h"

#include "ixheaacd_audioobjtypes.h"

VOID ixheaacd_downmix_to_monosbr(WORD16 *core_sample_buf, WORD32 ch_fac) {
  WORD32 i;
  WORD16 *ptr1 = &core_sample_buf[0];
  WORD16 *ptr2 = &core_sample_buf[1];

  for (i = MAX_FRAME_SIZE - 1; i >= 0; i--) {
    *ptr1 = ((*ptr1 >> 1) + (*ptr2 >> 1));

    ptr1 += ch_fac;
    ptr2 += ch_fac;
  }
}

static WORD32 ixheaacd_sbr_dec_reset(ia_sbr_dec_struct *ptr_sbr_dec,
                                     ia_sbr_header_data_struct *ptr_header_data,
                                     FLAG low_pow_flag,
                                     ixheaacd_misc_tables *pstr_common_tables,
                                     WORD32 pitch_in_bins,
                                     WORD32 audio_object_type) {
  WORD32 old_lsb, new_lsb;
  WORD32 l;
  WORD32 err = 0;
  WORD32 num_time_slots = ptr_sbr_dec->str_codec_qmf_bank.num_time_slots;
  WORD32 upsample_ratio_idx = ptr_header_data->sbr_ratio_idx;
  WORD32 op_delay = 6 + SBR_HF_ADJ_OFFSET;
  WORD32 hbe_flag = ptr_header_data->hbe_flag;
  WORD32 usac_flag = ptr_header_data->usac_flag;

  if (ptr_header_data->is_usf_4) {
    op_delay = op_delay + 6;
  }

  ixheaacd_reset_sbrenvelope_calc(&ptr_sbr_dec->str_sbr_calc_env);

  new_lsb = ptr_header_data->pstr_freq_band_data->sub_band_start;
  ptr_sbr_dec->str_synthesis_qmf_bank.lsb = new_lsb;
  ptr_sbr_dec->str_synthesis_qmf_bank.usb =
      ptr_header_data->pstr_freq_band_data->sub_band_end;

  old_lsb = ptr_sbr_dec->str_synthesis_qmf_bank.lsb;
  ptr_sbr_dec->str_codec_qmf_bank.lsb = 0;
  ptr_sbr_dec->str_codec_qmf_bank.usb = old_lsb;

  {
    WORD32 *plpc_filt_states_real =
        &ptr_sbr_dec->str_hf_generator.lpc_filt_states_real[0][old_lsb];
    WORD32 *plpc_filt_states_real_1 =
        &ptr_sbr_dec->str_hf_generator.lpc_filt_states_real[1][old_lsb];

    WORD32 *plpc_filt_states_imag =
        &ptr_sbr_dec->str_hf_generator.lpc_filt_states_imag[0][old_lsb];
    WORD32 *plpc_filt_states_imag_1 =
        &ptr_sbr_dec->str_hf_generator.lpc_filt_states_imag[1][old_lsb];

    for (l = new_lsb - old_lsb - 1; l >= 0; l--) {
      *plpc_filt_states_real++ = *plpc_filt_states_real_1++ = 0L;

      if (!low_pow_flag) {
        *plpc_filt_states_imag++ = *plpc_filt_states_imag_1++ = 0L;
      }
    }
  }

  if (usac_flag) {
    WORD32 start_band;
    WORD32 stop_band;
    WORD32 start_slot = SBR_HF_ADJ_OFFSET;
    WORD32 k;

    start_band = ptr_header_data->pstr_freq_band_data->qmf_sb_prev;
    stop_band = ptr_header_data->pstr_freq_band_data->sub_band_start;
    if (!hbe_flag) {
      for (l = 0; l < SBR_HF_ADJ_OFFSET; l++) {
        for (k = start_band; k < stop_band; k++) {
          ptr_sbr_dec->qmf_buf_real[l][k] = 0.0;
          ptr_sbr_dec->qmf_buf_imag[l][k] = 0.0;
        }
      }
      for (l = start_slot; l < op_delay; l++) {
        for (k = start_band; k < stop_band; k++) {
          ptr_sbr_dec->qmf_buf_real[l][k] = 0.0;
          ptr_sbr_dec->qmf_buf_imag[l][k] = 0.0;
        }
      }
    }
    if (hbe_flag && ptr_sbr_dec->p_hbe_txposer != NULL) {
      WORD32 k, i;
      WORD32 err = ixheaacd_qmf_hbe_data_reinit(
          ptr_sbr_dec->p_hbe_txposer,
          ptr_header_data->pstr_freq_band_data->freq_band_table,
          ptr_header_data->pstr_freq_band_data->num_sf_bands,
          ptr_header_data->is_usf_4);
      if (err) return err;

      for (k = 0; k < 2; k++) {
        if (!((upsample_ratio_idx == SBR_UPSAMPLE_IDX_4_1) && (k == 0))) {
          WORD32 xpos_delay = num_time_slots * k;
          if (upsample_ratio_idx == SBR_UPSAMPLE_IDX_4_1) {
            xpos_delay = num_time_slots * k - 32;
          }

          for (i = 0; i < 8; i++) {
            memmove(ptr_sbr_dec->ph_vocod_qmf_real[i],
                    ptr_sbr_dec->ph_vocod_qmf_real[num_time_slots + i],
                    64 * sizeof(FLOAT32));
            memmove(ptr_sbr_dec->ph_vocod_qmf_imag[i],
                    ptr_sbr_dec->ph_vocod_qmf_imag[num_time_slots + i],
                    64 * sizeof(FLOAT32));
          }

          err = ixheaacd_qmf_hbe_apply(
              ptr_sbr_dec->p_hbe_txposer,
              ptr_sbr_dec->qmf_buf_real + op_delay + xpos_delay,
              ptr_sbr_dec->qmf_buf_imag + op_delay + xpos_delay, num_time_slots,
              ptr_sbr_dec->ph_vocod_qmf_real + op_delay,
              ptr_sbr_dec->ph_vocod_qmf_imag + op_delay, pitch_in_bins);
          if (err) return err;

          if (upsample_ratio_idx == SBR_UPSAMPLE_IDX_4_1) {
            ixheaacd_hbe_repl_spec(&ptr_sbr_dec->p_hbe_txposer->x_over_qmf[0],
                                   ptr_sbr_dec->ph_vocod_qmf_real + op_delay,
                                   ptr_sbr_dec->ph_vocod_qmf_imag + op_delay,
                                   num_time_slots,
                                   ptr_sbr_dec->p_hbe_txposer->max_stretch);
          }
        }
      }
    }
  }

  if (!usac_flag) {
    err |= ixheaacd_reset_hf_generator(&ptr_sbr_dec->str_hf_generator,
                                       ptr_header_data, audio_object_type);

    err |= ixheaacd_derive_lim_band_tbl(
        ptr_header_data,
        ptr_sbr_dec->str_hf_generator.pstr_settings->str_patch_param,
        ptr_sbr_dec->str_hf_generator.pstr_settings->num_patches,
        pstr_common_tables);
  }

  return err;
}

WORD32 ixheaacd_prepare_upsamp(ia_sbr_header_data_struct **ptr_header_data,
                               ia_sbr_channel_struct *pstr_sbr_channel[2],
                               WORD32 num_channels) {
  WORD16 err = 0;
  WORD32 lr;
  ia_sbr_qmf_filter_bank_struct *sbr_qmf_bank;

  for (lr = 0; lr < num_channels; lr++) {
    ptr_header_data[lr]->pstr_freq_band_data->sub_band_start =
        NO_ANALYSIS_CHANNELS;
    ptr_header_data[lr]->pstr_freq_band_data->sub_band_end =
        NO_SYNTHESIS_CHANNELS;

    sbr_qmf_bank = &pstr_sbr_channel[lr]->str_sbr_dec.str_synthesis_qmf_bank;
    sbr_qmf_bank->lsb = NO_ANALYSIS_CHANNELS;
    sbr_qmf_bank->usb = NO_SYNTHESIS_CHANNELS;

    sbr_qmf_bank = &pstr_sbr_channel[lr]->str_sbr_dec.str_codec_qmf_bank;
    sbr_qmf_bank->lsb = 0;
    sbr_qmf_bank->usb = NO_ANALYSIS_CHANNELS;
    ptr_header_data[lr]->sync_state = UPSAMPLING;
  }
  return err;
}

WORD16 ixheaacd_applysbr(ia_handle_sbr_dec_inst_struct self,
                         ia_aac_dec_sbr_bitstream_struct *p_sbr_bit_stream,
                         WORD16 *core_sample_buf, WORD16 *codec_num_channels,
                         FLAG frame_status, FLAG down_samp_flag,
                         FLAG down_mix_flag,
                         ia_sbr_scr_struct *sbr_scratch_struct,
                         WORD32 ps_enable, WORD32 ch_fac, WORD32 slot_element,
                         ia_bit_buf_struct *it_bit_buff,
                         ia_drc_dec_struct *pstr_drc_dec, WORD eld_sbr_flag,
                         WORD32 audio_object_type) {
  WORD32 k;
  FLAG prev_ps_flag = 0;
  FLAG ps_flag = 0;
  FLAG stereo = 0;
  FLAG low_pow_flag = 0;
  FLAG header_flag = 1;
  FLAG dual_mono = 0;
  WORD32 err = 0;
  WORD32 num_channels = *codec_num_channels;
  FLAG prev_stereo;
  WORD32 ele_channels = 0;
  WORD32 num_elements = p_sbr_bit_stream->no_elements;
  WORD32 usac_flag = self->aot_usac_flag;

  ia_sbr_channel_struct *pstr_sbr_channel[2];
  ia_sbr_header_data_struct *ptr_header_data[MAXNRSBRCHANNELS];

  WORD32 initial_sync_state;

  ia_sbr_header_data_struct *ptr_sbr_dflt_header =
      (ia_sbr_header_data_struct *)(&self->str_sbr_dflt_header);

  ia_sbr_frame_info_data_struct *ptr_frame_data[2];

  for (k = 0; k < 2; k++) {
    ptr_frame_data[k] = (ia_sbr_frame_info_data_struct *)self->frame_buffer[k];

    pstr_sbr_channel[k] = self->pstr_sbr_channel[k];

    ptr_header_data[k] = self->pstr_sbr_header[k];

    if (audio_object_type == AOT_ER_AAC_ELD) {
      ptr_frame_data[k]->eld_sbr_flag = eld_sbr_flag;
      ptr_frame_data[k]->num_time_slots = ptr_header_data[0]->num_time_slots;
    }

    ptr_frame_data[k]->usac_independency_flag = self->usac_independency_flag;
    ptr_frame_data[k]->mps_sbr_flag = (self->stereo_config_idx == 3) ? 1 : 0;
    ptr_frame_data[k]->stereo_config_idx = self->stereo_config_idx;
    ptr_frame_data[k]->inter_tes_flag = self->inter_tes_flag;
    ptr_frame_data[k]->sbr_mode = self->sbr_mode;

    if (!usac_flag) {
      ptr_frame_data[k]->usac_independency_flag = 0;
      ptr_frame_data[k]->mps_sbr_flag = 0;
      ptr_frame_data[k]->stereo_config_idx = -1;
      ptr_frame_data[k]->inter_tes_flag = 0;
      ptr_frame_data[k]->sbr_mode = ORIG_SBR;
    }
  }

  for (k = 0; k < *codec_num_channels; k++) {
    ptr_header_data[k]->usac_flag = self->aot_usac_flag;

    ptr_header_data[k]->usac_independency_flag = self->usac_independency_flag;
    ptr_header_data[k]->hbe_flag = self->hbe_flag;
    ptr_header_data[k]->pvc_flag = self->pvc_flag;

    if (!usac_flag) {
      ptr_header_data[k]->usac_independency_flag = 0;
      ptr_header_data[k]->hbe_flag = 0;
      ptr_header_data[k]->pvc_flag = 0;
    }
  }

  initial_sync_state = ptr_header_data[0]->sync_state;

  low_pow_flag = !usac_flag;
  self->pstr_sbr_tables->sbr_rand_ph =
      self->pstr_sbr_tables->env_calc_tables_ptr->sbr_rand_ph;

  if (ps_enable) {
    if (num_channels == 1) {
      low_pow_flag = 0;
    }
  }

  if (audio_object_type == AOT_ER_AAC_ELD) {
    low_pow_flag = 0;
  }

  prev_stereo = (ptr_header_data[0]->channel_mode == SBR_STEREO);

  if (ps_enable) prev_ps_flag = (ptr_header_data[0]->channel_mode == PS_STEREO);

  ptr_header_data[0]->err_flag_prev = ptr_header_data[0]->err_flag;

  if (p_sbr_bit_stream->no_elements == 0) {
    frame_status = 0;
    ptr_header_data[0]->sync_state = UPSAMPLING;
    if (num_channels == 2) ptr_header_data[1]->sync_state = UPSAMPLING;
  }

  if ((usac_flag)) {
    if ((p_sbr_bit_stream->no_elements) &&
        (p_sbr_bit_stream->str_sbr_ele->size_payload > 0)) {
      num_elements = p_sbr_bit_stream->no_elements;
    } else {
      num_elements = 0;
    }
  }

  for (k = 0; k < num_elements; k++) {
    struct ia_bit_buf_struct local_bit_buf;
    ia_sbr_element_stream_struct *ptr_bit_str_ele =
        &p_sbr_bit_stream->str_sbr_ele[k];
    ele_channels =
        (p_sbr_bit_stream->str_sbr_ele[0].sbr_ele_id == SBR_ID_CPE) ? 2 : 1;

    switch (ptr_bit_str_ele->sbr_ele_id) {
      case SBR_ID_SCE:
      case SBR_ID_CCE:
        if (num_channels == 2) {
          dual_mono = 1;
        }
        stereo = 0;
        break;
      case SBR_ID_CPE:
        stereo = 1;
        ptr_header_data[1] = ptr_header_data[0];

        memcpy(self->pstr_sbr_header[1], self->pstr_sbr_header[0],
               sizeof(ia_sbr_header_data_struct));
        break;
      default:
        frame_status = 0;
    }

    if (frame_status) {
      if (!usac_flag) {
        ixheaacd_create_init_bit_buf(&local_bit_buf,
                                     (UWORD8 *)ptr_bit_str_ele->ptr_sbr_data,
                                     ptr_bit_str_ele->size_payload);

        it_bit_buff = &local_bit_buf;
        if (audio_object_type == AOT_ER_AAC_ELD) {
          if (eld_sbr_flag != 1) {
            ixheaacd_read_bits_buf(&local_bit_buf, LEN_NIBBLE);
          }
        } else {
          ixheaacd_read_bits_buf(&local_bit_buf, LEN_NIBBLE);
        }
      }
      if (ptr_bit_str_ele->extension_type == SBR_EXTENSION_CRC) {
        WORD32 crc_bits = 0;
        WORD32 crc_check_flag = 0;
        crc_check_flag = 1;
        crc_bits = (((ptr_bit_str_ele->size_payload - 1) << 3) +
                    (4 - SBR_CYC_REDCY_CHK_BITS));

        if (crc_bits < 0) {
          crc_check_flag = 0;
          frame_status = 0;
        }
        if (crc_check_flag)
          frame_status = ixheaacd_sbr_crccheck(it_bit_buff, crc_bits);
      }

      if (!usac_flag) header_flag = ixheaacd_read_bits_buf(it_bit_buff, 1);

      if (audio_object_type != AOT_ER_AAC_ELD) {
        if (header_flag) {
          header_flag = ixheaacd_sbr_read_header_data(
              ptr_header_data[k], it_bit_buff, stereo, ptr_sbr_dflt_header);
          if (usac_flag) {
            if ((self->ptr_pvc_data_str->prev_pvc_mode == 0) &&
                (ptr_header_data[k]->pvc_mode != 0)) {
              self->ptr_pvc_data_str->prev_pvc_id = 0;
            }
            self->ptr_pvc_data_str->prev_pvc_mode =
                ptr_header_data[k]->pvc_mode;
            if (ptr_header_data[k]->pvc_mode == 0) {
              ptr_frame_data[k]->sbr_mode = ORIG_SBR;
            } else {
              ptr_frame_data[k]->sbr_mode = PVC_SBR;
            }
          }
          if (header_flag == SBR_RESET) {
            err = ixheaacd_calc_frq_bnd_tbls(ptr_header_data[k],

                                             self->pstr_common_tables);
            if (!err) {
              WORD32 lr;
              WORD32 lr1 = ps_enable ? 2 : num_channels;
              for (lr = 0; lr < lr1; lr++) {
                ptr_frame_data[lr]->reset_flag = 1;

                err |= ixheaacd_sbr_dec_reset(
                    &(pstr_sbr_channel[lr]->str_sbr_dec), ptr_header_data[k],
                    low_pow_flag, self->pstr_common_tables,
                    ptr_frame_data[k]->pitch_in_bins, audio_object_type);
                if (err < 0) return err;
              }
            }

            if (err == 0) {
              ptr_header_data[k]->sync_state = SBR_ACTIVE;
            }
          }
        }
      } else {
        if (header_flag) {
          header_flag = ixheaacd_sbr_read_header_data(
              ptr_header_data[k], it_bit_buff, stereo, ptr_sbr_dflt_header);
          if (usac_flag) {
            if ((self->ptr_pvc_data_str->prev_pvc_mode == 0) &&
                (ptr_header_data[k]->pvc_mode != 0)) {
              self->ptr_pvc_data_str->prev_pvc_id = 0;
            }
            self->ptr_pvc_data_str->prev_pvc_mode =
                ptr_header_data[k]->pvc_mode;
            if (ptr_header_data[k]->pvc_mode == 0) {
              ptr_frame_data[k]->sbr_mode = ORIG_SBR;
            } else {
              ptr_frame_data[k]->sbr_mode = PVC_SBR;
            }
          }

          if (header_flag == SBR_RESET) {
            err = ixheaacd_calc_frq_bnd_tbls(ptr_header_data[k],

                                             self->pstr_common_tables);
            if (err) {
              return err;
            }
          }
        }

        {
          WORD32 lr;
          WORD32 lr1 = ps_enable ? 2 : num_channels;
          for (lr = 0; lr < lr1; lr++) {
            ptr_frame_data[lr]->reset_flag = 1;
            if (ptr_header_data[k]->status) {
              err |= ixheaacd_sbr_dec_reset(
                  &(pstr_sbr_channel[lr]->str_sbr_dec), ptr_header_data[k],
                  low_pow_flag, self->pstr_common_tables,
                  ptr_frame_data[k]->pitch_in_bins, audio_object_type);
              if (err < 0) return err;
            }
          }
          ptr_header_data[k]->status = 0;
        }

        if (err == 0) {
          ptr_header_data[k]->sync_state = SBR_ACTIVE;
        }
      }
    }

    if (err || (ptr_header_data[k]->sync_state == SBR_NOT_INITIALIZED)) {
      WORD32 lr1 = ps_enable ? 2 : num_channels;
      ixheaacd_prepare_upsamp(ptr_header_data, pstr_sbr_channel, lr1);
    }

    if (frame_status && (ptr_header_data[k]->sync_state == SBR_ACTIVE)) {
      if (stereo) {
        frame_status = ixheaacd_sbr_read_cpe(ptr_header_data[0], ptr_frame_data,
                                             it_bit_buff, self->pstr_sbr_tables,
                                             audio_object_type);
      } else {
        if (ps_enable) {
          if (down_mix_flag) {
            self->pstr_ps_stereo_dec->force_mono = 1;
          } else {
            self->pstr_ps_stereo_dec->force_mono = 0;
          }
        } else {
          self->pstr_ps_stereo_dec = 0;
        }
        if (ptr_frame_data[k]->sbr_mode == ORIG_SBR) {
          frame_status = ixheaacd_sbr_read_sce(
              ptr_header_data[k], ptr_frame_data[k], self->pstr_ps_stereo_dec,
              it_bit_buff, self->pstr_sbr_tables, audio_object_type);
        } else if (ptr_frame_data[k]->sbr_mode == PVC_SBR) {
          frame_status = ixheaacd_sbr_read_pvc_sce(
              ptr_frame_data[k], it_bit_buff, 0, self->ptr_pvc_data_str,
              self->pstr_sbr_tables, ptr_header_data[k]);
          if (frame_status < 0) return frame_status;
        }
      }
      if (audio_object_type != AOT_ER_AAC_ELD) {
        WORD32 total_bits_read;
        total_bits_read = ixheaacd_no_bits_read(it_bit_buff);
        if (total_bits_read > (ptr_bit_str_ele->size_payload << 3) ||
            total_bits_read < ((ptr_bit_str_ele->size_payload << 3) - 8)) {
          frame_status = 0;
        }
      }
    }
  }

  if (!usac_flag) {
    if (!frame_status || (ptr_header_data[0]->sync_state != SBR_ACTIVE) ||
        ptr_header_data[0]->err_flag) {
      ptr_header_data[0]->err_flag = 1;
      stereo = (num_channels == 2) ? 1 : 0;
      if (ptr_header_data[0]->channel_mode == 0) {
        ptr_header_data[0]->channel_mode = stereo ? SBR_STEREO : SBR_MONO;
      }
    }

    if (!(stereo || dual_mono)) {
      ptr_frame_data[0]->coupling_mode = COUPLING_OFF;
      ptr_frame_data[1]->coupling_mode = COUPLING_OFF;
    }

    if (ptr_header_data[0]->sync_state == SBR_NOT_INITIALIZED) {
      WORD32 lr1 = ps_enable ? 2 : num_channels;
      ixheaacd_prepare_upsamp(ptr_header_data, pstr_sbr_channel, lr1);
    }
  }

  if (ptr_header_data[0]->sync_state == SBR_ACTIVE) {
    if (ptr_frame_data[0]->sbr_mode == PVC_SBR) {
      ixheaacd_dec_sbrdata_for_pvc(ptr_header_data[0], ptr_frame_data[0],
                                   pstr_sbr_channel[0]->pstr_prev_frame_data);
    } else if (ptr_frame_data[0]->sbr_mode == ORIG_SBR) {
      err = ixheaacd_dec_sbrdata(
          ptr_header_data[0], ptr_header_data[1], ptr_frame_data[0],
          pstr_sbr_channel[0]->pstr_prev_frame_data,
          (stereo || dual_mono) ? ptr_frame_data[1] : NULL,
          (stereo || dual_mono) ? pstr_sbr_channel[1]->pstr_prev_frame_data
                                : NULL,
          self->pstr_common_tables);

      if (err) return err;
    }

    if (ptr_header_data[0]->channel_mode == PS_STEREO &&
        (audio_object_type != AOT_ER_AAC_ELD &&
         audio_object_type != AOT_ER_AAC_LD)) {
      ixheaacd_decode_ps_data(self->pstr_ps_stereo_dec);
      ps_flag = 1;
      self->ps_present = ps_flag;
    }

    ptr_frame_data[0]->max_qmf_subband_aac =
        ptr_header_data[0]->pstr_freq_band_data->sub_band_start;
    if (stereo) {
      ptr_frame_data[1]->max_qmf_subband_aac =
          ptr_header_data[1]->pstr_freq_band_data->sub_band_start;
    }
  }
  if (audio_object_type != AOT_ER_AAC_ELD) {
    if ((initial_sync_state == SBR_NOT_INITIALIZED) &&
        ptr_header_data[0]->err_flag) {
      ptr_header_data[0]->sync_state = SBR_NOT_INITIALIZED;
    }
  } else {
    ptr_header_data[0]->sync_state = SBR_ACTIVE;
  }

  if ((num_channels == 2) && !(stereo || dual_mono)) {
    ixheaacd_downmix_to_monosbr(&core_sample_buf[slot_element], ch_fac);
  }

  if ((!prev_stereo && !prev_ps_flag) && (ps_flag)) {
    WORD32 copy_size;
    if (down_samp_flag)
      copy_size = QMF_FILTER_STATE_SYN_SIZE_DOWN_SAMPLED;
    else
      copy_size = QMF_FILTER_STATE_SYN_SIZE;

    memcpy(
        pstr_sbr_channel[1]->str_sbr_dec.str_synthesis_qmf_bank.filter_states,
        pstr_sbr_channel[0]->str_sbr_dec.str_synthesis_qmf_bank.filter_states,
        copy_size * sizeof(WORD16));

    pstr_sbr_channel[1]->str_sbr_dec.str_sbr_scale_fact.st_syn_scale =
        pstr_sbr_channel[0]->str_sbr_dec.str_sbr_scale_fact.st_syn_scale;
  }

  if ((!prev_stereo && stereo && (num_channels == 2)) &&
      (audio_object_type != AOT_ER_AAC_ELD)) {
    WORD32 copy_size;
    if (down_samp_flag)
      copy_size = QMF_FILTER_STATE_SYN_SIZE_DOWN_SAMPLED;
    else
      copy_size = QMF_FILTER_STATE_SYN_SIZE;

    memcpy(
        pstr_sbr_channel[1]->str_sbr_dec.str_synthesis_qmf_bank.filter_states,
        pstr_sbr_channel[0]->str_sbr_dec.str_synthesis_qmf_bank.filter_states,
        copy_size * sizeof(WORD16));

    pstr_sbr_channel[1]->str_sbr_dec.str_sbr_scale_fact.st_syn_scale =
        pstr_sbr_channel[0]->str_sbr_dec.str_sbr_scale_fact.st_syn_scale;

    memcpy(
        pstr_sbr_channel[1]->str_sbr_dec.str_codec_qmf_bank.anal_filter_states,
        pstr_sbr_channel[0]->str_sbr_dec.str_codec_qmf_bank.anal_filter_states,
        QMF_FILTER_STATE_ANA_SIZE * sizeof(WORD16));

    pstr_sbr_channel[1]->str_sbr_dec.str_sbr_scale_fact.st_lb_scale =
        pstr_sbr_channel[0]->str_sbr_dec.str_sbr_scale_fact.st_lb_scale;

    memcpy(pstr_sbr_channel[1]->str_sbr_dec.ptr_sbr_overlap_buf,
           pstr_sbr_channel[0]->str_sbr_dec.ptr_sbr_overlap_buf,
           MAX_OV_COLS * NO_SYNTHESIS_CHANNELS * sizeof(WORD32));

    pstr_sbr_channel[1]->str_sbr_dec.str_sbr_scale_fact.ov_lb_scale =
        pstr_sbr_channel[0]->str_sbr_dec.str_sbr_scale_fact.ov_lb_scale;
    pstr_sbr_channel[1]->str_sbr_dec.str_sbr_scale_fact.ov_hb_scale =
        pstr_sbr_channel[0]->str_sbr_dec.str_sbr_scale_fact.ov_hb_scale;
  }
  pstr_sbr_channel[0]->str_sbr_dec.time_sample_buf = self->time_sample_buf[0];
  if (pstr_drc_dec == NULL) {
    WORD32 err_code = 0;
    err_code = ixheaacd_sbr_dec(
        &pstr_sbr_channel[0]->str_sbr_dec, core_sample_buf + slot_element,
        ptr_header_data[0], ptr_frame_data[0],
        pstr_sbr_channel[0]->pstr_prev_frame_data, self->pstr_ps_stereo_dec,
        &pstr_sbr_channel[1]->str_sbr_dec.str_synthesis_qmf_bank,
        &pstr_sbr_channel[1]->str_sbr_dec.str_sbr_scale_fact,
        (ptr_header_data[0]->sync_state == SBR_ACTIVE), low_pow_flag,
        sbr_scratch_struct->ptr_work_buf_core, self->pstr_sbr_tables,
        self->pstr_common_tables, ch_fac, self->ptr_pvc_data_str, 0, NULL,
        audio_object_type);
    if (err_code) return err_code;
  } else {
    WORD32 err_code = 0;
    err_code = ixheaacd_sbr_dec(
        &pstr_sbr_channel[0]->str_sbr_dec, core_sample_buf + slot_element,
        ptr_header_data[0], ptr_frame_data[0],
        pstr_sbr_channel[0]->pstr_prev_frame_data, self->pstr_ps_stereo_dec,
        &pstr_sbr_channel[1]->str_sbr_dec.str_synthesis_qmf_bank,
        &pstr_sbr_channel[1]->str_sbr_dec.str_sbr_scale_fact,
        (ptr_header_data[0]->sync_state == SBR_ACTIVE), low_pow_flag,
        sbr_scratch_struct->ptr_work_buf_core, self->pstr_sbr_tables,
        self->pstr_common_tables, ch_fac, self->ptr_pvc_data_str,
        pstr_drc_dec->drc_on,
        pstr_drc_dec->str_drc_channel_data[0].drc_factors_sbr,
        audio_object_type);
    if (err_code) return err_code;
  }

  if (!down_mix_flag && (stereo || dual_mono) && (num_channels == 2)) {
    pstr_sbr_channel[1]->str_sbr_dec.time_sample_buf = self->time_sample_buf[1];

    if (ele_channels == 1 && usac_flag) {
      WORD32 err_code = ixheaacd_esbr_dec(
          &pstr_sbr_channel[1]->str_sbr_dec, ptr_header_data[1],
          ptr_frame_data[1], (ptr_header_data[1]->sync_state == SBR_ACTIVE),
          low_pow_flag, self->pstr_sbr_tables, ch_fac);
      if (err_code) return err_code;
    } else {
      if (pstr_drc_dec == NULL) {
        WORD32 err_code = ixheaacd_sbr_dec(
            &pstr_sbr_channel[1]->str_sbr_dec,
            core_sample_buf + slot_element + 1, ptr_header_data[1],
            ptr_frame_data[1], pstr_sbr_channel[1]->pstr_prev_frame_data, NULL,
            NULL, NULL, (ptr_header_data[1]->sync_state == SBR_ACTIVE),
            low_pow_flag, sbr_scratch_struct->ptr_work_buf_core,
            self->pstr_sbr_tables, self->pstr_common_tables, ch_fac,
            self->ptr_pvc_data_str, 0, NULL, audio_object_type);
        if (err_code) return err_code;
      } else {
        WORD32 err_code = ixheaacd_sbr_dec(
            &pstr_sbr_channel[1]->str_sbr_dec,
            core_sample_buf + slot_element + 1, ptr_header_data[1],
            ptr_frame_data[1], pstr_sbr_channel[1]->pstr_prev_frame_data, NULL,
            NULL, NULL, (ptr_header_data[1]->sync_state == SBR_ACTIVE),
            low_pow_flag, sbr_scratch_struct->ptr_work_buf_core,
            self->pstr_sbr_tables, self->pstr_common_tables, ch_fac,
            self->ptr_pvc_data_str, pstr_drc_dec->drc_on,
            pstr_drc_dec->str_drc_channel_data[1].drc_factors_sbr,
            audio_object_type);
        if (err_code) return err_code;
      }
    }

  } else {
    if (audio_object_type != AOT_ER_AAC_ELD &&
        audio_object_type != AOT_ER_AAC_LD)

    {
      if (sub_d((WORD16)ptr_header_data[0]->channel_mode, PS_STEREO) == 0) {
        num_channels = 2;
      }
    }
  }
  *codec_num_channels = num_channels;
  self->sbr_mode = ptr_frame_data[0]->sbr_mode;

  if (pstr_drc_dec != NULL) {
    WORD32 i, j;
    for (i = 0; i < *codec_num_channels; i++) {
      for (j = 0; j < 32; j++) {
        memcpy(pstr_drc_dec->str_drc_channel_data[i].drc_factors_sbr[j],
               pstr_drc_dec->str_drc_channel_data[i].drc_factors_sbr[j + 32],
               64 * sizeof(WORD32));
      }
    }
  }

  return SBRDEC_OK;
}
