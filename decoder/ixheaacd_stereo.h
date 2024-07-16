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
#ifndef IXHEAACD_STEREO_H
#define IXHEAACD_STEREO_H

VOID ixheaacd_read_ms_data(ia_bit_buf_struct *it_bit_buff,
                           ia_aac_dec_channel_info_struct *ptr_aac_dec_ch_info);

VOID ixheaacd_ms_stereo_process(
    ia_aac_dec_channel_info_struct *ptr_aac_dec_channel_info[CHANNELS],
    ia_aac_dec_tables_struct *ptr_aac_tables);

VOID ixheaacd_intensity_stereo_process(
    ia_aac_dec_channel_info_struct *ptr_aac_dec_channel_info[CHANNELS],
    ia_aac_dec_tables_struct *ptr_aac_tables, WORD32 object_type, WORD32 aac_sf_data_resil_flag,
    WORD16 framelength);

#endif /* #ifndef IXHEAACD_STEREO_H */
