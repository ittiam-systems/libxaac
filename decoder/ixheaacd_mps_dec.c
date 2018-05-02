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
#include <stdlib.h>
#include <stdio.h>

#include <ixheaacd_type_def.h>

#include "ixheaacd_bitbuffer.h"

#include "ixheaacd_defines.h"
#include "ixheaacd_memory_standards.h"
#include "ixheaacd_sbrdecsettings.h"
#include "ixheaacd_env_extr_part.h"
#include <ixheaacd_aac_rom.h>
#include "ixheaacd_common_rom.h"
#include <ixheaacd_sbr_rom.h>
#include "ixheaacd_pulsedata.h"
#include "ixheaacd_pns.h"

#include "ixheaacd_sbr_common.h"
#include "ixheaacd_drc_data_struct.h"
#include "ixheaacd_drc_dec.h"

#include "ixheaacd_lt_predict.h"
#include "ixheaacd_channelinfo.h"
#include "ixheaacd_channel.h"
#include "ixheaacd_channelinfo.h"
#include "ixheaacd_sbrdecoder.h"
#include "ixheaacd_audioobjtypes.h"
#include "ixheaacd_latmdemux.h"
#include "ixheaacd_aacdec.h"
#include "ixheaacd_sbr_common.h"

#include "ixheaacd_mps_polyphase.h"
#include "ixheaacd_config.h"
#include "ixheaacd_mps_dec.h"
#include "ixheaacd_mps_interface.h"
#include "ixheaacd_struct_def.h"

#include "ixheaacd_config.h"
#include "ixheaacd_mps_interface.h"

#include "ixheaacd_mps_polyphase.h"

#include "ixheaacd_mps_dec.h"
#include "ixheaacd_mps_process.h"
#include "ixheaacd_mps_decor.h"
#include "ixheaacd_mps_hybfilter.h"
#include "ixheaacd_mps_nlc_dec.h"
#include "ixheaacd_mps_huff_tab.h"

#include "math.h"

#include <assert.h>
#include <string.h>

extern ia_huff_pt0_nodes_struct ixheaacd_huff_part0_nodes;
extern ia_huff_ipd_nodes_struct ixheaacd_huff_ipd_nodes;
extern ia_huff_lav_nodes_struct ixheaacd_huff_lav_idx_nodes;
extern ia_huff_pt0_nodes_struct ixheaacd_huff_pilot_nodes;
extern ia_huff_cld_nodes_struct ixheaacd_huff_cld_nodes;
extern ia_huff_icc_nodes_struct ixheaacd_huff_icc_nodes;
extern ia_huff_res_nodes_struct ixheaacd_huff_reshape_nodes;

ia_mps_dec_state_struct* ixheaacd_mps_create(
    WORD32 bs_frame_len, WORD32 residual_coding,
    ia_usac_dec_mps_config_struct* mps212_config) {
  WORD32 num_ch;
  WORD32 err_code = 0;

  ia_mps_dec_state_struct* self = NULL;
  ia_mps_bs_frame bs_frame;

  self = (ia_mps_dec_state_struct*)calloc(1, sizeof(ia_mps_dec_state_struct));
  if (self == NULL) return 0;

  self->num_parameter_sets = 1;
  self->qmf_band_count = 64;

  if (mps212_config) {
    self->config = mps212_config;
    self->frame_length = bs_frame_len;
    self->in_ch_count = 1;
    self->out_ch_count = 2;
    self->residual_coding = residual_coding;
    if (self->residual_coding) {
      self->bs_residual_present = 1;
      self->bs_residual_bands = mps212_config->bs_residual_bands;
      if (self->config->bs_phase_coding) {
        self->config->bs_phase_coding = 2;
      }
    }
  }

  err_code = ixheaacd_mps_header_decode(self);

  if (err_code == -1) {
    free(self);
    return 0;
  }

  if ((self->residual_coding) && (self->res_bands > 0)) self->res_ch_count++;

  ixheaacd_mps_env_init(self);

  ixheaacd_mps_synt_create(&self->poly_phase_filt_kernel, self->qmf_band_count);

  for (num_ch = 0; num_ch < self->out_ch_count; num_ch++) {
    ixheaacd_mps_synt_init(&self->qmf_filt_state[num_ch]);
  }

  ixheaacd_mps_qmf_hybrid_analysis_init(&self->hyb_filt_state[0]);

  if ((self->residual_coding) && (self->res_bands > 0))
    ixheaacd_mps_qmf_hybrid_analysis_init(&self->hyb_filt_state[1]);

  ixheaacd_mps_decor_init(&(self->mps_decor), self->hyb_band_count,
                          self->config->bs_decorr_config);

  ixheaacd_mps_init_pre_and_post_matrix(self);

  self->parse_nxt_frame = 1;

  bs_frame = self->bs_frame;
  memset(bs_frame.cld_idx_pre, 0, MAX_PARAMETER_BANDS * sizeof(WORD32));
  memset(bs_frame.icc_idx_pre, 0, MAX_PARAMETER_BANDS * sizeof(WORD32));
  memset(bs_frame.cmp_cld_idx_prev, 0, MAX_PARAMETER_BANDS * sizeof(WORD32));
  memset(bs_frame.cmp_icc_idx_prev, 0, MAX_PARAMETER_BANDS * sizeof(WORD32));

  self->subband_var.init_flag = 0;
  self->subband_var.update_old_ener = 0;
  self->subband_var.nrg_dir = 0;
  memset(self->subband_var.nrg_diff, 0, 2 * sizeof(FLOAT32));

  memset(self->opd_smooth.smooth_l_phase, 0,
         MAX_PARAMETER_BANDS * sizeof(WORD32));
  memset(self->opd_smooth.smooth_r_phase, 0,
         MAX_PARAMETER_BANDS * sizeof(WORD32));

  return self;
}

static FLOAT32 ixheaacd_tsd_mul_re[] = {
    1.0f,  0.707106781186548f,  0.0f, -0.707106781186548f,
    -1.0f, -0.707106781186548f, 0.0f, 0.707106781186548f};

static FLOAT32 ixheaacd_tsd_mul_im[] = {
    0.0f, 0.707106781186548f,  1.0f,  0.707106781186548f,
    0.0f, -0.707106781186548f, -1.0f, -0.707106781186548f};

VOID ixheaacd_mps_qmf_hyb_analysis(ia_mps_dec_state_struct* self) {
  ixheaacd_mps_qmf_hybrid_analysis(&self->hyb_filt_state[0], self->qmf_in[0],
                                   self->qmf_band_count, self->time_slots,
                                   self->hyb_in[0]);

  if ((self->residual_coding) && (self->res_bands > 0)) {
    ixheaacd_mps_qmf_hybrid_analysis(&self->hyb_filt_state[self->in_ch_count],
                                     self->qmf_in[1], self->qmf_band_count,
                                     self->time_slots, self->hyb_res);
  }
}

