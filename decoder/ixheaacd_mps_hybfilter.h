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
#ifndef IXHEAACD_MPS_HYBFILTER_H
#define IXHEAACD_MPS_HYBFILTER_H

VOID ixheaacd_mps_qmf_hybrid_analysis_init(ia_mps_hybrid_filt_struct *handle);

VOID ixheaacd_mps_qmf_hybrid_analysis(
    ia_mps_hybrid_filt_struct *handle,
    ia_cmplx_flt_struct in_qmf[MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS_MPS_NEW],
    WORD32 num_bands, WORD32 num_samples,
    ia_cmplx_flt_struct out_hyb[MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS]);

VOID ixheaacd_mps_qmf_hybrid_synthesis(
    ia_cmplx_flt_struct in_hyb[MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS],
    WORD32 num_bands, WORD32 num_samples,
    ia_cmplx_flt_struct in_qmf[MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS_MPS]);

#endif
