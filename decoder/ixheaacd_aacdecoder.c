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
#include <stdio.h>
#include <string.h>
#include "ixheaacd_sbr_common.h"
#include <ixheaacd_type_def.h>

#include "ixheaacd_constants.h"
#include <ixheaacd_basic_ops32.h>
#include <ixheaacd_basic_ops16.h>
#include <ixheaacd_basic_ops40.h>
#include "ixheaacd_basic_ops.h"
#include "ixheaacd_bitbuffer.h"

#include <ixheaacd_basic_op.h>
#include "ixheaacd_intrinsics.h"

#include "ixheaacd_defines.h"

#include <ixheaacd_aac_rom.h>

#include "ixheaacd_definitions.h"

#include "ixheaacd_error_codes.h"

#include "ixheaacd_pulsedata.h"

#include "ixheaacd_pns.h"
#include "ixheaacd_drc_data_struct.h"

#include "ixheaacd_lt_predict.h"

#include "ixheaacd_channelinfo.h"
#include "ixheaacd_cnst.h"
#include "ixheaacd_drc_dec.h"
#include "ixheaacd_sbrdecoder.h"
#include "ixheaacd_block.h"
#include "ixheaacd_channel.h"

#include "ixheaacd_sbr_payload.h"
#include "ixheaacd_common_rom.h"

#include <ixheaacd_type_def.h>

#include "ixheaacd_sbrdecsettings.h"
#include "ixheaacd_sbr_scale.h"
#include "ixheaacd_env_extr_part.h"
#include <ixheaacd_sbr_rom.h>
#include "ixheaacd_stereo.h"
#include "ixheaacd_lpp_tran.h"
#include "ixheaacd_hybrid.h"
#include "ixheaacd_ps_dec.h"
#include "ixheaacd_env_extr.h"
#include "ixheaacd_adts.h"
#include "ixheaacd_audioobjtypes.h"
#include "ixheaacd_memory_standards.h"
#include "ixheaacd_latmdemux.h"
#include "ixheaacd_aacdec.h"
#include "ixheaacd_mps_polyphase.h"
#include "ixheaacd_config.h"
#include "ixheaacd_mps_dec.h"
#include "ixheaacd_struct_def.h"
#include "ixheaacd_headerdecode.h"
#include "ixheaacd_multichannel.h"
#include "ixheaacd_adts_crc_check.h"

#include "ixheaacd_hcr.h"

#define SIZEOF_INT(x) ((sizeof(x) + sizeof(WORD32) - 1) / sizeof(WORD32))

#define EXT_FILL_DATA 1
#define EXT_FIL 0

WORD32 ixheaacd_aacdec_decodeframe(
    ia_exhaacplus_dec_api_struct *p_obj_exhaacplus_dec,
    ia_aac_dec_scratch_struct *aac_scratch_ptrs, WORD16 *time_data,
    FLAG frame_status, WORD *type, WORD *ch_idx, WORD init_flag, WORD channel,
    WORD *element_index_order, WORD skip_full_decode, WORD ch_fac,
    WORD slot_element, WORD max_channels, WORD32 total_channels,
    WORD32 frame_length, WORD32 frame_size, ia_drc_dec_struct *pstr_drc_dec,
    WORD32 object_type, WORD32 ch_config,
    ia_eld_specific_config_struct eld_specific_config, WORD16 adtsheader,
    ia_drc_dec_struct *drc_dummy)