VOID ixheaacd_mps_qmf_hyb_synthesis(ia_mps_dec_state_struct* self) {
  WORD32 ch;

  for (ch = 0; ch < self->out_ch_count; ch++) {
    ixheaacd_mps_qmf_hybrid_synthesis(self->hyb_dir_out[ch],
                                      self->qmf_band_count, self->time_slots,
                                      self->qmf_out_dir[ch]);
  }
}

VOID ixheaacd_mps_decor(ia_mps_dec_state_struct* self) {
  WORD32 k, sb_sample, idx;

  ia_cmplx_flt_struct(*scratch)[MAX_HYBRID_BANDS_MPS];

  ia_cmplx_flt_struct coeff;
  WORD32 band_start = 7;

  scratch = self->scratch;

  for (k = self->dir_sig_count; k < self->dir_sig_count + self->decor_sig_count;
       k++) {
    if (self->bs_tsd_enable) {
      for (sb_sample = 0; sb_sample < self->time_slots; sb_sample++) {
        if (self->bs_tsd_sep_data[sb_sample]) {
          for (idx = band_start; idx < self->mps_decor.num_bins; idx++) {
            scratch[sb_sample][idx].re = self->v[k][sb_sample][idx].re;
            scratch[sb_sample][idx].im = self->v[k][sb_sample][idx].im;
            self->v[k][sb_sample][idx].re = 0.0f;
            self->v[k][sb_sample][idx].im = 0.0f;
          }
        }
      }
    }

    ixheaacd_mps_decor_apply(&self->mps_decor, self->v[k], self->w_diff[k],
                             self->time_slots);

    if (self->bs_tsd_enable) {
      for (sb_sample = 0; sb_sample < self->time_slots; sb_sample++) {
        if (self->bs_tsd_sep_data[sb_sample]) {
          coeff.re = ixheaacd_tsd_mul_re[self->bs_tsd_tr_phase_data[sb_sample]];
          coeff.im = ixheaacd_tsd_mul_im[self->bs_tsd_tr_phase_data[sb_sample]];

          for (idx = band_start; idx < self->mps_decor.num_bins; idx++) {
            self->w_diff[k][sb_sample][idx].re +=
                coeff.re * scratch[sb_sample][idx].re -
                coeff.im * scratch[sb_sample][idx].im;
            self->w_diff[k][sb_sample][idx].im +=
                coeff.im * scratch[sb_sample][idx].re +
                coeff.re * scratch[sb_sample][idx].im;
          }
        }
      }
    }
  }
}

VOID ixheaacd_mps_mix_res_decor(ia_mps_dec_state_struct* self) {
  WORD32 ts, qs, row, indx;

  for (ts = 0; ts < self->time_slots; ts++) {
    for (qs = 0; qs < self->hyb_band_count; qs++) {
      indx = self->hyb_band_to_processing_band_table[qs];

      for (row = 0; row < self->dir_sig_count; row++) {
        self->w_dir[row][ts][qs].re = self->v[row][ts][qs].re;
        self->w_dir[row][ts][qs].im = self->v[row][ts][qs].im;
      }

      for (row = self->dir_sig_count;
           row < (self->dir_sig_count + self->decor_sig_count); row++) {
        if (indx < self->res_bands) {
          self->w_dir[row][ts][qs].re = self->hyb_res[ts][qs].re;
          self->w_dir[row][ts][qs].im = self->hyb_res[ts][qs].im;
        } else {
          self->w_dir[row][ts][qs].re = 0.0f;
          self->w_dir[row][ts][qs].im = 0.0f;
        }
      }

      for (row = 0; row < self->dir_sig_count; row++) {
        self->w_diff[row][ts][qs].re = 0.0f;
        self->w_diff[row][ts][qs].im = 0.0f;
      }

      for (row = self->dir_sig_count;
           row < (self->dir_sig_count + self->decor_sig_count); row++) {
        if (indx < self->res_bands) {
          self->w_diff[row][ts][qs].re = 0.0f;
          self->w_diff[row][ts][qs].im = 0.0f;
        }
      }
    }
  }
}

VOID ixheaacd_mps_create_w(ia_mps_dec_state_struct* self) {
  ixheaacd_mps_decor(self);
  ixheaacd_mps_mix_res_decor(self);
}
VOID ixheaacd_mps_apply(ia_mps_dec_state_struct* self,
                        FLOAT32** input_buffer[4],
                        FLOAT32 (*output_buffer)[4096]) {
  WORD32 ch, ts, qs;
  WORD32 time_slots = self->time_slots;
  WORD32 in_ch_count = self->in_ch_count + self->res_ch_count;

  self->output_buffer = output_buffer;

  assert(self->present_time_slot + time_slots <= self->time_slots);

  for (ts = 0; ts < time_slots; ts++) {
    for (ch = 0; ch < in_ch_count; ch++) {
      for (qs = 0; qs < self->qmf_band_count; qs++) {
        self->qmf_in[ch][self->present_time_slot + ts][qs].re =
            self->input_gain * input_buffer[2 * ch][ts][qs];
        self->qmf_in[ch][self->present_time_slot + ts][qs].im =
            self->input_gain * input_buffer[2 * ch + 1][ts][qs];
      }
    }
  }

  self->present_time_slot += time_slots;

  if (self->present_time_slot < self->time_slots) return;

  self->present_time_slot = 0;

  ixheaacd_mps_frame_decode(self);

  ixheaacd_mps_qmf_hyb_analysis(self);

  ixheaacd_pre_and_mix_matrix_calculation(self);

  ixheaacd_mps_pre_matrix_mix_matrix_smoothing(self);

  ixheaacd_mps_apply_pre_matrix(self);

  ixheaacd_mps_create_w(self);

  ixheaacd_mps_apply_mix_matrix(self);

  if (self->config->bs_temp_shape_config == 2) {
    ixheaacd_mps_time_env_shaping(self);
  }

  ixheaacd_mps_temp_process(self);

  self->parse_nxt_frame = 1;
}

#define min(a, b) (((a) < (b)) ? (a) : (b))

