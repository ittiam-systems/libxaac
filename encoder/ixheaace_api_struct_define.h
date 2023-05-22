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

typedef struct {
  iaace_config config;
  ixheaace_element_info element_info;
  ixheaace_psy_out psy_out;
  ixheaace_psy_kernel psy_kernel;
  ixheaace_qc_state qc_kernel;
  ixheaace_qc_out qc_out;
  ixheaace_bitstream_enc_init bse_init;
  ixheaace_stereo_pre_pro_struct str_stereo_pre_pro;
  WORD32 downmix;
  WORD32 downmix_fac;
  WORD32 dual_mono;
  WORD32 bandwidth_90_dB;
  iaace_scratch *pstr_aac_scratch;
} iexheaac_encoder_str;
