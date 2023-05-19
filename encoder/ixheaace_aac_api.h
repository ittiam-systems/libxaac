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

#pragma once

IA_ERRORCODE ia_enhaacplus_enc_aac_enc_open(iexheaac_encoder_str **ppstr_exheaac_encoder,
                                            const iaace_config config,
                                            iaace_scratch *pstr_aac_scratch,
                                            ixheaace_aac_tables *pstr_aac_tabs, WORD32 ele_type,
                                            WORD32 element_instance_tag, WORD32 init, WORD32 aot);

IA_ERRORCODE ia_enhaacplus_enc_aac_core_encode(
    iexheaac_encoder_str **pstr_aac_enc, FLOAT32 *ptr_time_signal, UWORD32 time_sn_stride,
    const UWORD8 *ptr_anc_bytes, UWORD8 *num_anc_bytes, UWORD8 *ptr_out_bytes,
    WORD32 *num_out_bytes, ixheaace_aac_tables *pstr_aac_tables, VOID *ptr_bit_stream_handle,
    VOID *ptr_bit_stream, FLAG flag_last_element, WORD32 *write_program_config_element,
    WORD32 i_num_coup_channels, WORD32 i_channels_mask, WORD32 ele_idx, WORD32 *total_fill_bits,
    WORD32 total_channels, WORD32 aot, WORD32 adts_flag, WORD32 num_bs_elements);

VOID ia_enhaacplus_enc_aac_enc_close(iexheaac_encoder_str *pstr_exheaac_encoder);
IA_ERRORCODE ia_enhaacplus_enc_finalize_bit_consumption(ixheaace_qc_state *pstr_qc_kernel,
                                                        ixheaace_qc_out *pstr_qc_out,
                                                        WORD32 flag_last_element, WORD32 cnt_bits,
                                                        WORD32 *tot_fill_bits,
                                                        iexheaac_encoder_str **pstr_aac_enc,
                                                        WORD32 num_bs_elements, WORD32 aot);