static WORD32 ixheaacd_mps_pcm_decode(ia_handle_bit_buf_struct it_bit_buff,
                                      WORD32* out_data_1, WORD32* out_data_2,
                                      WORD32 ixheaacd_drc_offset,
                                      WORD32 num_val, WORD32 num_levels) {
  WORD32 i = 0, j = 0, idx = 0;
  WORD32 max_grp_len = 0, grp_len = 0, next_val = 0, grp_val = 0;
  UWORD32 data = 0;

  FLOAT32 ld_nlev = 0.f;

  WORD32 pcm_chunk_size[7] = {0};

  switch (num_levels) {
    case 3:
      max_grp_len = 5;
      break;
    case 7:
      max_grp_len = 6;
      break;
    case 11:
      max_grp_len = 2;
      break;
    case 13:
      max_grp_len = 4;
      break;
    case 19:
      max_grp_len = 4;
      break;
    case 25:
      max_grp_len = 3;
      break;
    case 51:
      max_grp_len = 4;
      break;
    case 4:
    case 8:
    case 15:
    case 16:
    case 26:
    case 31:
      max_grp_len = 1;
      break;
    default:
      assert(0);
  }

  ld_nlev = (FLOAT32)(log((FLOAT32)num_levels) / log(2.f));

  for (i = 1; i <= max_grp_len; i++) {
    pcm_chunk_size[i] = (WORD32)ceil((FLOAT32)(i)*ld_nlev);
  }

  for (i = 0; i < num_val; i += max_grp_len) {
    grp_len = min(max_grp_len, num_val - i);
    data = ixheaacd_read_bits_buf(it_bit_buff, pcm_chunk_size[grp_len]);

    grp_val = data;

    for (j = 0; j < grp_len; j++) {
      idx = i + (grp_len - j - 1);
      next_val = grp_val % num_levels;

      if (out_data_2 == NULL) {
        out_data_1[idx] = next_val - ixheaacd_drc_offset;
      } else if (out_data_1 == NULL) {
        out_data_2[idx] = next_val - ixheaacd_drc_offset;
      } else {
        if (idx % 2) {
          out_data_2[idx / 2] = next_val - ixheaacd_drc_offset;
        } else {
          out_data_1[idx / 2] = next_val - ixheaacd_drc_offset;
        }
      }

      grp_val = (grp_val - next_val) / num_levels;
    }
  }

  return 1;
}

static WORD32 ixheaacd_mps_huff_read(ia_handle_bit_buf_struct it_bit_buff,
                                     const WORD32 (*node_tab)[][2],
                                     WORD32* out_data) {
  WORD32 node = 0;
  UWORD32 next_bit = 0;

  do {
    next_bit = ixheaacd_read_bits_buf(it_bit_buff, 1);
    node = (*node_tab)[node][next_bit];
  } while (node > 0);

  *out_data = node;

  return 1;
}

static WORD32 ixheaacd_mps_huff_read_2d(ia_handle_bit_buf_struct it_bit_buff,
                                        const WORD32 (*node_tab)[][2],
                                        WORD32 out_data[2], WORD32* escape)

{
  WORD32 huff_2d_8bit = 0;
  WORD32 node = 0;

  if (!ixheaacd_mps_huff_read(it_bit_buff, node_tab, &node)) return 0;
  *escape = (node == 0);

  if (*escape) {
    out_data[0] = 0;
    out_data[1] = 1;
  } else {
    huff_2d_8bit = -(node + 1);
    out_data[0] = huff_2d_8bit >> 4;
    out_data[1] = huff_2d_8bit & 0xf;
  }

  return 1;
}

static WORD32 ixheaacd_mps_sym_restore(ia_handle_bit_buf_struct it_bit_buff,
                                       WORD32 lav, WORD32 data[2]) {
  WORD32 tmp = 0;
  UWORD32 sym_bit = 0;

  WORD32 sum_val = data[0] + data[1];
  WORD32 diff_val = data[0] - data[1];

  if (sum_val > lav) {
    data[0] = -sum_val + (2 * lav + 1);
    data[1] = -diff_val;
  } else {
    data[0] = sum_val;
    data[1] = diff_val;
  }

  if (data[0] + data[1] != 0) {
    sym_bit = ixheaacd_read_bits_buf(it_bit_buff, 1);
    if (sym_bit) {
      data[0] = -data[0];
      data[1] = -data[1];
    }
  }

  if (data[0] - data[1] != 0) {
    sym_bit = ixheaacd_read_bits_buf(it_bit_buff, 1);
    if (sym_bit) {
      tmp = data[0];
      data[0] = data[1];
      data[1] = tmp;
    }
  }

  return 1;
}

static WORD32 ixheaacd_mps_sym_restoreipd(ia_handle_bit_buf_struct it_bit_buff,
                                          WORD32 lav, WORD32 data[2]) {
  WORD32 tmp = 0;
  UWORD32 sym_bit = 0;

  WORD32 sum_val = data[0] + data[1];
  WORD32 diff_val = data[0] - data[1];

  if (sum_val > lav) {
    data[0] = -sum_val + (2 * lav + 1);
    data[1] = -diff_val;
  } else {
    data[0] = sum_val;
    data[1] = diff_val;
  }

  if (data[0] - data[1] != 0) {
    sym_bit = ixheaacd_read_bits_buf(it_bit_buff, 1);
    if (sym_bit) {
      tmp = data[0];
      data[0] = data[1];
      data[1] = tmp;
    }
  }

  return 1;
}

static WORD32 ixheaacd_mps_huff_dec_pilot(ia_handle_bit_buf_struct it_bit_buff,
                                          const WORD32 (*node_tab)[][2],
                                          WORD32* pilot_data) {
  WORD32 node = 0;

  if (!ixheaacd_mps_huff_read(it_bit_buff, node_tab, &node)) return 0;
  *pilot_data = -(node + 1);

  return 1;
}

static WORD32 ixheaacd_mps_huff_dec_cld_1d(
    ia_handle_bit_buf_struct it_bit_buff,
    const ia_huff_cld_node_1d_struct* huff_nodes, WORD32* out_data,
    WORD32 num_val, WORD32 p0_flag) {
  WORD32 i = 0, node = 0, ixheaacd_drc_offset = 0;
  WORD32 od = 0, od_sign = 0;
  UWORD32 data = 0;

  if (p0_flag) {
    if (!ixheaacd_mps_huff_read(
            it_bit_buff, (ia_huff_node_struct)&ixheaacd_huff_part0_nodes.cld,
            &node))
      return 0;
    out_data[0] = -(node + 1);
    ixheaacd_drc_offset = 1;
  }

  for (i = ixheaacd_drc_offset; i < num_val; i++) {
    if (!ixheaacd_mps_huff_read(
            it_bit_buff, (ia_huff_node_struct)&huff_nodes->node_tab, &node))
      return 0;
    od = -(node + 1);

    if (od != 0) {
      data = ixheaacd_read_bits_buf(it_bit_buff, 1);
      od_sign = data;

      if (od_sign) od = -od;
    }

    out_data[i] = od;
  }

  return 1;
}

