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

#include "ixheaac_type_def.h"
#include "ixheaace_wave_read.h"

WORD32 ia_enhaacplus_enc_get_wav_info(UWORD8 *inptr, wav_file_info *wav_info) {
  UWORD8 *wav_hdr = inptr;
  WORD8 data_start[4];
  WORD16 num_ch;
  UWORD16 f_samp;

  if (wav_hdr[0] != 'R' && wav_hdr[1] != 'I' && wav_hdr[2] != 'F' && wav_hdr[3] != 'F') {
    return 0;
  }

  if (wav_hdr[20] != 01 && wav_hdr[21] != 00) {
    return 0;
  }
  num_ch = (WORD16)((UWORD8)wav_hdr[23] * 256 + (UWORD8)wav_hdr[22]);
  f_samp = ((UWORD8)wav_hdr[27] * 256 * 256 * 256);
  f_samp += ((UWORD8)wav_hdr[26] * 256 * 256);
  f_samp += ((UWORD8)wav_hdr[25] * 256);
  f_samp += ((UWORD8)wav_hdr[24]);

  wav_info->num_channels = num_ch;
  wav_info->sample_rate = f_samp;

  data_start[0] = wav_hdr[36];
  data_start[1] = wav_hdr[37];
  data_start[2] = wav_hdr[38];
  data_start[3] = wav_hdr[39];

  if (!(data_start[0] == 'd' && data_start[1] == 'a' && data_start[2] == 't' &&
        data_start[3] == 'a')) {
    return 0;
  }

  return 1;
}
