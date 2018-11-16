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
#include <math.h>
#include <ixheaacd_type_def.h>
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_config.h"

#include "ixheaacd_mps_polyphase.h"

#include "ixheaacd_mps_dec.h"
#include "ixheaacd_mps_interface.h"
#include "ixheaacd_mps_nlc_dec.h"
#include "ixheaacd_mps_hybfilter.h"

#include <assert.h>
#include <stdio.h>

#define min(a, b) ((a) < (b) ? (a) : (b))

#define max(a, b) ((a) > (b) ? (a) : (b))

static int ixheaacd_freq_res_table[] = {0, 28, 20, 14, 10, 7, 5, 4};

static int
    ixheaacd_hybrid_band_71_to_processing_band_4_map[MAX_HYBRID_BANDS_MPS] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};

static int
    ixheaacd_hybrid_band_71_to_processing_band_5_map[MAX_HYBRID_BANDS_MPS] = {
        0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

static int
    ixheaacd_hybrid_band_71_to_processing_band_7_map[MAX_HYBRID_BANDS_MPS] = {
        0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};

static int
    ixheaacd_hybrid_band_71_to_processing_band_10_map[MAX_HYBRID_BANDS_MPS] = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 7, 7, 7, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9};

static int
    ixheaacd_hybrid_band_71_to_processing_band_14_map[MAX_HYBRID_BANDS_MPS] = {
        0,  0,  0,  0,  1,  1,  2,  3,  4,  4,  5,  6,  6,  7,  7,  8,  8,  8,
        9,  9,  9,  10, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12,
        12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
        13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13};

int ixheaacd_hybrid_band_71_to_processing_band_20_map[MAX_HYBRID_BANDS_MPS] = {
    1,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 14,
    15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19};

int ixheaacd_hybrid_band_71_to_processing_band_28_map[MAX_HYBRID_BANDS_MPS] = {
    1,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
    16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 23,
    23, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26,
    26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27};

static float ixheaacd_mps_clip_gain_table[] = {1.000000f, 1.189207f, 1.414213f,
                                               1.681792f, 2.000000f, 2.378414f,
                                               2.828427f, 4.000000f};

static int ixheaacd_mps_stride_table[] = {1, 2, 5, 28};

static float ixheaacd_cld_de_quant_table[] = {
    -150.0, -45.0, -40.0, -35.0, -30.0, -25.0, -22.0, -19.0,
    -16.0,  -13.0, -10.0, -8.0,  -6.0,  -4.0,  -2.0,  0.0,
    2.0,    4.0,   6.0,   8.0,   10.0,  13.0,  16.0,  19.0,
    22.0,   25.0,  30.0,  35.0,  40.0,  45.0,  150.0};

static float ixheaacd_icc_de_quant_table[] = {
    1.0000f, 0.9370f, 0.84118f, 0.60092f, 0.36764f, 0.0f, -0.5890f, -0.9900f};

float ixheaacd_ipd_de_quant_table[] = {
    0.f,          0.392699082f, 0.785398163f, 1.178097245f,
    1.570796327f, 1.963495408f, 2.35619449f,  2.748893572f,
    3.141592654f, 3.534291735f, 3.926990817f, 4.319689899f,
    4.71238898f,  5.105088062f, 5.497787144f, 5.890486225f};
int ixheaacd_ipd_de_quant_table_q28[] = {
    0,          105414360,  210828720,  316243072, 421657440,  527071776,
    632486144,  737900480,  843314880,  948729216, 1054143552, 1159557888,
    1264972288, 1370386688, 1475800960, 1581215360};
static int ixheaacd_smoothing_time_table[] = {64, 128, 256, 512};

static int ixheaacd_inverse_smoothing_time_table_q30[] = {16777216, 8388608,
                                                          4194304, 2097152};

static WORD32 bound_check(WORD32 var, WORD32 lower_bound, WORD32 upper_bound) {
  var = min(var, upper_bound);
  var = max(var, lower_bound);
  return var;
}

static VOID ixheaacd_longmult1(unsigned short a[], unsigned short b,
                               unsigned short d[], int len) {
  int k;
  UWORD32 tmp;
  UWORD32 b0 = (UWORD32)b;

  tmp = ((UWORD32)a[0]) * b0;
  d[0] = (unsigned short)tmp;

  for (k = 1; k < len; k++) {
    tmp = (tmp >> 16) + ((UWORD32)a[k]) * b0;
    d[k] = (unsigned short)tmp;
  }
}

static VOID ixheaacd_longdiv(unsigned short b[], unsigned short a,
                             unsigned short d[], unsigned short *pr, int len) {
  UWORD32 r;
  UWORD32 tmp;
  UWORD32 temp;
  int k;

  assert(a != 0);

  r = 0;

  for (k = len - 1; k >= 0; k--) {
    tmp = ((UWORD32)b[k]) + (r << 16);

    if (tmp) {
      d[k] = (unsigned short)(tmp / a);
      temp = d[k] * a;
      r = tmp - temp;
    } else {
      d[k] = 0;
    }
  }
  *pr = (unsigned short)r;
}

