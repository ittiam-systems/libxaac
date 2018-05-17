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
#ifndef IXHEAACD_PS_DEC_H
#define IXHEAACD_PS_DEC_H

#define MAX_NUM_COLUMNS 32
#define NUM_OF_QUAD_MIRROR_FILTER_CHNLS 64
#define NUM_OF_ALL_PASS_CHNLS 23
#define NUM_OF_DEL_CHNLS \
  (NUM_OF_QUAD_MIRROR_FILTER_CHNLS - NUM_OF_ALL_PASS_CHNLS)
#define DEL_ALL_PASS 2
#define SMALL_DEL_STRT 12
#define SMALL_DEL 1
#define HIGH_DEL 14
#define NUM_OF_QUAD_MIRROR_FILTER_ALL_PASS_CHNLS \
  (NUM_OF_ALL_PASS_CHNLS - NO_QMF_CHANNELS_IN_HYBRID)
#define NUM_OF_QUAD_MIRROR_FILTER_ICC_CHNLS \
  (NUM_OF_QUAD_MIRROR_FILTER_ALL_PASS_CHNLS + NUM_OF_DEL_CHNLS)
#define PEAK_DECAYING_FACT 0x620a
#define NUM_SER_AP_LINKS 3
#define MAXIM_NUM_OF_PS_ENVLOPS 5
#define PSC_SQRT05F (0x5a82)
#define NUM_OF_BINS (20)
#define NUM_BANDS_FINE (34)

typedef WORD16((*REVERB_BUFFERS_RI)[NUM_SER_AP_LINKS])[32 * 2];
typedef WORD16((REVERB_BUFFERS_CH_RI[5])[NUM_SER_AP_LINKS])[16 * 2];

typedef struct {
  WORD16 (*delay_buf_qmf_ap_re_im)[32 * 2];

  WORD16 (*delay_buf_qmf_ld_re_im)[SMALL_DEL_STRT * 2];

  WORD16 (*delay_buf_qmf_sd_re_im)[2 * 32];

  WORD16 delay_buf_idx_ser[NUM_SER_AP_LINKS];
  WORD16 delay_sample_ser[NUM_SER_AP_LINKS];

  REVERB_BUFFERS_RI delay_buf_qmf_ser_re_im;
  WORD16 delay_buf_idx;
  WORD16 delay_buf_idx_long;

  WORD32 *peak_decay_diff;
  WORD32 *energy_prev;
  WORD32 *peak_decay_diff_prev;

  WORD32 *ptr_hyb_left_re;
  WORD32 *ptr_hyb_left_im;
  WORD32 *ptr_hyb_right_re;
  WORD32 *ptr_hyb_right_im;

  WORD16 delay_buf_qmf_sub_re_im[DEL_ALL_PASS][16 * 2];
  REVERB_BUFFERS_CH_RI delay_buf_qmf_sub_ser_re_im;

  WORD16 h11_h12_vec[2 * 24];
  WORD16 h21_h22_vec[2 * 24];

  WORD16 H11_H12[2 * 24];
  WORD16 H21_H22[2 * 24];

  WORD16 delta_h11_h12[2 * 24];
  WORD16 delta_h21_h22[2 * 24];

  FLAG force_mono;

  WORD16 delay_buffer_scale;
  WORD16 usb;

  WORD16 iid_par_prev[NUM_BANDS_FINE];
  WORD16 icc_par_prev[NUM_BANDS_FINE];

  FLAG ps_data_present;

  FLAG enable_iid;
  FLAG enable_icc;

  FLAG enable_ext;

  WORD16 iid_mode;
  WORD16 icc_mode;
  FLAG iid_quant;

  FLAG frame_class;
  WORD16 num_env;
  WORD16 border_position[MAXIM_NUM_OF_PS_ENVLOPS + 2];

  FLAG iid_dt[MAXIM_NUM_OF_PS_ENVLOPS];
  FLAG icc_dt[MAXIM_NUM_OF_PS_ENVLOPS];

  WORD16 iid_par_table[MAXIM_NUM_OF_PS_ENVLOPS + 2][NUM_BANDS_FINE];
  WORD16 icc_par_table[MAXIM_NUM_OF_PS_ENVLOPS + 2][NUM_BANDS_FINE];

  ia_hybrid_struct str_hybrid;
} ia_ps_dec_struct;

