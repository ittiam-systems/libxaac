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

#include "ixheaacd_adts_crc_check.h"

VOID ixheaacd_byte_align(ia_bit_buf_struct *it_bit_buff,
                         WORD32 *align_bits_cnt) {
  WORD alignment;
  alignment = (WORD)((*align_bits_cnt - it_bit_buff->cnt_bits) & 0x07);

  if (alignment) {
    ixheaacd_read_bits_buf(it_bit_buff, (8 - alignment));
  }

  *align_bits_cnt = it_bit_buff->cnt_bits;
}

WORD32 ixheaacd_show_bits_buf(ia_bit_buf_struct *it_bit_buff, WORD no_of_bits) {
  UWORD32 ret_val;
  UWORD8 *ptr_read_next = it_bit_buff->ptr_read_next;
  WORD bit_pos = it_bit_buff->bit_pos;

  ret_val = (UWORD32)*ptr_read_next;

  bit_pos -= no_of_bits;
  while (bit_pos < 0) {
    bit_pos += 8;
    ptr_read_next++;

    if (ptr_read_next > it_bit_buff->ptr_bit_buf_end) {
      ptr_read_next = it_bit_buff->ptr_bit_buf_base;
    }

    ret_val <<= 8;

    ret_val |= (UWORD32)*ptr_read_next;
  }

  ret_val = ret_val << ((31 - no_of_bits) - bit_pos) >> (32 - no_of_bits);

  return ret_val;
}

WORD32 ixheaacd_read_bits_buf(ia_bit_buf_struct *it_bit_buff, WORD no_of_bits) {
  UWORD32 ret_val;
  UWORD8 *ptr_read_next = it_bit_buff->ptr_read_next;
  WORD bit_pos = it_bit_buff->bit_pos;

  if (no_of_bits == 0) {
    return 0;
  }

  it_bit_buff->cnt_bits -= no_of_bits;
  ret_val = (UWORD32)*ptr_read_next;

  bit_pos -= no_of_bits;
  while (bit_pos < 0) {
    bit_pos += 8;
    ptr_read_next++;

    if (ptr_read_next > it_bit_buff->ptr_bit_buf_end) {
      ptr_read_next = it_bit_buff->ptr_bit_buf_base;
    }

    ret_val <<= 8;

    ret_val |= (UWORD32)*ptr_read_next;
  }

  ret_val = ret_val << ((31 - no_of_bits) - bit_pos) >> (32 - no_of_bits);
  it_bit_buff->ptr_read_next = ptr_read_next;
  it_bit_buff->bit_pos = (WORD16)bit_pos;
  return ret_val;
}

UWORD32 ixheaacd_aac_read_byte(UWORD8 **ptr_read_next, WORD32 *bit_pos,
                               WORD32 *readword) {
  UWORD8 *v = *ptr_read_next;
  WORD32 bits_consumed = *bit_pos;

  if ((bits_consumed -= 8) >= 0) {
    *readword = (*readword << 8) | *v;
    v++;
  } else {
    bits_consumed += 8;
  }
  *bit_pos = bits_consumed;
  *ptr_read_next = v;
  return 1;
}

UWORD32 ixheaacd_aac_read_2bytes(UWORD8 **ptr_read_next, WORD32 *bit_pos,
                                 WORD32 *readword) {
  UWORD8 *v = *ptr_read_next;
  WORD32 bits_consumed = *bit_pos;

  if ((bits_consumed - 16) >= 0) {
    *readword = (*readword << 8) | *v;
    v++;
    *readword = (*readword << 8) | *v;
    v++;
    bits_consumed -= 16;

  } else if ((bits_consumed - 8) >= 0) {
    *readword = (*readword << 8) | *v;
    v++;
    bits_consumed -= 8;
  }

  *bit_pos = bits_consumed;
  *ptr_read_next = v;
  return 1;
}

UWORD32 ixheaacd_aac_read_byte_corr1(UWORD8 **ptr_read_next,
                                     WORD16 *ptr_bit_pos, WORD32 *readword) {
  UWORD8 *v = *ptr_read_next;
  WORD16 bits_consumed = *ptr_bit_pos;

  while (bits_consumed >= 8) {
    if ((bits_consumed -= 8) >= 0) {
      {
        *readword = (*readword << 8) | *v;
        v++;
      }
    } else {
      bits_consumed += 8;
    }
  }
  *ptr_bit_pos = bits_consumed;
  *ptr_read_next = v;
  return 1;
}