static VOID ixheaacd_longsub(unsigned short a[], unsigned short b[], int lena,
                             int lenb) {
  int h;
  WORD32 carry = 0;

  assert(lena >= lenb);
  for (h = 0; h < lenb; h++) {
    carry = carry + (WORD32)(a[h] - b[h]);
    a[h] = (unsigned short)carry;
    carry = carry >> 16;
  }

  for (; h < lena; h++) {
    carry = ((UWORD32)a[h]) + carry;
    a[h] = (unsigned short)carry;
    carry = carry >> 16;
  }

  assert(carry == 0);
  return;
}

static int ixheaacd_longcompare(unsigned short a[], unsigned short b[],
                                int len) {
  int i;

  for (i = len - 1; i > 0; i--) {
    if (a[i] != b[i]) break;
  }
  return (a[i] >= b[i]) ? 1 : 0;
}

static VOID ixheaacd_mps_coarse2fine(int *data, WORD32 data_type,
                                     int band_start, int ixheaacd_num_bands) {
  int i;

  for (i = band_start; i < band_start + ixheaacd_num_bands; i++) {
    data[i] <<= 1;
  }

  if (data_type == CLD) {
    for (i = band_start; i < band_start + ixheaacd_num_bands; i++) {
      if (data[i] == -14)
        data[i] = -15;
      else if (data[i] == 14)
        data[i] = 15;
    }
  }
}

static VOID ixheaacd_mps_fine2coarse(int *data, int ixheaacd_num_bands) {
  int i;

  for (i = 0; i < ixheaacd_num_bands; i++) {
    data[i] /= 2;
  }
}

static int ixheaacd_mps_getstridemap(int freq_res_stride, int band_start,
                                     int band_stop, int *strides) {
  int i, pb, ch_fac, data_bands, start_offset;

  ch_fac = ixheaacd_mps_stride_table[freq_res_stride];
  data_bands = (band_stop - band_start - 1) / ch_fac + 1;

  strides[0] = band_start;
  for (pb = 1; pb <= data_bands; pb++) {
    strides[pb] = strides[pb - 1] + ch_fac;
  }
  start_offset = 0;
  while (strides[data_bands] > band_stop) {
    if (start_offset < data_bands)
      start_offset++;
    else
      start_offset = 1;

    for (i = start_offset; i <= data_bands; i++) {
      strides[i]--;
    }
  }

  return data_bands;
}

static VOID ixheaacd_mps_ecdata_decoding(
    ia_mps_dec_state_struct *self, ia_handle_bit_buf_struct bitstream,
    int data[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS], int datatype) {
  int i, j, pb, set_index, bs_data_pair, data_bands, old_quant_coarse_xxx;
  int strides[MAX_PARAMETER_BANDS + 1] = {0};
  int band_stop = 0;

  int *lastdata = NULL;
  ia_mps_data_struct *frame_xxx_data = NULL;
  int default_val = 0;

  ia_mps_bs_frame *frame = &(self->bs_frame);

  if (datatype == 0) {
    frame_xxx_data = &frame->cld_data;
    lastdata = frame->cmp_cld_idx_prev;
    band_stop = self->bs_param_bands;
  } else if (datatype == 1) {
    frame_xxx_data = &frame->icc_data;
    lastdata = frame->cmp_icc_idx_prev;
    band_stop = self->bs_param_bands;
  } else if (datatype == 2) {
    frame_xxx_data = &frame->ipd_data;
    lastdata = frame->ipd_idx_data_prev;
    band_stop = self->num_bands_ipd;
  } else {
    frame_xxx_data = &frame->cld_data;
    lastdata = frame->cmp_cld_idx_prev;
    band_stop = self->bs_param_bands;
  }

  for (i = 0; i < self->num_parameter_sets; i++) {
    frame_xxx_data->bs_xxx_data_mode[i] = ixheaacd_read_bits_buf(bitstream, 2);
  }

  set_index = 0;
  bs_data_pair = 0;
  old_quant_coarse_xxx = frame_xxx_data->bs_quant_coarse_xxx_prev;

  for (i = 0; i < self->num_parameter_sets; i++) {
    if (frame_xxx_data->bs_xxx_data_mode[i] == 0) {
      for (pb = 0; pb < band_stop; pb++) {
        lastdata[pb] = default_val;
      }

      old_quant_coarse_xxx = 0;
    }

    if (frame_xxx_data->bs_xxx_data_mode[i] == 3) {
      if (bs_data_pair) {
        bs_data_pair = 0;
      } else {
        bs_data_pair = ixheaacd_read_bits_buf(bitstream, 1);
        frame_xxx_data->bs_quant_coarse_xxx[set_index] =
            ixheaacd_read_bits_buf(bitstream, 1);
        frame_xxx_data->bs_freq_res_stride_xxx[set_index] =
            ixheaacd_read_bits_buf(bitstream, 2);

        if (frame_xxx_data->bs_quant_coarse_xxx[set_index] !=
            old_quant_coarse_xxx) {
          if (old_quant_coarse_xxx) {
            ixheaacd_mps_coarse2fine(lastdata, datatype, 0, band_stop - 0);
          } else {
            ixheaacd_mps_fine2coarse(lastdata, band_stop);
          }
        }

        data_bands = ixheaacd_mps_getstridemap(
            frame_xxx_data->bs_freq_res_stride_xxx[set_index], 0, band_stop,
            strides);

        for (pb = 0; pb < data_bands; pb++) {
          lastdata[pb] = lastdata[strides[pb]];
        }

        ixheaacd_mps_ecdatapairdec(
            bitstream, data, lastdata, datatype, set_index, data_bands,
            bs_data_pair, frame_xxx_data->bs_quant_coarse_xxx[set_index],
            frame->independency_flag && (i == 0));

        for (pb = 0; pb < data_bands; pb++) {
          for (j = strides[pb]; j < strides[pb + 1]; j++) {
            if (datatype == IPD) {
              if (frame_xxx_data->bs_quant_coarse_xxx[set_index]) {
                lastdata[j] = data[set_index + bs_data_pair][pb] & 7;
              } else {
                lastdata[j] = data[set_index + bs_data_pair][pb] & 15;
              }
            } else {
              lastdata[j] = data[set_index + bs_data_pair][pb];
            }
          }
        }

        old_quant_coarse_xxx = frame_xxx_data->bs_quant_coarse_xxx[set_index];

        if (bs_data_pair) {
          frame_xxx_data->bs_quant_coarse_xxx[set_index + 1] =
              frame_xxx_data->bs_quant_coarse_xxx[set_index];
          frame_xxx_data->bs_freq_res_stride_xxx[set_index + 1] =
              frame_xxx_data->bs_freq_res_stride_xxx[set_index];
        }
        set_index += bs_data_pair + 1;
      }
    }
  }
}