static WORD32 ixheaacd_mps_huff_dec_ipd_1d(
    ia_handle_bit_buf_struct it_bit_buff,
    const ia_huff_ipd_node_1d_struct* huff_nodes, WORD32* out_data,
    WORD32 num_val, WORD32 p0_flag) {
  WORD32 i = 0, node = 0, ixheaacd_drc_offset = 0;
  WORD32 od = 0;

  if (p0_flag) {
    if (!ixheaacd_mps_huff_read(
            it_bit_buff,
            (ia_huff_node_struct)&ixheaacd_huff_ipd_nodes.hp0.node_tab, &node))
      return 0;
    out_data[0] = -(node + 1);
    ixheaacd_drc_offset = 1;
  }

  for (i = ixheaacd_drc_offset; i < num_val; i++) {
    if (!ixheaacd_mps_huff_read(
            it_bit_buff, (ia_huff_node_struct)&huff_nodes->node_tab, &node))
      return 0;
    od = -(node + 1);
    out_data[i] = od;
  }

  return 1;
}

static WORD32 ixheaacd_mps_huff_dec_icc_1d(
    ia_handle_bit_buf_struct it_bit_buff,
    const ia_huff_icc_node_1d_struct* huff_nodes, WORD32* out_data,
    WORD32 num_val, WORD32 p0_flag) {
  WORD32 i = 0, node = 0, ixheaacd_drc_offset = 0;
  WORD32 od = 0, od_sign = 0;
  UWORD32 data = 0;

  if (p0_flag) {
    if (!ixheaacd_mps_huff_read(
            it_bit_buff, (ia_huff_node_struct)&ixheaacd_huff_part0_nodes.icc,
            &node))
      return 0;
    out_data[0] = -(node + 1);
    ixheaacd_drc_offset = 1;
  }

  for (i = ixheaacd_drc_offset; i < num_val; i++) {
    if (!ixheaacd_mps_huff_read(
            it_bit_buff, (ia_huff_node_struct)&huff_nodes->node_tab, &node))
      return 0;
    od = -(node + 1);

    if (od != 0) {
      data = ixheaacd_read_bits_buf(it_bit_buff, 1);
      od_sign = data;

      if (od_sign) od = -od;
    }

    out_data[i] = od;
  }

  return 1;
}

static WORD32 ixheaacd_mps_huff_dec_cld_2d(
    ia_handle_bit_buf_struct it_bit_buff,
    const ia_huff_cld_node_2d_struct* huff_nodes, WORD32 out_data[][2],
    WORD32 num_val, WORD32 ch_fac, WORD32* p0_data[2]) {
  WORD32 i = 0, lav = 0, escape = 0, esc_contrl = 0;
  WORD32 node = 0;
  UWORD32 data = 0;

  WORD32 esc_data[MAXBANDS][2] = {{0}};
  WORD32 esc_idx[MAXBANDS] = {0};

  if (!ixheaacd_mps_huff_read(
          it_bit_buff,
          (ia_huff_node_struct)&ixheaacd_huff_lav_idx_nodes.node_tab, &node))
    return 0;
  data = -(node + 1);

  lav = 2 * data + 3;

  if (p0_data[0] != NULL) {
    if (!ixheaacd_mps_huff_read(
            it_bit_buff, (ia_huff_node_struct)&ixheaacd_huff_part0_nodes.cld,
            &node))
      return 0;
    *p0_data[0] = -(node + 1);
  }
  if (p0_data[1] != NULL) {
    if (!ixheaacd_mps_huff_read(
            it_bit_buff, (ia_huff_node_struct)&ixheaacd_huff_part0_nodes.cld,
            &node))
      return 0;
    *p0_data[1] = -(node + 1);
  }

  for (i = 0; i < num_val; i += ch_fac) {
    switch (lav) {
      case 3:
        if (!ixheaacd_mps_huff_read_2d(it_bit_buff,
                                       (ia_huff_node_struct)&huff_nodes->lav3,
                                       out_data[i], &escape))
          return 0;
        break;
      case 5:
        if (!ixheaacd_mps_huff_read_2d(it_bit_buff,
                                       (ia_huff_node_struct)&huff_nodes->lav5,
                                       out_data[i], &escape))
          return 0;
        break;
      case 7:
        if (!ixheaacd_mps_huff_read_2d(it_bit_buff,
                                       (ia_huff_node_struct)&huff_nodes->lav7,
                                       out_data[i], &escape))
          return 0;
        break;
      case 9:
        if (!ixheaacd_mps_huff_read_2d(it_bit_buff,
                                       (ia_huff_node_struct)&huff_nodes->lav9,
                                       out_data[i], &escape))
          return 0;
        break;
      default:
        break;
    }

    if (escape) {
      esc_idx[esc_contrl++] = i;
    } else {
      if (!ixheaacd_mps_sym_restore(it_bit_buff, lav, out_data[i])) return 0;
    }
  }

  if (esc_contrl > 0) {
    if (!ixheaacd_mps_pcm_decode(it_bit_buff, esc_data[0], esc_data[1], 0,
                                 2 * esc_contrl, (2 * lav + 1)))
      return 0;

    for (i = 0; i < esc_contrl; i++) {
      out_data[esc_idx[i]][0] = esc_data[0][i] - lav;
      out_data[esc_idx[i]][1] = esc_data[1][i] - lav;
    }
  }

  return 1;
}

static WORD32 ixheaacd_mps_huff_dec_icc_2d(
    ia_handle_bit_buf_struct it_bit_buff,
    const ia_huff_icc_node_2d_struct* huff_nodes, WORD32 out_data[][2],
    WORD32 num_val, WORD32 ch_fac, WORD32* p0_data[2]) {
  WORD32 i = 0, lav = 0, escape = 0, esc_contrl = 0;
  WORD32 node = 0;
  UWORD32 data = 0;

  WORD32 esc_data[2][MAXBANDS] = {{0}};
  WORD32 esc_idx[MAXBANDS] = {0};

  if (!ixheaacd_mps_huff_read(
          it_bit_buff,
          (ia_huff_node_struct)&ixheaacd_huff_lav_idx_nodes.node_tab, &node))
    return 0;
  data = -(node + 1);

  lav = 2 * data + 1;

  if (p0_data[0] != NULL) {
    if (!ixheaacd_mps_huff_read(
            it_bit_buff, (ia_huff_node_struct)&ixheaacd_huff_part0_nodes.icc,
            &node))
      return 0;
    *p0_data[0] = -(node + 1);
  }
  if (p0_data[1] != NULL) {
    if (!ixheaacd_mps_huff_read(
            it_bit_buff, (ia_huff_node_struct)&ixheaacd_huff_part0_nodes.icc,
            &node))
      return 0;
    *p0_data[1] = -(node + 1);
  }

  for (i = 0; i < num_val; i += ch_fac) {
    switch (lav) {
      case 1:
        if (!ixheaacd_mps_huff_read_2d(it_bit_buff,
                                       (ia_huff_node_struct)&huff_nodes->lav1,
                                       out_data[i], &escape))
          return 0;
        break;
      case 3:
        if (!ixheaacd_mps_huff_read_2d(it_bit_buff,
                                       (ia_huff_node_struct)&huff_nodes->lav3,
                                       out_data[i], &escape))
          return 0;
        break;
      case 5:
        if (!ixheaacd_mps_huff_read_2d(it_bit_buff,
                                       (ia_huff_node_struct)&huff_nodes->lav5,
                                       out_data[i], &escape))
          return 0;
        break;
      case 7:
        if (!ixheaacd_mps_huff_read_2d(it_bit_buff,
                                       (ia_huff_node_struct)&huff_nodes->lav7,
                                       out_data[i], &escape))
          return 0;
        break;
    }

    if (escape) {
      esc_idx[esc_contrl++] = i;
    } else {
      if (!ixheaacd_mps_sym_restore(it_bit_buff, lav, out_data[i])) return 0;
    }
  }

  if (esc_contrl > 0) {
    if (!ixheaacd_mps_pcm_decode(it_bit_buff, esc_data[0], esc_data[1], 0,
                                 2 * esc_contrl, (2 * lav + 1)))
      return 0;

    for (i = 0; i < esc_contrl; i++) {
      out_data[esc_idx[i]][0] = esc_data[0][i] - lav;
      out_data[esc_idx[i]][1] = esc_data[1][i] - lav;
    }
  }

  return 1;
}

