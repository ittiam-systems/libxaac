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
#ifndef IA_EAACP_DEC_MUTLCHANNEL_H
#define IA_EAACP_DEC_MUTLCHANNEL_H

WORD16 ixheaacd_dec_coupling_channel_element(
    ia_handle_bit_buf_struct bs, ia_aac_decoder_struct *aac_handle,
    WORD32 sample_rate_idx, ia_aac_dec_tables_struct *ptr_aac_tables,
    ixheaacd_misc_tables *common_tables_ptr, WORD *element_index_order,
    ia_enhaacplus_dec_ind_cc *ind_channel_info, WORD32 total_channels,
    WORD32 frame_size, WORD32 audio_object_type,
    ia_eld_specific_config_struct eld_specific_config, WORD32 ele_type);

void ixheaacd_dec_ind_coupling(
    ia_exhaacplus_dec_api_struct *p_obj_enhaacplus_dec, WORD16 *coup_ch_output,
    WORD16 frame_size, WORD total_channels, WORD16 *ptr_time_data);

void ixheaacd_dec_downmix_to_stereo(
    ia_exhaacplus_dec_api_struct *p_obj_enhaacplus_dec, WORD16 frame_size,
    WORD total_elements, WORD16 *ptr_time_data, WORD total_channels);

#endif /* IA_EAACP_DEC_MUTLCHANNEL_H */