VOID ixheaacd_mps_frame_parsing(ia_mps_dec_state_struct *self,
                                int usac_independency_flag,
                                ia_handle_bit_buf_struct bitstream) {
  int i, bs_frame_type, data_bands, bs_temp_shape_enable, num_of_temp_shape_ch;
  int ps, pg, ts, pb;
  int env_shape_data[MAX_TIME_SLOTS];

  int bits_param_slot = 0;

  ia_mps_bs_frame *frame = &(self->bs_frame);

  if (self->parse_nxt_frame == 0) return;

  self->num_parameter_sets_prev = self->num_parameter_sets;

  if (self->bs_high_rate_mode) {
    bs_frame_type = ixheaacd_read_bits_buf(bitstream, 1);
    self->num_parameter_sets = ixheaacd_read_bits_buf(bitstream, 3) + 1;
  } else {
    bs_frame_type = 0;
    self->num_parameter_sets = 1;
  }

  if (self->time_slots == 32)
    bits_param_slot = 5;
  else if (self->time_slots == 64)
    bits_param_slot = 6;

  if (bs_frame_type) {
    for (i = 0; i < self->num_parameter_sets; i++) {
      self->param_slots[i] = ixheaacd_read_bits_buf(bitstream, bits_param_slot);
    }
  } else {
    self->param_slots[0] = self->time_slots - 1;
  }

  if (!usac_independency_flag) {
    frame->independency_flag = ixheaacd_read_bits_buf(bitstream, 1);
  } else {
    frame->independency_flag = 1;
  }

  ixheaacd_mps_ecdata_decoding(self, bitstream, frame->cmp_cld_idx, CLD);
  ixheaacd_mps_ecdata_decoding(self, bitstream, frame->cmp_icc_idx, ICC);

  if (self->config->bs_phase_coding) {
    self->bs_phase_mode = ixheaacd_read_bits_buf(bitstream, 1);

    if (!self->bs_phase_mode) {
      for (pb = 0; pb < self->num_bands_ipd; pb++) {
        frame->ipd_idx_data_prev[pb] = 0;
        for (i = 0; i < self->num_parameter_sets; i++) {
          frame->ipd_idx_data[i][pb] = 0;
          self->bs_frame.ipd_idx[i][pb] = 0;
        }
        self->bs_frame.ipd_idx_prev[pb] = 0;
      }
      self->opd_smoothing_mode = 0;
    } else {
      self->opd_smoothing_mode = ixheaacd_read_bits_buf(bitstream, 1);
      ixheaacd_mps_ecdata_decoding(self, bitstream, frame->ipd_idx_data, IPD);
    }
  }

  else {
    self->bs_phase_mode = 0;
    for (pb = 0; pb < self->num_bands_ipd; pb++) {
      frame->ipd_idx_data_prev[pb] = 0;
      for (i = 0; i < self->num_parameter_sets; i++) {
        frame->ipd_idx_data[i][pb] = 0;
        self->bs_frame.ipd_idx[i][pb] = 0;
      }
      self->bs_frame.ipd_idx_prev[pb] = 0;
    }
    self->opd_smoothing_mode = 0;
  }

  if (self->bs_high_rate_mode) {
    for (ps = 0; ps < self->num_parameter_sets; ps++) {
      frame->bs_smooth_mode[ps] = ixheaacd_read_bits_buf(bitstream, 2);
      if (frame->bs_smooth_mode[ps] >= 2) {
        frame->bs_smooth_time[ps] = ixheaacd_read_bits_buf(bitstream, 2);
      }
      if (frame->bs_smooth_mode[ps] == 3) {
        frame->bs_freq_res_stride_smg[ps] =
            ixheaacd_read_bits_buf(bitstream, 2);
        data_bands =
            (self->bs_param_bands - 1) /
                ixheaacd_mps_stride_table[frame->bs_freq_res_stride_smg[ps]] +
            1;
        for (pg = 0; pg < data_bands; pg++) {
          frame->bs_smg_data[ps][pg] = ixheaacd_read_bits_buf(bitstream, 1);
        }
      }
    }
  } else {
    for (ps = 0; ps < self->num_parameter_sets; ps++) {
      frame->bs_smooth_mode[ps] = 0;
    }
  }

  for (i = 0; i < 2; i++) {
    self->temp_shape_enable_ch_stp[i] = 0;
    self->temp_shape_enable_ch_ges[i] = 0;
  }

  self->bs_tsd_enable = 0;
  if (self->config->bs_temp_shape_config == 3) {
    self->bs_tsd_enable = ixheaacd_read_bits_buf(bitstream, 1);
  } else if (self->config->bs_temp_shape_config != 0) {
    bs_temp_shape_enable = ixheaacd_read_bits_buf(bitstream, 1);
    if (bs_temp_shape_enable) {
      num_of_temp_shape_ch = 2;
      switch (self->config->bs_temp_shape_config) {
        case 1:
          for (i = 0; i < num_of_temp_shape_ch; i++) {
            self->temp_shape_enable_ch_stp[i] =
                ixheaacd_read_bits_buf(bitstream, 1);
          }
          break;
        case 2:
          for (i = 0; i < num_of_temp_shape_ch; i++) {
            self->temp_shape_enable_ch_ges[i] =
                ixheaacd_read_bits_buf(bitstream, 1);
          }
          for (i = 0; i < num_of_temp_shape_ch; i++) {
            if (self->temp_shape_enable_ch_ges[i]) {
              ixheaacd_mps_huff_decode(bitstream, env_shape_data,
                                       self->time_slots);
              for (ts = 0; ts < self->time_slots; ts++) {
                self->env_shape_data[i][ts] = (float)pow(
                    2, (float)env_shape_data[ts] /
                               (self->config->bs_env_quant_mode + 2) -
                           1);
              }
            }
          }
          break;
        default:
          assert(0);
      }
    }
  }

  if (self->bs_tsd_enable) {
    unsigned short s[4];
    UWORD64 s_64;
    unsigned short c[5];
    UWORD64 c_64;
    unsigned short b;
    unsigned short r[1];
    unsigned short table_64[] = {6,  11, 16, 20, 23, 27, 30, 33, 35, 38, 40,
                                 42, 44, 46, 48, 49, 51, 52, 53, 55, 56, 57,
                                 58, 58, 59, 60, 60, 60, 61, 61, 61, 61};
    unsigned short table_32[] = {5,  9,  13, 16, 18, 20, 22, 24,
                                 25, 26, 27, 28, 29, 29, 30, 30};
    unsigned short *tab = NULL;
    int k;
    unsigned short h;
    WORD32 nbits_tr_slots = 0;

    if (self->time_slots == 32) {
      nbits_tr_slots = 4;
      tab = table_32;
    } else if (self->time_slots == 64) {
      nbits_tr_slots = 5;
      tab = table_64;
    }

    self->tsd_num_tr_slots = ixheaacd_read_bits_buf(bitstream, nbits_tr_slots);
    self->tsd_num_tr_slots++;
    self->tsd_codeword_len = tab[self->tsd_num_tr_slots - 1];

    if (self->tsd_codeword_len > 48) {
      s[3] = ixheaacd_read_bits_buf(bitstream, self->tsd_codeword_len - 48);
      s_64 = s[3];
      s[2] = ixheaacd_read_bits_buf(bitstream, 16);
      s_64 = (s_64 << 16) | s[2];
      s[1] = ixheaacd_read_bits_buf(bitstream, 16);
      s_64 = (s_64 << 16) | s[1];
      s[0] = ixheaacd_read_bits_buf(bitstream, 16);
      s_64 = (s_64 << 16) | s[0];
    } else if (self->tsd_codeword_len > 32) {
      s[3] = 0;
      s_64 = s[3];
      s[2] = ixheaacd_read_bits_buf(bitstream, self->tsd_codeword_len - 32);
      s_64 = (s_64 << 16) | s[2];
      s[1] = ixheaacd_read_bits_buf(bitstream, 16);
      s_64 = (s_64 << 16) | s[1];
      s[0] = ixheaacd_read_bits_buf(bitstream, 16);
      s_64 = (s_64 << 16) | s[0];
    } else if (self->tsd_codeword_len > 16) {
      s[3] = 0;
      s_64 = s[3];
      s[2] = 0;
      s_64 = (s_64 << 16) | s[2];
      s[1] = ixheaacd_read_bits_buf(bitstream, self->tsd_codeword_len - 16);
      s_64 = (s_64 << 16) | s[1];
      s[0] = ixheaacd_read_bits_buf(bitstream, 16);
      s_64 = (s_64 << 16) | s[0];
    } else {
      s[3] = 0;
      s_64 = s[3];
      s[2] = 0;
      s_64 = (s_64 << 16) | s[2];
      s[1] = 0;
      s_64 = (s_64 << 16) | s[1];
      s[0] = ixheaacd_read_bits_buf(bitstream, self->tsd_codeword_len);
      s_64 = (s_64 << 16) | s[0];
    }

    {
      int p = self->tsd_num_tr_slots;

      for (i = 0; i < self->time_slots; i++) self->bs_tsd_sep_data[i] = 0;

      for (k = self->time_slots - 1; k >= 0; k--) {
        if (p > k) {
          for (; k >= 0; k--) self->bs_tsd_sep_data[k] = 1;
          break;
        }

        c[0] = k - p + 1;
        c_64 = c[0];
        for (i = 1; i < 5; i++) c[i] = 0;

        for (h = 2; h <= p; h++) {
          b = k - p + h;
          c_64 = c_64 * (b / h);
          ixheaacd_longmult1(c, b, c, 5);
          b = h;
          ixheaacd_longdiv(c, b, c, r, 5);
        }

        if (ixheaacd_longcompare(s, c, 4)) {
          ixheaacd_longsub(s, c, 4, 4);
          self->bs_tsd_sep_data[k] = 1;
          p--;
          if (p == 0) break;
        }
      }
    }

    for (i = 0; i < self->time_slots; i++) {
      if (self->bs_tsd_sep_data[i])
        self->bs_tsd_tr_phase_data[i] = ixheaacd_read_bits_buf(bitstream, 3);
    }
  }

  self->parse_nxt_frame = 0;
}