static WORD32 ixheaacd_mps_huff_dec_ipd_2d(
    ia_handle_bit_buf_struct it_bit_buff,
    const ia_huff_ipd_node_2d_struct* huff_nodes, WORD32 out_data[][2],
    WORD32 num_val, WORD32 ch_fac, WORD32* p0_data[2]) {
  WORD32 i = 0, lav = 0, escape = 0, esc_contrl = 0;
  WORD32 node = 0;
  UWORD32 data = 0;

  WORD32 esc_data[2][MAXBANDS] = {{0}};
  WORD32 esc_idx[MAXBANDS] = {0};

  if (!ixheaacd_mps_huff_read(
          it_bit_buff,
          (ia_huff_node_struct)&ixheaacd_huff_lav_idx_nodes.node_tab, &node))
    return 0;

  data = -(node + 1);
  if (data == 0)
    data = 3;
  else
    data--;

  lav = 2 * data + 1;

  if (p0_data[0] != NULL) {
    if (!ixheaacd_mps_huff_read(
            it_bit_buff,
            (ia_huff_node_struct)&ixheaacd_huff_ipd_nodes.hp0.node_tab, &node))
      return 0;
    *p0_data[0] = -(node + 1);
  }
  if (p0_data[1] != NULL) {
    if (!ixheaacd_mps_huff_read(
            it_bit_buff,
            (ia_huff_node_struct)&ixheaacd_huff_ipd_nodes.hp0.node_tab, &node))
      return 0;
    *p0_data[1] = -(node + 1);
  }

  for (i = 0; i < num_val; i += ch_fac) {
    switch (lav) {
      case 1:
        if (!ixheaacd_mps_huff_read_2d(it_bit_buff,
                                       (ia_huff_node_struct)&huff_nodes->lav1,
                                       out_data[i], &escape))
          return 0;
        break;
      case 3:
        if (!ixheaacd_mps_huff_read_2d(it_bit_buff,
                                       (ia_huff_node_struct)&huff_nodes->lav3,
                                       out_data[i], &escape))
          return 0;
        break;
      case 5:
        if (!ixheaacd_mps_huff_read_2d(it_bit_buff,
                                       (ia_huff_node_struct)&huff_nodes->lav5,
                                       out_data[i], &escape))
          return 0;
        break;
      case 7:
        if (!ixheaacd_mps_huff_read_2d(it_bit_buff,
                                       (ia_huff_node_struct)&huff_nodes->lav7,
                                       out_data[i], &escape))
          return 0;
        break;
    }

    if (escape) {
      esc_idx[esc_contrl++] = i;
    } else {
      if (!ixheaacd_mps_sym_restoreipd(it_bit_buff, lav, out_data[i])) return 0;
    }
  }

  if (esc_contrl > 0) {
    if (!ixheaacd_mps_pcm_decode(it_bit_buff, esc_data[0], esc_data[1], 0,
                                 2 * esc_contrl, (2 * lav + 1)))
      return 0;

    for (i = 0; i < esc_contrl; i++) {
      out_data[esc_idx[i]][0] = esc_data[0][i] - lav;
      out_data[esc_idx[i]][1] = esc_data[1][i] - lav;
    }
  }

  return 1;
}

