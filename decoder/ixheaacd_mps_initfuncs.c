/******************************************************************************
 *
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
#include <string.h>
#include "ixheaacd_type_def.h"
#include "ixheaacd_mps_struct_def.h"
#include "ixheaacd_mps_res_rom.h"
#include "ixheaacd_mps_aac_struct.h"
#include "ixheaacd_sbr_common.h"
#include "ixheaacd_constants.h"
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_aac_rom.h"
#include "ixheaacd_error_codes.h"
#include "ixheaacd_pulsedata.h"
#include "ixheaacd_pns.h"
#include "ixheaacd_drc_data_struct.h"
#include "ixheaacd_channelinfo.h"
#include "ixheaacd_sbrdecoder.h"
#include "ixheaacd_common_rom.h"
#include "ixheaacd_sbrdecsettings.h"
#include "ixheaacd_sbr_scale.h"
#include "ixheaacd_env_extr_part.h"
#include "ixheaacd_sbr_rom.h"
#include "ixheaacd_lpp_tran.h"
#include "ixheaacd_hybrid.h"
#include "ixheaacd_ps_dec.h"
#include "ixheaacd_env_extr.h"
#include "ixheaacd_audioobjtypes.h"
#include "ixheaacd_memory_standards.h"
#include "ixheaacd_latmdemux.h"
#include "ixheaacd_aacdec.h"
#include "ixheaacd_mps_polyphase.h"
#include "ixheaacd_config.h"
#include "ixheaacd_qmf_dec.h"
#include "ixheaacd_mps_dec.h"
#include "ixheaacd_struct_def.h"
#include "ixheaacd_mps_decor.h"
#include "ixheaacd_mps_bitdec.h"
#include "ixheaacd_mps_mdct_2_qmf.h"
#include "ixheaacd_mps_tonality.h"
#include "ixheaacd_mps_reshape_bb_env.h"
#include "ixheaacd_mps_hybfilter.h"
#include "ixheaacd_mps_blind.h"
#include "ixheaacd_mps_macro_def.h"
#include "ixheaacd_mps_tables.h"

#define ALIGN_SIZE64(x) ((((x) + 7) >> 3) << 3)

#define ALIGN_SIZE32(x) ((((x) + 3) >> 2) << 2)

WORD32 ixheaacd_getsize_mps_persistent() { return (ALIGN_SIZE64(sizeof(ia_mps_persistent_mem))); }

static WORD32 ixheaacd_calc_decorr_size() {
  WORD32 matrix_alloc_size, decorr_filter_size, num_den_size;
  WORD32 fraction_alloc_size, ducker_create_size, decor_dec_size;
  WORD32 state_alloc_size, alloc_size, dec_type = 0;

  matrix_alloc_size =
      2 * (MAX_HYBRID_BANDS * (MAX_TIME_SLOTS + MAX_NO_TIME_SLOTS_DELAY) * sizeof(WORD32) +
           MAX_HYBRID_BANDS * sizeof(VOID *)) *
      MAX_NO_DECORR_CHANNELS;
  decorr_filter_size = MAX_NO_DECORR_CHANNELS * MAX_HYBRID_BANDS *
                       sizeof(ia_mps_dec_decorr_filter_instance_struct);
  num_den_size =
      (MAX_NUM_DEN_LENGTH) * sizeof(WORD32) * MAX_NO_DECORR_CHANNELS * MAX_HYBRID_BANDS;

  if (dec_type == 1)
    fraction_alloc_size = 4 * num_den_size;
  else
    fraction_alloc_size = 2 * num_den_size;

  state_alloc_size =
      2 * (MAX_NUM_DEN_LENGTH) * sizeof(WORD32) * MAX_NO_DECORR_CHANNELS * MAX_HYBRID_BANDS;

  ducker_create_size = MAX_NO_DECORR_CHANNELS * (sizeof(ia_mps_dec_ducker_interface) +
                                                 sizeof(ia_mps_dec_duck_instance_struct));
  decor_dec_size = sizeof(ia_mps_dec_decorr_dec_struct) * MAX_NO_DECORR_CHANNELS;

  alloc_size = matrix_alloc_size + decorr_filter_size + fraction_alloc_size + ducker_create_size +
               decor_dec_size + state_alloc_size;

  return (2 * alloc_size);
}

WORD32 ixheaacd_mps_persistent_buffer_sizes() {
  WORD32 buffer_size;

  buffer_size = sizeof(ia_heaac_mps_state_struct);

  buffer_size += PREV_GAINAT;

  buffer_size += ARBDMX_ALPHA;

  buffer_size += M1_PREV;

  buffer_size += M1_PREV;

  buffer_size += M2_PREV_DECOR;

  buffer_size += M2_PREV_DECOR;

  buffer_size += M2_PREV_RESID;

  buffer_size += M2_PREV_RESID;

  buffer_size += QMF_DELAY_INPUT;

  buffer_size += QMF_DELAY_INPUT;

  buffer_size += ANA_BUF_SIZE;

  buffer_size += SYN_BUF_SIZE;

  buffer_size += ixheaacd_calc_decorr_size();

  buffer_size += HYB_FILTER_STATE_SIZE;

  buffer_size += TONALITY_STATE_SIZE;

  buffer_size += SMOOTHING_STATE_SIZE;

  buffer_size += RESHAPE_STATE_SIZE;

  buffer_size += SUBBAND_TP_SIZE;

  buffer_size += BLIND_DECODER_SIZE;

  buffer_size += sizeof(ia_mps_dec_spatial_bs_frame_struct);

  buffer_size += ARRAY_STRUCT_SIZE;

  return buffer_size;
}

VOID ixheaacd_set_mps_persistent_buffers(ia_heaac_mps_state_struct *pstr_mps_state,
                                         WORD32 *persistent_used, WORD32 num_channel,
                                         VOID *persistent_mem) {
  WORD32 used_persistent = *persistent_used;

  struct ia_mps_persistent_mem *mps_persistent_mem = &(pstr_mps_state->mps_persistent_mem);

  WORD32 decorr_size;

  num_channel = max(2, num_channel);

  mps_persistent_mem->prev_gain_at = (WORD32 *)((WORD8 *)persistent_mem);

  memset(mps_persistent_mem->prev_gain_at, 0, PREV_GAINAT);

  used_persistent += PREV_GAINAT;

  mps_persistent_mem->arbdmx_alpha_prev = (WORD32 *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->arbdmx_alpha_prev, 0, ARBDMX_ALPHA);
  used_persistent += ARBDMX_ALPHA;

  mps_persistent_mem->m1_param_real_prev = (WORD32 *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->m1_param_real_prev, 0, M1_PREV);
  used_persistent += M1_PREV;

  mps_persistent_mem->m1_param_imag_prev = (WORD32 *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->m1_param_imag_prev, 0, M1_PREV);
  used_persistent += M1_PREV;

  mps_persistent_mem->m2_decor_real_prev = (WORD32 *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->m2_decor_real_prev, 0, M2_PREV_DECOR);
  used_persistent += M2_PREV_DECOR;

  mps_persistent_mem->m2_decor_imag_prev = (WORD32 *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->m2_decor_imag_prev, 0, M2_PREV_DECOR);
  used_persistent += M2_PREV_DECOR;

  mps_persistent_mem->m2_resid_real_prev = (WORD32 *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->m2_resid_real_prev, 0, M2_PREV_RESID);
  used_persistent += M2_PREV_RESID;

  mps_persistent_mem->m2_resid_imag_prev = (WORD32 *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->m2_resid_imag_prev, 0, M2_PREV_RESID);
  used_persistent += M2_PREV_RESID;

  mps_persistent_mem->qmf_input_delay_real =
      (WORD32 *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->qmf_input_delay_real, 0, QMF_DELAY_INPUT);
  used_persistent += QMF_DELAY_INPUT;

  mps_persistent_mem->qmf_input_delay_imag =
      (WORD32 *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->qmf_input_delay_imag, 0, QMF_DELAY_INPUT);
  used_persistent += QMF_DELAY_INPUT;

  mps_persistent_mem->syn_qmf_states_buffer =
      (WORD32 *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->syn_qmf_states_buffer, 0, SYN_BUF_SIZE);
  used_persistent += SYN_BUF_SIZE;

  mps_persistent_mem->ana_qmf_states_buffer =
      (WORD32 *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->ana_qmf_states_buffer, 0, ANA_BUF_SIZE);
  used_persistent += ANA_BUF_SIZE;

  decorr_size = ixheaacd_calc_decorr_size();

  mps_persistent_mem->decorr_ptr = (WORD32 *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->decorr_ptr, 0, decorr_size);
  used_persistent += decorr_size;

  mps_persistent_mem->hyb_filter_state =
      (ia_mps_dec_thyb_filter_state_struct *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->hyb_filter_state, 0, HYB_FILTER_STATE_SIZE);
  used_persistent += HYB_FILTER_STATE_SIZE;

  mps_persistent_mem->ton_state =
      (ia_mps_dec_tonality_state_struct *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->ton_state, 0, TONALITY_STATE_SIZE);
  used_persistent += TONALITY_STATE_SIZE;

  mps_persistent_mem->smooth_state =
      (ia_mps_dec_smoothing_state_struct *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->smooth_state, 0, SMOOTHING_STATE_SIZE);
  used_persistent += SMOOTHING_STATE_SIZE;

  mps_persistent_mem->reshape_bb_env_state =
      (ia_mps_dec_reshape_bb_env_state_struct *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->reshape_bb_env_state, 0, RESHAPE_STATE_SIZE);
  used_persistent += RESHAPE_STATE_SIZE;

  mps_persistent_mem->sub_band_params =
      (ia_mps_dec_subband_tp_params_struct *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->sub_band_params, 0, SUBBAND_TP_SIZE);
  used_persistent += SUBBAND_TP_SIZE;

  mps_persistent_mem->blind_decoder =
      (ia_mps_dec_blind_decoder_struct *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->blind_decoder, 0, BLIND_DECODER_SIZE);
  used_persistent += BLIND_DECODER_SIZE;

  mps_persistent_mem->p_bs_frame =
      (ia_mps_dec_spatial_bs_frame_struct *)((WORD8 *)persistent_mem + used_persistent);
  memset(mps_persistent_mem->p_bs_frame, 0, sizeof(ia_mps_dec_spatial_bs_frame_struct));
  used_persistent += sizeof(ia_mps_dec_spatial_bs_frame_struct);

  pstr_mps_state->array_struct =
      (ia_mps_dec_reuse_array_struct *)((WORD8 *)persistent_mem + used_persistent);
  memset(pstr_mps_state->array_struct, 0, ARRAY_STRUCT_SIZE);
  used_persistent += ARRAY_STRUCT_SIZE;

  *persistent_used = used_persistent;
}

VOID ixheaacd_set_scratch_buffers(ia_heaac_mps_state_struct *pstr_mps_state, VOID *scratch_mem) {
  WORD32 scratch_used;
  ia_mps_dec_reuse_array_struct *p_array_struct = pstr_mps_state->array_struct;

  p_array_struct->qmf_residual_real = scratch_mem;
  scratch_used = QMF_RES_BUF_SIZE;
  p_array_struct->qmf_residual_imag = (WORD32 *)((WORD8 *)scratch_mem + scratch_used);
  scratch_used += QMF_RES_BUF_SIZE;

  p_array_struct->res_mdct = (WORD32 *)((WORD8 *)scratch_mem + scratch_used);
  scratch_used += MDCT_RES_BUF_SIZE;

  p_array_struct->m_qmf_real = (WORD32 *)((WORD8 *)scratch_mem + scratch_used);
  scratch_used += QMF_BUF_SIZE;
  p_array_struct->m_qmf_imag = (WORD32 *)((WORD8 *)scratch_mem + scratch_used);
  scratch_used += QMF_BUF_SIZE;

  p_array_struct->buf_real = (WORD32 *)((WORD8 *)scratch_mem + scratch_used);
  scratch_used += BUF_SIZE;
  p_array_struct->buf_imag = (WORD32 *)((WORD8 *)scratch_mem + scratch_used);
  scratch_used += BUF_SIZE;

  p_array_struct->hyb_output_real_dry = p_array_struct->res_mdct;
  p_array_struct->hyb_output_imag_dry =
      p_array_struct->res_mdct + MAX_OUTPUT_CHANNELS_AT_MPS * TSXHB;

  p_array_struct->x_real = p_array_struct->hyb_output_real_dry;
  p_array_struct->x_imag = p_array_struct->hyb_output_imag_dry;

  p_array_struct->time_out = p_array_struct->hyb_output_real_dry;

  p_array_struct->w_dry_real = p_array_struct->m_qmf_real;
  p_array_struct->w_dry_imag = p_array_struct->m_qmf_imag;

  p_array_struct->env_dmx_0 = p_array_struct->m_qmf_real + TSXHBX5;
  p_array_struct->env_dmx_1 = p_array_struct->env_dmx_0 + MAX_TIME_SLOTS;

  p_array_struct->qmf_residual_real_pre = p_array_struct->qmf_residual_real;
  p_array_struct->qmf_residual_real_post = p_array_struct->qmf_residual_real + RES_CHXQMFXTS;

  p_array_struct->qmf_residual_imag_pre = p_array_struct->qmf_residual_imag;
  p_array_struct->qmf_residual_imag_post = p_array_struct->qmf_residual_imag + RES_CHXQMFXTS;

  p_array_struct->buffer_real = p_array_struct->qmf_residual_real_post;
  p_array_struct->buffer_imag = p_array_struct->qmf_residual_imag_post;

  p_array_struct->m1_param = (ia_mps_dec_m1_param_struct *)p_array_struct->buffer_real;

  pstr_mps_state->aux_struct =
      (ia_mps_dec_auxilary_struct *)((WORD8 *)scratch_mem + scratch_used);
  scratch_used += sizeof(ia_mps_dec_auxilary_struct);

  pstr_mps_state->aux_struct->m2_param =
      (ia_mps_dec_m2_param_struct *)((WORD8 *)scratch_mem + scratch_used);
  scratch_used += sizeof(ia_mps_dec_m2_param_struct);

  pstr_mps_state->mps_scratch_mem_v = (VOID *)((WORD8 *)scratch_mem + scratch_used);
}
VOID ixheaacd_ana_filter_bank_init(ia_heaac_mps_state_struct *pstr_mps_state,
                                   ia_mps_dec_qmf_ana_filter_bank *qmf_bank) {
  memset(qmf_bank->qmf_states_buffer, 0,
         QMF_FILTER_STATE_ANA_SIZE_MPS * sizeof(qmf_bank->qmf_states_buffer[0]));
  qmf_bank->p_filter_ana =
      pstr_mps_state->ia_mps_dec_mps_table.qmf_table_ptr->ia_mps_enc_qmf_64_640;
  qmf_bank->flag = 0;
  qmf_bank->offset = 0;
  qmf_bank->ref_co_eff_ptr_l = qmf_bank->p_filter_ana + 10;
  qmf_bank->ref_co_eff_ptr_r = qmf_bank->p_filter_ana + QMF_FILTER_STATE_ANA_SIZE_MPS;
  qmf_bank->offset_l = 5;
  qmf_bank->offset_r = 5;
}

VOID ixheaacd_syn_filter_bank_create(ia_heaac_mps_state_struct *pstr_mps_state) {
  ia_mps_dec_qmf_syn_filter_bank *qmf_bank = &(pstr_mps_state->syn_qmf_bank);

  memset(qmf_bank->sbr_qmf_states_synthesis, 0, SYN_BUFFER_SIZE);
  qmf_bank->p_filter_syn =
      pstr_mps_state->ia_mps_dec_mps_table.tp_process_table_ptr->ia_mps_dec_qmf_64_640;
}

static IA_ERRORCODE ixheaacd_set_m2_params(ia_heaac_mps_state_struct *pstr_mps_state) {
  ia_heaac_mps_state_struct *curr_state = pstr_mps_state;
  WORD32 up_mix_type = curr_state->up_mix_type;

  switch (curr_state->tree_config) {
    case TREE_5151:
      if (up_mix_type == 2) {
        curr_state->m2_param_imag_present = 1;

        curr_state->m2_param_present[0][1] = 1;
        curr_state->m2_param_present[1][1] = 1;
        curr_state->m2_param_present[0][0] = 2;
        curr_state->m2_param_present[1][0] = 2;

        curr_state->m1_param_present[0][0] = 1;
        curr_state->m1_param_present[1][0] = 1;
      } else {
        if (up_mix_type == 3) {
          curr_state->m2_param_present[0][3] = 1;
          curr_state->m2_param_present[1][3] = 1;
          curr_state->m2_param_present[0][0] = 2;
          curr_state->m2_param_present[1][0] = 2;

          curr_state->m1_param_present[0][0] = 1;
          curr_state->m1_param_present[3][0] = 1;
        } else {
          curr_state->m2_param_present[0][0] = 3;
          curr_state->m2_param_present[0][1] = 3;
          curr_state->m2_param_present[0][2] = 3;
          curr_state->m2_param_present[0][3] = 3;
          curr_state->m2_param_present[1][0] = 3;
          curr_state->m2_param_present[1][1] = 3;
          curr_state->m2_param_present[1][2] = 3;
          curr_state->m2_param_present[1][3] = 3;
          curr_state->m2_param_present[2][0] = 3;
          curr_state->m2_param_present[2][1] = 3;
          curr_state->m2_param_present[2][2] = 3;
          curr_state->m2_param_present[3][0] = 3;
          curr_state->m2_param_present[4][0] = 3;
          curr_state->m2_param_present[4][1] = 3;
          curr_state->m2_param_present[4][4] = 3;
          curr_state->m2_param_present[5][0] = 3;
          curr_state->m2_param_present[5][1] = 3;
          curr_state->m2_param_present[5][4] = 3;

          curr_state->m1_param_present[0][0] = 1;
          curr_state->m1_param_present[1][0] = 1;
          curr_state->m1_param_present[2][0] = 1;
          curr_state->m1_param_present[3][0] = 1;
          curr_state->m1_param_present[4][0] = 1;
        }
      }
      break;
    case TREE_5152:
      if (up_mix_type == 2) {
        curr_state->m2_param_imag_present = 1;

        curr_state->m2_param_present[0][1] = 1;
        curr_state->m2_param_present[1][1] = 1;
        curr_state->m2_param_present[0][0] = 2;
        curr_state->m2_param_present[1][0] = 2;

        curr_state->m1_param_present[0][0] = 1;
        curr_state->m1_param_present[1][0] = 1;
      } else {
        if (up_mix_type == 3) {
          curr_state->m2_param_present[0][2] = 1;
          curr_state->m2_param_present[1][2] = 1;
          curr_state->m2_param_present[0][0] = 2;
          curr_state->m2_param_present[1][0] = 2;

          curr_state->m1_param_present[0][0] = 1;
          curr_state->m1_param_present[2][0] = 1;
        } else {
          curr_state->m2_param_present[0][0] = 3;
          curr_state->m2_param_present[0][1] = 3;
          curr_state->m2_param_present[0][2] = 3;
          curr_state->m2_param_present[0][3] = 3;
          curr_state->m2_param_present[1][0] = 3;
          curr_state->m2_param_present[1][1] = 3;
          curr_state->m2_param_present[1][2] = 3;
          curr_state->m2_param_present[1][3] = 3;
          curr_state->m2_param_present[2][0] = 3;
          curr_state->m2_param_present[2][1] = 3;
          curr_state->m2_param_present[2][2] = 3;
          curr_state->m2_param_present[2][4] = 3;
          curr_state->m2_param_present[3][0] = 3;
          curr_state->m2_param_present[3][1] = 3;
          curr_state->m2_param_present[3][2] = 3;
          curr_state->m2_param_present[3][4] = 3;
          curr_state->m2_param_present[4][0] = 3;
          curr_state->m2_param_present[4][1] = 3;
          curr_state->m2_param_present[5][0] = 3;

          curr_state->m1_param_present[0][0] = 1;
          curr_state->m1_param_present[1][0] = 1;
          curr_state->m1_param_present[2][0] = 1;
          curr_state->m1_param_present[3][0] = 1;
          curr_state->m1_param_present[4][0] = 1;
        }
      }
      break;
    case TREE_525:
      if (up_mix_type == 1) {
        curr_state->m2_param_present[0][3] = 1;
        curr_state->m2_param_present[1][3] = 1;
        curr_state->m2_param_present[2][4] = 1;
        curr_state->m2_param_present[3][4] = 1;

        curr_state->m2_param_present[0][0] = 2;
        curr_state->m2_param_present[1][0] = 2;
        curr_state->m2_param_present[2][1] = 2;
        curr_state->m2_param_present[3][1] = 2;
        curr_state->m2_param_present[4][2] = 2;

        curr_state->m1_param_present[0][0] = 1;
        curr_state->m1_param_present[0][1] = 1;
        curr_state->m1_param_present[0][2] = 1;
        curr_state->m1_param_present[1][0] = 1;
        curr_state->m1_param_present[1][1] = 1;
        curr_state->m1_param_present[1][2] = 1;
        curr_state->m1_param_present[2][0] = 1;
        curr_state->m1_param_present[2][1] = 1;
        curr_state->m1_param_present[2][2] = 1;
      } else if (up_mix_type == 2) {
        if (curr_state->binaural_quality == 1) {
        } else {
          curr_state->m2_param_imag_present = 1;

          curr_state->m2_param_present[0][0] = 2;
          curr_state->m2_param_present[0][1] = 2;
          curr_state->m2_param_present[1][0] = 2;
          curr_state->m2_param_present[1][1] = 2;
          if (curr_state->arbitrary_downmix == 2) {
            curr_state->m2_param_present[0][2] = 2;
            curr_state->m2_param_present[0][3] = 2;
            curr_state->m2_param_present[1][2] = 2;
            curr_state->m2_param_present[1][3] = 2;
          }

          curr_state->m1_param_present[0][0] = 1;
          curr_state->m1_param_present[1][1] = 1;
          curr_state->m1_param_present[2][3] = 1;
          curr_state->m1_param_present[3][4] = 1;
        }
      } else {
        curr_state->m2_param_present[0][0] = 3;
        curr_state->m2_param_present[0][3] = 3;
        curr_state->m2_param_present[1][0] = 3;
        curr_state->m2_param_present[1][3] = 3;
        curr_state->m2_param_present[2][1] = 3;
        curr_state->m2_param_present[2][4] = 3;
        curr_state->m2_param_present[3][1] = 3;
        curr_state->m2_param_present[3][4] = 3;
        curr_state->m2_param_present[4][2] = 3;
        curr_state->m2_param_present[5][2] = 3;

        curr_state->m1_param_present[0][0] = 1;
        curr_state->m1_param_present[0][1] = 1;
        curr_state->m1_param_present[0][2] = 1;
        curr_state->m1_param_present[0][3] = 1;
        curr_state->m1_param_present[0][4] = 1;
        curr_state->m1_param_present[1][0] = 1;
        curr_state->m1_param_present[1][1] = 1;
        curr_state->m1_param_present[1][2] = 1;
        curr_state->m1_param_present[1][3] = 1;
        curr_state->m1_param_present[1][4] = 1;
        curr_state->m1_param_present[2][0] = 1;
        curr_state->m1_param_present[2][1] = 1;
        curr_state->m1_param_present[2][2] = 1;
        curr_state->m1_param_present[2][3] = 1;
        curr_state->m1_param_present[2][4] = 1;
      }
      break;
    case TREE_7271:
      if (up_mix_type == 0) {
        curr_state->m2_param_present[0][3] = 3;
        curr_state->m2_param_present[0][6] = 3;
        curr_state->m2_param_present[1][3] = 3;
        curr_state->m2_param_present[1][6] = 3;
        curr_state->m2_param_present[2][3] = 3;
        curr_state->m2_param_present[3][4] = 3;
        curr_state->m2_param_present[3][7] = 3;
        curr_state->m2_param_present[4][4] = 3;
        curr_state->m2_param_present[4][7] = 3;
        curr_state->m2_param_present[5][4] = 3;

        curr_state->m2_param_present[0][0] = 2;
        curr_state->m2_param_present[1][0] = 2;
        curr_state->m2_param_present[2][0] = 2;
        curr_state->m2_param_present[3][1] = 2;
        curr_state->m2_param_present[4][1] = 2;
        curr_state->m2_param_present[5][1] = 2;
        curr_state->m2_param_present[6][2] = 2;
        curr_state->m2_param_present[7][2] = 2;

        curr_state->m1_param_present[0][0] = 1;
        curr_state->m1_param_present[0][1] = 1;
        curr_state->m1_param_present[0][2] = 1;
        curr_state->m1_param_present[0][3] = 1;
        curr_state->m1_param_present[0][4] = 1;
        curr_state->m1_param_present[1][0] = 1;
        curr_state->m1_param_present[1][1] = 1;
        curr_state->m1_param_present[1][2] = 1;
        curr_state->m1_param_present[1][3] = 1;
        curr_state->m1_param_present[1][4] = 1;
        curr_state->m1_param_present[2][0] = 1;
        curr_state->m1_param_present[2][1] = 1;
        curr_state->m1_param_present[2][2] = 1;
        curr_state->m1_param_present[2][3] = 1;
        curr_state->m1_param_present[2][4] = 1;
      } else if (up_mix_type == 2) {
        if (curr_state->binaural_quality == 1) {
        } else {
          curr_state->m2_param_imag_present = 1;

          curr_state->m2_param_present[0][0] = 2;
          curr_state->m2_param_present[0][1] = 2;
          curr_state->m2_param_present[1][0] = 2;
          curr_state->m2_param_present[1][1] = 2;
          if (curr_state->arbitrary_downmix == 2) {
            curr_state->m2_param_present[0][2] = 2;
            curr_state->m2_param_present[0][3] = 2;
            curr_state->m2_param_present[1][2] = 2;
            curr_state->m2_param_present[1][3] = 2;
          }

          curr_state->m1_param_present[0][0] = 1;
          curr_state->m1_param_present[1][1] = 1;
          curr_state->m1_param_present[2][3] = 1;
          curr_state->m1_param_present[3][4] = 1;
        }
      }
      break;
    case TREE_7272:
      if (up_mix_type == 0) {
        curr_state->m2_param_present[0][3] = 3;
        curr_state->m2_param_present[1][3] = 3;
        curr_state->m2_param_present[1][6] = 3;
        curr_state->m2_param_present[2][3] = 3;
        curr_state->m2_param_present[2][6] = 3;
        curr_state->m2_param_present[3][4] = 3;
        curr_state->m2_param_present[4][4] = 3;
        curr_state->m2_param_present[4][7] = 3;
        curr_state->m2_param_present[5][4] = 3;
        curr_state->m2_param_present[5][7] = 3;

        curr_state->m2_param_present[0][0] = 2;
        curr_state->m2_param_present[1][0] = 2;
        curr_state->m2_param_present[2][0] = 2;
        curr_state->m2_param_present[3][1] = 2;
        curr_state->m2_param_present[4][1] = 2;
        curr_state->m2_param_present[5][1] = 2;
        curr_state->m2_param_present[6][2] = 2;
        curr_state->m2_param_present[7][2] = 2;

        curr_state->m1_param_present[0][0] = 1;
        curr_state->m1_param_present[0][1] = 1;
        curr_state->m1_param_present[0][2] = 1;
        curr_state->m1_param_present[0][3] = 1;
        curr_state->m1_param_present[0][4] = 1;
        curr_state->m1_param_present[1][0] = 1;
        curr_state->m1_param_present[1][1] = 1;
        curr_state->m1_param_present[1][2] = 1;
        curr_state->m1_param_present[1][3] = 1;
        curr_state->m1_param_present[1][4] = 1;
        curr_state->m1_param_present[2][0] = 1;
        curr_state->m1_param_present[2][1] = 1;
        curr_state->m1_param_present[2][2] = 1;
        curr_state->m1_param_present[2][3] = 1;
        curr_state->m1_param_present[2][4] = 1;
      } else if (up_mix_type == 2) {
        if (curr_state->binaural_quality == 1) {
        } else {
          curr_state->m2_param_imag_present = 1;

          curr_state->m2_param_present[0][0] = 2;
          curr_state->m2_param_present[0][1] = 2;
          curr_state->m2_param_present[1][0] = 2;
          curr_state->m2_param_present[1][1] = 2;
          if (curr_state->arbitrary_downmix == 2) {
            curr_state->m2_param_present[0][2] = 2;
            curr_state->m2_param_present[0][3] = 2;
            curr_state->m2_param_present[1][2] = 2;
            curr_state->m2_param_present[1][3] = 2;
          }

          curr_state->m1_param_present[0][0] = 1;
          curr_state->m1_param_present[1][1] = 1;
          curr_state->m1_param_present[2][3] = 1;
          curr_state->m1_param_present[3][4] = 1;
        }
      }
      break;
    case TREE_7571:
      curr_state->m2_param_present[0][6] = 3;
      curr_state->m2_param_present[1][6] = 3;
      curr_state->m2_param_present[3][7] = 3;
      curr_state->m2_param_present[4][7] = 3;

      curr_state->m2_param_present[0][0] = 2;
      curr_state->m2_param_present[1][0] = 2;
      curr_state->m2_param_present[2][4] = 2;
      curr_state->m2_param_present[3][1] = 2;
      curr_state->m2_param_present[4][1] = 2;
      curr_state->m2_param_present[5][5] = 2;
      curr_state->m2_param_present[6][2] = 2;
      curr_state->m2_param_present[7][3] = 2;

      curr_state->m1_param_present[0][0] = 1;
      curr_state->m1_param_present[1][1] = 1;
      curr_state->m1_param_present[2][2] = 1;
      curr_state->m1_param_present[3][3] = 1;
      curr_state->m1_param_present[4][4] = 1;
      curr_state->m1_param_present[5][5] = 1;
      curr_state->m1_param_present[6][0] = 1;
      curr_state->m1_param_present[7][1] = 1;
      break;
    case TREE_7572:
      curr_state->m2_param_present[1][6] = 3;
      curr_state->m2_param_present[2][6] = 3;
      curr_state->m2_param_present[4][7] = 3;
      curr_state->m2_param_present[5][7] = 3;

      curr_state->m2_param_present[0][0] = 2;
      curr_state->m2_param_present[1][4] = 2;
      curr_state->m2_param_present[2][4] = 2;
      curr_state->m2_param_present[3][1] = 2;
      curr_state->m2_param_present[4][5] = 2;
      curr_state->m2_param_present[5][5] = 2;
      curr_state->m2_param_present[6][2] = 2;
      curr_state->m2_param_present[7][3] = 2;

      curr_state->m1_param_present[0][0] = 1;
      curr_state->m1_param_present[1][1] = 1;
      curr_state->m1_param_present[2][2] = 1;
      curr_state->m1_param_present[3][3] = 1;
      curr_state->m1_param_present[4][4] = 1;
      curr_state->m1_param_present[5][5] = 1;
      curr_state->m1_param_present[6][4] = 1;
      curr_state->m1_param_present[7][5] = 1;
      break;
    default:
      return IA_XHEAAC_MPS_DEC_EXE_FATAL_UNSUPPRORTED_TREE_CONFIG;
      break;
  };

  return IA_NO_ERROR;
}

VOID ixheaacd_sb_tp_init(ia_heaac_mps_state_struct *pstr_mps_state) {
  WORD32 ch;
  ia_mps_dec_subband_tp_params_struct *sub_band_tp =
      pstr_mps_state->mps_persistent_mem.sub_band_params;
  WORD32 *prev_tp_scale = sub_band_tp->prev_tp_scale;
  WORD16 *q_prev_tp_scale = sub_band_tp->q_prev_tp_scale;

  WORD32 *old_wet_ener = sub_band_tp->old_wet_ener;
  WORD16 *q_old_wet_ener = sub_band_tp->q_old_wet_ener;

  WORD32 *run_wet_ener = sub_band_tp->run_wet_ener;
  WORD16 *q_run_wet_ener = sub_band_tp->q_run_wet_ener;

  for (ch = 0; ch < MAX_OUTPUT_CHANNELS_MPS; ch++) {
    prev_tp_scale[ch] = ONE_IN_Q15;
    q_prev_tp_scale[ch] = 15;

    old_wet_ener[ch] = ONE_IN_Q30;
    q_old_wet_ener[ch] = 0;
    run_wet_ener[ch] = 0;
    q_run_wet_ener[ch] = 0;
  }
  for (ch = 0; ch < MAX_INPUT_CHANNELS_MPS; ch++) {
    sub_band_tp->old_dry_ener[ch] = ONE_IN_Q30;
    sub_band_tp->q_old_dry_ener[ch] = 0;
    sub_band_tp->run_dry_ener[ch] = 0;
    sub_band_tp->q_run_dry_ener[ch] = 0;
  }
}

VOID ixheaacd_decorr_init(ia_heaac_mps_state_struct *pstr_mps_state) {
  VOID *decorr_persistent = pstr_mps_state->mps_persistent_mem.decorr_ptr;
  WORD32 i, k;
  WORD32 hybrid_bands = pstr_mps_state->hybrid_bands;
  WORD32 dec_type = pstr_mps_state->dec_type;

  for (k = 0; k < MAX_NO_DECORR_CHANNELS; k++) {
    pstr_mps_state->ap_decor[k] = decorr_persistent;
    decorr_persistent = (WORD8 *)decorr_persistent + sizeof(ia_mps_dec_decorr_dec_struct);
  }
  for (k = 0; k < MAX_NO_DECORR_CHANNELS; k++) {
    pstr_mps_state->ap_decor[k]->ducker = decorr_persistent;
    decorr_persistent = (WORD8 *)decorr_persistent + sizeof(ia_mps_dec_ducker_interface) +
                        sizeof(ia_mps_dec_duck_instance_struct);
  }

  for (k = 0; k < MAX_NO_DECORR_CHANNELS; k++) {
    for (i = 0; i < hybrid_bands; i++) {
      pstr_mps_state->ap_decor[k]->filter[i] = decorr_persistent;
      decorr_persistent =
          (WORD8 *)decorr_persistent + sizeof(ia_mps_dec_decorr_filter_instance_struct);
    }
  }

  if (dec_type == 1) {
    for (k = 0; k < MAX_NO_DECORR_CHANNELS; k++) {
      for (i = 0; i < hybrid_bands; i++) {
        pstr_mps_state->ap_decor[k]->filter[i]->numerator_real = decorr_persistent;
        decorr_persistent = (WORD8 *)decorr_persistent + sizeof(WORD32) * MAX_NUM_DEN_LENGTH;
        pstr_mps_state->ap_decor[k]->filter[i]->denominator_real = decorr_persistent;
        decorr_persistent = (WORD8 *)decorr_persistent + sizeof(WORD32) * MAX_NUM_DEN_LENGTH;
        pstr_mps_state->ap_decor[k]->filter[i]->numerator_imag = decorr_persistent;
        decorr_persistent = (WORD8 *)decorr_persistent + sizeof(WORD32) * MAX_NUM_DEN_LENGTH;
        pstr_mps_state->ap_decor[k]->filter[i]->denominator_imag = decorr_persistent;
        decorr_persistent = (WORD8 *)decorr_persistent + sizeof(WORD32) * MAX_NUM_DEN_LENGTH;
      }
    }
  } else {
    for (k = 0; k < MAX_NO_DECORR_CHANNELS; k++) {
      for (i = 0; i < hybrid_bands; i++) {
        pstr_mps_state->ap_decor[k]->filter[i]->numerator_real = decorr_persistent;
        decorr_persistent = (WORD8 *)decorr_persistent + sizeof(WORD32) * MAX_NUM_DEN_LENGTH;
        pstr_mps_state->ap_decor[k]->filter[i]->denominator_real = decorr_persistent;
        decorr_persistent = (WORD8 *)decorr_persistent + sizeof(WORD32) * MAX_NUM_DEN_LENGTH;
      }
    }
  }

  for (k = 0; k < MAX_NO_DECORR_CHANNELS; k++) {
    for (i = 0; i < hybrid_bands; i++) {
      pstr_mps_state->ap_decor[k]->filter[i]->state_real = decorr_persistent;
      decorr_persistent = (WORD8 *)decorr_persistent + sizeof(WORD32) * MAX_NUM_DEN_LENGTH;
      pstr_mps_state->ap_decor[k]->filter[i]->state_imag = decorr_persistent;
      decorr_persistent = (WORD8 *)decorr_persistent + sizeof(WORD32) * MAX_NUM_DEN_LENGTH;
    }
  }

  for (k = 0; k < MAX_NO_DECORR_CHANNELS; k++) {
    pstr_mps_state->ap_decor[k]->delay_buffer_real =
        (WORD32 **)ALIGN_SIZE64((SIZE_T)(decorr_persistent));
    decorr_persistent = (WORD8 *)decorr_persistent + 8 * hybrid_bands;
    for (i = 0; i < hybrid_bands; i++) {
      pstr_mps_state->ap_decor[k]->delay_buffer_real[i] =
          (WORD32 *)ALIGN_SIZE64((SIZE_T)(decorr_persistent));

      decorr_persistent =
          (WORD8 *)decorr_persistent + 4 * (MAX_TIME_SLOTS + MAX_NO_TIME_SLOTS_DELAY);
    }

    pstr_mps_state->ap_decor[k]->delay_buffer_imag =
        (WORD32 **)ALIGN_SIZE64((SIZE_T)(decorr_persistent));
    decorr_persistent = (WORD8 *)decorr_persistent + 8 * hybrid_bands;

    for (i = 0; i < hybrid_bands; i++) {
      pstr_mps_state->ap_decor[k]->delay_buffer_imag[i] =
          (WORD32 *)ALIGN_SIZE64((SIZE_T)(decorr_persistent));

      decorr_persistent =
          (WORD8 *)decorr_persistent + 4 * (MAX_TIME_SLOTS + MAX_NO_TIME_SLOTS_DELAY);
    }
  }
}

VOID ixheaacd_bs_frame_init(ia_heaac_mps_state_struct *pstr_mps_state) {
  ia_mps_dec_spatial_bs_frame_struct *bs_frame = pstr_mps_state->bs_frame;

  memset(bs_frame->ott_cld_idx_prev, 0,
         MAX_NUM_OTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->ott_cld_idx_prev[0][0]));
  memset(bs_frame->ott_icc_idx_prev, 0,
         MAX_NUM_OTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->ott_icc_idx_prev[0][0]));
  memset(bs_frame->cmp_ott_cld_idx_prev, 0,
         MAX_NUM_OTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->cmp_ott_cld_idx_prev[0][0]));
  memset(bs_frame->cmp_ott_icc_idx_prev, 0,
         MAX_NUM_OTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->cmp_ott_icc_idx_prev[0][0]));

  memset(bs_frame->ttt_cpc_1_idx_prev, 0,
         MAX_NUM_TTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->ttt_cpc_1_idx_prev[0][0]));
  memset(bs_frame->ttt_cpc_2_idx_prev, 0,
         MAX_NUM_TTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->ttt_cpc_2_idx_prev[0][0]));
  memset(bs_frame->ttt_cld_1_idx_prev, 0,
         MAX_NUM_TTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->ttt_cld_1_idx_prev[0][0]));
  memset(bs_frame->ttt_cld_2_idx_prev, 0,
         MAX_NUM_TTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->ttt_cld_2_idx_prev[0][0]));
  memset(bs_frame->ttt_icc_idx_prev, 0,
         MAX_NUM_TTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->ttt_icc_idx_prev[0][0]));
  memset(bs_frame->cmp_ttt_cpc_1_idx_prev, 0,
         MAX_NUM_TTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->cmp_ttt_cpc_1_idx_prev[0][0]));
  memset(bs_frame->cmp_ttt_cpc_2_idx_prev, 0,
         MAX_NUM_TTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->cmp_ttt_cpc_2_idx_prev[0][0]));
  memset(bs_frame->cmp_ttt_cld_1_idx_prev, 0,
         MAX_NUM_TTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->cmp_ttt_cld_1_idx_prev[0][0]));
  memset(bs_frame->cmp_ttt_cld_2_idx_prev, 0,
         MAX_NUM_TTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->cmp_ttt_cld_2_idx_prev[0][0]));
  memset(bs_frame->cmp_ttt_icc_idx_prev, 0,
         MAX_NUM_TTT * MAX_PARAMETER_BANDS * sizeof(bs_frame->cmp_ttt_icc_idx_prev[0][0]));

  memset(bs_frame->arbdmx_gain_idx_prev, 0,
         MAX_INPUT_CHANNELS_MPS * MAX_PARAMETER_BANDS *
             sizeof(bs_frame->arbdmx_gain_idx_prev[0][0]));
  memset(bs_frame->cmp_arbdmx_gain_idx_prev, 0,
         MAX_INPUT_CHANNELS_MPS * MAX_PARAMETER_BANDS *
             sizeof(bs_frame->cmp_arbdmx_gain_idx_prev[0][0]));
}

IA_ERRORCODE ixheaacd_modules_init(ia_heaac_mps_state_struct *pstr_mps_state) {
  IA_ERRORCODE err_code = IA_NO_ERROR;
  ia_mps_dec_thyb_filter_state_struct *hyb_filter_state =
      pstr_mps_state->mps_persistent_mem.hyb_filter_state;
  WORD32 up_mix_type = pstr_mps_state->up_mix_type;
  WORD32 in_channels = pstr_mps_state->num_input_channels;
  WORD32 n_ch;
  if (pstr_mps_state->smooth_config) ixheaacd_init_tonality(pstr_mps_state);

  if (up_mix_type != 2) {
    if (pstr_mps_state->temp_shape_config == 2) {
      ixheaacd_init_bb_env(pstr_mps_state);
    }
  }

  if (pstr_mps_state->scaling_enable == 1) ixheaacd_sb_tp_init(pstr_mps_state);

  err_code = ixheaacd_syn_filt_bank_init(pstr_mps_state->syn, pstr_mps_state->qmf_bands);

  if (err_code != IA_NO_ERROR) return err_code;

  for (n_ch = 0; n_ch < in_channels; n_ch++) {
    ixheaacd_init_ana_hyb_filt_bank(&hyb_filter_state[n_ch]);
  }

  for (n_ch = 0; n_ch < in_channels; n_ch++) {
    pstr_mps_state->aux_struct->arbdmx_alpha[n_ch] = 0;
  }

  memset(&pstr_mps_state->aux_struct->ttt_cld_1[0], 0, 8 * 28 * sizeof(WORD32));

  if (pstr_mps_state->residual_coding) {
    WORD32 offset = in_channels;
    for (n_ch = 0; n_ch < pstr_mps_state->num_ott_boxes + pstr_mps_state->num_ttt_boxes; n_ch++) {
      if (pstr_mps_state->res_bands[n_ch] > 0) {
        ixheaacd_init_ana_hyb_filt_bank(&hyb_filter_state[offset + n_ch]);
      }
    }
  }

  if (pstr_mps_state->arbitrary_downmix == 2) {
    WORD32 offset = in_channels + pstr_mps_state->num_ott_boxes + pstr_mps_state->num_ttt_boxes;
    for (n_ch = 0; n_ch < in_channels; n_ch++) {
      ixheaacd_init_ana_hyb_filt_bank(&hyb_filter_state[offset + n_ch]);
    }
  }
  if (up_mix_type == 1) {
    ixheaacd_init_blind(pstr_mps_state);
  }

  pstr_mps_state->parse_next_bitstream_frame = 1;

  ixheaacd_bs_frame_init(pstr_mps_state);

  return err_code;
}

IA_ERRORCODE ixheaacd_header_parse(ia_heaac_mps_state_struct *pstr_mps_state) {
  IA_ERRORCODE err_code = IA_NO_ERROR;
  WORD32 up_mix_type = pstr_mps_state->up_mix_type;
  WORD32 sac_header_len, alignment_bits = 0;
  ia_bit_buf_struct *mps_bit_buf = pstr_mps_state->ptr_mps_bit_buff;

  if (up_mix_type != 1) {
    pstr_mps_state->sac_time_align_flag = ixheaacd_read_bits_buf(mps_bit_buf, 1);

    sac_header_len = ixheaacd_read_bits_buf(mps_bit_buf, 7);
    if (sac_header_len == 127) {
      sac_header_len += ixheaacd_read_bits_buf(mps_bit_buf, 16);
    }

    err_code = ixheaacd_parse_specific_config(pstr_mps_state, sac_header_len);
    if (err_code != IA_NO_ERROR) return err_code;

    if (pstr_mps_state->bs_config.bs_temp_shape_config == 1) pstr_mps_state->scaling_enable = 1;

    ixheaacd_byte_align(mps_bit_buf, &alignment_bits);
  } else {
    err_code = ixheaacd_default_specific_config(pstr_mps_state, pstr_mps_state->sampling_freq);
    if (err_code != IA_NO_ERROR) return err_code;
  }
  return err_code;
}

IA_ERRORCODE ixheaacd_aac_mps_create(ia_heaac_mps_state_struct *pstr_mps_state) {
  WORD32 i, j, k;
  WORD32 *prev_gain_at = pstr_mps_state->mps_persistent_mem.prev_gain_at;
  IA_ERRORCODE err_code = IA_NO_ERROR;
  WORD32 dec_type = pstr_mps_state->dec_type;
  WORD32 up_mix_type = pstr_mps_state->up_mix_type;
  WORD32 in_channels = pstr_mps_state->num_input_channels;

  if (pstr_mps_state == 0) return IA_XHEAAC_DEC_INIT_FATAL_DEC_INIT_FAIL;

  pstr_mps_state->num_parameter_sets = 1;

  for (i = 0; i < MAX_OUTPUT_CHANNELS_AT_MPS; i++) {
    for (j = 0; j < MAX_PARAMETER_BANDS; j++) {
      *prev_gain_at++ = ONE_IN_Q15;
    }
  }

  for (i = 0; i < in_channels; i++) {
    ixheaacd_ana_filter_bank_init(pstr_mps_state, &(pstr_mps_state->qmf_bank[i]));
  }

  ixheaacd_syn_filter_bank_create(pstr_mps_state);

  err_code = ixheaacd_set_current_state_parameters(pstr_mps_state);
  if (err_code != IA_NO_ERROR) return err_code;

  err_code = ixheaacd_set_m2_params(pstr_mps_state);
  if (err_code != IA_NO_ERROR) return err_code;

  err_code = ixheaacd_mdct2qmf_create(pstr_mps_state);
  if (err_code != IA_NO_ERROR) return err_code;

  ixheaacd_decorr_init(pstr_mps_state);

  for (k = 0; k < MAX_NO_DECORR_CHANNELS; k++) {
    WORD32 idec;

    if (up_mix_type == 3) {
      idec = 0;
    } else {
      idec = k;
    }

    err_code = ixheaacd_decorr_create((pstr_mps_state->ap_decor[k]), pstr_mps_state->hybrid_bands,
                                      idec, dec_type, pstr_mps_state->decorr_config,
                                      &(pstr_mps_state->ia_mps_dec_mps_table));
    if (err_code != IA_NO_ERROR) return err_code;
  }

  err_code = ixheaacd_modules_init(pstr_mps_state);

  return err_code;
}

VOID ixheaacd_wf_table_init(ia_heaac_mps_state_struct *pstr_mps_state) {
  WORD32 const **wf = pstr_mps_state->wf_tab.wf;
  ia_mps_dec_mps_tables_struct *mps_table_ptr = &pstr_mps_state->ia_mps_dec_mps_table;

  wf[0] = NULL;
  wf[1] = ixheaacd_mps_dec_wf_tables.wf_02;
  wf[2] = ixheaacd_mps_dec_wf_tables.wf_03;
  wf[3] = ixheaacd_mps_dec_wf_tables.wf_04;
  wf[4] = NULL;
  wf[5] = NULL;
  wf[6] = NULL;
  wf[7] = NULL;
  wf[8] = NULL;
  wf[9] = NULL;
  wf[10] = NULL;
  wf[11] = NULL;
  wf[12] = NULL;
  wf[13] = NULL;
  wf[14] = ixheaacd_mps_dec_wf_tables.wf_15;
  wf[15] = ixheaacd_mps_dec_wf_tables.wf_16;
  wf[16] = NULL;
  wf[17] = ixheaacd_mps_dec_wf_tables.wf_18;
  wf[18] = NULL;
  wf[19] = NULL;
  wf[20] = NULL;
  wf[21] = NULL;
  wf[22] = NULL;
  wf[23] = ixheaacd_mps_dec_wf_tables.wf_24;
  wf[24] = NULL;
  wf[25] = NULL;
  wf[26] = NULL;
  wf[27] = NULL;
  wf[28] = NULL;
  wf[29] = ixheaacd_mps_dec_wf_tables.wf_30;
  wf[30] = NULL;
  wf[31] = ixheaacd_mps_dec_wf_tables.wf_32;

  mps_table_ptr->wf_tab_ptr = &(pstr_mps_state->wf_tab);
}

VOID ixheaacd_res_huff_tables_init(ia_mps_dec_residual_aac_tables_struct *aac_tables_ptr) {
  aac_tables_ptr->code_book[0] = 0;
  aac_tables_ptr->code_book[1] = aac_tables_ptr->res_huffmann_tables_ptr->huffman_code_book_1;
  aac_tables_ptr->code_book[2] = aac_tables_ptr->res_huffmann_tables_ptr->huffman_code_book_2;
  aac_tables_ptr->code_book[3] = aac_tables_ptr->res_huffmann_tables_ptr->huffman_code_book_3;
  aac_tables_ptr->code_book[4] = aac_tables_ptr->res_huffmann_tables_ptr->huffman_code_book_4;
  aac_tables_ptr->code_book[5] = aac_tables_ptr->res_huffmann_tables_ptr->huffman_code_book_5;
  aac_tables_ptr->code_book[6] = aac_tables_ptr->res_huffmann_tables_ptr->huffman_code_book_6;
  aac_tables_ptr->code_book[7] = aac_tables_ptr->res_huffmann_tables_ptr->huffman_code_book_7;
  aac_tables_ptr->code_book[8] = aac_tables_ptr->res_huffmann_tables_ptr->huffman_code_book_8;
  aac_tables_ptr->code_book[9] = aac_tables_ptr->res_huffmann_tables_ptr->huffman_code_book_9;
  aac_tables_ptr->code_book[10] = aac_tables_ptr->res_huffmann_tables_ptr->huffman_code_book_10;
  aac_tables_ptr->code_book[11] = aac_tables_ptr->res_huffmann_tables_ptr->huffman_code_book_10;
  aac_tables_ptr->code_book[12] = aac_tables_ptr->res_huffmann_tables_ptr->huffman_code_book_10;

  aac_tables_ptr->scale_factor_bands_short[0] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_96_128;
  aac_tables_ptr->scale_factor_bands_short[1] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_96_128;
  aac_tables_ptr->scale_factor_bands_short[2] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_96_128;
  aac_tables_ptr->scale_factor_bands_short[3] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_48_128;
  aac_tables_ptr->scale_factor_bands_short[4] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_48_128;
  aac_tables_ptr->scale_factor_bands_short[5] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_48_128;
  aac_tables_ptr->scale_factor_bands_short[6] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_24_128;
  aac_tables_ptr->scale_factor_bands_short[7] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_24_128;
  aac_tables_ptr->scale_factor_bands_short[8] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_16_128;
  aac_tables_ptr->scale_factor_bands_short[9] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_16_128;
  aac_tables_ptr->scale_factor_bands_short[10] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_16_128;
  aac_tables_ptr->scale_factor_bands_short[11] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_8_128;
  aac_tables_ptr->scale_factor_bands_short[12] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_96_120;
  aac_tables_ptr->scale_factor_bands_short[13] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_96_120;
  aac_tables_ptr->scale_factor_bands_short[14] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_96_120;
  aac_tables_ptr->scale_factor_bands_short[15] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_48_120;
  aac_tables_ptr->scale_factor_bands_short[16] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_48_120;
  aac_tables_ptr->scale_factor_bands_short[17] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_48_120;
  aac_tables_ptr->scale_factor_bands_short[18] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_24_120;
  aac_tables_ptr->scale_factor_bands_short[19] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_24_120;
  aac_tables_ptr->scale_factor_bands_short[20] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_16_120;
  aac_tables_ptr->scale_factor_bands_short[21] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_16_120;
  aac_tables_ptr->scale_factor_bands_short[22] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_16_120;
  aac_tables_ptr->scale_factor_bands_short[23] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_8_120;

  aac_tables_ptr->scale_factor_bands_long[0] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_96_1024;
  aac_tables_ptr->scale_factor_bands_long[1] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_96_1024;
  aac_tables_ptr->scale_factor_bands_long[2] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_64_1024;
  aac_tables_ptr->scale_factor_bands_long[3] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_48_1024;
  aac_tables_ptr->scale_factor_bands_long[4] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_48_1024;
  aac_tables_ptr->scale_factor_bands_long[5] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_32_1024;
  aac_tables_ptr->scale_factor_bands_long[6] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_24_1024;
  aac_tables_ptr->scale_factor_bands_long[7] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_24_1024;
  aac_tables_ptr->scale_factor_bands_long[8] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_16_1024;
  aac_tables_ptr->scale_factor_bands_long[9] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_16_1024;
  aac_tables_ptr->scale_factor_bands_long[10] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_16_1024;
  aac_tables_ptr->scale_factor_bands_long[11] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_8_1024;
  aac_tables_ptr->scale_factor_bands_long[12] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_96_960;
  aac_tables_ptr->scale_factor_bands_long[13] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_96_960;
  aac_tables_ptr->scale_factor_bands_long[14] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_64_960;
  aac_tables_ptr->scale_factor_bands_long[15] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_48_960;
  aac_tables_ptr->scale_factor_bands_long[16] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_48_960;
  aac_tables_ptr->scale_factor_bands_long[17] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_48_960;
  aac_tables_ptr->scale_factor_bands_long[18] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_24_960;
  aac_tables_ptr->scale_factor_bands_long[19] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_24_960;
  aac_tables_ptr->scale_factor_bands_long[20] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_16_960;
  aac_tables_ptr->scale_factor_bands_long[21] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_16_960;
  aac_tables_ptr->scale_factor_bands_long[22] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_16_960;
  aac_tables_ptr->scale_factor_bands_long[23] =
      aac_tables_ptr->res_huffmann_tables_ptr->sfb_8_960;
}

static VOID ixheaacd_table_ptr_init(ia_heaac_mps_state_struct *pstr_mps_state) {
  ia_mps_dec_mps_tables_struct *mps_table_ptr = &pstr_mps_state->ia_mps_dec_mps_table;
  ia_mps_dec_residual_aac_tables_struct *aac_table_ptr = &pstr_mps_state->aac_table;

  mps_table_ptr->qmf_table_ptr = (ia_mps_dec_qmf_tables_struct *)&ixheaacd_mps_dec_qmf_tables;
  mps_table_ptr->common_table_ptr =
      (ia_mps_dec_common_tables_struct *)&ixheaacd_mps_dec_common_tables;
  mps_table_ptr->hybrid_table_ptr =
      (ia_mps_dec_hybrid_tables_struct *)&ixheaacd_mps_dec_hybrid_tables;
  mps_table_ptr->m1_m2_table_ptr =
      (ia_mps_dec_m1_m2_tables_struct *)&ixheaacd_mps_dec_m1_m2_tables;
  mps_table_ptr->decor_table_ptr =
      (ia_mps_dec_decorr_tables_struct *)&ixheaacd_mps_dec_decorr_tables;
  mps_table_ptr->tp_process_table_ptr =
      (ia_mps_dec_tp_process_tables_struct *)&ixheaacd_mps_dec_tp_process_tables;

  mps_table_ptr->mdct2qmf_table_ptr =
      (ia_mps_dec_mdct2qmf_table_struct *)&ixheaacd_mps_dec_mdct2qmf_table;

  mps_table_ptr->tonality_table_ptr =
      (ia_mps_dec_tonality_tables_struct *)&ixheaacd_mps_dec_tonality_tables;

  mps_table_ptr->bitdec_table_ptr =
      (ia_mps_dec_bitdec_tables_struct *)&ixheaacd_mps_dec_bitdec_tables;
  mps_table_ptr->blind_table_ptr =
      (ia_mps_dec_blind_tables_struct *)&ixheaacd_mps_dec_blind_tables;

  mps_table_ptr->mdct2qmfcos_table_ptr =
      (ia_mps_dec_mdct2qmf_tables_struct *)&ixheaacd_mps_dec_mdct2qmf_tables;
  mps_table_ptr->mdct2qmfcos_tab_ptr =
      (ia_mps_dec_mdct2qmf_cos_table_struct *)&pstr_mps_state->ia_mps_dec_mdct2qmfcos_table;
  aac_table_ptr->res_huffmann_tables_ptr =
      (ia_mps_dec_res_huffmann_tables_struct *)&ixheaacd_mps_dec_res_huffmann_tables;
  aac_table_ptr->res_block_tables_ptr =
      (ia_mps_dec_res_block_tables_struct *)&ixheaacd_mps_dec_res_block_tables;

  ixheaacd_res_huff_tables_init(aac_table_ptr);

  mps_table_ptr->aac_tab = aac_table_ptr;

  ixheaacd_wf_table_init(pstr_mps_state);
}

IA_ERRORCODE ixheaacd_aac_mps_init(ia_exhaacplus_dec_api_struct *p_obj_mps_dec, UWORD8 *databuf,
                                   WORD32 buffer_size, WORD32 sample_rate) {
  WORD32 i;

  VOID *buf_ptr;
  ia_mps_persistent_mem *persistent_mem =
      &p_obj_mps_dec->p_state_aac->heaac_mps_handle.mps_persistent_mem;

  IA_ERRORCODE err_code = IA_NO_ERROR;
  ia_heaac_mps_state_struct *curr_state = &p_obj_mps_dec->p_state_aac->heaac_mps_handle;
  ia_mps_spatial_bs_config_struct *p_bs_config = &curr_state->bs_config;
  ixheaacd_table_ptr_init(curr_state);

  curr_state->ptr_mps_bit_buff =
      ixheaacd_create_bit_buf(&curr_state->mps_bit_buf, (UWORD8 *)databuf, buffer_size);
  curr_state->ptr_mps_bit_buff->cnt_bits += (8 * buffer_size);

  curr_state->ptr_mps_bit_buff->xaac_jmp_buf = &p_obj_mps_dec->p_state_aac->xaac_jmp_buf;

  if (sample_rate < 27713) {
    curr_state->qmf_bands = 32;
  } else if (sample_rate >= 55426) {
    curr_state->qmf_bands = 128;
  } else {
    curr_state->qmf_bands = 64;
  }

  curr_state->sampling_freq = sample_rate;
  curr_state->num_input_channels = p_obj_mps_dec->p_state_aac->p_config->ui_n_channels;
  curr_state->bits_per_sample = p_obj_mps_dec->p_state_aac->p_config->ui_pcm_wdsz;
  curr_state->dec_type = p_bs_config->ui_dec_type;
  curr_state->up_mix_type = p_bs_config->ui_upmix_type;
  curr_state->binaural_quality = p_bs_config->ui_binaural_quality;
  curr_state->hrtf_model = p_bs_config->ui_hrtf_model;
  curr_state->is_buried_flag = p_bs_config->ui_bs_is_buried;

  curr_state->bs_frame = persistent_mem->p_bs_frame;

  buf_ptr = persistent_mem->syn_qmf_states_buffer;
  curr_state->syn = buf_ptr;

  curr_state->syn_qmf_bank.sbr_qmf_states_synthesis =
      (WORD32 *)((WORD8 *)buf_ptr + sizeof(ia_mps_dec_synthesis_interface));

  buf_ptr = persistent_mem->ana_qmf_states_buffer;

  memset(curr_state->m2_param_present, 0,
         MAX_M2_OUTPUT * MAX_M2_INPUT * sizeof(curr_state->m2_param_present[0][0]));
  memset(&(curr_state->bs_config), 0, sizeof(ia_mps_spatial_bs_config_struct));
  memset(&(curr_state->res_block_type), 0,
         sizeof(WORD32) * MAX_RESIDUAL_CHANNELS_MPS * MAX_RESIDUAL_FRAMES);

  curr_state->bs_config.ui_pcm_wdsz = 16;
  curr_state->bs_config.ui_samp_freq = 48000;
  curr_state->bs_config.ui_in_channels = 2;
  curr_state->bs_config.ui_qmf_bands = 64;

  err_code = ixheaacd_header_parse(curr_state);
  if (err_code != IA_NO_ERROR) return err_code;

  for (i = 0; i < curr_state->num_input_channels; i++) {
    curr_state->qmf_bank[i].qmf_states_buffer = buf_ptr;
    buf_ptr = (WORD8 *)buf_ptr + QMF_FILTER_STATE_ANA_SIZE_MPS * sizeof(WORD32);
  }

  curr_state->i_bytes_consumed_mps = (WORD32)(curr_state->ptr_mps_bit_buff->ptr_read_next -
                                              curr_state->ptr_mps_bit_buff->ptr_bit_buf_base);

  curr_state->bytes_remaining = buffer_size - curr_state->i_bytes_consumed_mps;

  if (curr_state->bytes_remaining != 0) {
    for (WORD32 ii = 0; ii < curr_state->bytes_remaining; ii++) {
      curr_state->temp_buf[ii] = databuf[ii + curr_state->i_bytes_consumed_mps];
    }
  }

  err_code = ixheaacd_aac_mps_create(curr_state);
  if (err_code != IA_NO_ERROR) return err_code;

  curr_state->mps_init_done = 1;

  return err_code;
}