static VOID ixheaacd_mps_createmapping(int map[MAX_PARAMETER_BANDS + 1],
                                       int band_start, int band_stop,
                                       int ch_fac) {
  int input_bands, out_bands, bands_achived, bands_diff, incr, k, i;
  int vdk[MAX_PARAMETER_BANDS + 1];
  input_bands = band_stop - band_start;
  out_bands = (input_bands - 1) / ch_fac + 1;
  if (out_bands < 1) {
    out_bands = 1;
  }

  bands_achived = out_bands * ch_fac;
  bands_diff = input_bands - bands_achived;
  for (i = 0; i < out_bands; i++) {
    vdk[i] = ch_fac;
  }

  if (bands_diff > 0) {
    incr = -1;
    k = out_bands - 1;
  } else {
    incr = 1;
    k = 0;
  }

  while (bands_diff != 0) {
    vdk[k] = vdk[k] - incr;
    k = k + incr;
    bands_diff = bands_diff + incr;
    if (k >= out_bands) {
      if (bands_diff > 0) {
        k = out_bands - 1;
      } else if (bands_diff < 0) {
        k = 0;
      }
    }
  }
  map[0] = band_start;
  for (i = 0; i < out_bands; i++) {
    map[i + 1] = map[i] + vdk[i];
  }
}

