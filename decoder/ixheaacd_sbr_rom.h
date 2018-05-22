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
#ifndef IXHEAACD_SBR_ROM_H
#define IXHEAACD_SBR_ROM_H

#define INV_INT_TABLE_SIZE 49
#define SBR_NF_NO_RANDOM_VAL 512
#define NUM_SER_AP_LINKS 3
#define SUBQMF_GROUPS (10)
#define QMF_GROUPS (12)
#define NO_IID_GROUPS (SUBQMF_GROUPS + QMF_GROUPS)
#define NUM_IID_LEVELS (7)
#define NUM_IID_LEVELS_FINE (15)
#define NUM_ICC_LEVELS (8)
#define NO_IID_LEVELS (2 * NUM_IID_LEVELS + 1)
#define NO_IID_LEVELS_FINE (2 * NUM_IID_LEVELS_FINE + 1)
#define NO_ICC_LEVELS (NUM_ICC_LEVELS)

typedef struct {
  WORD16 sbr_lim_gains_m[8];

  WORD16 sbr_lim_bands_per_octave_q13[4];
  WORD16 sbr_smooth_filter[4];
  WORD16 sbr_inv_int_table[INV_INT_TABLE_SIZE];

  WORD32 sbr_rand_ph[SBR_NF_NO_RANDOM_VAL + MAX_FREQ_COEFFS];

} ia_env_calc_tables_struct;

extern const ia_env_calc_tables_struct ixheaacd_aac_dec_env_calc_tables;

typedef struct {
  WORD16 w_32[2 * 30];
  WORD16 w_16[2 * 12];
  WORD32 dig_rev_table2_32[4];
  WORD32 dig_rev_table4_16[2];

  WORD16 sbr_sin_cos_twiddle_l64[64];
  WORD16 sbr_alt_sin_twiddle_l64[32];
  WORD16 sbr_cos_sin_twiddle_ds_l32[32 + 32];
  WORD16 sbr_sin_cos_twiddle_l32[32];
  WORD16 sbr_alt_sin_twiddle_l32[16];
  WORD16 sbr_t_cos_sin_l32[32 + 32];

  WORD16 post_fft_tbl[18];
  WORD16 dct23_tw[66];

  WORD16 qmf_c[1280];

  UWORD8 dig_rev_table2_128[4];
  WORD32 w1024[1536];
  WORD32 esbr_qmf_c[1280];
  WORD32 esbr_qmf_c_24[480];
  WORD32 esbr_w_32[2 * 30];
  WORD32 esbr_w_16[2 * 12];

  WORD32 esbr_sin_cos_twiddle_l64[64];
  WORD32 esbr_alt_sin_twiddle_l64[32];

  WORD32 esbr_sin_cos_twiddle_l32[32];
  WORD32 esbr_alt_sin_twiddle_l32[16];
  WORD32 esbr_t_cos_sin_l32[32 + 32];

  WORD32 esbr_sin_cos_twiddle_l24[24];
  WORD32 esbr_alt_sin_twiddle_l24[12];
  WORD32 esbr_t_cos_sin_l24[24 + 24];

  WORD32 esbr_sin_cos_twiddle_l16[16];
  WORD32 esbr_alt_sin_twiddle_l16[8];
  WORD32 esbr_t_cos_sin_l16[16 + 16];

  WORD16 ixheaacd_sbr_t_cos_sin_l32_eld[32 + 32];

  WORD16 qmf_c_eld[640];
  WORD16 qmf_c_eld2[640];

  WORD16 qmf_c_eld3[640];

  WORD16 ixheaacd_sbr_synth_cos_sin_l32[64 + 64];

} ia_qmf_dec_tables_struct;

extern const ia_qmf_dec_tables_struct ixheaacd_aac_qmf_dec_tables;