static WORD32 ixheaacd_huff_decode(ia_handle_bit_buf_struct it_bit_buff,
                                   WORD32* out_data_1, WORD32* out_data_2,
                                   WORD32 data_type, WORD32 diff_type_1,
                                   WORD32 diff_type_2, WORD32 pilot_coding_flag,
                                   WORD32* pilot_data, WORD32 num_val,
                                   WORD32* cdg_scheme) {
  WORD32 diff_type;

  WORD32 i = 0;
  UWORD32 data = 0;

  WORD32 pair_vec[MAXBANDS][2];

  WORD32* p0_data_1[2] = {NULL, NULL};
  WORD32* p0_data_2[2] = {NULL, NULL};

  WORD32 p0_flag[2];

  WORD32 num_val_1_int = num_val;
  WORD32 num_val_2_int = num_val;

  WORD32* out_data_1_int = out_data_1;
  WORD32* out_data_2_int = out_data_2;

  WORD32 df_rest_flag_1 = 0;
  WORD32 df_rest_flag_2 = 0;

  WORD32 huff_yy_1;
  WORD32 huff_yy_2;
  WORD32 huff_yy;

  if (pilot_coding_flag) {
    switch (data_type) {
      case CLD:
        if (out_data_1 != NULL) {
          if (!ixheaacd_mps_huff_dec_pilot(
                  it_bit_buff,
                  (ia_huff_node_struct)&ixheaacd_huff_pilot_nodes.cld,
                  pilot_data))
            return 0;
        }
        break;

      case ICC:
        if (out_data_1 != NULL) {
          if (!ixheaacd_mps_huff_dec_pilot(
                  it_bit_buff,
                  (ia_huff_node_struct)&ixheaacd_huff_pilot_nodes.icc,
                  pilot_data))
            return 0;
        }
        break;

      default:
        if (out_data_1 != NULL) {
          return 0;
        }
        break;
    }
  }

  data = ixheaacd_read_bits_buf(it_bit_buff, 1);
  *cdg_scheme = data << PAIR_SHIFT;

  if (*cdg_scheme >> PAIR_SHIFT == HUFF_2D) {
    if ((out_data_1 != NULL) && (out_data_2 != NULL)) {
      data = ixheaacd_read_bits_buf(it_bit_buff, 1);
      *cdg_scheme |= data;
    } else {
      *cdg_scheme |= FREQ_PAIR;
    }
  }

  if (pilot_coding_flag) {
    huff_yy_1 = PCM_PLT;
    huff_yy_2 = PCM_PLT;
  } else {
    huff_yy_1 = diff_type_1;
    huff_yy_2 = diff_type_2;
  }

  switch (*cdg_scheme >> PAIR_SHIFT) {
    case HUFF_1D:

      p0_flag[0] = (diff_type_1 == DIFF_FREQ) && !pilot_coding_flag;
      p0_flag[1] = (diff_type_2 == DIFF_FREQ) && !pilot_coding_flag;

      switch (data_type) {
        case CLD:
          if (out_data_1 != NULL) {
            if (!ixheaacd_mps_huff_dec_cld_1d(
                    it_bit_buff, &ixheaacd_huff_cld_nodes.h_1_dim[huff_yy_1],
                    out_data_1, num_val_1_int, p0_flag[0]))
              return 0;
          }
          if (out_data_2 != NULL) {
            if (!ixheaacd_mps_huff_dec_cld_1d(
                    it_bit_buff, &ixheaacd_huff_cld_nodes.h_1_dim[huff_yy_2],
                    out_data_2, num_val_2_int, p0_flag[1]))
              return 0;
          }

          break;

        case ICC:
          if (out_data_1 != NULL) {
            if (!ixheaacd_mps_huff_dec_icc_1d(
                    it_bit_buff, &ixheaacd_huff_icc_nodes.h_1_dim[huff_yy_1],
                    out_data_1, num_val_1_int, p0_flag[0]))
              return 0;
          }
          if (out_data_2 != NULL) {
            if (!ixheaacd_mps_huff_dec_icc_1d(
                    it_bit_buff, &ixheaacd_huff_icc_nodes.h_1_dim[huff_yy_2],
                    out_data_2, num_val_2_int, p0_flag[1]))
              return 0;
          }

          break;

        case IPD:
          if (out_data_1 != NULL) {
            if (!ixheaacd_mps_huff_dec_ipd_1d(
                    it_bit_buff, &ixheaacd_huff_ipd_nodes.h_1_dim[huff_yy_1],
                    out_data_1, num_val_1_int, p0_flag[0]))
              return 0;
          }
          if (out_data_2 != NULL) {
            if (!ixheaacd_mps_huff_dec_ipd_1d(
                    it_bit_buff, &ixheaacd_huff_ipd_nodes.h_1_dim[huff_yy_2],
                    out_data_2, num_val_2_int, p0_flag[1]))
              return 0;
          }

          break;

        default:
          break;
      }

      break;

    case HUFF_2D:

      switch (*cdg_scheme & PAIR_MASK) {
        case FREQ_PAIR:

          if (out_data_1 != NULL) {
            if (!pilot_coding_flag && diff_type_1 == DIFF_FREQ) {
              p0_data_1[0] = &out_data_1[0];
              p0_data_1[1] = NULL;

              num_val_1_int -= 1;
              out_data_1_int += 1;
            }
            df_rest_flag_1 = num_val_1_int % 2;
            if (df_rest_flag_1) num_val_1_int -= 1;
          }
          if (out_data_2 != NULL) {
            if (!pilot_coding_flag && diff_type_2 == DIFF_FREQ) {
              p0_data_2[0] = NULL;
              p0_data_2[1] = &out_data_2[0];

              num_val_2_int -= 1;
              out_data_2_int += 1;
            }
            df_rest_flag_2 = num_val_2_int % 2;
            if (df_rest_flag_2) num_val_2_int -= 1;
          }

          switch (data_type) {
            case CLD:

              if (out_data_1 != NULL) {
                if (!ixheaacd_mps_huff_dec_cld_2d(
                        it_bit_buff,
                        &ixheaacd_huff_cld_nodes.h_2_dim[huff_yy_1][FREQ_PAIR],
                        pair_vec, num_val_1_int, 2, p0_data_1))
                  return 0;
                if (df_rest_flag_1) {
                  if (!ixheaacd_mps_huff_dec_cld_1d(
                          it_bit_buff,
                          &ixheaacd_huff_cld_nodes.h_1_dim[huff_yy_1],
                          out_data_1_int + num_val_1_int, 1, 0))
                    return 0;
                }
              }
              if (out_data_2 != NULL) {
                if (!ixheaacd_mps_huff_dec_cld_2d(
                        it_bit_buff,
                        &ixheaacd_huff_cld_nodes.h_2_dim[huff_yy_2][FREQ_PAIR],
                        pair_vec + 1, num_val_2_int, 2, p0_data_2))
                  return 0;
                if (df_rest_flag_2) {
                  if (!ixheaacd_mps_huff_dec_cld_1d(
                          it_bit_buff,
                          &ixheaacd_huff_cld_nodes.h_1_dim[huff_yy_2],
                          out_data_2_int + num_val_2_int, 1, 0))
                    return 0;
                }
              }
              break;

            case ICC:
              if (out_data_1 != NULL) {
                if (!ixheaacd_mps_huff_dec_icc_2d(
                        it_bit_buff,
                        &ixheaacd_huff_icc_nodes.h_2_dim[huff_yy_1][FREQ_PAIR],
                        pair_vec, num_val_1_int, 2, p0_data_1))
                  return 0;
                if (df_rest_flag_1) {
                  if (!ixheaacd_mps_huff_dec_icc_1d(
                          it_bit_buff,
                          &ixheaacd_huff_icc_nodes.h_1_dim[huff_yy_1],
                          out_data_1_int + num_val_1_int, 1, 0))
                    return 0;
                }
              }
              if (out_data_2 != NULL) {
                if (!ixheaacd_mps_huff_dec_icc_2d(
                        it_bit_buff,
                        &ixheaacd_huff_icc_nodes.h_2_dim[huff_yy_2][FREQ_PAIR],
                        pair_vec + 1, num_val_2_int, 2, p0_data_2))
                  return 0;
                if (df_rest_flag_2) {
                  if (!ixheaacd_mps_huff_dec_icc_1d(
                          it_bit_buff,
                          &ixheaacd_huff_icc_nodes.h_1_dim[huff_yy_2],
                          out_data_2_int + num_val_2_int, 1, 0))
                    return 0;
                }
              }
              break;

            case IPD:
              if (out_data_1 != NULL) {
                if (!ixheaacd_mps_huff_dec_ipd_2d(
                        it_bit_buff,
                        &ixheaacd_huff_ipd_nodes.h_2_dim[huff_yy_1][FREQ_PAIR],
                        pair_vec, num_val_1_int, 2, p0_data_1))
                  return 0;
                if (df_rest_flag_1) {
                  if (!ixheaacd_mps_huff_dec_ipd_1d(
                          it_bit_buff,
                          &ixheaacd_huff_ipd_nodes.h_1_dim[huff_yy_1],
                          out_data_1_int + num_val_1_int, 1, 0))
                    return 0;
                }
              }
              if (out_data_2 != NULL) {
                if (!ixheaacd_mps_huff_dec_ipd_2d(
                        it_bit_buff,
                        &ixheaacd_huff_ipd_nodes.h_2_dim[huff_yy_2][FREQ_PAIR],
                        pair_vec + 1, num_val_2_int, 2, p0_data_2))
                  return 0;
                if (df_rest_flag_2) {
                  if (!ixheaacd_mps_huff_dec_ipd_1d(
                          it_bit_buff,
                          &ixheaacd_huff_ipd_nodes.h_1_dim[huff_yy_2],
                          out_data_2_int + num_val_2_int, 1, 0))
                    return 0;
                }
              }
              break;

            default:
              break;
          }

          if (out_data_1 != NULL) {
            for (i = 0; i < num_val_1_int - 1; i += 2) {
              out_data_1_int[i] = pair_vec[i][0];
              out_data_1_int[i + 1] = pair_vec[i][1];
            }
          }
          if (out_data_2 != NULL) {
            for (i = 0; i < num_val_2_int - 1; i += 2) {
              out_data_2_int[i] = pair_vec[i + 1][0];
              out_data_2_int[i + 1] = pair_vec[i + 1][1];
            }
          }

          break;

        case TIME_PAIR:

          if (!pilot_coding_flag &&
              ((diff_type_1 == DIFF_FREQ) || (diff_type_2 == DIFF_FREQ))) {
            p0_data_1[0] = &out_data_1[0];
            p0_data_1[1] = &out_data_2[0];

            out_data_1_int += 1;
            out_data_2_int += 1;

            num_val_1_int -= 1;
          }

          if ((diff_type_1 == DIFF_TIME) || (diff_type_2 == DIFF_TIME)) {
            diff_type = DIFF_TIME;
          } else {
            diff_type = DIFF_FREQ;
          }
          if (pilot_coding_flag) {
            huff_yy = PCM_PLT;
          } else {
            huff_yy = diff_type;
          }

          switch (data_type) {
            case CLD:
              if (!ixheaacd_mps_huff_dec_cld_2d(
                      it_bit_buff,
                      &ixheaacd_huff_cld_nodes.h_2_dim[huff_yy][TIME_PAIR],
                      pair_vec, num_val_1_int, 1, p0_data_1))
                return 0;
              break;

            case ICC:
              if (!ixheaacd_mps_huff_dec_icc_2d(
                      it_bit_buff,
                      &ixheaacd_huff_icc_nodes.h_2_dim[huff_yy][TIME_PAIR],
                      pair_vec, num_val_1_int, 1, p0_data_1))
                return 0;
              break;

            case IPD:
              if (!ixheaacd_mps_huff_dec_ipd_2d(
                      it_bit_buff,
                      &ixheaacd_huff_ipd_nodes.h_2_dim[huff_yy][TIME_PAIR],
                      pair_vec, num_val_1_int, 1, p0_data_1))
                return 0;
              break;

            default:
              break;
          }

          for (i = 0; i < num_val_1_int; i++) {
            out_data_1_int[i] = pair_vec[i][0];
            out_data_2_int[i] = pair_vec[i][1];
          }

          break;

        default:
          break;
      }

      break;

    default:
      break;
  }

  return 1;
}