static VOID ixheaacd_mps_mapfrequency(int *in, int *out, int *map,
                                      int data_bands) {
  int i, j, band_start, band_stop, value;
  int start_band_0 = map[0];

  for (i = 0; i < data_bands; i++) {
    value = in[i + start_band_0];

    band_start = map[i];
    band_stop = map[i + 1];
    for (j = band_start; j < band_stop; j++) {
      out[j] = value;
    }
  }
}

static float ixheaacd_mps_de_quantize(int value, int param_type) {
  switch (param_type) {
    case CLD:
      return ixheaacd_cld_de_quant_table[value + 15];

    case ICC:
      return ixheaacd_icc_de_quant_table[value];

    case IPD:
      return ixheaacd_ipd_de_quant_table[(value & 15)];

    default:
      assert(0);
      return 0.0;
  }
}

static WORD32 ixheaacd_mps_mapindexdata(
    ia_mps_dec_state_struct *self, ia_mps_data_struct *frame_xxx_data,
    float out_data[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS],
    int out_idx_data[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS],
    int cmp_idx_data[MAX_PARAMETER_SETS_MPS][MAX_PARAMETER_BANDS],
    int idx_prev[MAX_PARAMETER_BANDS], int param_type) {
  int interpolate_local[MAX_PARAMETER_SETS_MPS];
  int map[MAX_PARAMETER_BANDS + 1];

  int set_index, i, band, parm_slot;
  int data_bands, ch_fac;
  int ps;

  int i1, i2, x1, xi, x2;
  int band_start = 0;
  int ext_frame_flag = self->ext_frame_flag;
  int *param_slots = self->param_slots;
  int num_parameter_sets = self->num_parameter_sets;
  int band_stop = self->bs_param_bands;
  int default_val = 0;

  set_index = 0;

  for (i = 0; i < num_parameter_sets; i++) {
    if (frame_xxx_data->bs_xxx_data_mode[i] == 0) {
      frame_xxx_data->quant_coarse_xxx_flag[i] = 0;
      for (band = band_start; band < band_stop; band++) {
        out_idx_data[i][band] = default_val;
      }
      for (band = band_start; band < band_stop; band++) {
        idx_prev[band] = out_idx_data[i][band];
      }

      frame_xxx_data->bs_quant_coarse_xxx_prev = 0;
    }

    if (frame_xxx_data->bs_xxx_data_mode[i] == 1) {
      for (band = band_start; band < band_stop; band++) {
        out_idx_data[i][band] = idx_prev[band];
      }
      frame_xxx_data->quant_coarse_xxx_flag[i] =
          frame_xxx_data->bs_quant_coarse_xxx_prev;
    }

    if (frame_xxx_data->bs_xxx_data_mode[i] == 2) {
      for (band = band_start; band < band_stop; band++) {
        out_idx_data[i][band] = idx_prev[band];
      }
      frame_xxx_data->quant_coarse_xxx_flag[i] =
          frame_xxx_data->bs_quant_coarse_xxx_prev;
      interpolate_local[i] = 1;
    } else {
      interpolate_local[i] = 0;
    }

    if (frame_xxx_data->bs_xxx_data_mode[i] == 3) {
      parm_slot = i;
      ch_fac =
          ixheaacd_mps_stride_table[frame_xxx_data
                                        ->bs_freq_res_stride_xxx[set_index]];
      data_bands = (band_stop - band_start - 1) / ch_fac + 1;
      ixheaacd_mps_createmapping(map, band_start, band_stop, ch_fac);
      ixheaacd_mps_mapfrequency(&cmp_idx_data[set_index][0],
                                &out_idx_data[parm_slot][0], map, data_bands);

      for (band = band_start; band < band_stop; band++) {
        idx_prev[band] = out_idx_data[parm_slot][band];
      }

      frame_xxx_data->bs_quant_coarse_xxx_prev =
          frame_xxx_data->bs_quant_coarse_xxx[set_index];
      frame_xxx_data->quant_coarse_xxx_flag[i] =
          frame_xxx_data->bs_quant_coarse_xxx[set_index];

      set_index++;
    }
  }

  for (i = 0; i < num_parameter_sets; i++) {
    if (frame_xxx_data->quant_coarse_xxx_flag[i] == 1) {
      ixheaacd_mps_coarse2fine(out_idx_data[i], param_type, band_start,
                               band_stop - band_start);
      frame_xxx_data->quant_coarse_xxx_flag[i] = 0;
    }
  }

  i1 = -1;
  x1 = 0;
  i2 = 0;
  for (i = 0; i < num_parameter_sets; i++) {
    if (interpolate_local[i] != 1) {
      i1 = i;
    }
    i2 = i;
    while (interpolate_local[i2] == 1) {
      i2++;
    }
    if (i1 == -1) {
      x1 = 0;
      i1 = 0;
    } else {
      x1 = param_slots[i1];
    }
    xi = param_slots[i];
    x2 = param_slots[i2];

    if (interpolate_local[i] == 1) {
      if (i2 < num_parameter_sets) {
        return -1;
      }
      for (band = band_start; band < band_stop; band++) {
        int yi, y1, y2;
        yi = 0;
        y1 = out_idx_data[i1][band];
        y2 = out_idx_data[i2][band];
        if (param_type == IPD) {
          if (y2 - y1 > 8) y1 += 16;
          if (y1 - y2 > 8) y2 += 16;

          if (x2 != x1) {
            yi = (y1 + (xi - x1) * (y2 - y1) / (x2 - x1)) % 16;
          }
        } else {
          if (x2 != x1) {
            yi = y1 + (xi - x1) * (y2 - y1) / (x2 - x1);
          }
        }
        out_idx_data[i][band] = yi;
      }
    }
  }

  for (ps = 0; ps < num_parameter_sets; ps++) {
    for (band = band_start; band < band_stop; band++) {
      if (param_type == CLD) {
        out_idx_data[ps][band] = bound_check(out_idx_data[ps][band], -15, 15);
      } else if (param_type == ICC)  // param_type is ICC
      {
        out_idx_data[ps][band] = bound_check(out_idx_data[ps][band], 0, 7);
      }
      out_data[ps][band] =
          ixheaacd_mps_de_quantize(out_idx_data[ps][band], param_type);
    }
  }

  if (ext_frame_flag) {
    for (band = band_start; band < band_stop; band++) {
      out_data[num_parameter_sets][band] =
          out_data[num_parameter_sets - 1][band];
      out_idx_data[num_parameter_sets][band] =
          out_idx_data[num_parameter_sets - 1][band];
    }
  }

  return 0;
}