{
  WORD ch, ele_type;
  ia_aac_dec_state_struct *p_state_enhaacplus_dec;
  ia_aac_decoder_struct *aac_dec_handle;
  ia_bit_buf_struct *it_bit_buff;
  ixheaacd_latm_struct *latm_element;

  WORD error_code = (WORD)frame_status;
  WORD previous_element;
  WORD prev_data_ele_present = 0;
  WORD new_element;
  WORD32 num_ch = 0;

  WORD32 crc_reg = 0;
  ia_adts_crc_info_struct *ptr_adts_crc_info;

  WORD32 cnt_bits = 0;

  WORD32 eld_sbr_flag = eld_specific_config.ld_sbr_flag_present;
  WORD32 ld_sbr_crc_flag = eld_specific_config.ld_sbr_crc_flag;
  WORD32 aac_spect_data_resil_flag =
      eld_specific_config.aac_spect_data_resil_flag;

  WORD32 ele_ch = 0;

  ia_aac_sfb_code_book_struct *ptr_aac_sfb_code_book_data[CHANNELS];
  ia_pns_stereo_data_struct *ptr_pns_stereo_data;

  WORD32 *work_buffer_core = aac_scratch_ptrs->base_scr_8k;
  WORD32 *work_buffer_1 = aac_scratch_ptrs->extra_scr_4k[0];
  WORD32 *work_buffer_2 = aac_scratch_ptrs->extra_scr_4k[2];
  p_state_enhaacplus_dec = p_obj_exhaacplus_dec->p_state_aac;

  aac_dec_handle = p_state_enhaacplus_dec->pstr_aac_dec_info[*ch_idx];
  it_bit_buff = p_state_enhaacplus_dec->ptr_bit_stream;

  ptr_adts_crc_info = it_bit_buff->pstr_adts_crc_info;

  latm_element = &p_state_enhaacplus_dec->latm_struct_element;

  ptr_pns_stereo_data =
      (ia_pns_stereo_data_struct
           *)&work_buffer_1[2 * SIZEOF_INT(ia_aac_dec_channel_info_struct) +
                            2 * SIZEOF_INT(ia_aac_sfb_code_book_struct)];

  aac_dec_handle->frame_status = 1;

  for (ch = 0; ch < channel; ch++) {
    const ia_aac_dec_imdct_tables_struct *pstr_imdct_tables;
    aac_dec_handle->pstr_aac_dec_ch_info[ch] =
        (ia_aac_dec_channel_info_struct
             *)&work_buffer_1[ch * SIZEOF_INT(ia_aac_dec_channel_info_struct)];
    ptr_aac_sfb_code_book_data[ch] =
        (ia_aac_sfb_code_book_struct
             *)&work_buffer_1[2 * SIZEOF_INT(ia_aac_dec_channel_info_struct) +
                              (ch * SIZEOF_INT(ia_aac_sfb_code_book_struct))];

    aac_dec_handle->pstr_aac_dec_ch_info[ch]->ptr_scale_factor =
        ptr_aac_sfb_code_book_data[ch]->scale_factor;
    aac_dec_handle->pstr_aac_dec_ch_info[ch]->ptr_code_book =
        ptr_aac_sfb_code_book_data[ch]->code_book;

    aac_dec_handle->pstr_aac_dec_ch_info[ch]->ptr_spec_coeff =
        &work_buffer_core[ch * MAX_BINS_LONG];

    if (object_type == AOT_ER_AAC_ELD) {
      aac_dec_handle->pstr_aac_dec_ch_info[ch]->ptr_spec_coeff =
          &work_buffer_core[2 * ch * MAX_BINS_LONG];
    }

    aac_dec_handle->pstr_aac_dec_ch_info[ch]->pstr_stereo_info =
        &ptr_pns_stereo_data->str_stereo_info;
    aac_dec_handle->pstr_aac_dec_ch_info[ch]->pstr_pns_corr_info =
        &ptr_pns_stereo_data->str_pns_corr_info;
    aac_dec_handle->pstr_aac_dec_ch_info[ch]->pstr_pns_rand_vec_data =
        aac_dec_handle->pstr_pns_rand_vec_data;

    pstr_imdct_tables = aac_dec_handle->pstr_aac_tables->pstr_imdct_tables;

    aac_dec_handle->pstr_aac_dec_overlap_info[ch]->ptr_long_window[0] =
        pstr_imdct_tables->only_long_window_sine;
    aac_dec_handle->pstr_aac_dec_overlap_info[ch]->ptr_short_window[0] =
        pstr_imdct_tables->only_short_window_sine;
    aac_dec_handle->pstr_aac_dec_overlap_info[ch]->ptr_long_window[1] =
        pstr_imdct_tables->only_long_window_kbd;
    aac_dec_handle->pstr_aac_dec_overlap_info[ch]->ptr_short_window[1] =
        pstr_imdct_tables->only_short_window_kbd;

    aac_dec_handle->ptr_aac_dec_static_channel_info[ch]->ptr_long_window[0] =
        pstr_imdct_tables->only_long_window_sine;
    aac_dec_handle->ptr_aac_dec_static_channel_info[ch]->ptr_short_window[0] =
        pstr_imdct_tables->only_short_window_sine;
    aac_dec_handle->ptr_aac_dec_static_channel_info[ch]->ptr_long_window[1] =
        pstr_imdct_tables->only_long_window_kbd;
    aac_dec_handle->ptr_aac_dec_static_channel_info[ch]->ptr_short_window[1] =
        pstr_imdct_tables->only_short_window_kbd;

    if (object_type == AOT_ER_AAC_ELD || object_type == AOT_ER_AAC_LD ||
        object_type == AOT_AAC_LTP) {
      aac_dec_handle->pstr_aac_dec_ch_info[ch]->str_ics_info.frame_length =
          frame_length;

      if (512 == aac_dec_handle->samples_per_frame) {
        if (object_type != AOT_ER_AAC_ELD) {
          aac_dec_handle->pstr_aac_dec_overlap_info[ch]->ptr_long_window[1] =
              (WORD16 *)pstr_imdct_tables->low_overlap_win;
          aac_dec_handle->pstr_aac_dec_overlap_info[ch]->ptr_long_window[0] =
              (WORD16 *)pstr_imdct_tables->window_sine_512;

          aac_dec_handle->ptr_aac_dec_static_channel_info[ch]
              ->ptr_long_window[1] =
              (WORD16 *)pstr_imdct_tables->low_overlap_win;
          aac_dec_handle->ptr_aac_dec_static_channel_info[ch]
              ->ptr_long_window[0] =
              (WORD16 *)pstr_imdct_tables->window_sine_512;
        } else {
          aac_dec_handle->pstr_aac_dec_overlap_info[ch]->ptr_long_window[1] =
              (WORD16 *)pstr_imdct_tables->window_sine_512_eld;
          aac_dec_handle->pstr_aac_dec_overlap_info[ch]->ptr_long_window[0] =
              (WORD16 *)pstr_imdct_tables->window_sine_512_eld;
        }
      } else if (480 == aac_dec_handle->samples_per_frame) {
        if (object_type != AOT_ER_AAC_ELD) {
          aac_dec_handle->pstr_aac_dec_overlap_info[ch]->ptr_long_window[1] =
              (WORD16 *)pstr_imdct_tables->low_overlap_win_480;
          aac_dec_handle->pstr_aac_dec_overlap_info[ch]->ptr_long_window[0] =
              (WORD16 *)pstr_imdct_tables->window_sine_480;

          aac_dec_handle->ptr_aac_dec_static_channel_info[ch]
              ->ptr_long_window[1] =
              (WORD16 *)pstr_imdct_tables->low_overlap_win_480;
          aac_dec_handle->ptr_aac_dec_static_channel_info[ch]
              ->ptr_long_window[0] =
              (WORD16 *)pstr_imdct_tables->window_sine_480;

        } else {
          aac_dec_handle->pstr_aac_dec_overlap_info[ch]->ptr_long_window[1] =
              (WORD16 *)pstr_imdct_tables->window_sine_480_eld;
          aac_dec_handle->pstr_aac_dec_overlap_info[ch]->ptr_long_window[0] =
              (WORD16 *)pstr_imdct_tables->window_sine_480_eld;
        }
      }
    }
    if ((object_type == AOT_ER_AAC_LD) || (object_type == AOT_AAC_LTP)) {
      if (aac_dec_handle->samples_per_frame <= 512) {
        aac_dec_handle->pstr_aac_dec_ch_info[ch]->str_ics_info.ltp.lag =
            aac_dec_handle->ptr_aac_dec_static_channel_info[ch]->ltp_lag;
      }
      aac_dec_handle->pstr_aac_dec_ch_info[ch]->ltp_buf =
          aac_dec_handle->ptr_aac_dec_static_channel_info[ch]->ltp_buf;
      aac_dec_handle->pstr_aac_dec_ch_info[ch]->ltp_lag =
          aac_dec_handle->ptr_aac_dec_static_channel_info[ch]->ltp_lag;
    }

    aac_dec_handle->pstr_aac_dec_ch_info[ch]->scratch_buf_ptr = work_buffer_2;
    if (object_type == AOT_ER_AAC_ELD) {
      aac_dec_handle->pstr_aac_dec_ch_info[ch]->pulse_scratch =
          aac_scratch_ptrs->extra_scr_4k[3];
    }
  }

  if (channel == 2) {
    if (aac_dec_handle->pstr_aac_dec_ch_info[1]->ptr_spec_coeff ==
        aac_scratch_ptrs->extra_scr_4k[0]) {
      aac_dec_handle->pstr_aac_dec_ch_info[1]->ptr_spec_coeff =
          aac_dec_handle->pstr_aac_dec_ch_info[0]->ptr_spec_coeff;
    }
  }

  for (ch = 0; ch < channel; ch++) {
    ia_pns_info_struct *ptr_pns_info =
        &aac_dec_handle->pstr_aac_dec_ch_info[ch]->str_pns_info;
    memset(ptr_pns_info, 0, sizeof(ia_pns_info_struct));
  }

  {
    ia_pns_correlation_info_struct *ptr_corr_info =
        aac_dec_handle->pstr_aac_dec_ch_info[0]->pstr_pns_corr_info;
    memset(ptr_corr_info->correlated, 0, sizeof(UWORD8) * PNS_BAND_FLAGS_SIZE);
  }

  for (ch = 0; ch < channel; ch++) {
    memset(&aac_dec_handle->pstr_aac_dec_ch_info[ch]->str_hcr_info, 0,
           sizeof(ia_hcr_info_struct));
    ixheaacd_huff_code_reorder_tbl_init(
        &aac_dec_handle->pstr_aac_dec_ch_info[ch]->str_hcr_info);
  }

  for (ch = 0; ch < channel; ch++) {
    aac_dec_handle->pstr_aac_dec_ch_info[ch]->str_ics_info.ltp.data_present = 0;
    aac_dec_handle->pstr_aac_dec_ch_info[ch]->str_ics_info.ltp2.data_present =
        0;
  }

  previous_element = ID_END;

  aac_dec_handle->pstr_sbr_bitstream->no_elements = 0;
  new_element = 0;
  ele_type = *type;

  cnt_bits = it_bit_buff->cnt_bits;

  if (((object_type != AOT_ER_AAC_ELD) && (object_type != AOT_ER_AAC_LD)) ||
      (object_type < ER_OBJECT_START)) {
    while (ele_type != ID_END && aac_dec_handle->frame_status) {
      ele_type = (WORD)ixheaacd_read_bits_buf(it_bit_buff, 3);
      ixheaacd_read_bidirection(it_bit_buff, -3);

      if (it_bit_buff->cnt_bits < 3) {
        it_bit_buff->cnt_bits = -1;
        error_code = (WORD16)(
            (WORD32)IA_ENHAACPLUS_DEC_EXE_NONFATAL_INSUFFICIENT_INPUT_BYTES);
        break;
      }

      if ((ele_type == ID_FIL) || (ele_type == ID_DSE) || (new_element == 0)) {
        ele_type = (WORD)ixheaacd_read_bits_buf(it_bit_buff, 3);
        new_element = 1;
      } else if ((ele_type != ID_END)) {
        ele_type = -1;
        break;
      } else {
        ele_type = (WORD)ixheaacd_read_bits_buf(it_bit_buff, 3);
      }

      if (it_bit_buff->cnt_bits < 0) {
        aac_dec_handle->frame_status = 0;
      }

      switch (ele_type) {
        case ID_SCE:
        case ID_CPE:
        case ID_LFE:

          if (aac_dec_handle->frame_status) {
            ia_aac_dec_channel_info_struct *pstr_aac_dec_ch_info =
                aac_dec_handle->pstr_aac_dec_ch_info[LEFT];
            ia_ics_info_struct *ptr_ics_info =
                &pstr_aac_dec_ch_info->str_ics_info;
            ele_ch = 1;
            if (ele_type == ID_CPE) {
              ele_ch = 2;
            } else {
              ele_ch = 1;
            }

            prev_data_ele_present = 1;

            if (ptr_adts_crc_info->crc_active == 1 &&
                ptr_adts_crc_info->no_reg < 7) {
              crc_reg = ixheaacd_adts_crc_start_reg(
                  ptr_adts_crc_info, it_bit_buff, CRC_ADTS_RAW_DATA_BLK_LEN);
            }

            pstr_aac_dec_ch_info->element_instance_tag =
                (WORD16)ixheaacd_read_bits_buf(it_bit_buff, 4);

            element_index_order[*ch_idx] =
                pstr_aac_dec_ch_info->element_instance_tag;
            pstr_aac_dec_ch_info->common_window = 0;

            ptr_ics_info->num_swb_window = 0;
            ptr_ics_info->sampling_rate_index =
                aac_dec_handle->sampling_rate_index;
            if ((object_type == AOT_ER_AAC_LD) ||
                (object_type == AOT_AAC_LTP)) {
              ptr_ics_info->ltp.data_present = 0;
              ptr_ics_info->ltp2.data_present = 0;
              ptr_ics_info->predictor_data_present = 0;
            }

            if (ele_ch > 1) {
              aac_dec_handle->pstr_aac_dec_ch_info[RIGHT]
                  ->str_ics_info.num_swb_window = 0;
              aac_dec_handle->pstr_aac_dec_ch_info[RIGHT]
                  ->str_ics_info.sampling_rate_index =
                  aac_dec_handle->sampling_rate_index;

              pstr_aac_dec_ch_info->common_window =
                  (WORD16)ixheaacd_read_bits_buf(it_bit_buff, 1);

              if (pstr_aac_dec_ch_info->common_window) {
                error_code = ixheaacd_ics_read(
                    it_bit_buff, ptr_ics_info, aac_dec_handle->num_swb_window,
                    object_type, pstr_aac_dec_ch_info->common_window,
                    aac_dec_handle->samples_per_frame);
                if (error_code) {
                  if (it_bit_buff->cnt_bits < 0) {
                    error_code = (WORD16)(
                        (WORD32)
                            IA_ENHAACPLUS_DEC_EXE_NONFATAL_INSUFFICIENT_INPUT_BYTES);
                  }

                  goto _ia_handle_error;
                }

                aac_dec_handle->pstr_aac_dec_ch_info[RIGHT]->str_ics_info =
                    pstr_aac_dec_ch_info->str_ics_info;

                ixheaacd_read_ms_data(it_bit_buff, pstr_aac_dec_ch_info);
              }
            }

            error_code = ixheaacd_individual_ch_stream(
                it_bit_buff, aac_dec_handle, ele_ch, frame_length,
                total_channels, object_type, eld_specific_config, ele_type);

            if (ptr_adts_crc_info->crc_active == 1) {
              ixheaacd_adts_crc_end_reg(ptr_adts_crc_info, it_bit_buff,
                                        crc_reg);
            }

            if (it_bit_buff->cnt_bits < 0) {
              error_code = (WORD16)(
                  (WORD32)
                      IA_ENHAACPLUS_DEC_EXE_NONFATAL_INSUFFICIENT_INPUT_BYTES);
            }

            if (error_code) {
              goto _ia_handle_error;
            }

          _ia_handle_error:
            if (error_code) {
              aac_dec_handle->frame_status = 0;
              if ((ele_type >= ID_SCE) && (ele_type <= ID_LFE))
                num_ch = num_ch + ele_ch;
              break;
            } else {
              ixheaacd_channel_pair_process(
                  aac_dec_handle->pstr_aac_dec_ch_info, ele_ch,
                  aac_dec_handle->pstr_aac_tables, total_channels, object_type,
                  aac_spect_data_resil_flag,
                  eld_specific_config.aac_sf_data_resil_flag,
                  aac_scratch_ptrs->in_data, aac_scratch_ptrs->out_data,
                  (void *)aac_dec_handle);
              num_ch = num_ch + ele_ch;
            }
          }

          break;
        case ID_CCE:
          if (max_channels > 2) {
            prev_data_ele_present = 1;
            error_code = ixheaacd_dec_coupling_channel_element(
                it_bit_buff, aac_dec_handle,
                aac_dec_handle->sampling_rate_index,
                aac_dec_handle->pstr_aac_tables,
                aac_dec_handle->pstr_common_tables,
                &element_index_order[*ch_idx],
                (ia_enhaacplus_dec_ind_cc *)aac_dec_handle->p_ind_channel_info,
                total_channels, frame_length, object_type, eld_specific_config,
                ele_type);

            num_ch = num_ch + 1;

            if (error_code) {
              aac_dec_handle->frame_status = 0;

              break;
            } else {
              ixheaacd_channel_pair_process(
                  aac_dec_handle->pstr_aac_dec_ch_info, 1,
                  aac_dec_handle->pstr_aac_tables, total_channels, object_type,
                  aac_spect_data_resil_flag,
                  eld_specific_config.aac_sf_data_resil_flag,
                  aac_scratch_ptrs->in_data, aac_scratch_ptrs->out_data,
                  (void *)aac_dec_handle);
            }
          } else {
            error_code =
                (WORD32)((WORD32)IA_ENHAACPLUS_DEC_EXE_FATAL_UNIMPLEMENTED_CCE);
          }
          if (it_bit_buff->cnt_bits < 0) {
            error_code = (WORD16)((
                WORD32)IA_ENHAACPLUS_DEC_EXE_NONFATAL_INSUFFICIENT_INPUT_BYTES);
            goto _ia_handle_error;
          }
          break;

        case ID_DSE:
        case ID_PCE:
        case ID_FIL:

        {
          WORD32 flag = 1;

          if ((ele_type != ID_FIL) && (ptr_adts_crc_info->crc_active == 1) &&
              (ptr_adts_crc_info->no_reg < 7)) {
            crc_reg =
                ixheaacd_adts_crc_start_reg(ptr_adts_crc_info, it_bit_buff, 0);
          }
          if (ele_type == ID_DSE) {
            ixheaacd_read_data_stream_element(
                it_bit_buff, &aac_dec_handle->byte_align_bits,
                p_obj_exhaacplus_dec->p_state_aac->pstr_drc_dec);
          }

          else if (ele_type == ID_PCE) {
            error_code = ixheaacd_decode_pce(
                it_bit_buff,
                &p_obj_exhaacplus_dec->aac_config.ui_pce_found_in_hdr,
                &p_obj_exhaacplus_dec->aac_config.str_prog_config);
            if (error_code != 0) {
              if (it_bit_buff->cnt_bits < 0) {
                error_code = (WORD16)(
                    (WORD32)
                        IA_ENHAACPLUS_DEC_EXE_NONFATAL_INSUFFICIENT_INPUT_BYTES);
                goto _ia_handle_error;
              }
              aac_dec_handle->frame_status = 0;
              if (error_code > 0) {
                error_code = IA_ENHAACPLUS_DEC_EXE_NONFATAL_DECODE_FRAME_ERROR;
                return error_code;
              } else {
                return error_code;
              }
            }
          }

          else if (ele_type == ID_FIL) {
            WORD32 bits_decoded = 0;
            if (object_type == AOT_ER_AAC_ELD) {
              bits_decoded = (it_bit_buff->size - it_bit_buff->cnt_bits);
              cnt_bits = (frame_size * 8 - bits_decoded);
              if (adtsheader == 1) {
                if (cnt_bits > it_bit_buff->cnt_bits)
                  return IA_ENHAACPLUS_DEC_EXE_NONFATAL_INSUFFICIENT_INPUT_BYTES;
              }
            }

            if (ixheaacd_check_for_sbr_payload(
                    it_bit_buff, aac_dec_handle->pstr_sbr_bitstream,
                    (WORD16)previous_element, pstr_drc_dec, object_type,
                    adtsheader, cnt_bits, ld_sbr_crc_flag, drc_dummy)) {
              flag = 0;
            }
          }

          if (it_bit_buff->cnt_bits < 0) {
            error_code = (WORD16)((
                WORD32)IA_ENHAACPLUS_DEC_EXE_NONFATAL_INSUFFICIENT_INPUT_BYTES);
            goto _ia_handle_error;
          }

          if (flag) {
            if (prev_data_ele_present == 0) {
              new_element = 0;
            }
          }
          if ((ele_type != ID_FIL) && (ptr_adts_crc_info->crc_active == 1)) {
            ixheaacd_adts_crc_end_reg(ptr_adts_crc_info, it_bit_buff, crc_reg);
          }

          if (ele_type == ID_PCE) {
            if (ptr_adts_crc_info->str_crc_reg_data[crc_reg].bit_buf_cnt) {
              ptr_adts_crc_info->str_crc_reg_data[crc_reg].max_bits =
                  ptr_adts_crc_info->str_crc_reg_data[crc_reg].bit_buf_cnt;
            }
          }
        }

        break;

        case ID_END:
          error_code = 0;
          break;
      }

      previous_element = ele_type;

      if (init_flag) {
        if ((ele_type >= ID_SCE) && (ele_type <= ID_LFE)) {
          p_obj_exhaacplus_dec->aac_config.element_type[*ch_idx] = ele_type;
        }
      }
    }
  } else {
    {
      switch (ch_config) {
        default:
          if (aac_dec_handle->frame_status) {
            ia_aac_dec_channel_info_struct *pstr_aac_dec_ch_info =
                aac_dec_handle->pstr_aac_dec_ch_info[LEFT];
            ia_ics_info_struct *ptr_ics_info =
                &pstr_aac_dec_ch_info->str_ics_info;

            if (ch_config == 2)
              ele_ch = 2, ele_type = 1;
            else
              ele_ch = 1, ele_type = 0;

            prev_data_ele_present = 1;

            if ((ptr_adts_crc_info->crc_active == 1) &&
                (ptr_adts_crc_info->no_reg < 7)) {
              crc_reg = ixheaacd_adts_crc_start_reg(
                  ptr_adts_crc_info, it_bit_buff, CRC_ADTS_RAW_DATA_BLK_LEN);
            }

            if (object_type != AOT_ER_AAC_ELD)
              pstr_aac_dec_ch_info->element_instance_tag =
                  (WORD16)ixheaacd_read_bits_buf(it_bit_buff, 4);

            element_index_order[*ch_idx] =
                pstr_aac_dec_ch_info->element_instance_tag;
            pstr_aac_dec_ch_info->common_window = 0;

            ptr_ics_info->num_swb_window = 0;
            ptr_ics_info->sampling_rate_index =
                aac_dec_handle->sampling_rate_index;

            if (object_type == AOT_ER_AAC_LD) {
              ptr_ics_info->ltp.data_present = 0;
              ptr_ics_info->ltp2.data_present = 0;
              ptr_ics_info->predictor_data_present = 0;
            }
            if (ele_ch > 1) {
              aac_dec_handle->pstr_aac_dec_ch_info[RIGHT]
                  ->str_ics_info.num_swb_window = 0;
              aac_dec_handle->pstr_aac_dec_ch_info[RIGHT]
                  ->str_ics_info.sampling_rate_index =
                  aac_dec_handle->sampling_rate_index;

              if (object_type != 39)
                pstr_aac_dec_ch_info->common_window =
                    (WORD16)ixheaacd_read_bits_buf(it_bit_buff, 1);
              else
                pstr_aac_dec_ch_info->common_window = 1;

              if (pstr_aac_dec_ch_info->common_window) {
                error_code = ixheaacd_ics_read(
                    it_bit_buff, ptr_ics_info, aac_dec_handle->num_swb_window,
                    object_type, pstr_aac_dec_ch_info->common_window,
                    aac_dec_handle->samples_per_frame);
                if (error_code) {
                  if (it_bit_buff->cnt_bits < 0) {
                    error_code = (WORD16)(
                        (WORD32)
                            IA_ENHAACPLUS_DEC_EXE_NONFATAL_INSUFFICIENT_INPUT_BYTES);
                  }

                  goto _ia_handle_error1;
                }

                aac_dec_handle->pstr_aac_dec_ch_info[RIGHT]->str_ics_info =
                    pstr_aac_dec_ch_info->str_ics_info;

                ixheaacd_read_ms_data(it_bit_buff, pstr_aac_dec_ch_info);

                {
                  if (object_type == AOT_ER_AAC_LD) {
                    WORD16 temp = ixheaacd_ltp_decode(
                        it_bit_buff, ptr_ics_info, object_type,
                        aac_dec_handle->samples_per_frame, LEFT);

                    if (temp != 0) {
                      return temp;
                    }
                  }
                }
              }
            }

            error_code = ixheaacd_individual_ch_stream(
                it_bit_buff, aac_dec_handle, ele_ch, frame_length,
                total_channels, object_type, eld_specific_config, ele_type);

            if (ptr_adts_crc_info->crc_active == 1) {
              ixheaacd_adts_crc_end_reg(ptr_adts_crc_info, it_bit_buff,
                                        crc_reg);
            }

            if (it_bit_buff->cnt_bits < 0) {
              error_code = (WORD16)(
                  (WORD32)
                      IA_ENHAACPLUS_DEC_EXE_NONFATAL_INSUFFICIENT_INPUT_BYTES);
            }

            if (error_code) {
              goto _ia_handle_error1;
            }

          _ia_handle_error1:
            if (error_code) {
              aac_dec_handle->frame_status = 0;
              if ((ele_type >= ID_SCE) && (ele_type <= ID_LFE))
                num_ch = num_ch + ele_ch;
              break;
            } else {
              ixheaacd_channel_pair_process(
                  aac_dec_handle->pstr_aac_dec_ch_info, ele_ch,
                  aac_dec_handle->pstr_aac_tables, total_channels, object_type,
                  aac_spect_data_resil_flag,
                  eld_specific_config.aac_sf_data_resil_flag,
                  aac_scratch_ptrs->in_data, aac_scratch_ptrs->out_data,
                  (void *)aac_dec_handle);
              num_ch = num_ch + ele_ch;
            }
          }

          p_obj_exhaacplus_dec->aac_config.element_type[*ch_idx] = ele_ch - 1;
          break;
      }

      if ((object_type != AOT_ER_AAC_ELD) || (!eld_sbr_flag)) {
        WORD32 bits_decoded, cnt_bits;
        bits_decoded = (it_bit_buff->size - it_bit_buff->cnt_bits);

        cnt_bits = (frame_size * 8 - bits_decoded);
        if (cnt_bits >= 8) {
          ixheaacd_extension_payload(it_bit_buff, cnt_bits);
        }

        if (((object_type == AOT_ER_AAC_ELD) ||
             (object_type == AOT_ER_AAC_LD)) &&
            (p_obj_exhaacplus_dec->aac_config.ld_decoder != 1)) {
          if (it_bit_buff->cnt_bits) {
            WORD32 alignment = it_bit_buff->bit_pos & 0x07;
            it_bit_buff->cnt_bits = (it_bit_buff->cnt_bits + alignment) & 7;
            it_bit_buff->bit_pos = 7;
            it_bit_buff->ptr_read_next++;
          }
        } else {
          if (it_bit_buff->bit_pos != 7) {
            WORD32 alignment = it_bit_buff->bit_pos & 0x07;
            it_bit_buff->cnt_bits -= alignment + 1;
            it_bit_buff->bit_pos += 7 - alignment;
            it_bit_buff->ptr_read_next++;
          }
        }
      } else {
        WORD32 bits_decoded, cnt_bits;
        bits_decoded = (it_bit_buff->size - it_bit_buff->cnt_bits);
        cnt_bits = (frame_size * 8 - bits_decoded);
        if (adtsheader == 1) {
          if (cnt_bits > it_bit_buff->cnt_bits)
            return IA_ENHAACPLUS_DEC_EXE_NONFATAL_INSUFFICIENT_INPUT_BYTES;
        }
        ixheaacd_check_for_sbr_payload(
            it_bit_buff, aac_dec_handle->pstr_sbr_bitstream,
            (WORD16)(ch_config - 1), pstr_drc_dec, object_type, adtsheader,
            cnt_bits, ld_sbr_crc_flag, drc_dummy);
      }
    }
  }

  if (ele_type == ID_END &&
      p_state_enhaacplus_dec->bs_format == LOAS_BSFORMAT) {
    WORD16 tmp;
    tmp = ((WORD16)latm_element->layer_info[0][0].frame_len_bits) -
          (it_bit_buff->initial_cnt_bits - it_bit_buff->cnt_bits);

    if (tmp > 0) ixheaacd_read_bidirection(it_bit_buff, tmp);

    if (latm_element->other_data_present) {
      tmp = latm_element->other_data_length;
      ixheaacd_read_bidirection(it_bit_buff, tmp);
    }
  }

  if (p_obj_exhaacplus_dec->aac_config.ui_drc_enable) {
    for (ch = 0; ch < num_ch; ch++) {
      pstr_drc_dec->is_longblock[*ch_idx + ch] =
          (aac_dec_handle->pstr_aac_dec_ch_info[ch]
               ->str_ics_info.window_sequence == EIGHT_SHORT_SEQUENCE)
              ? 0
              : 1;
    }
  }

  if (object_type == AOT_ER_AAC_LD) {
    for (ch = 0; ch < channel; ch++) {
      aac_dec_handle->ptr_aac_dec_static_channel_info[ch]->ltp_lag =
          aac_dec_handle->pstr_aac_dec_ch_info[ch]->str_ics_info.ltp.lag;
    }
  }
  aac_dec_handle->frame_status = aac_dec_handle->frame_status && frame_status;

  aac_dec_handle->channels = num_ch;

  if (error_code == 0)
    if ((skip_full_decode == 0) || ((skip_full_decode == 1) && error_code)) {
      ia_ics_info_struct str_ics_info[2];
      WORD32 *spec_coef[2];
      WORD32 *scratch[2];

      for (ch = 0; ch < channel; ch++) {
        str_ics_info[ch] =
            aac_dec_handle->pstr_aac_dec_ch_info[ch]->str_ics_info;
        spec_coef[ch] =
            aac_dec_handle->pstr_aac_dec_ch_info[ch]->ptr_spec_coeff;
      }

      scratch[0] = (WORD32 *)aac_scratch_ptrs->extra_scr_4k[2];
      scratch[1] = (WORD32 *)aac_scratch_ptrs->extra_scr_4k[1];

      ixheaacd_drc_map_channels(
          pstr_drc_dec, aac_dec_handle->channels,
          aac_dec_handle->pstr_aac_dec_ch_info[0]->str_ics_info.frame_length);

      for (ch = 0; ch < aac_dec_handle->channels; ch++) {
        WORD32 *overlap1 = aac_dec_handle->ptr_aac_dec_static_channel_info[ch]
                               ->overlap_add_data.ptr_overlap_buf;
        const WORD16 *ptr_long_window_next =
            aac_dec_handle->ptr_aac_dec_static_channel_info[ch]
                ->ptr_long_window[(int)str_ics_info[ch].window_shape];
        const WORD16 *ptr_short_window_next =
            aac_dec_handle->ptr_aac_dec_static_channel_info[ch]
                ->ptr_short_window[(int)str_ics_info[ch].window_shape];
        if (pstr_drc_dec->drc_on) {
          ixheaacd_drc_apply(pstr_drc_dec, spec_coef[ch],
                             str_ics_info[ch].window_sequence, ch,
                             str_ics_info[ch].frame_length);
        }
        if (skip_full_decode == 0) {
          ixheaacd_imdct_process(
              aac_dec_handle->pstr_aac_dec_overlap_info[ch], spec_coef[ch],
              &str_ics_info[ch], time_data + slot_element, (WORD16)ch_fac,
              scratch[ch], aac_dec_handle->pstr_aac_tables, object_type);

          aac_dec_handle->ptr_aac_dec_static_channel_info[ch]
              ->overlap_add_data.win_shape = str_ics_info[ch].window_shape;
          aac_dec_handle->ptr_aac_dec_static_channel_info[ch]
              ->overlap_add_data.win_seq = str_ics_info[ch].window_sequence;
          if ((object_type == AOT_ER_AAC_LD) || (object_type == AOT_AAC_LTP)) {
            {
              if ((str_ics_info[ch].window_sequence == ONLY_LONG_SEQUENCE) ||
                  (str_ics_info[ch].window_sequence == LONG_STOP_SEQUENCE)) {
                ixheaacd_lt_update_state(
                    aac_dec_handle->ptr_aac_dec_static_channel_info[ch]
                        ->ltp_buf,
                    time_data + slot_element, overlap1,
                    aac_dec_handle->samples_per_frame, object_type,
                    (WORD16)ch_fac, str_ics_info[ch].window_sequence,
                    (WORD16 *)ptr_long_window_next);
              } else {
                ixheaacd_lt_update_state(
                    aac_dec_handle->ptr_aac_dec_static_channel_info[ch]
                        ->ltp_buf,
                    time_data + slot_element, overlap1,
                    aac_dec_handle->samples_per_frame, object_type,
                    (WORD16)ch_fac, str_ics_info[ch].window_sequence,
                    (WORD16 *)ptr_short_window_next);
              }
            }
          }
          slot_element++;
        }
      }
    }

  if (ele_type == ID_END) {
    ixheaacd_byte_align(it_bit_buff, &aac_dec_handle->byte_align_bits);
    if (p_state_enhaacplus_dec->bs_format == LOAS_BSFORMAT) {
      ixheaacd_byte_align(it_bit_buff, &it_bit_buff->audio_mux_align);
    }
  }
  *type = ele_type;

  aac_dec_handle->block_number =
      ixheaacd_add32(aac_dec_handle->block_number, 1);
  return error_code;
}

WORD32 ixheaacd_extension_payload(ia_bit_buf_struct *it_bit_buff, WORD32 cnt) {
  WORD16 extension_type, discard;
  WORD32 i;
  WORD32 fill_nibble;

  WORD32 err = 0;
  extension_type = (WORD16)ixheaacd_read_bits_buf(it_bit_buff, 4);
  switch (extension_type) {
    case EXT_FILL_DATA:

      fill_nibble = ixheaacd_read_bits_buf(it_bit_buff, 4);

      if (fill_nibble == 0) {
        for (i = 0; i < (cnt >> 3) - 1; i++) {
          ixheaacd_read_bits_buf(it_bit_buff, 8);
        }

      } else
        err = -1;
      break;
    case EXT_FIL:
    default:
      for (i = 0; i < ((cnt)-8) + 4; i++) {
        discard = (WORD16)ixheaacd_read_bits_buf(it_bit_buff, 1);
      }
  }

  return err;
}