static VOID ixheaacd_diff_freq_decode(WORD32* diff_data, WORD32* out_data,
                                      WORD32 num_val) {
  WORD32 i = 0;

  out_data[0] = diff_data[0];

  for (i = 1; i < num_val; i++) {
    out_data[i] = out_data[i - 1] + diff_data[i];
  }
}

static VOID ixheaacd_mps_diff_time_dec_bwd(WORD32* prev_data, WORD32* diff_data,
                                           WORD32* out_data,
                                           WORD32 mixed_diff_type,
                                           WORD32 num_val) {
  WORD32 i = 0;

  if (mixed_diff_type) {
    out_data[0] = diff_data[0];
    for (i = 1; i < num_val; i++) {
      out_data[i] = prev_data[i] + diff_data[i];
    }
  } else {
    for (i = 0; i < num_val; i++) {
      out_data[i] = prev_data[i] + diff_data[i];
    }
  }
}

static VOID ixheaacd_mps_diff_time_dec_fwd(WORD32* prev_data, WORD32* diff_data,
                                           WORD32* out_data,
                                           WORD32 mixed_diff_type,
                                           WORD32 num_val) {
  WORD32 i = 0;

  if (mixed_diff_type) {
    out_data[0] = diff_data[0];
    for (i = 1; i < num_val; i++) {
      out_data[i] = prev_data[i] - diff_data[i];
    }
  } else {
    for (i = 0; i < num_val; i++) {
      out_data[i] = prev_data[i] - diff_data[i];
    }
  }
}

static WORD32 ixheaacd_attach_lsb(ia_handle_bit_buf_struct it_bit_buff,
                                  WORD32* in_data_msb,
                                  WORD32 ixheaacd_drc_offset, WORD32 num_lsb,
                                  WORD32 num_val, WORD32* out_data) {
  WORD32 i = 0, lsb = 0, msb = 0;
  UWORD32 data = 0;

  for (i = 0; i < num_val; i++) {
    msb = in_data_msb[i];

    if (num_lsb > 0) {
      data = ixheaacd_read_bits_buf(it_bit_buff, num_lsb);
      lsb = data;

      out_data[i] = ((msb << num_lsb) | lsb) - ixheaacd_drc_offset;
    } else
      out_data[i] = msb - ixheaacd_drc_offset;
  }

  return 0;
}

WORD32 ixheaacd_mps_ecdatapairdec(ia_handle_bit_buf_struct it_bit_buff,
                                  WORD32 outdata[][MAXBANDS],
                                  WORD32 history[MAXBANDS], WORD32 data_type,
                                  WORD32 set_idx, WORD32 data_bands,
                                  WORD32 pair_flag, WORD32 coarse_flag,
                                  WORD32 independency_flag)