static WORD32 ixheaacd_mps_dec_and_mapframeott(ia_mps_dec_state_struct *self) {
  ia_mps_bs_frame *cur_bit_stream_ptr = &(self->bs_frame);
  WORD32 err_code = 0;

  err_code = ixheaacd_mps_mapindexdata(
      self, &cur_bit_stream_ptr->cld_data, self->cld_data,
      cur_bit_stream_ptr->cld_idx, cur_bit_stream_ptr->cmp_cld_idx,
      cur_bit_stream_ptr->cld_idx_pre, CLD);
  if (err_code != 0) return err_code;

  err_code = ixheaacd_mps_mapindexdata(
      self, &cur_bit_stream_ptr->icc_data, self->icc_data,
      cur_bit_stream_ptr->icc_idx, cur_bit_stream_ptr->cmp_icc_idx,
      cur_bit_stream_ptr->icc_idx_pre, ICC);
  if (err_code != 0) return err_code;
  if ((self->config->bs_phase_coding)) {
    err_code = ixheaacd_mps_mapindexdata(
        self, &cur_bit_stream_ptr->ipd_data, self->ipd_data,
        cur_bit_stream_ptr->ipd_idx, cur_bit_stream_ptr->ipd_idx_data,
        cur_bit_stream_ptr->ipd_idx_prev, IPD);

    if (err_code != 0) return err_code;
  }

  return 0;
}