typedef struct {
  ia_frame_info_struct sbr_frame_info1_2_4_16[3 + 1];

  ia_sbr_header_data_struct str_sbr_default_header;
  WORD16 ixheaacd_t_huffman_env_bal_1_5db_inp_table[50];
  WORD16 ixheaacd_f_huffman_env_bal_1_5db_inp_table[50];
  WORD16 ixheaacd_t_huffman_env_bal_3_0db_inp_table[26];
  WORD16 ixheaacd_f_huffman_env_bal_3_0db_inp_table[26];
  WORD16 ixheaacd_t_huffman_noise_3_0db_inp_table[64];
  WORD16 ixheaacd_t_huffman_noise_bal_3_0db_inp_table[26];
  WORD16 ixheaacd_t_huffman_env_1_5db_inp_table[122];
  WORD16 ixheaacd_f_huffman_env_1_5db_inp_table[122];
  WORD16 ixheaacd_t_huffman_env_3_0db_inp_table[64];
  WORD16 ixheaacd_f_huffman_env_3_0db_inp_table[64];

  WORD32 ixheaacd_t_huffman_env_bal_1_5db_idx_table[20];
  WORD32 ixheaacd_f_huffman_env_bal_1_5db_idx_table[23];
  WORD32 ixheaacd_t_huffman_env_bal_3_0db_idx_table[16];
  WORD32 ixheaacd_f_huffman_env_bal_3_0db_idx_table[17];
  WORD32 ixheaacd_t_huffman_noise_3_0db_idx_table[17];
  WORD32 ixheaacd_t_huffman_noise_bal_3_0db_idx_table[11];
  WORD32 ixheaacd_t_huffman_env_1_5db_idx_table[27];
  WORD32 ixheaacd_f_huffman_env_1_5db_idx_table[28];
  WORD32 ixheaacd_t_huffman_env_3_0db_idx_table[26];
  WORD32 ixheaacd_f_huffman_env_3_0db_idx_table[25];

  WORD8 start_min[12];
  WORD8 offset_idx[12];
  WORD8 ixheaacd_drc_offset[7][16];
  WORD8 stop_min[12];
  WORD8 stop_off[12][14];

} ia_env_extr_tables_struct;

extern const ia_env_extr_tables_struct ixheaacd_aac_dec_env_extr_tables;

typedef struct {
  WORD16 decay_scale_factor[72];

  WORD16 hyb_resol[3];
  WORD16 rev_link_decay_ser[NUM_SER_AP_LINKS];
  WORD16 rev_link_delay_ser[3];
  WORD16 borders_group[NO_IID_GROUPS + 1];
  WORD16 group_shift[6];
  WORD16 group_to_bin[NO_IID_GROUPS];
  WORD16 hybrid_to_bin[SUBQMF_GROUPS];
  WORD16 delay_to_bin[32];

  WORD16 frac_delay_phase_fac_qmf_re_im[24 * 2];
  WORD16 frac_delay_phase_fac_qmf_sub_re_im[16 * 2];
  WORD16 frac_delay_phase_fac_qmf_ser_re_im[3][32 * 2];
  WORD16 frac_delay_phase_fac_qmf_sub_ser_re_im[3][16 * 2];

  WORD16 scale_factors[NO_IID_LEVELS];
  WORD16 scale_factors_fine[NO_IID_LEVELS_FINE];
  WORD16 alpha_values[NO_ICC_LEVELS];
  WORD16 p2_6[6];
  WORD16 p8_13[13];
  WORD16 huff_iid_dt[28];
  WORD16 huff_iid_df[28];
  WORD16 huff_icc_dt[14];
  WORD16 huff_icc_df[14];
  WORD16 huff_iid_dt_fine[60];
  WORD16 huff_iid_df_fine[60];
  WORD32 dummy;

} ia_ps_tables_struct;

extern const ia_ps_tables_struct ixheaacd_aac_dec_ps_tables;

typedef struct {
  ia_env_calc_tables_struct *env_calc_tables_ptr;
  ia_qmf_dec_tables_struct *qmf_dec_tables_ptr;
  ia_env_extr_tables_struct *env_extr_tables_ptr;
  ia_ps_tables_struct *ps_tables_ptr;
  WORD32 *sbr_rand_ph;
} ia_sbr_tables_struct;

#endif