UWORD32 ixheaacd_aac_read_byte_corr(UWORD8 **ptr_read_next, WORD32 *ptr_bit_pos,
                                    WORD32 *readword, UWORD8 *p_bit_buf_end) {
  UWORD8 *v = *ptr_read_next;
  WORD32 bits_consumed = *ptr_bit_pos;

  if ((bits_consumed -= 8) >= 0) {
    if (p_bit_buf_end < v)
      bits_consumed += 8;
    else {
      *readword = (*readword << 8) | *v;
      v++;
    }
  } else {
    bits_consumed += 8;
  }
  *ptr_bit_pos = bits_consumed;
  *ptr_read_next = v;
  return 1;
}

WORD32 ixheaacd_aac_read_bit(ia_bit_buf_struct *it_bit_buff) {
  UWORD8 ret_val;
  UWORD8 *ptr_read_next = it_bit_buff->ptr_read_next;
  WORD bit_pos = it_bit_buff->bit_pos;
  UWORD32 temp;
  WORD no_of_bits = 1;

  if (bit_pos < 0) {
    bit_pos = 7;
    ptr_read_next--;
  }

  it_bit_buff->cnt_bits += no_of_bits;
  ret_val = *ptr_read_next;
  bit_pos -= no_of_bits;

  temp = (ret_val << 24) << (bit_pos + no_of_bits);
  it_bit_buff->ptr_read_next = ptr_read_next;
  it_bit_buff->bit_pos = (WORD16)bit_pos;

  return temp >> (32 - no_of_bits);
}

WORD32 ixheaacd_aac_read_bit_rev(ia_bit_buf_struct *it_bit_buff) {
  UWORD8 ret_val;
  UWORD8 *ptr_read_next = it_bit_buff->ptr_read_next;
  WORD bit_pos = it_bit_buff->bit_pos;
  UWORD32 temp;
  WORD no_of_bits = 1;

  if (bit_pos >= 8) {
    bit_pos -= 8;
    ptr_read_next++;
  }

  it_bit_buff->cnt_bits -= no_of_bits;
  ret_val = *ptr_read_next;
  bit_pos += no_of_bits;

  temp = (ret_val << 24) << (bit_pos - no_of_bits);
  it_bit_buff->ptr_read_next = ptr_read_next;
  it_bit_buff->bit_pos = (WORD16)bit_pos;

  return temp >> (32 - no_of_bits);
}

VOID ixheaacd_write_bit(ia_bit_buf_struct *it_bit_buff, WORD32 value,
                        WORD32 no_of_bits)

{
  WORD32 mask;

  if (no_of_bits == 0) return;

  mask = 0x1;
  mask <<= no_of_bits - 1;

  it_bit_buff->bit_count += no_of_bits;

  while (no_of_bits > 0) {
    while (no_of_bits > 0 && it_bit_buff->valid_bits < 8) {
      it_bit_buff->byte <<= 1;
      if (value & mask) it_bit_buff->byte |= 0x1;
      value <<= 1;
      no_of_bits--;
      it_bit_buff->valid_bits++;
    }
    if (it_bit_buff->valid_bits == 8) {
      *it_bit_buff->byte_ptr++ = it_bit_buff->byte;
      it_bit_buff->byte = 0;
      it_bit_buff->valid_bits = 0;
    }
  }
}

WORD32 ixheaacd_read_bit(ia_bit_buf_struct *it_bit_buff, WORD32 no_of_bits) {
  UWORD32 ret_val;
  UWORD8 *ptr_read_next = it_bit_buff->byte_ptr;

  if (no_of_bits == 0) {
    return 0;
  }

  ret_val = ixheaacd_aac_showbits_32(ptr_read_next);
  it_bit_buff->byte_ptr += (no_of_bits >> 3);

  if (it_bit_buff->valid_bits != 8) {
    UWORD8 *v = it_bit_buff->byte_ptr;
    ret_val = (ret_val << (8 - it_bit_buff->valid_bits)) |
              (*v >> it_bit_buff->valid_bits);
  }

  it_bit_buff->valid_bits -= (no_of_bits % 8);

  ret_val = ret_val >> (32 - no_of_bits);

  return ret_val;
}