static VOID ixheaacd_mps_dec_and_mapframesmg(ia_mps_dec_state_struct *self) {
  int ps, pb, pg, ch_fac, data_bands, param_band_start, param_band_stop,
      group_to_band[MAX_PARAMETER_BANDS + 1];
  ia_mps_bs_frame *frame = &(self->bs_frame);
  for (ps = 0; ps < self->num_parameter_sets; ps++) {
    switch (frame->bs_smooth_mode[ps]) {
      case 0:
        self->smoothing_time[ps] = 256;
        self->inv_smoothing_time[ps] = 4194304;

        for (pb = 0; pb < self->bs_param_bands; pb++) {
          self->smoothing_data[ps][pb] = 0;
        }
        break;

      case 1:
        if (ps > 0) {
          self->smoothing_time[ps] = self->smoothing_time[ps - 1];
          self->inv_smoothing_time[ps] = self->inv_smoothing_time[ps - 1];
        } else {
          self->smoothing_time[ps] = self->smoothing_filt_state.prev_smg_time;
          self->inv_smoothing_time[ps] =
              self->smoothing_filt_state.inv_prev_smg_time;
        }

        for (pb = 0; pb < self->bs_param_bands; pb++) {
          if (ps > 0)
            self->smoothing_data[ps][pb] = self->smoothing_data[ps - 1][pb];
          else
            self->smoothing_data[ps][pb] =
                self->smoothing_filt_state.prev_smg_data[pb];
        }
        break;

      case 2:
        self->smoothing_time[ps] =
            ixheaacd_smoothing_time_table[frame->bs_smooth_time[ps]];
        self->inv_smoothing_time[ps] =
            ixheaacd_inverse_smoothing_time_table_q30[frame
                                                          ->bs_smooth_time[ps]];
        for (pb = 0; pb < self->bs_param_bands; pb++) {
          self->smoothing_data[ps][pb] = 1;
        }
        break;

      case 3:
        self->smoothing_time[ps] =
            ixheaacd_smoothing_time_table[frame->bs_smooth_time[ps]];
        self->inv_smoothing_time[ps] =
            ixheaacd_inverse_smoothing_time_table_q30[frame
                                                          ->bs_smooth_time[ps]];

        ch_fac = ixheaacd_mps_stride_table[frame->bs_freq_res_stride_smg[ps]];
        data_bands = (self->bs_param_bands - 1) / ch_fac + 1;
        ixheaacd_mps_createmapping(group_to_band, 0, self->bs_param_bands,
                                   ch_fac);
        for (pg = 0; pg < data_bands; pg++) {
          param_band_start = group_to_band[pg];
          param_band_stop = group_to_band[pg + 1];
          for (pb = param_band_start; pb < param_band_stop; pb++) {
            self->smoothing_data[ps][pb] = frame->bs_smg_data[ps][pg];
          }
        }
        break;
    }
  }

  self->smoothing_filt_state.prev_smg_time =
      self->smoothing_time[self->num_parameter_sets - 1];
  self->smoothing_filt_state.inv_prev_smg_time =
      self->inv_smoothing_time[self->num_parameter_sets - 1];
  for (pb = 0; pb < self->bs_param_bands; pb++) {
    self->smoothing_filt_state.prev_smg_data[pb] =
        self->smoothing_data[self->num_parameter_sets - 1][pb];
  }

  if (self->ext_frame_flag) {
    self->smoothing_time[self->num_parameter_sets] =
        self->smoothing_time[self->num_parameter_sets - 1];
    self->inv_smoothing_time[self->num_parameter_sets] =
        self->inv_smoothing_time[self->num_parameter_sets - 1];
    for (pb = 0; pb < self->bs_param_bands; pb++) {
      self->smoothing_data[self->num_parameter_sets][pb] =
          self->smoothing_data[self->num_parameter_sets - 1][pb];
    }
  }
}

