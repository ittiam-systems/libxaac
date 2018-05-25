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
#ifndef IXHEAACD_ESBR_ROM_H
#define IXHEAACD_ESBR_ROM_H

extern const FLOAT32 ixheaacd_sub_samp_qmf_window_coeff[40 + 80 + 120 + 160 +
                                                        200 + 240 + 320 + 400];
extern const FLOAT32 ixheaacd_random_phase[512][2];
extern const FLOAT32 ixheaacd_hphase_tbl[2][8];

extern const FLOAT32 ixheaacd_g_lim_gains[4];

typedef const FLOAT32 FIR_FILTER[5];
extern FIR_FILTER ixheaacd_fir_0;
extern FIR_FILTER ixheaacd_fir_1;
extern FIR_FILTER ixheaacd_fir_2;
extern FIR_FILTER ixheaacd_fir_3;
extern FIR_FILTER ixheaacd_fir_4;
extern FIR_FILTER *ixheaacd_fir_table[5];
extern const FLOAT32 ixheaacd_q_gamma_table[4];
extern const WORD32 ixheaacd_start_subband2kL_tbl[33];
extern const FLOAT32 ixheaacd_cos_table_trans_qmf[7][32 * 2];

extern const FLOAT32 ixheaacd_phase_vocoder_cos_table[64];
extern const FLOAT32 ixheaacd_phase_vocoder_sin_table[64];
extern const FLOAT32 ixheaacd_hbe_post_anal_proc_interp_coeff[4][2];

extern const FLOAT32 ixheaacd_hbe_x_prod_cos_table_trans_2[2 * (128 + 128)];
extern const FLOAT32 ixheaacd_hbe_x_prod_cos_table_trans_3[2 * (128 + 128)];
extern const FLOAT32 ixheaacd_hbe_x_prod_cos_table_trans_4[2 * (128 + 128)];
extern const FLOAT32 ixheaacd_hbe_x_prod_cos_table_trans_4_1[2 * (128 + 128)];

extern const FLOAT32 ixheaacd_synth_cos_table_kl_4[8 * 4];
extern const FLOAT32 ixheaacd_synth_cos_table_kl_8[16 * 8];
extern const FLOAT32 ixheaacd_synth_cos_table_kl_12[24 * 12];
extern const FLOAT32 ixheaacd_synth_cos_table_kl_16[32 * 16];
extern const FLOAT32 ixheaacd_synth_cos_table_kl_20[40 * 20];
extern const FLOAT32 ixheaacd_analy_cos_sin_table_kl_8[8 * 8 * 2];
extern const FLOAT32 ixheaacd_analy_cos_sin_table_kl_16[16 * 16 * 2];
extern const FLOAT32 ixheaacd_analy_cos_sin_table_kl_24[24 * 24 * 2];
extern const FLOAT32 ixheaacd_analy_cos_sin_table_kl_32[32 * 32 * 2];
extern const FLOAT32 ixheaacd_analy_cos_sin_table_kl_40[40 * 80 * 2];
extern const FLOAT32 ixheaacd_sel_case[4][8];
#endif
