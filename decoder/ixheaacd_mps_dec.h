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
#ifndef IXHEAACD_MPS_DEC_H
#define IXHEAACD_MPS_DEC_H

#define ABS_THR (1e-9f * 32768 * 32768)

#define MAX_NUM_QMF_BANDS_MPS (128)
#define MAX_NUM_QMF_BANDS_MPS_NEW (64)

#define MAX_PARAMETER_SETS_MPS (9)
#define MAX_M2_OUTPUT_MP (MAX_OUTPUT_CHANNELS_MPS)
#define BUFFER_LEN_HF_MPS ((QMF_HYBRID_FILT_ORDER - 1) / 2 + MAX_TIME_SLOTS)
#define MAX_OUTPUT_CHANNELS_MPS_AT (2)
#define HYBRID_BAND_BORDER (12)

#define DECORR_FILT_0_ORD (10)
#define DECORR_FILT_1_ORD (8)
#define DECORR_FILT_2_ORD (3)
#define DECORR_FILT_3_ORD (2)

#define MAX_DECORR_FIL_ORDER (DECORR_FILT_0_ORD)

typedef struct {
  FLOAT32 re;
  FLOAT32 im;

} ia_cmplx_flt_struct;

typedef struct {
  WORD32 re;
  WORD32 im;
} ia_cmplx_w32_struct;

typedef struct {
  FLOAT32 *re;
  FLOAT32 *im;
} ia_cmplx_fltp_struct;

typedef struct ia_mps_decor_filt_struct {
  WORD32 state_len;
  WORD32 num_len;
  WORD32 den_len;

  ia_cmplx_flt_struct state[MAX_DECORR_FIL_ORDER + 1];
  FLOAT32 *num;
  FLOAT32 *den;

} ia_mps_decor_filt_struct;

typedef struct ia_mps_decor_struct *ia_mps_decor_struct_handle;

#define MAX_HYBRID_BANDS_MPS (MAX_NUM_QMF_BANDS_MPS_NEW - 3 + 10)
#define MAX_TIME_SLOTS (72)
#define MAX_PARAMETER_BANDS (28)

#define MAX_M1_INPUT (2)
#define MAX_M1_OUTPUT (2)
#define MAX_M2_INPUT (2)

#define MAX_M_INPUT (2)
#define MAX_M_OUTPUT (2)

#define QMF_BANDS_TO_HYBRID (3)
#define MAX_HYBRID_ONLY_BANDS_PER_QMF (8)
#define QMF_HYBRID_FILT_ORDER (13)
#define BUFFER_LEN_LF_MPS (QMF_HYBRID_FILT_ORDER - 1 + MAX_TIME_SLOTS)
#define MAX_NO_TIME_SLOTS_DELAY (14)

#define MAXNRSBRCHANNELS 2

typedef struct ixheaacd_mps_decor_energy_adjust_filt_struct {
  WORD32 num_bins;
  FLOAT32 smooth_in_energy[MAX_PARAMETER_BANDS];
  FLOAT32 smooth_out_energy[MAX_PARAMETER_BANDS];

} ixheaacd_mps_decor_energy_adjust_filt_struct;

typedef struct ia_mps_decor_struct {
  WORD32 num_bins;
  ia_mps_decor_filt_struct filter[MAX_HYBRID_BANDS_MPS];
  ixheaacd_mps_decor_energy_adjust_filt_struct decor_nrg_smooth;

  WORD32 delay_sample_count[MAX_HYBRID_BANDS_MPS];

  ia_cmplx_flt_struct
      decor_delay_buffer[71][MAX_TIME_SLOTS + MAX_NO_TIME_SLOTS_DELAY];

} ia_mps_decor_struct;

typedef struct ia_mps_hybrid_filt_struct {
  ia_cmplx_flt_struct hf_buffer[MAX_NUM_QMF_BANDS_MPS][BUFFER_LEN_HF_MPS];
  ia_cmplx_w32_struct lf_buffer[QMF_BANDS_TO_HYBRID][BUFFER_LEN_LF_MPS];
} ia_mps_hybrid_filt_struct;