WORD32 ixheaacd_mps_frame_decode(ia_mps_dec_state_struct *self) {
  int i;
  WORD32 err_code = 0;
  if (self->parse_nxt_frame == 1) return 0;

  self->ext_frame_flag = 0;
  if (self->param_slots[self->num_parameter_sets - 1] != self->time_slots - 1) {
    self->ext_frame_flag = 1;
  }

  err_code = ixheaacd_mps_dec_and_mapframeott(self);

  if (err_code != 0) return err_code;

  ixheaacd_mps_dec_and_mapframesmg(self);

  if (self->ext_frame_flag) {
    self->num_parameter_sets++;
    self->param_slots[self->num_parameter_sets - 1] = self->time_slots - 1;
  }
  self->param_slot_diff[0] = self->param_slots[0] + 1;
  self->inv_param_slot_diff[0] = (float)1 / self->param_slot_diff[0];
  self->inv_param_slot_diff_Q30[0] =
      (int)floor(self->inv_param_slot_diff[0] * 1073741824 + 0.5);
  for (i = 1; i < self->num_parameter_sets; i++) {
    self->param_slot_diff[i] = self->param_slots[i] - self->param_slots[i - 1];
    self->inv_param_slot_diff[i] = (float)1 / self->param_slot_diff[i];
    self->inv_param_slot_diff_Q30[i] =
        (int)floor(self->inv_param_slot_diff[i] * 1073741824 + 0.5);
  }

  return 0;
}

WORD32 ixheaacd_mps_header_decode(ia_mps_dec_state_struct *self) {
  self->time_slots = self->frame_length + 1;
  self->frame_len = self->time_slots * self->qmf_band_count;
  self->bs_param_bands = ixheaacd_freq_res_table[self->config->bs_freq_res];

  self->hyb_band_count = self->qmf_band_count - QMF_BANDS_TO_HYBRID + 10;

  switch (self->bs_param_bands) {
    case 4:

      self->hyb_band_to_processing_band_table =
          ixheaacd_hybrid_band_71_to_processing_band_4_map;
      break;
    case 5:

      self->hyb_band_to_processing_band_table =
          ixheaacd_hybrid_band_71_to_processing_band_5_map;
      break;
    case 7:

      self->hyb_band_to_processing_band_table =
          ixheaacd_hybrid_band_71_to_processing_band_7_map;
      break;
    case 10:

      self->hyb_band_to_processing_band_table =
          ixheaacd_hybrid_band_71_to_processing_band_10_map;
      break;
    case 14:

      self->hyb_band_to_processing_band_table =
          ixheaacd_hybrid_band_71_to_processing_band_14_map;
      break;
    case 20:

      self->hyb_band_to_processing_band_table =
          ixheaacd_hybrid_band_71_to_processing_band_20_map;
      break;
    case 28:

      self->hyb_band_to_processing_band_table =
          ixheaacd_hybrid_band_71_to_processing_band_28_map;
      break;
    default:
      self->hyb_band_to_processing_band_table = NULL;
      return -1;
      break;
  };

  self->in_ch_count = 1;
  self->out_ch_count = 2;

  self->input_gain =
      ixheaacd_mps_clip_gain_table[self->config->bs_fixed_gain_dmx];

  if (self->config->bs_ott_bands_phase_present) {
    self->num_bands_ipd = self->config->bs_ott_bands_phase;
  } else {
    switch (self->bs_param_bands) {
      case 4:
      case 5:
        self->num_bands_ipd = 2;
        break;
      case 7:
        self->num_bands_ipd = 3;
        break;
      case 10:
        self->num_bands_ipd = 5;
        break;
      case 14:
        self->num_bands_ipd = 7;
        break;
      case 20:
      case 28:
        self->num_bands_ipd = 10;
        break;
      default:
        assert(0);
        break;
    }
  }

  if (self->residual_coding) {
    self->num_bands_ipd = max(self->bs_residual_bands, self->num_bands_ipd);
    self->max_res_bands = 0;
    if (self->bs_residual_present) {
      self->res_bands = self->bs_residual_bands;
      if (self->res_bands > self->max_res_bands) {
        self->max_res_bands = self->res_bands;
      }
    } else {
      self->res_bands = 0;
    }
  }

  if (self->num_bands_ipd > MAX_PARAMETER_BANDS) return -1;

  self->dir_sig_count = 1;
  self->decor_sig_count = 1;

  self->bs_high_rate_mode = self->config->bs_high_rate_mode;

  return 0;
}
