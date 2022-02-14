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
#ifndef IXHEAACD_MPS_DECOR_H
#define IXHEAACD_MPS_DECOR_H

#define DECOR_ALPHA (0.8f)
#define ONE_MINUS_DECOR_ALPHA (1 - DECOR_ALPHA)
#define DECOR_GAMMA (1.5f)

IA_ERRORCODE ixheaacd_mps_decor_init(ia_mps_decor_struct_handle, int, int);

VOID ixheaacd_mps_decor_apply(
    ia_mps_decor_struct_handle self,
    ia_cmplx_flt_struct in[MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS],
    ia_cmplx_flt_struct out[MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS],
    WORD32 length, WORD32 res_bands);

#endif