typedef struct ia_mps_data_struct {
  WORD32 bs_xxx_data_mode[MAX_PARAMETER_SETS_MPS];
  WORD32 quant_coarse_xxx_flag[MAX_PARAMETER_SETS_MPS];
  WORD32 bs_freq_res_stride_xxx[MAX_PARAMETER_SETS_MPS];
  WORD8 bs_quant_coarse_xxx[MAX_PARAMETER_SETS_MPS];
  WORD8 bs_quant_coarse_xxx_prev;
} ia_mps_data_struct;

typedef struct ia_mps_bs_frame {
  WORD8 independency_flag;

  WORD32 cld_idx[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];
  WORD32 icc_idx[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];

  WORD32 cld_idx_pre[MAX_PARAMETER_BANDS];
  WORD32 icc_idx_pre[MAX_PARAMETER_BANDS];

  WORD32 cmp_cld_idx[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];
  WORD32 cmp_icc_idx[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];

  WORD32 cmp_cld_idx_prev[MAX_PARAMETER_BANDS];
  WORD32 cmp_icc_idx_prev[MAX_PARAMETER_BANDS];

  ia_mps_data_struct cld_data;
  ia_mps_data_struct icc_data;
  ia_mps_data_struct ipd_data;

  WORD32 ipd_idx_data[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];
  WORD32 ipd_idx_data_prev[MAX_PARAMETER_BANDS];
  WORD32 ipd_idx[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];
  WORD32 ipd_idx_prev[MAX_PARAMETER_BANDS];

  WORD32 bs_smooth_mode[MAX_PARAMETER_SETS_MPS];
  WORD32 bs_smooth_time[MAX_PARAMETER_SETS_MPS];
  WORD32 bs_freq_res_stride_smg[MAX_PARAMETER_SETS_MPS];
  WORD32 bs_smg_data[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];

} ia_mps_bs_frame;

typedef struct ia_mps_smoothing_struct {
  WORD32 prev_smg_time;
  WORD32 inv_prev_smg_time;
  WORD32 prev_smg_data[MAX_PARAMETER_BANDS];
} ia_mps_smoothing_struct;

typedef struct ia_mps_env_reshape_struct {
  FLOAT32 pb_energy_prev[3][MAX_PARAMETER_BANDS];
  FLOAT32 avg_energy_prev[3];
  FLOAT32 frame_energy_prev[3];
} ia_mps_env_reshape_struct;

#define BP_SIZE 25

typedef struct ia_mps_stp_struct {
  FLOAT32 nrg_dir;
  FLOAT32 nrg_diff[2];
  FLOAT32 nrg_dir_prev;
  FLOAT32 nrg_diff_prev[2];
  FLOAT32 tp_scale_last[2];
  WORD32 init_flag;
  WORD32 update_old_ener;
} ia_mps_stp_struct;

typedef struct ia_mps_opd_smooth_struct {
  WORD32 smooth_l_phase[MAX_PARAMETER_BANDS];
  WORD32 smooth_r_phase[MAX_PARAMETER_BANDS];
} ia_mps_opd_smooth_struct;