{
  WORD32 diff_time_back_flag = !independency_flag || (set_idx > 0);
  WORD32 attach_lsb_flag = 0;
  WORD32 pcm_coding_flag = 0;
  WORD32 pilot_coding_flag = 0;
  WORD32 pilot_data[2] = {0, 0};
  WORD32 mixed_time_pair = 0, pcm_val = 0;
  WORD32 quant_levels = 0, quant_offset = 0;
  UWORD32 data = 0;
  WORD32 band_start = 0;

  WORD32 data_pair[2][MAXBANDS] = {{0}};
  WORD32 data_diff[2][MAXBANDS] = {{0}};

  WORD32 msb_state[MAXBANDS] = {0};

  WORD32* data_array[2] = {NULL, NULL};

  WORD32 diff_type[2] = {DIFF_FREQ, DIFF_FREQ};
  WORD32 cdg_scheme = HUFF_1D;
  WORD32 direction = BACKWARDS;

  switch (data_type) {
    case CLD:
      if (coarse_flag) {
        attach_lsb_flag = 0;
        quant_levels = 15;
        quant_offset = 7;
      } else {
        attach_lsb_flag = 0;
        quant_levels = 31;
        quant_offset = 15;
      }

      break;

    case ICC:
      if (coarse_flag) {
        attach_lsb_flag = 0;
        quant_levels = 4;
        quant_offset = 0;
      } else {
        attach_lsb_flag = 0;
        quant_levels = 8;
        quant_offset = 0;
      }

      break;

    case IPD:
      if (coarse_flag) {
        attach_lsb_flag = 0;
        quant_levels = 8;
        quant_offset = 0;
      } else {
        attach_lsb_flag = 1;
        quant_levels = 16;
        quant_offset = 0;
      }
      break;

    default:
      fprintf(stderr, "Unknown type of data!\n");
      return 0;
  }

  data = ixheaacd_read_bits_buf(it_bit_buff, 1);
  pcm_coding_flag = data;

  pilot_coding_flag = 0;

  if (pcm_coding_flag && !pilot_coding_flag) {
    if (pair_flag) {
      data_array[0] = data_pair[0];
      data_array[1] = data_pair[1];
      pcm_val = 2 * data_bands;
    } else {
      data_array[0] = data_pair[0];
      data_array[1] = NULL;
      pcm_val = data_bands;
    }

    if (!ixheaacd_mps_pcm_decode(it_bit_buff, data_array[0], data_array[1],
                                 quant_offset, pcm_val, quant_levels))
      return 0;

  } else {
    if (pair_flag) {
      data_array[0] = data_diff[0];
      data_array[1] = data_diff[1];
    } else {
      data_array[0] = data_diff[0];
      data_array[1] = NULL;
    }

    diff_type[0] = DIFF_FREQ;
    diff_type[1] = DIFF_FREQ;

    direction = BACKWARDS;

    if (!pilot_coding_flag) {
      if (pair_flag || diff_time_back_flag) {
        data = ixheaacd_read_bits_buf(it_bit_buff, 1);
        diff_type[0] = data;
      }

      if (pair_flag && ((diff_type[0] == DIFF_FREQ) || diff_time_back_flag)) {
        data = ixheaacd_read_bits_buf(it_bit_buff, 1);
        diff_type[1] = data;
      }
    }

    if (!ixheaacd_huff_decode(it_bit_buff, data_array[0], data_array[1],
                              data_type, diff_type[0], diff_type[1],
                              pilot_coding_flag, pilot_data, data_bands,
                              &cdg_scheme)) {
      return 0;
    }

    if ((diff_type[0] == DIFF_TIME) || (diff_type[1] == DIFF_TIME)) {
      if (pair_flag) {
        if ((diff_type[0] == DIFF_TIME) && !diff_time_back_flag) {
          direction = FORWARDS;
        } else if (diff_type[1] == DIFF_TIME) {
          direction = BACKWARDS;
        } else {
          data = ixheaacd_read_bits_buf(it_bit_buff, 1);
          direction = data;
        }
      } else {
        direction = BACKWARDS;
      }
    }

    mixed_time_pair = (diff_type[0] != diff_type[1]) &&
                      ((cdg_scheme & PAIR_MASK) == TIME_PAIR);

    if (direction == BACKWARDS) {
      if (diff_type[0] == DIFF_FREQ) {
        ixheaacd_diff_freq_decode(data_diff[0], data_pair[0], data_bands);
      } else {
        WORD32 i;
        for (i = 0; i < data_bands; i++) {
          msb_state[i] = history[i + band_start] + quant_offset;
          if (attach_lsb_flag) {
            msb_state[i] >>= 1;
          }
        }
        ixheaacd_mps_diff_time_dec_bwd(msb_state, data_diff[0], data_pair[0],
                                       mixed_time_pair, data_bands);
      }
      if (diff_type[1] == DIFF_FREQ) {
        ixheaacd_diff_freq_decode(data_diff[1], data_pair[1], data_bands);
      } else {
        ixheaacd_mps_diff_time_dec_bwd(data_pair[0], data_diff[1], data_pair[1],
                                       mixed_time_pair, data_bands);
      }
    } else {
      ixheaacd_diff_freq_decode(data_diff[1], data_pair[1], data_bands);

      if (diff_type[0] == DIFF_FREQ) {
        ixheaacd_diff_freq_decode(data_diff[0], data_pair[0], data_bands);
      } else {
        ixheaacd_mps_diff_time_dec_fwd(data_pair[1], data_diff[0], data_pair[0],
                                       mixed_time_pair, data_bands);
      }
    }

    ixheaacd_attach_lsb(it_bit_buff, data_pair[0], quant_offset,
                        attach_lsb_flag ? 1 : 0, data_bands, data_pair[0]);
    if (pair_flag) {
      ixheaacd_attach_lsb(it_bit_buff, data_pair[1], quant_offset,
                          attach_lsb_flag ? 1 : 0, data_bands, data_pair[1]);
    }
  }

  memcpy(outdata[set_idx] + band_start, data_pair[0],
         sizeof(WORD32) * data_bands);
  if (pair_flag) {
    memcpy(outdata[set_idx + 1] + band_start, data_pair[1],
           sizeof(WORD32) * data_bands);
  }

  return 1;
}

WORD32 ixheaacd_mps_huff_decode(ia_handle_bit_buf_struct it_bit_buff,
                                WORD32* out_data, WORD32 num_val) {
  WORD32 val_rcvd = 0, dummy = 0, i = 0, val = 0, len = 0;
  WORD32 rl_data[2] = {0};

  while (val_rcvd < num_val) {
    if (!ixheaacd_mps_huff_read_2d(
            it_bit_buff, (ia_huff_node_struct)&ixheaacd_huff_reshape_nodes,
            rl_data, &dummy))
      return 0;
    val = rl_data[0];
    len = rl_data[1] + 1;
    for (i = val_rcvd; i < val_rcvd + len; i++) {
      out_data[i] = val;
    }
    val_rcvd += len;
  }

  return 1;
}
