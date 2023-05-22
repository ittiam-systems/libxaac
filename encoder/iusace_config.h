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
#define BS_MAX_NUM_OUT_CHANNELS (255)
#define MINIMUM_BITRATE 8000

typedef struct {
  FLOAT64 *p_fd_mdct_windowed_long_buf;
  FLOAT64 *p_fd_mdct_windowed_short_buf;
  FLOAT32 *p_fft_mdct_buf;
  FLOAT64 *p_sort_grouping_scratch;
  WORD32 *p_degroup_scratch;
  WORD32 *p_arith_map_prev_scratch;
  WORD32 *p_arith_map_pres_scratch;
  FLOAT64 *p_noise_filling_highest_tone;
  FLOAT32 *p_lpd_frm_enc_scratch;
  FLOAT64 *p_quant_spectrum_spec_scratch;
  UWORD8 *ptr_scratch_buf;
  FLOAT32 *p_synth_tcx_buf;
  FLOAT32 *p_synth_buf;
  FLOAT32 *p_wsig_buf;
  FLOAT32 *p_wsyn_buf;
  FLOAT32 *p_wsyn_tcx_buf;
  FLOAT32 *p_temp_wsyn_buf;
  FLOAT32 *p_buf_aut_corr;
  FLOAT32 *p_buf_synthesis_tool;
  FLOAT32 *p_buf_speech;
  FLOAT32 *p_buf_res;
  FLOAT32 *p_buf_signal;
  FLOAT32 *p_lp_filter_coeff;
  FLOAT32 *p_lp_filter_coeff_q;
  WORD32 *p_prm_tcx;
  FLOAT32 *p_wsp_prev_buf;
  FLOAT32 *p_xn2;
  FLOAT32 *p_fac_dec;
  FLOAT32 *p_right_fac_spec;
  FLOAT32 *p_x2;
  WORD32 *p_param;
  FLOAT32 *p_x;
  FLOAT32 *p_xn_2;
  FLOAT32 *p_fac_window;
  FLOAT32 *p_temp_mdct;
  WORD16 *p_fac_bits_word;
  FLOAT64 *p_left_fac_time_data;
  FLOAT32 *p_left_fac_timedata_flt;
  FLOAT32 *p_left_fac_spec;
  FLOAT64 *p_fac_win;
  WORD32 *p_fac_prm;
  FLOAT32 *p_acelp_folded_scratch;
  FLOAT32 *p_xn1_tcx;
  FLOAT32 *p_xn_buf_tcx;
  FLOAT32 *p_x_tcx;
  FLOAT32 *p_x_tmp_tcx;
  FLOAT32 *p_en_tcx;
  FLOAT32 *p_alfd_gains_tcx;
  FLOAT32 *p_sq_enc_tcx;
  WORD32 *p_sq_quant_tcx;
  FLOAT32 *p_gain1_tcx;
  FLOAT32 *p_gain2_tcx;
  FLOAT32 *p_facelp_tcx;
  FLOAT32 *p_xn2_tcx;
  FLOAT32 *p_fac_window_tcx;
  FLOAT32 *p_x1_tcx;
  FLOAT32 *p_x2_tcx;
  WORD32 *p_y_tcx;
  FLOAT32 *p_in_out_tcx;
  FLOAT32 *p_time_signal;
  FLOAT32 *p_complex_fft;
  WORD32 *p_tonal_flag;
  FLOAT32 *p_pow_spec;
  FLOAT64 *p_tns_scratch;
  FLOAT64 *p_tns_filter;
  FLOAT32 *p_exp_spec;
  FLOAT32 *p_mdct_spec_float;
  FLOAT32 *p_fir_sig_buf;
  FLOAT32 *p_sq_gain_en;
  FLOAT32 *p_acelp_ir_buf;
  FLOAT32 *p_acelp_exc_buf;
  FLOAT32 *p_adjthr_ptr_exp_spec;
  FLOAT32 *p_adjthr_mdct_spec_float;
  WORD16 *p_adjthr_quant_spec_temp;
  FLOAT64 *p_cmpx_mdct_temp_buf;
  FLOAT32 *p_fft_p2_y;
  FLOAT32 *p_fft_p3_data_3;
  FLOAT32 *p_fft_p3_y;
  FLOAT32 *p_tcx_input;
  FLOAT32 *p_tcx_output;
  FLOAT64 *p_reconstructed_time_signal[MAX_TIME_CHANNELS];
  FLOAT32 *p_ol_pitch_buf_tmp;
  FLOAT32 *p_ol_pitch_speech_buf;
  FLOAT32 *p_ol_pitch_w_table;
  FLOAT32 *p_ol_pitch_R;
  FLOAT32 *p_ol_pitch_R0;
  WORD32 *ptr_num_fac_bits;
  WORD32 *ptr_tns_data_present;
  FLOAT32 *ptr_tmp_lp_res;

  FLOAT32 *ptr_sfb_form_fac[MAX_TIME_CHANNELS];
  FLOAT32 *ptr_sfb_num_relevant_lines[MAX_TIME_CHANNELS];
  FLOAT32 *ptr_sfb_ld_energy[MAX_TIME_CHANNELS];
  WORD32 *ptr_num_scfs;
  WORD32 *ptr_max_ch_dyn_bits;
  FLOAT32 *ptr_ch_bit_dist;
  pUWORD8 ptr_fd_scratch;
  pUWORD8 ptr_lpd_scratch;
  FLOAT32 *ptr_tcx_scratch;
  FLOAT64 *ptr_tns_scratch;
  WORD32 *ptr_next_win_scratch;
  FLOAT32 *ptr_acelp_scratch;
  FLOAT32 mixed_rad_fft[2 * LEN_SUPERFRAME];
  pVOID drc_scratch;
  VOID *ptr_drc_scratch_buf;
  VOID *ptr_stack_mem;
} iusace_scratch_mem;