typedef struct ia_mps_dec_state_struct {
  WORD32 in_ch_count;
  WORD32 out_ch_count;

  FLOAT32 input_gain;
  WORD32 dir_sig_count;

  WORD32 decor_sig_count;

  WORD32 time_slots;
  WORD32 present_time_slot;
  WORD32 frame_len;

  WORD32 temp_shape_enable_ch_stp[2];
  WORD32 temp_shape_enable_ch_ges[2];

  FLOAT32 env_shape_data[2][MAX_TIME_SLOTS];

  WORD8 parse_nxt_frame;

  WORD32 qmf_band_count;
  WORD32 hyb_band_count;
  WORD32 *hyb_band_to_processing_band_table;

  WORD32 res_ch_count;

  WORD32 res_bands;
  WORD32 max_res_bands;
  WORD32 bs_param_bands;

  WORD32 ext_frame_flag;
  WORD32 num_parameter_sets;
  WORD32 num_parameter_sets_prev;
  WORD32 param_slots[MAX_PARAMETER_SETS_MPS];
  WORD32 param_slot_diff[MAX_PARAMETER_SETS_MPS];
  FLOAT32 inv_param_slot_diff[MAX_PARAMETER_SETS_MPS];
  WORD32 inv_param_slot_diff_Q30[MAX_PARAMETER_SETS_MPS];

  WORD32 frame_length;
  WORD32 residual_coding;
  WORD32 bs_residual_present;

  WORD32 bs_residual_bands;

  ia_usac_dec_mps_config_struct *config;
  ia_mps_bs_frame bs_frame;

  WORD32 smoothing_time[MAX_PARAMETER_SETS_MPS];
  WORD32 inv_smoothing_time[MAX_PARAMETER_SETS_MPS];
  WORD32 smoothing_data[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];

  WORD32 bs_tsd_enable;
  WORD32 bs_tsd_sep_data[MAX_TIME_SLOTS];
  WORD32 bs_tsd_tr_phase_data[MAX_TIME_SLOTS];
  WORD32 tsd_num_tr_slots;
  WORD32 tsd_codeword_len;

  FLOAT32 cld_data[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];
  FLOAT32 icc_data[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];
  FLOAT32 ipd_data[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];

  WORD32 bs_phase_mode;
  WORD32 opd_smoothing_mode;
  WORD32 num_bands_ipd;

  FLOAT32 phase_l[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];
  FLOAT32 phase_r[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];
  FLOAT32 phase_l_prev[MAX_PARAMETER_BANDS];
  FLOAT32 phase_r_prev[MAX_PARAMETER_BANDS];

  WORD32 phase_l_fix[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];
  WORD32 phase_r_fix[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS];

  WORD32 m1_param_re[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                    [MAX_M_INPUT];
  WORD32 m1_param_im[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                    [MAX_M_INPUT];
  WORD32 m2_decor_re[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                    [MAX_M_INPUT];
  WORD32 m2_decor_im[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                    [MAX_M_INPUT];
  WORD32 m2_resid_re[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                    [MAX_M_INPUT];
  WORD32 m2_resid_im[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                    [MAX_M_INPUT];

  WORD32 m1_param_re_prev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
  WORD32 m1_param_im_prev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
  WORD32 m2_decor_re_prev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
  WORD32 m2_decor_im_prev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
  WORD32 m2_resid_re_prev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];
  WORD32 m2_resid_im_prev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT];

  ia_cmplx_flt_struct qmf_in[2][MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS_MPS_NEW];
  ia_cmplx_flt_struct hyb_in[2][MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS];
  ia_cmplx_flt_struct hyb_res[MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS];
  ia_cmplx_flt_struct v[MAX_M1_OUTPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS];
  ia_cmplx_flt_struct w_diff[MAX_M2_INPUT][MAX_TIME_SLOTS]
                            [MAX_HYBRID_BANDS_MPS];
  ia_cmplx_flt_struct w_dir[MAX_M2_INPUT][MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS];
  ia_cmplx_flt_struct hyb_dir_out[2][MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS];
  ia_cmplx_flt_struct hyb_diff_out[2][MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS];

  ia_cmplx_flt_struct qmf_out_dir[2][MAX_TIME_SLOTS][MAX_NUM_QMF_BANDS_MPS];
  ia_cmplx_flt_struct scratch[MAX_TIME_SLOTS][MAX_HYBRID_BANDS_MPS];

  FLOAT32 (*output_buffer)[4096];

  ia_mps_hybrid_filt_struct hyb_filt_state[2];
  ia_mps_poly_phase_synth_struct qmf_filt_state[2];

  ia_mps_decor_struct mps_decor;

  ia_mps_smoothing_struct smoothing_filt_state;

  ia_mps_env_reshape_struct guided_env_shaping;

  WORD32 bs_high_rate_mode;

  WORD32 tmp_buf[84 * MAX_NUM_QMF_BANDS_SAC];

  FLOAT32 r_out_re_in_m1[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                        [MAX_M_INPUT];
  FLOAT32 r_out_im_in_m1[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                        [MAX_M_INPUT];
  WORD32 r_out_re_scratch_m1[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                            [MAX_M_INPUT];
  WORD32 r_out_im_scratch_m1[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                            [MAX_M_INPUT];

  FLOAT32 r_out_re_in_m2[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                        [MAX_M_INPUT];
  FLOAT32 r_out_im_in_m2[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                        [MAX_M_INPUT];
  FLOAT32 r_out_diff_re_in_m2[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                             [MAX_M_INPUT];
  FLOAT32 r_out_diff_im_in_m2[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                             [MAX_M_INPUT];

  WORD32 r_out_re_fix_in_m2[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                           [MAX_M_INPUT];
  WORD32 r_out_im_fix_in_m2[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                           [MAX_M_INPUT];
  WORD32 r_diff_out_re_fix_in_m2[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS]
                                [MAX_M_OUTPUT][MAX_M_INPUT];
  WORD32 r_diff_out_im_fix_in_m2[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS]
                                [MAX_M_OUTPUT][MAX_M_INPUT];

  FLOAT32 r_out_ph_re_in_m2[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2];
  FLOAT32 r_out_ph_im_in_m2[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2];

  ia_mps_stp_struct subband_var;
  ia_mps_opd_smooth_struct opd_smooth;
  ia_mps_poly_phase_struct poly_phase_filt_kernel;
  VOID *p_sbr_dec[MAXNRSBRCHANNELS];
  VOID *p_sbr_frame[MAXNRSBRCHANNELS];
  VOID *p_sbr_header[MAXNRSBRCHANNELS];
} ia_mps_dec_state_struct;

VOID ixheaacd_mps_init_pre_and_post_matrix(ia_mps_dec_state_struct *self);
VOID ixheaacd_pre_and_mix_matrix_calculation(ia_mps_dec_state_struct *self);
WORD32 ixheaacd_mps_apply_pre_matrix(ia_mps_dec_state_struct *self);
WORD32 ixheaacd_mps_apply_mix_matrix(ia_mps_dec_state_struct *self);

VOID ixheaacd_mps_config(ia_mps_dec_state_struct *self, WORD32 frame_len,
                         WORD32 residual_coding,
                         ia_usac_dec_mps_config_struct *mps212_config);

WORD32 ixheaacd_mps_frame_decode(ia_mps_dec_state_struct *self);

WORD32 ixheaacd_mps_header_decode(ia_mps_dec_state_struct *self);

VOID ixheaacd_mps_env_init(ia_mps_dec_state_struct *self);
VOID ixheaacd_mps_time_env_shaping(ia_mps_dec_state_struct *self);

VOID ixheaacd_mps_pre_matrix_mix_matrix_smoothing(
    ia_mps_dec_state_struct *self);
VOID ixheaacd_mps_smoothing_opd(ia_mps_dec_state_struct *self);
WORD32 ixheaacd_mps_temp_process(ia_mps_dec_state_struct *self);

VOID ixheaacd_mps_par2umx_ps(ia_mps_dec_state_struct *self,
                             ia_mps_bs_frame *curr_bit_stream, WORD32 *h_real,
                             WORD32 param_set_idx);

VOID ixheaacd_mps_par2umx_ps_ipd_opd(ia_mps_dec_state_struct *self,
                                     ia_mps_bs_frame *curr_bit_stream,
                                     WORD32 *h_real, WORD32 param_set_idx);

VOID ixheaacd_mps_par2umx_pred(ia_mps_dec_state_struct *self,
                               ia_mps_bs_frame *curr_bit_stream, WORD32 *h_imag,
                               WORD32 *h_real, WORD32 param_set_idx,
                               WORD32 res_bands);

WORD32 ixheaacd_mps_upmix_interp(
    WORD32 m_matrix[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                   [MAX_M_INPUT],
    WORD32 r_matrix[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][MAX_M_OUTPUT]
                   [MAX_M_INPUT],
    WORD32 m_matrix_prev[MAX_PARAMETER_BANDS][MAX_M_OUTPUT][MAX_M_INPUT],
    WORD32 num_rows, WORD32 num_cols, ia_mps_dec_state_struct *self);

VOID ixheaacd_mps_phase_interpolation(
    FLOAT32 pl[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS],
    FLOAT32 pr[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS],
    FLOAT32 pl_prev[MAX_PARAMETER_BANDS], FLOAT32 pr_prev[MAX_PARAMETER_BANDS],
    FLOAT32 r_re[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2],
    FLOAT32 r_im[MAX_TIME_SLOTS][MAX_PARAMETER_BANDS][2],
    ia_mps_dec_state_struct *self);

#endif
