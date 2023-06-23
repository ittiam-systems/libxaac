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
IA_ERRORCODE ixheaace_get_lib_id_strings(pVOID pv_output);
IA_ERRORCODE ixheaace_create(pVOID pv_input, pVOID pv_output);
IA_ERRORCODE ixheaace_process(pVOID pv_ia_process_api_obj, pVOID pstr_in_cfg, pVOID pstr_out_cfg);
IA_ERRORCODE ixheaace_delete(pVOID pv_output);

#define BW_FAC (0.172265625f)
#define IUSACE_FLOAT64_SCRATCH_SIZE (47704)
#define IUSACE_FLOAT32_SCRATCH_SIZE (2048 + 1024)

#define CH_MASK_CENTER_FRONT (0x4)
#define CH_MASK_LEFT_RIGHT_FRONT (0x3)
#define CH_MASK_REAR_CENTER (0x100)
#define CH_MASK_LEFT_RIGHT_BACK (0X30)
#define CH_MASK_LFE (0x08)

VOID ia_enhaacplus_enc_get_shared_bufs(VOID *scr, WORD32 **shared_buf1, WORD32 **shared_buf2,
                                       WORD32 **shared_buf3, WORD8 **shared_buf4,
                                       WORD32 aacenc_blocksize);
VOID ia_enhaacplus_enc_get_scratch_bufs(VOID *scr, FLOAT32 **shared_buf1_ring,
                                        FLOAT32 **shared_buf2_ring);