WORD32 ixheaacd_create_psdec(ia_ps_dec_struct *ptr_ps_dec,
                             VOID *sbr_persistent_mem, WORD32 *ptr_overlap_buf);

VOID ixheaacd_decorr_filter1_dec(ia_ps_dec_struct *ptr_ps_dec,
                                 ia_ps_tables_struct *ps_tables_ptr,
                                 WORD16 *transient_ratio);

VOID ixheaacd_decorr_filter1_armv7(ia_ps_dec_struct *ptr_ps_dec,
                                   ia_ps_tables_struct *ps_tables_ptr,
                                   WORD16 *transient_ratio);

VOID ixheaacd_decorr_filter2_dec(
    ia_ps_dec_struct *ptr_ps_dec, WORD32 *p_buf_left_real,
    WORD32 *p_buf_left_imag, WORD32 *p_buf_right_real, WORD32 *p_buf_right_imag,
    ia_ps_tables_struct *ps_tables_ptr, WORD16 *transient_ratio);

VOID ixheaacd_decorr_filter2_armv7(
    ia_ps_dec_struct *ptr_ps_dec, WORD32 *p_buf_left_real,
    WORD32 *p_buf_left_imag, WORD32 *p_buf_right_real, WORD32 *p_buf_right_imag,
    ia_ps_tables_struct *ps_tables_ptr, WORD16 *transient_ratio);

WORD32 ixheaacd_divide16_pos_dec(WORD32 op1, WORD32 op2);

WORD32 ixheaacd_divide16_pos_armv7(WORD32 op1, WORD32 op2);

VOID ixheaacd_decorrelation_dec(ia_ps_dec_struct *ptr_ps_dec,
                                WORD32 *p_buf_left_real,
                                WORD32 *p_buf_left_imag,
                                WORD32 *p_buf_right_real,
                                WORD32 *p_buf_right_imag,
                                ia_ps_tables_struct *ps_tables_ptr);

VOID ixheaacd_decorrelation_armv7(ia_ps_dec_struct *ptr_ps_dec,
                                  WORD32 *p_buf_left_real,
                                  WORD32 *p_buf_left_imag,
                                  WORD32 *p_buf_right_real,
                                  WORD32 *p_buf_right_imag,
                                  ia_ps_tables_struct *ps_tables_ptr);

VOID ixheaacd_init_ps_scale(ia_ps_dec_struct *ptr_ps_dec,
                            ia_sbr_scale_fact_struct *sbr_scale_factor);

VOID ixheaacd_init_rot_env(ia_ps_dec_struct *ptr_ps_dec, WORD16 env, WORD16 usb,
                           ia_sbr_tables_struct *sbr_tables_ptr,
                           const WORD16 *cos_sin_lookup_tab);

VOID ixheaacd_apply_ps(ia_ps_dec_struct *ptr_ps_dec, WORD32 **real_buf_left,
                       WORD32 **imag_buf_left, WORD32 *real_buf_right,
                       WORD32 *imag_buf_right,
                       ia_sbr_scale_fact_struct *sbr_scale_factor, WORD16 slot,
                       ia_sbr_tables_struct *sbr_tables_ptr);

VOID ixheaacd_apply_rot_dec(ia_ps_dec_struct *ptr_ps_dec, WORD32 *p_qmf_left_re,
                            WORD32 *p_qmf_left_im, WORD32 *p_qmf_right_re,
                            WORD32 *p_qmf_right_im,
                            ia_sbr_tables_struct *sbr_tables_ptr,
                            const WORD16 *ptr_res);

VOID ixheaacd_apply_rot_armv7(ia_ps_dec_struct *ptr_ps_dec,
                              WORD32 *qmf_left_real, WORD32 *qmf_left_imag,
                              WORD32 *qmf_right_real, WORD32 *qmf_right_imag,
                              ia_sbr_tables_struct *sbr_tables_ptr,
                              const WORD16 *ptr_resol);

VOID ixheaacd_scale_ps_states(ia_ps_dec_struct *ptr_ps_dec, WORD16 scale);

extern WORD16 ixheaacd_divideby2(WORD32 op);
extern WORD16 ixheaacd_divideby3(WORD32 op);

#endif
