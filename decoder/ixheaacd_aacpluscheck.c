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

#include <ixheaacd_basic_op.h>
#include "ixheaacd_intrinsics.h"
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_defines.h"

#include "ixheaacd_pns.h"

#include <ixheaacd_aac_rom.h>
#include "ixheaacd_pulsedata.h"
#include "ixheaacd_drc_data_struct.h"

#include "ixheaacd_lt_predict.h"

#include "ixheaacd_channelinfo.h"
#include "ixheaacd_drc_dec.h"

#include "ixheaacd_sbrdecoder.h"
#include "ixheaacd_sbr_payload.h"
#include "ixheaacd_audioobjtypes.h"

#define SBR_EXTENSION_MPEG SBR_EXTENSION

#define SBR_EXTENSION_CRC_MPEG SBR_EXTENSION_CRC

FLAG ixheaacd_check_for_sbr_payload(
    ia_bit_buf_struct *it_bit_buff,
    ia_aac_dec_sbr_bitstream_struct *pstr_stream_sbr, WORD16 prev_element,
    ia_drc_dec_struct *pstr_drc_dec, WORD32 object_type, WORD32 adtsheader,
    WORD32 cnt_bits, WORD32 ld_sbr_crc_flag, ia_drc_dec_struct *drc_dummy) {
  FLAG ret = 0;
  WORD32 count;

  if (object_type == AOT_ER_AAC_ELD) {
    count = it_bit_buff->cnt_bits >> 3;
    if (adtsheader == 1) count = cnt_bits >> 3;
  } else {
    count = ixheaacd_read_bits_buf(it_bit_buff, 4);

    if ((count - 15) == 0) {
      WORD32 esc_count;
      esc_count = ixheaacd_read_bits_buf(it_bit_buff, 8);
      count = (esc_count + 14);
    }
  }

  if (count > 0) {
    WORD32 extension_type;

    if (object_type == AOT_ER_AAC_ELD)
      extension_type = ld_sbr_crc_flag ? SBR_EXTENSION_CRC : SBR_EXTENSION;
    else
      extension_type = ixheaacd_read_bits_buf(it_bit_buff, 4);

    if (((count < MAXSBRBYTES)) && (((extension_type == SBR_EXTENSION)) ||
                                    ((extension_type == SBR_EXTENSION_CRC))) &&
        ((prev_element == SBR_ID_SCE) || (prev_element == SBR_ID_CPE) ||
         sub_d(prev_element, SBR_ID_CCE) == 0)

            ) {
      WORD32 no_elements = pstr_stream_sbr->no_elements;
      WORD32 byte_count;
      ia_sbr_element_stream_struct *ptr_stream_sbr;

      ret = 1;

      ptr_stream_sbr = &pstr_stream_sbr->str_sbr_ele[no_elements];
      ptr_stream_sbr->size_payload = count;
      byte_count = ptr_stream_sbr->size_payload;
      ptr_stream_sbr->extension_type = extension_type;
      ptr_stream_sbr->sbr_ele_id = prev_element;
      pstr_stream_sbr->no_elements = no_elements + 1;

      if (pstr_drc_dec) pstr_drc_dec->sbr_found = 1;

      if (byte_count > 0 && sub_d(byte_count, MAXSBRBYTES) <= 0) {
        WORD32 i;
        WORD8 *ptr_sbr_data;
        if (object_type != AOT_ER_AAC_ELD) {
          ptr_sbr_data = &ptr_stream_sbr->ptr_sbr_data[1];
          ptr_stream_sbr->ptr_sbr_data[0] =
              (WORD8)ixheaacd_read_bits_buf(it_bit_buff, 4);
        } else
          ptr_sbr_data = ptr_stream_sbr->ptr_sbr_data;

        for (i = byte_count - 2; i >= 0; i--) {
          *ptr_sbr_data++ = (WORD8)ixheaacd_read_bits_buf(it_bit_buff, 8);
          if (object_type == AOT_ER_AAC_ELD) {
            if (adtsheader == 1) {
              cnt_bits = cnt_bits - 8;
            }
          }
        }

        if (object_type == AOT_ER_AAC_ELD) {
          *ptr_sbr_data++ = (WORD8)ixheaacd_read_bits_buf(it_bit_buff, 8);
          if (adtsheader == 1) {
            cnt_bits = cnt_bits - 8;
            if (cnt_bits > 0) {
              WORD32 unaligned_bits = (8 - it_bit_buff->cnt_bits);
              *ptr_sbr_data =
                  (WORD8)ixheaacd_read_bits_buf(it_bit_buff, cnt_bits);
              *ptr_sbr_data = *ptr_sbr_data << unaligned_bits;
              ptr_stream_sbr->size_payload++;
            }
          } else {
            if (it_bit_buff->cnt_bits > 0) {
              WORD32 unaligned_bits = (8 - it_bit_buff->cnt_bits);
              *ptr_sbr_data = (WORD8)ixheaacd_read_bits_buf(
                  it_bit_buff, it_bit_buff->cnt_bits);
              *ptr_sbr_data = *ptr_sbr_data << unaligned_bits;
              ptr_stream_sbr->size_payload++;
            }
          }
        }
      }

    } else if (extension_type == EXT_DYNAMIC_RANGE) {
      pstr_drc_dec->drc_element_found = 1;
      count -=
          ixheaacd_dec_drc_read_element(pstr_drc_dec, drc_dummy, it_bit_buff);
    } else {
      ixheaacd_read_bits_buf(it_bit_buff, 4);

      it_bit_buff->ptr_read_next += count - 1;
      it_bit_buff->cnt_bits -= ((count - 1) << 3);

      if (it_bit_buff->ptr_read_next > it_bit_buff->ptr_bit_buf_end) {
        it_bit_buff->ptr_read_next = it_bit_buff->ptr_bit_buf_base;
      }
    }
  }
  if (it_bit_buff->cnt_bits < 0) ret = -1;
  return (ret);
}
