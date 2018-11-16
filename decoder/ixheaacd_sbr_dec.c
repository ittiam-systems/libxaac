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
#include <string.h>
#include "ixheaacd_sbr_common.h"
#include <ixheaacd_type_def.h>

#include "ixheaacd_constants.h"
#include <ixheaacd_basic_ops32.h>
#include <ixheaacd_basic_ops16.h>
#include <ixheaacd_basic_ops40.h>
#include "ixheaacd_basic_ops.h"

#include <ixheaacd_basic_op.h>
#include "ixheaacd_intrinsics.h"
#include "ixheaacd_common_rom.h"
#include "ixheaacd_basic_funcs.h"
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_defines.h"

#include "ixheaacd_pns.h"

#include <ixheaacd_aac_rom.h>
#include "ixheaacd_pulsedata.h"

#include "ixheaacd_drc_data_struct.h"
#include "ixheaacd_lt_predict.h"
#include "ixheaacd_channelinfo.h"
#include "ixheaacd_drc_dec.h"
#include "ixheaacd_sbrdecoder.h"

#include "ixheaacd_defines.h"
#include "ixheaacd_sbrdecoder.h"
#include "ixheaacd_definitions.h"
#include "ixheaacd_error_codes.h"

#include "ixheaacd_pulsedata.h"

#include "ixheaacd_sbrdecsettings.h"
#include "ixheaacd_sbr_scale.h"
#include "ixheaacd_lpp_tran.h"
#include "ixheaacd_env_extr_part.h"
#include <ixheaacd_sbr_rom.h>
#include "ixheaacd_hybrid.h"
#include "ixheaacd_ps_dec.h"
#include "ixheaacd_env_extr.h"

#include "ixheaacd_qmf_dec.h"

#include "ixheaacd_env_calc.h"
#include "ixheaacd_sbr_const.h"

#include "ixheaacd_pvc_dec.h"
#include "ixheaacd_sbr_dec.h"
#include "ixheaacd_env_extr.h"
#include "ixheaacd_env_calc.h"
#include "ixheaacd_ps_dec.h"
#include "ixheaacd_function_selector.h"

#include "ixheaacd_audioobjtypes.h"

VOID ixheaacd_qmf_enrg_calc(ia_sbr_dec_struct *ptr_sbr_dec,
                            WORD32 upsample_ratio_idx, WORD32 low_pow_flag) {
  WORD32 i, j;
  if (upsample_ratio_idx == SBR_UPSAMPLE_IDX_4_1) {
    for (i = 0; i < ptr_sbr_dec->str_codec_qmf_bank.num_time_slots; i++) {
      for (j = 0; j < 16; j++) {
        ptr_sbr_dec->qmf_energy_buf[i][j] =
            ptr_sbr_dec->qmf_buf_real[2 + i][j] *
            ptr_sbr_dec->qmf_buf_real[2 + i][j];
        if (!low_pow_flag)
          ptr_sbr_dec->qmf_energy_buf[i][j] +=
              (ptr_sbr_dec->qmf_buf_imag[2 + i][j] *
               ptr_sbr_dec->qmf_buf_imag[2 + i][j]);
      }
    }

    for (i = 0; i < 16; i++) {
      for (j = 0; j < 16; j++) {
        ptr_sbr_dec->pvc_qmf_enrg_arr[32 * i + j] =
            (ptr_sbr_dec->qmf_energy_buf[4 * i + 0][j] +
             ptr_sbr_dec->qmf_energy_buf[4 * i + 1][j] +
             ptr_sbr_dec->qmf_energy_buf[4 * i + 2][j] +
             ptr_sbr_dec->qmf_energy_buf[4 * i + 3][j]) *
            0.25f;
      }
    }
  } else {
    for (i = 0; i < ptr_sbr_dec->str_codec_qmf_bank.num_time_slots; i++) {
      for (j = 0; j < 32; j++) {
        ptr_sbr_dec->qmf_energy_buf[i][j] =
            ptr_sbr_dec->qmf_buf_real[2 + i][j] *
            ptr_sbr_dec->qmf_buf_real[2 + i][j];
        if (!low_pow_flag)
          ptr_sbr_dec->qmf_energy_buf[i][j] +=
              (ptr_sbr_dec->qmf_buf_imag[2 + i][j] *
               ptr_sbr_dec->qmf_buf_imag[2 + i][j]);
      }
    }

    for (i = 0; i < 16; i++) {
      for (j = 0; j < 32; j++) {
        ptr_sbr_dec->pvc_qmf_enrg_arr[32 * i + j] =
            (ptr_sbr_dec->qmf_energy_buf[2 * i + 0][j] +
             ptr_sbr_dec->qmf_energy_buf[2 * i + 1][j]) *
            0.5f;
      }
    }
  }
}

VOID ixheaacd_hbe_repl_spec(WORD32 x_over_qmf[MAX_NUM_PATCHES],
                            FLOAT32 qmf_buf_real[][64],
                            FLOAT32 qmf_buf_imag[][64], WORD32 no_bins,
                            WORD32 max_stretch) {
  WORD32 patch_bands;
  WORD32 patch, band, col, target, source_bands, i;
  WORD32 num_patches = 0;

  for (i = 1; i < MAX_NUM_PATCHES; i++) {
    if (x_over_qmf[i] != 0) {
      num_patches++;
    }
  }

  for (patch = (max_stretch - 1); patch < num_patches; patch++) {
    patch_bands = x_over_qmf[patch + 1] - x_over_qmf[patch];
    target = x_over_qmf[patch];
    source_bands = x_over_qmf[max_stretch - 1] - x_over_qmf[max_stretch - 2];
    while (patch_bands > 0) {
      WORD32 ixheaacd_num_bands = source_bands;
      WORD32 start_band = x_over_qmf[max_stretch - 1] - 1;
      if (target + ixheaacd_num_bands >= x_over_qmf[patch + 1]) {
        ixheaacd_num_bands = x_over_qmf[patch + 1] - target;
      }
      if ((((target + ixheaacd_num_bands - 1) & 1) +
           ((x_over_qmf[max_stretch - 1] - 1) & 1)) &
          1) {
        if (ixheaacd_num_bands == source_bands) {
          ixheaacd_num_bands--;
        } else {
          start_band--;
        }
      }
      for (col = 0; col < no_bins; col++) {
        WORD32 i = 0;
        band = target + ixheaacd_num_bands - 1;
        if (64 <= band) {
          band = 63;
        }
        if (x_over_qmf[patch + 1] <= band) {
          band = x_over_qmf[patch + 1] - 1;
        }
        for (i = 0; i < ixheaacd_num_bands; i++, band--) {
          qmf_buf_real[col][band] = qmf_buf_real[col][start_band - i];
          qmf_buf_imag[col][band] = qmf_buf_imag[col][start_band - i];
        }
      }
      target += ixheaacd_num_bands;
      patch_bands -= ixheaacd_num_bands;
    }
  }
}

VOID ixheaacd_esbr_analysis_filt_block(ia_sbr_dec_struct *ptr_sbr_dec,
                                       ia_sbr_tables_struct *sbr_tables_ptr,
                                       WORD32 op_delay) {
  FLOAT32 *core_coder_samples;
  WORD32 *ptr_filt_states;
  WORD32 *ptr_filt_states_1;
  WORD32 *ptr_filt_states_2;
  WORD32 *ptr_temp;
  WORD32 *ptr_win_coeffs_1;
  WORD32 *ptr_win_coeffs_2;
  WORD32 *ptr_win_coeffs;
  WORD32 *ploc_qmf_buf_real;
  WORD32 *ploc_qmf_buf_imag;
  WORD32 local_qmf_buffer[128] = {0};
  WORD32 anal_buf[2 * 32];
  WORD32 idx, z;
  WORD32 core_syn_ch_index;
  FLOAT32 gain;
  WORD32 filt_offset;
  WORD32 num_columns;
  ia_qmf_dec_tables_struct *qmf_dec_tables_ptr =
      sbr_tables_ptr->qmf_dec_tables_ptr;
  ia_sbr_qmf_filter_bank_struct *pstr_qmf_anal_bank =
      &ptr_sbr_dec->str_codec_qmf_bank;
  core_coder_samples = ptr_sbr_dec->time_sample_buf;
  ptr_filt_states = pstr_qmf_anal_bank->state_new_samples_pos_low_32;
  ptr_win_coeffs_1 = pstr_qmf_anal_bank->filter_pos_32;
  num_columns = pstr_qmf_anal_bank->no_channels;

  switch (num_columns) {
    case 16:
      ptr_win_coeffs_2 = ptr_win_coeffs_1 + 64;
      gain = 128.0f;
      filt_offset = 64;
      break;
    case 24:
      ptr_win_coeffs_2 = ptr_win_coeffs_1 + 24;
      gain = 12.0f;
      filt_offset = 24;
      break;
    case 32:
      ptr_win_coeffs_2 = ptr_win_coeffs_1 + 64;
      gain = 256.0f;
      filt_offset = 64;
      break;
    default:
      ptr_win_coeffs_2 = ptr_win_coeffs_1 + 64;
      gain = 256.0f;
      filt_offset = 64;
      break;
  }
  gain = 1.0f / gain;

  pstr_qmf_anal_bank->usb = num_columns;

  ploc_qmf_buf_real = &local_qmf_buffer[0];
  ploc_qmf_buf_imag = &local_qmf_buffer[64];

  ptr_filt_states_1 = pstr_qmf_anal_bank->anal_filter_states_32;
  ptr_filt_states_2 = pstr_qmf_anal_bank->anal_filter_states_32 + num_columns;

  for (idx = 0; idx < ptr_sbr_dec->str_codec_qmf_bank.num_time_slots; idx++) {
    for (z = 0; z < num_columns; z++) {
      ptr_filt_states[num_columns - 1 - z] =
          (WORD32)(core_coder_samples[z] * (1 << 15));
    }
    ixheaacd_esbr_qmfanal32_winadd(ptr_filt_states_1, ptr_filt_states_2,
                                   ptr_win_coeffs_1, ptr_win_coeffs_2, anal_buf,
                                   num_columns);

    core_coder_samples += num_columns;

    ptr_filt_states -= num_columns;
    if (ptr_filt_states < pstr_qmf_anal_bank->anal_filter_states_32) {
      ptr_filt_states = pstr_qmf_anal_bank->anal_filter_states_32 +
                        10 * num_columns - num_columns;
    }

    ptr_temp = ptr_filt_states_1;
    ptr_filt_states_1 = ptr_filt_states_2;
    ptr_filt_states_2 = ptr_temp;

    ptr_win_coeffs_1 += filt_offset;
    ptr_win_coeffs_2 += filt_offset;

    ptr_win_coeffs = ptr_win_coeffs_1;
    ptr_win_coeffs_1 = ptr_win_coeffs_2;
    ptr_win_coeffs_2 = ptr_win_coeffs;

    if (ptr_win_coeffs_2 >
        (pstr_qmf_anal_bank->analy_win_coeff_32 + filt_offset * 10)) {
      ptr_win_coeffs_1 = pstr_qmf_anal_bank->analy_win_coeff_32;
      ptr_win_coeffs_2 = pstr_qmf_anal_bank->analy_win_coeff_32 + filt_offset;
    }

    ixheaacd_esbr_fwd_modulation(anal_buf, &ploc_qmf_buf_real[0],
                                 &ploc_qmf_buf_imag[0], pstr_qmf_anal_bank,
                                 qmf_dec_tables_ptr);
    core_syn_ch_index = num_columns;

    for (z = 0; z < core_syn_ch_index; z++) {
      ptr_sbr_dec->qmf_buf_real[op_delay + idx][z] =
          ((FLOAT32)ploc_qmf_buf_real[z] * gain);
      ptr_sbr_dec->qmf_buf_imag[op_delay + idx][z] =
          ((FLOAT32)ploc_qmf_buf_imag[z] * gain);
    }
    for (z = core_syn_ch_index; z < num_columns; z++) {
      ptr_sbr_dec->qmf_buf_real[op_delay + idx][z] = (FLOAT32)0.0f;
      ptr_sbr_dec->qmf_buf_imag[op_delay + idx][z] = (FLOAT32)0.0f;
    }
  }

  pstr_qmf_anal_bank->filter_pos_32 = ptr_win_coeffs_1;
  pstr_qmf_anal_bank->state_new_samples_pos_low_32 = ptr_filt_states;
}

VOID ixheaacd_esbr_synthesis_regrp(
    FLOAT32 *qmf_buf_real, FLOAT32 *qmf_buf_imag,
    ia_sbr_dec_struct *ptr_sbr_dec,
    ia_sbr_frame_info_data_struct *ptr_frame_data,
    ia_sbr_header_data_struct *ptr_header_data, WORD32 stereo_config_idx,
    WORD32 apply_processing) {
  WORD32 i, k;
  WORD32 stop_border = 0;
  WORD32 num_anal_bands = ptr_sbr_dec->str_codec_qmf_bank.no_channels;
  WORD32 x_over_band = num_anal_bands;

  if (apply_processing) {
    if (ptr_header_data->sbr_ratio_idx == SBR_UPSAMPLE_IDX_4_1) {
      stop_border = 4 * ptr_frame_data->str_frame_info_details.border_vec[0];
    } else {
      stop_border = 2 * ptr_frame_data->str_frame_info_details.border_vec[0];
    }
    x_over_band = ptr_header_data->pstr_freq_band_data->qmf_sb_prev;
  }

  if (stereo_config_idx > 0) {
    for (i = 0; i < stop_border; i++) {
      for (k = 0; k < 3; k++) {
        *qmf_buf_real++ =
            ptr_sbr_dec->qmf_buf_real[(2) + i + HYBRID_FILTER_DELAY][k];
        *qmf_buf_imag++ =
            ptr_sbr_dec->qmf_buf_imag[(2) + i + HYBRID_FILTER_DELAY][k];
      }

      for (; k < x_over_band; k++) {
        *qmf_buf_real++ = ptr_sbr_dec->qmf_buf_real[(2) + i][k];
        *qmf_buf_imag++ = ptr_sbr_dec->qmf_buf_imag[(2) + i][k];
      }

      for (; k < 64; k++) {
        *qmf_buf_real++ = ptr_sbr_dec->sbr_qmf_out_real[(2) + i][k];
        *qmf_buf_imag++ = ptr_sbr_dec->sbr_qmf_out_imag[(2) + i][k];
      }

      qmf_buf_real += 14;
      qmf_buf_imag += 14;
    }

    x_over_band = ptr_header_data->pstr_freq_band_data->sub_band_start;

    for (; i < ptr_sbr_dec->str_codec_qmf_bank.num_time_slots; i++) {
      for (k = 0; k < 3; k++) {
        *qmf_buf_real++ =
            ptr_sbr_dec->qmf_buf_real[(2) + i + HYBRID_FILTER_DELAY][k];
        *qmf_buf_imag++ =
            ptr_sbr_dec->qmf_buf_imag[(2) + i + HYBRID_FILTER_DELAY][k];
      }

      for (; k < x_over_band; k++) {
        *qmf_buf_real++ = ptr_sbr_dec->qmf_buf_real[(2) + i][k];
        *qmf_buf_imag++ = ptr_sbr_dec->qmf_buf_imag[(2) + i][k];
      }

      for (; k < 64; k++) {
        *qmf_buf_real++ = ptr_sbr_dec->sbr_qmf_out_real[(2) + i][k];
        *qmf_buf_imag++ = ptr_sbr_dec->sbr_qmf_out_imag[(2) + i][k];
      }

      qmf_buf_real += 14;
      qmf_buf_imag += 14;
    }

  } else {
    for (i = 0; i < stop_border; i++) {
      for (k = 0; k < x_over_band; k++) {
        *qmf_buf_real++ = ptr_sbr_dec->qmf_buf_real[(2) + i][k];
        *qmf_buf_imag++ = ptr_sbr_dec->qmf_buf_imag[(2) + i][k];
      }

      for (; k < 64; k++) {
        *qmf_buf_real++ = ptr_sbr_dec->sbr_qmf_out_real[(2) + i][k];
        *qmf_buf_imag++ = ptr_sbr_dec->sbr_qmf_out_imag[(2) + i][k];
      }

      qmf_buf_real += 14;
      qmf_buf_imag += 14;
    }

    x_over_band = ptr_header_data->pstr_freq_band_data->sub_band_start;

    for (; i < ptr_sbr_dec->str_codec_qmf_bank.num_time_slots; i++) {
      for (k = 0; k < x_over_band; k++) {
        *qmf_buf_real++ = ptr_sbr_dec->qmf_buf_real[(2) + i][k];
        *qmf_buf_imag++ = ptr_sbr_dec->qmf_buf_imag[(2) + i][k];
      }

      for (; k < 64; k++) {
        *qmf_buf_real++ = ptr_sbr_dec->sbr_qmf_out_real[(2) + i][k];
        *qmf_buf_imag++ = ptr_sbr_dec->sbr_qmf_out_imag[(2) + i][k];
      }

      qmf_buf_real += 14;
      qmf_buf_imag += 14;
    }
  }
}

VOID ixheaacd_mps_esbr_synthesis_regrp(FLOAT32 *qmf_buf_real,
                                       FLOAT32 *qmf_buf_imag,
                                       ia_sbr_dec_struct *ptr_sbr_dec,
                                       WORD32 stereo_config_idx) {
  WORD32 i, k;
  WORD32 num_anal_bands = ptr_sbr_dec->str_codec_qmf_bank.no_channels;
  WORD32 x_over_band = num_anal_bands;

  if (stereo_config_idx > 0) {
    for (i = 0; i < ptr_sbr_dec->str_codec_qmf_bank.num_time_slots; i++) {
      for (k = 0; k < 3; k++) {
        *qmf_buf_real++ =
            ptr_sbr_dec->qmf_buf_real[(2) + i + HYBRID_FILTER_DELAY][k];
        *qmf_buf_imag++ =
            ptr_sbr_dec->qmf_buf_imag[(2) + i + HYBRID_FILTER_DELAY][k];
      }

      for (; k < x_over_band; k++) {
        *qmf_buf_real++ = ptr_sbr_dec->qmf_buf_real[(2) + i][k];
        *qmf_buf_imag++ = ptr_sbr_dec->qmf_buf_imag[(2) + i][k];
      }

      for (; k < 64; k++) {
        *qmf_buf_real++ = 0;
        *qmf_buf_imag++ = 0;
      }

      qmf_buf_real += 14;
      qmf_buf_imag += 14;
    }
  } else {
    for (i = 0; i < ptr_sbr_dec->str_codec_qmf_bank.num_time_slots; i++) {
      for (k = 0; k < x_over_band; k++) {
        *qmf_buf_real++ = ptr_sbr_dec->qmf_buf_real[(2) + i][k];
        *qmf_buf_imag++ = ptr_sbr_dec->qmf_buf_imag[(2) + i][k];
      }

      for (; k < 64; k++) {
        *qmf_buf_real++ = 0.0f;
        *qmf_buf_imag++ = 0.0f;
      }

      qmf_buf_real += 14;
      qmf_buf_imag += 14;
    }
  }
}

VOID ixheaacd_esbr_synthesis_filt_block(
    ia_sbr_dec_struct *ptr_sbr_dec, ia_sbr_header_data_struct *ptr_header_data,
    ia_sbr_frame_info_data_struct *ptr_frame_data, WORD32 apply_processing,
    FLOAT32 **qmf_buf_real, FLOAT32 **qmf_buf_imag, WORD32 stereo_config_idx,
    ia_sbr_tables_struct *sbr_tables_ptr, WORD32 mps_sbr_flag, WORD32 ch_fac) {
  WORD32 i, k, p1;
  WORD32 *ptr_filt_states;
  WORD32 *ptr_filt_states_1;
  WORD32 *ptr_filt_states_2;
  WORD32 *filter_l;
  WORD32 *ploc_qmf_buf_real;
  WORD32 *ploc_qmf_buf_imag;
  WORD32 out_scalefactor;
  WORD32 sixty4;
  WORD32 no_synthesis_channels;
  WORD32 ixheaacd_drc_offset;
  FLOAT32 *syn_buffer;
  WORD32 *local_qmf_buffer = ptr_sbr_dec->sbr_scratch_local;
  WORD32 *time_out = &(ptr_sbr_dec->sbr_scratch_local[128]);

  ia_sbr_qmf_filter_bank_struct *qmf_bank =
      &ptr_sbr_dec->str_synthesis_qmf_bank;
  ia_qmf_dec_tables_struct *qmf_dec_tables_ptr =
      sbr_tables_ptr->qmf_dec_tables_ptr;

  if (!mps_sbr_flag) {
    ixheaacd_esbr_synthesis_regrp(&qmf_buf_real[0][0], &qmf_buf_imag[0][0],
                                  ptr_sbr_dec, ptr_frame_data, ptr_header_data,
                                  stereo_config_idx, apply_processing);
  } else {
    ixheaacd_mps_esbr_synthesis_regrp(&qmf_buf_real[0][0], &qmf_buf_imag[0][0],
                                      ptr_sbr_dec, stereo_config_idx);
  }

  out_scalefactor = 5;
  qmf_bank->no_channels = 64;
  qmf_bank->esbr_cos_twiddle =
      (WORD32 *)qmf_dec_tables_ptr->esbr_sin_cos_twiddle_l64;
  qmf_bank->esbr_alt_sin_twiddle =
      (WORD32 *)qmf_dec_tables_ptr->esbr_alt_sin_twiddle_l64;

  qmf_bank->filter_pos_syn_32 +=
      qmf_dec_tables_ptr->esbr_qmf_c - qmf_bank->p_filter_32;
  qmf_bank->p_filter_32 = qmf_dec_tables_ptr->esbr_qmf_c;

  sixty4 = NO_SYNTHESIS_CHANNELS;

  ptr_filt_states = qmf_bank->filter_states_32;

  no_synthesis_channels = qmf_bank->no_channels;
  ptr_filt_states_1 = &ptr_filt_states[0];
  ptr_filt_states_2 = ptr_filt_states_1 + no_synthesis_channels;

  filter_l = qmf_bank->filter_pos_syn_32;

  p1 = 0;

  ixheaacd_drc_offset = qmf_bank->ixheaacd_drc_offset;

  for (i = 0; i < ptr_sbr_dec->str_codec_qmf_bank.num_time_slots; i++) {
    for (k = 0; k < 64; k++) {
      local_qmf_buffer[k + 0] = (WORD32)(qmf_buf_real[i][k] * 64);
      local_qmf_buffer[k + 64] = (WORD32)(qmf_buf_imag[i][k] * 64);
    }
    ploc_qmf_buf_real = local_qmf_buffer;
    ploc_qmf_buf_imag = local_qmf_buffer + 64;

    ixheaacd_esbr_inv_modulation(ploc_qmf_buf_real,
                                 &ptr_sbr_dec->str_synthesis_qmf_bank,
                                 sbr_tables_ptr->qmf_dec_tables_ptr);

    ixheaacd_shiftrountine_with_rnd_hq(ploc_qmf_buf_real, ploc_qmf_buf_imag,
                                       &ptr_filt_states[ixheaacd_drc_offset],
                                       no_synthesis_channels,
                                       out_scalefactor + 1);

    ixheaacd_esbr_qmfsyn64_winadd(ptr_filt_states_1, ptr_filt_states_2,
                                  filter_l, &time_out[0], ch_fac);

    syn_buffer = ptr_sbr_dec->time_sample_buf + i * 64;
    for (k = 0; k < 64; k++) {
      syn_buffer[k] = (FLOAT32)time_out[k] / (1 << 16);
    }

    ptr_filt_states_1 += sixty4;
    ptr_filt_states_2 -= sixty4;
    sixty4 = -sixty4;
    ixheaacd_drc_offset -= 128;

    if (ixheaacd_drc_offset < 0) ixheaacd_drc_offset += 1280;

    filter_l += 64;

    if (filter_l == qmf_bank->p_filter_32 + 640)
      filter_l = (WORD32 *)qmf_bank->p_filter_32;

    p1 += no_synthesis_channels;
  }

  qmf_bank->filter_pos_syn_32 = filter_l;
  qmf_bank->ixheaacd_drc_offset = ixheaacd_drc_offset;

  if (!mps_sbr_flag) ptr_frame_data->reset_flag = 0;

  if (apply_processing && !mps_sbr_flag) {
    ptr_header_data->pstr_freq_band_data->qmf_sb_prev =
        ptr_header_data->pstr_freq_band_data->sub_band_start;
  }
}

WORD32 ixheaacd_sbr_dec(ia_sbr_dec_struct *ptr_sbr_dec, WORD16 *ptr_time_data,
                        ia_sbr_header_data_struct *ptr_header_data,
                        ia_sbr_frame_info_data_struct *ptr_frame_data,
                        ia_sbr_prev_frame_data_struct *ptr_frame_data_prev,
                        ia_ps_dec_struct *ptr_ps_dec,
                        ia_sbr_qmf_filter_bank_struct *ptr_qmf_synth_bank_r,
                        ia_sbr_scale_fact_struct *ptr_sbr_sf_r,
                        FLAG apply_processing, FLAG low_pow_flag,
                        WORD32 *ptr_work_buf_core,
                        ia_sbr_tables_struct *sbr_tables_ptr,
                        ixheaacd_misc_tables *pstr_common_tables, WORD ch_fac,
                        ia_pvc_data_struct *ptr_pvc_data, FLAG drc_on,
                        WORD32 drc_sbr_factors[][64],
                        WORD32 audio_object_type) {
  WORD i;
  WORD slot, reserve;
  WORD save_lb_scale;
  WORD op_delay;

  WORD32 *p_arr_qmf_buf_real[MAX_ENV_COLS + MAX_ENV_COLS];
  WORD32 **p_arr_qmf_buf_imag = &p_arr_qmf_buf_real[MAX_ENV_COLS];
  WORD32 *ptr;
  WORD hbe_flag = ptr_header_data->hbe_flag;

  FLOAT32 **pp_qmf_buf_real = NULL;
  FLOAT32 **pp_qmf_buf_imag = NULL;
  FLOAT32 pvc_dec_out_buf[16 * 64];

  WORD upsample_ratio_idx = ptr_header_data->sbr_ratio_idx;
  WORD no_bins;
  WORD mps_sbr_flag = ptr_frame_data->mps_sbr_flag;
  WORD stereo_config_idx = ptr_frame_data->stereo_config_idx;
  WORD sbr_mode = ptr_frame_data->sbr_mode;
  WORD usac_flag = ptr_header_data->usac_flag;

  FLOAT32 *pvc_qmf_enrg_arr = (FLOAT32 *)ptr_sbr_dec->pvc_qmf_enrg_arr;

  memset(pvc_dec_out_buf, 0, 1024 * sizeof(FLOAT32));
  memset(pvc_qmf_enrg_arr, 0, 512 * sizeof(FLOAT32));
  if (audio_object_type == AOT_ER_AAC_ELD) {
    op_delay = 0;
  } else {
    op_delay = 6;
  }
  if (usac_flag) {
    pp_qmf_buf_real = ptr_sbr_dec->pp_qmf_buf_real;
    pp_qmf_buf_imag = ptr_sbr_dec->pp_qmf_buf_imag;
    if (upsample_ratio_idx == SBR_UPSAMPLE_IDX_4_1) {
      op_delay = 2 * 6;
    }
  }

  no_bins = (ptr_header_data->num_time_slots * ptr_header_data->time_step);

  if (!usac_flag) {
    WORD32 num = op_delay;
    WORD32 *ptr_pers_qmf_real = ptr_sbr_dec->ptr_sbr_overlap_buf;
    WORD32 *p_scr_qmf_real = ptr_work_buf_core + (2 << (6 + !low_pow_flag));
    if (!low_pow_flag) {
      num = num << 1;
    }
    if (audio_object_type != AOT_ER_AAC_ELD) {
      memcpy(p_scr_qmf_real, ptr_pers_qmf_real,
             sizeof(WORD32) * NO_SYNTHESIS_CHANNELS * num);
    }
    ptr = p_scr_qmf_real;

    for (slot = 0; slot < op_delay + no_bins; slot++) {
      p_arr_qmf_buf_real[slot] = ptr;
      ptr += NO_SYNTHESIS_CHANNELS;

      if (!low_pow_flag) {
        p_arr_qmf_buf_imag[slot] = ptr;
        ptr += NO_SYNTHESIS_CHANNELS;
      }
    }

    ptr_sbr_dec->str_sbr_scale_fact.lb_scale = 0;

    if (apply_processing) {
      ixheaacd_rescale_x_overlap(ptr_sbr_dec, ptr_header_data, ptr_frame_data,
                                 ptr_frame_data_prev, p_arr_qmf_buf_real,
                                 low_pow_flag);
    }
  }

  if (usac_flag) {
    WORD32 num_anal_bands = ptr_sbr_dec->str_codec_qmf_bank.no_channels;
    WORD32 codec_x_delay = 0;
    WORD32 core_syn_ch_index = 0;
    WORD32 frame_move = 288;
    WORD32 core_frame_size = ptr_header_data->core_frame_size;

    if (hbe_flag) {
      codec_x_delay = 32;
    }
    if (upsample_ratio_idx == SBR_UPSAMPLE_IDX_4_1) {
      codec_x_delay = 2 * codec_x_delay;
    }

    if (hbe_flag || mps_sbr_flag) {
      core_syn_ch_index = num_anal_bands;
    } else {
      core_syn_ch_index = ptr_header_data->pstr_freq_band_data->sub_band_start;
    }

    frame_move = 9 * num_anal_bands;

    memmove(ptr_sbr_dec->core_sample_buf,
            ptr_sbr_dec->core_sample_buf + core_frame_size,
            frame_move * sizeof(FLOAT32));
    memcpy(&ptr_sbr_dec->core_sample_buf[frame_move],
           &ptr_sbr_dec->time_sample_buf[0], core_frame_size * sizeof(FLOAT32));

    memmove(
        &ptr_sbr_dec->qmf_buf_real[0][0],
        &ptr_sbr_dec
             ->qmf_buf_real[ptr_sbr_dec->str_codec_qmf_bank.num_time_slots][0],
        (op_delay + SBR_HF_ADJ_OFFSET + codec_x_delay) * sizeof(FLOAT32) * 64);

    memmove(
        &ptr_sbr_dec->qmf_buf_imag[0][0],
        &ptr_sbr_dec
             ->qmf_buf_imag[ptr_sbr_dec->str_codec_qmf_bank.num_time_slots][0],
        (op_delay + SBR_HF_ADJ_OFFSET + codec_x_delay) * sizeof(FLOAT32) * 64);

    memmove(&ptr_sbr_dec->sbr_qmf_out_real[0][0],
            &ptr_sbr_dec->sbr_qmf_out_real[ptr_sbr_dec->str_codec_qmf_bank
                                               .num_time_slots][0],
            (op_delay + SBR_HF_ADJ_OFFSET) * sizeof(FLOAT32) * 64);

    memmove(&ptr_sbr_dec->sbr_qmf_out_imag[0][0],
            &ptr_sbr_dec->sbr_qmf_out_imag[ptr_sbr_dec->str_codec_qmf_bank
                                               .num_time_slots][0],
            (op_delay + SBR_HF_ADJ_OFFSET) * sizeof(FLOAT32) * 64);

    if (hbe_flag) {
      memmove(&ptr_sbr_dec->ph_vocod_qmf_real[0][0],
              &ptr_sbr_dec->ph_vocod_qmf_real[ptr_sbr_dec->str_codec_qmf_bank
                                                  .num_time_slots][0],
              64 * sizeof(FLOAT32) * (op_delay + SBR_HF_ADJ_OFFSET));

      memmove(ptr_sbr_dec->ph_vocod_qmf_imag,
              ptr_sbr_dec->ph_vocod_qmf_imag +
                  ptr_sbr_dec->str_codec_qmf_bank.num_time_slots,
              64 * sizeof(FLOAT32) * (op_delay + SBR_HF_ADJ_OFFSET));
    }
    ixheaacd_esbr_analysis_filt_block(
        ptr_sbr_dec, sbr_tables_ptr,
        op_delay + codec_x_delay + SBR_HF_ADJ_OFFSET);

    if (hbe_flag) {
      WORD32 err_code = ixheaacd_qmf_hbe_apply(
          ptr_sbr_dec->p_hbe_txposer,
          ptr_sbr_dec->qmf_buf_real + (op_delay + SBR_HF_ADJ_OFFSET) +
              ESBR_HBE_DELAY_OFFSET,
          ptr_sbr_dec->qmf_buf_imag + (op_delay + SBR_HF_ADJ_OFFSET) +
              ESBR_HBE_DELAY_OFFSET,
          ptr_sbr_dec->str_codec_qmf_bank.num_time_slots,
          ptr_sbr_dec->ph_vocod_qmf_real + (op_delay + SBR_HF_ADJ_OFFSET),
          ptr_sbr_dec->ph_vocod_qmf_imag + (op_delay + SBR_HF_ADJ_OFFSET),
          ptr_frame_data->pitch_in_bins);
      if (err_code) return err_code;

      if (upsample_ratio_idx == SBR_UPSAMPLE_IDX_4_1) {
        ixheaacd_hbe_repl_spec(
            &ptr_sbr_dec->p_hbe_txposer->x_over_qmf[0],
            ptr_sbr_dec->ph_vocod_qmf_real + (op_delay + SBR_HF_ADJ_OFFSET),
            ptr_sbr_dec->ph_vocod_qmf_imag + (op_delay + SBR_HF_ADJ_OFFSET),
            ptr_sbr_dec->str_codec_qmf_bank.num_time_slots,
            ptr_sbr_dec->p_hbe_txposer->max_stretch);
      }
    }
    ixheaacd_qmf_enrg_calc(ptr_sbr_dec, upsample_ratio_idx, low_pow_flag);

    if (!mps_sbr_flag && apply_processing) {
      WORD32 err_code = 0;
      err_code = ixheaacd_generate_hf(
          ptr_sbr_dec->qmf_buf_real + (SBR_HF_ADJ_OFFSET),
          ptr_sbr_dec->qmf_buf_imag + (SBR_HF_ADJ_OFFSET),
          ptr_sbr_dec->ph_vocod_qmf_real + (SBR_HF_ADJ_OFFSET),
          ptr_sbr_dec->ph_vocod_qmf_imag + (SBR_HF_ADJ_OFFSET),
          ptr_sbr_dec->sbr_qmf_out_real + (SBR_HF_ADJ_OFFSET),
          ptr_sbr_dec->sbr_qmf_out_imag + (SBR_HF_ADJ_OFFSET), ptr_frame_data,
          ptr_header_data);
      if (err_code) return err_code;

      ptr_pvc_data->pvc_rate = ptr_header_data->upsamp_fac;

      if (sbr_mode == PVC_SBR) {
        ixheaacd_pvc_process(
            ptr_pvc_data, ptr_header_data->pstr_freq_band_data->sub_band_start,
            ptr_frame_data->str_pvc_frame_info.border_vec[0],
            &pvc_qmf_enrg_arr[0], &pvc_dec_out_buf[0]);
        ptr_pvc_data->prev_pvc_flg = 1;
      } else {
        memset(pvc_dec_out_buf, 0, 1024 * sizeof(FLOAT32));
        ptr_pvc_data->prev_pvc_flg = 0;
      }

      ptr_pvc_data->prev_first_bnd_idx =
          ptr_header_data->pstr_freq_band_data->sub_band_start;
      ptr_pvc_data->prev_pvc_rate = ptr_pvc_data->pvc_rate;

      ptr_frame_data->pstr_sbr_header = ptr_header_data;
      err_code = ixheaacd_sbr_env_calc(
          ptr_frame_data, ptr_sbr_dec->sbr_qmf_out_real + (SBR_HF_ADJ_OFFSET),
          ptr_sbr_dec->sbr_qmf_out_imag + (SBR_HF_ADJ_OFFSET),
          ptr_sbr_dec->qmf_buf_real + (SBR_HF_ADJ_OFFSET),
          ptr_sbr_dec->qmf_buf_imag + (SBR_HF_ADJ_OFFSET),
          (ptr_header_data->hbe_flag == 0)
              ? NULL
              : ptr_sbr_dec->p_hbe_txposer->x_over_qmf,
          ptr_sbr_dec->scratch_buff, pvc_dec_out_buf);
      if (err_code) return err_code;

    } else {
      for (i = 0; i < 64; i++) {
        memset(ptr_sbr_dec->sbr_qmf_out_real[i], 0, 64 * sizeof(FLOAT32));
        memset(ptr_sbr_dec->sbr_qmf_out_imag[i], 0, 64 * sizeof(FLOAT32));
      }
    }

    ixheaacd_esbr_synthesis_filt_block(
        ptr_sbr_dec, ptr_header_data, ptr_frame_data, apply_processing,
        pp_qmf_buf_real, pp_qmf_buf_imag, stereo_config_idx, sbr_tables_ptr,
        mps_sbr_flag, ch_fac);

    ptr_frame_data->prev_sbr_mode = sbr_mode;

    return 0;
  }

  ixheaacd_cplx_anal_qmffilt(
      ptr_time_data, &ptr_sbr_dec->str_sbr_scale_fact,
      &p_arr_qmf_buf_real[op_delay], &p_arr_qmf_buf_imag[op_delay],
      &ptr_sbr_dec->str_codec_qmf_bank, sbr_tables_ptr->qmf_dec_tables_ptr,
      ch_fac, low_pow_flag, audio_object_type);

  {
    WORD shift1, shift2;
    WORD min_shift;
    WORD shift_over;
    WORD reserve_ov1, reserve_ov2;
    WORD reservea[2];
    WORD i = 0;
    WORD usb = ptr_sbr_dec->str_codec_qmf_bank.usb;
    WORD iter_val = 1;
    if (audio_object_type == AOT_ER_AAC_ELD ||
        audio_object_type == AOT_ER_AAC_LD) {
      iter_val = 0;
    }
    do {
      WORD t1 = op_delay;
      WORD t2 = no_bins + op_delay;
      if (i) {
        t1 = 0;
        t2 = op_delay;
      }
      reservea[i] = (*ixheaacd_ixheaacd_expsubbandsamples)(
          p_arr_qmf_buf_real, p_arr_qmf_buf_imag, 0, usb, t1, t2, low_pow_flag);
      i++;
    } while (i <= iter_val);
    ;

    reserve = reservea[0];
    if (audio_object_type != AOT_ER_AAC_ELD &&
        audio_object_type != AOT_ER_AAC_LD)
      reserve_ov1 = reservea[1];
    else
      reserve_ov1 = reserve;
    ptr_sbr_dec->max_samp_val = ixheaacd_min32(reserve, reserve_ov1);

    reserve_ov2 = (*ixheaacd_ixheaacd_expsubbandsamples)(
        ptr_sbr_dec->str_hf_generator.lpc_filt_states_real,
        ptr_sbr_dec->str_hf_generator.lpc_filt_states_imag, 0, usb, 0,
        LPC_ORDER, low_pow_flag);

    reserve_ov1 = ixheaacd_min32(reserve_ov1, reserve_ov2);

    shift1 = ptr_sbr_dec->str_sbr_scale_fact.lb_scale + reserve;

    shift2 = ptr_sbr_dec->str_sbr_scale_fact.ov_lb_scale + reserve_ov1;
    min_shift = ixheaacd_min32(shift1, shift2);
    shift_over = (shift2 - min_shift);
    reserve -= (shift1 - min_shift);

    ptr_sbr_dec->str_sbr_scale_fact.ov_lb_scale += (reserve_ov1 - shift_over);

    (*ixheaacd_adjust_scale)(p_arr_qmf_buf_real, p_arr_qmf_buf_imag, 0, usb, 0,
                             op_delay, reserve_ov1 - shift_over, low_pow_flag);

    (*ixheaacd_adjust_scale)(p_arr_qmf_buf_real, p_arr_qmf_buf_imag, 0, usb,
                             op_delay, (no_bins + op_delay), reserve,
                             low_pow_flag);

    (*ixheaacd_adjust_scale)(ptr_sbr_dec->str_hf_generator.lpc_filt_states_real,
                             ptr_sbr_dec->str_hf_generator.lpc_filt_states_imag,
                             0, usb, 0, LPC_ORDER, reserve_ov1 - shift_over,
                             low_pow_flag);

    ptr_sbr_dec->str_sbr_scale_fact.lb_scale += reserve;

    save_lb_scale = ptr_sbr_dec->str_sbr_scale_fact.lb_scale;
  }

  {
    WORD32 num = no_bins;
    WORD32 *p_loc_qmf_real =
        &p_arr_qmf_buf_real[op_delay][NO_ANALYSIS_CHANNELS];

    if (!low_pow_flag) {
      num = num << 1;
    }

    ixheaacd_clr_subsamples(p_loc_qmf_real, num - 1, (NO_SYN_ANA_CHANNELS));
  }

  if (apply_processing) {
    WORD16 degree_alias[NO_SYNTHESIS_CHANNELS];
    WORD16 *border_vec = ptr_frame_data->str_frame_info_details.border_vec;

    if (low_pow_flag) {
      memset(degree_alias, 0, NO_SYNTHESIS_CHANNELS * sizeof(WORD16));
    }

    if (low_pow_flag) {
      WORD32 com_low_band_scale;
      ixheaacd_low_pow_hf_generator(
          &ptr_sbr_dec->str_hf_generator, p_arr_qmf_buf_real, degree_alias,
          border_vec[0] * ptr_header_data->time_step,
          ptr_header_data->time_step *
              ixheaacd_sub16_sat(
                  border_vec[ptr_frame_data->str_frame_info_details.num_env],
                  ptr_header_data->num_time_slots),
          ptr_header_data->pstr_freq_band_data->num_if_bands,
          ptr_frame_data->max_qmf_subband_aac, ptr_frame_data->sbr_invf_mode,
          ptr_frame_data_prev->sbr_invf_mode, ptr_sbr_dec->max_samp_val,
          ptr_work_buf_core);

      com_low_band_scale =
          ixheaacd_min32(ptr_sbr_dec->str_sbr_scale_fact.ov_lb_scale,
                         ptr_sbr_dec->str_sbr_scale_fact.lb_scale);

      ptr_sbr_dec->str_sbr_scale_fact.hb_scale =
          (WORD16)(com_low_band_scale - 2);
    } else {
      ixheaacd_hf_generator(
          &ptr_sbr_dec->str_hf_generator, &ptr_sbr_dec->str_sbr_scale_fact,
          p_arr_qmf_buf_real, p_arr_qmf_buf_imag, ptr_header_data->time_step,
          border_vec[0],
          ixheaacd_sub16_sat(
              border_vec[ptr_frame_data->str_frame_info_details.num_env],
              ptr_header_data->num_time_slots),
          ptr_header_data->pstr_freq_band_data->num_if_bands,
          ptr_frame_data->max_qmf_subband_aac, ptr_frame_data->sbr_invf_mode,
          ptr_frame_data_prev->sbr_invf_mode, ptr_work_buf_core,
          audio_object_type);
    }

    ixheaacd_calc_sbrenvelope(
        &ptr_sbr_dec->str_sbr_scale_fact, &ptr_sbr_dec->str_sbr_calc_env,
        ptr_header_data, ptr_frame_data, ptr_frame_data_prev,
        p_arr_qmf_buf_real, p_arr_qmf_buf_imag, degree_alias, low_pow_flag,
        sbr_tables_ptr, pstr_common_tables,
        ptr_work_buf_core + (LPC_ORDER << (6 + !low_pow_flag)),
        audio_object_type);

    memcpy(ptr_frame_data_prev->sbr_invf_mode, ptr_frame_data->sbr_invf_mode,
           ptr_header_data->pstr_freq_band_data->num_if_bands * sizeof(WORD32));

    ptr_frame_data_prev->coupling_mode = ptr_frame_data->coupling_mode;
    ptr_frame_data_prev->max_qmf_subband_aac =
        ptr_frame_data->max_qmf_subband_aac;
    ptr_frame_data_prev->end_position =
        border_vec[ptr_frame_data->str_frame_info_details.num_env];
    ptr_frame_data_prev->amp_res = ptr_frame_data->amp_res;
  } else {
    ptr_sbr_dec->str_sbr_scale_fact.hb_scale = save_lb_scale;
  }

  if (!low_pow_flag) {
    for (i = 0; i < LPC_ORDER; i++) {
      WORD32 *p_loc_qmf_real = &p_arr_qmf_buf_real[no_bins - LPC_ORDER + i][0];
      WORD32 *p_loc_qmf_imag = &p_arr_qmf_buf_imag[no_bins - LPC_ORDER + i][0];
      WORD32 *plpc_filt_states_real =
          &ptr_sbr_dec->str_hf_generator.lpc_filt_states_real[i][0];
      WORD32 *plpc_filt_states_imag =
          &ptr_sbr_dec->str_hf_generator.lpc_filt_states_imag[i][0];

      memcpy(plpc_filt_states_real, p_loc_qmf_real,
             sizeof(WORD32) * (ptr_sbr_dec->str_codec_qmf_bank.usb));
      memcpy(plpc_filt_states_imag, p_loc_qmf_imag,
             sizeof(WORD32) * (ptr_sbr_dec->str_codec_qmf_bank.usb));
    }
  } else {
    for (i = 0; i < LPC_ORDER; i++) {
      WORD32 *p_loc_qmf_real = &p_arr_qmf_buf_real[no_bins - LPC_ORDER + i][0];
      WORD32 *plpc_filt_states_real =
          &ptr_sbr_dec->str_hf_generator.lpc_filt_states_real[i][0];
      memcpy(plpc_filt_states_real, p_loc_qmf_real,
             sizeof(WORD32) * (ptr_sbr_dec->str_codec_qmf_bank.usb));
    }
  }

  if (apply_processing && ptr_header_data->channel_mode == PS_STEREO &&
      ((audio_object_type != AOT_ER_AAC_ELD) &&
       (audio_object_type != AOT_ER_AAC_LD))) {
    WORD32 ps_scale;

    ixheaacd_init_ps_scale(ptr_ps_dec, &ptr_sbr_dec->str_sbr_scale_fact);

    ixheaacd_cplx_synt_qmffilt(p_arr_qmf_buf_real, p_arr_qmf_buf_imag, op_delay,
                               &ptr_sbr_dec->str_sbr_scale_fact, ptr_time_data,
                               &ptr_sbr_dec->str_synthesis_qmf_bank, ptr_ps_dec,
                               1, 0, sbr_tables_ptr, pstr_common_tables, ch_fac,
                               drc_on, drc_sbr_factors, audio_object_type);

    ps_scale = ptr_sbr_dec->str_sbr_scale_fact.ps_scale;
    ptr_sbr_sf_r->ov_lb_scale = ps_scale;
    ptr_sbr_sf_r->lb_scale = ps_scale;
    ptr_sbr_sf_r->hb_scale = ps_scale;

    ixheaacd_cplx_synt_qmffilt(p_arr_qmf_buf_real, p_arr_qmf_buf_imag, op_delay,
                               ptr_sbr_sf_r, ptr_time_data + 1,
                               ptr_qmf_synth_bank_r, ptr_ps_dec, 0, 0,
                               sbr_tables_ptr, pstr_common_tables, ch_fac,
                               drc_on, drc_sbr_factors, audio_object_type);
  } else {
    ixheaacd_cplx_synt_qmffilt(p_arr_qmf_buf_real, p_arr_qmf_buf_imag, op_delay,
                               &ptr_sbr_dec->str_sbr_scale_fact, ptr_time_data,
                               &ptr_sbr_dec->str_synthesis_qmf_bank, ptr_ps_dec,
                               0, low_pow_flag, sbr_tables_ptr,
                               pstr_common_tables, ch_fac, drc_on,
                               drc_sbr_factors, audio_object_type);
  }

  {
    WORD32 num = op_delay;
    WORD32 *p_loc_qmf_real = ptr_sbr_dec->ptr_sbr_overlap_buf;
    WORD32 *p_loc_qmf_real_1 = &p_arr_qmf_buf_real[no_bins][0];

    if (!low_pow_flag) {
      num = num << 1;
    }

    memcpy(p_loc_qmf_real, p_loc_qmf_real_1,
           sizeof(WORD32) * NO_SYNTHESIS_CHANNELS * num);
  }

  ptr_sbr_dec->str_sbr_scale_fact.ov_lb_scale = save_lb_scale;
  return 0;
}

WORD32 ixheaacd_esbr_dec(ia_sbr_dec_struct *ptr_sbr_dec,
                         ia_sbr_header_data_struct *ptr_header_data,
                         ia_sbr_frame_info_data_struct *ptr_frame_data,
                         FLAG apply_processing, FLAG low_pow_flag,
                         ia_sbr_tables_struct *ptr_sbr_tables, WORD ch_fac) {
  WORD32 i;
  WORD32 op_delay;

  WORD32 codec_x_delay = 0;

  FLOAT32 **pp_qmf_buf_real = ptr_sbr_dec->pp_qmf_buf_real;
  FLOAT32 **pp_qmf_buf_imag = ptr_sbr_dec->pp_qmf_buf_imag;

  WORD32 upsample_ratio_idx = ptr_header_data->sbr_ratio_idx;
  WORD32 core_frame_size = ptr_header_data->core_frame_size;

  WORD32 no_bins;
  WORD32 mps_sbr_flag = ptr_frame_data->mps_sbr_flag;
  WORD32 stereo_config_idx = ptr_frame_data->stereo_config_idx;
  WORD32 hbe_flag = ptr_header_data->hbe_flag;
  WORD32 sbr_mode = ptr_frame_data->sbr_mode;

  op_delay = 6;
  if (upsample_ratio_idx == SBR_UPSAMPLE_IDX_4_1) {
    op_delay = 2 * 6;
  }

  no_bins = ptr_sbr_dec->str_codec_qmf_bank.num_time_slots;

  ptr_sbr_dec->str_sbr_scale_fact.lb_scale = 0;
  {
    WORD32 num_anal_bands = ptr_sbr_dec->str_codec_qmf_bank.no_channels;
    WORD32 frame_move = 288;
    if (hbe_flag) {
      codec_x_delay = 32;
    }
    if (upsample_ratio_idx == SBR_UPSAMPLE_IDX_4_1) {
      codec_x_delay = 2 * codec_x_delay;
    }

    frame_move = 9 * num_anal_bands;

    memmove(ptr_sbr_dec->core_sample_buf,
            ptr_sbr_dec->core_sample_buf + core_frame_size,
            frame_move * sizeof(FLOAT32));

    memcpy(&ptr_sbr_dec->core_sample_buf[frame_move],
           &ptr_sbr_dec->time_sample_buf[0], core_frame_size * sizeof(FLOAT32));

    memmove(
        &ptr_sbr_dec->qmf_buf_real[0][0],
        &ptr_sbr_dec
             ->qmf_buf_real[ptr_sbr_dec->str_codec_qmf_bank.num_time_slots][0],
        (op_delay + SBR_HF_ADJ_OFFSET + codec_x_delay) * sizeof(FLOAT32) * 64);
    memmove(
        &ptr_sbr_dec->qmf_buf_imag[0][0],
        &ptr_sbr_dec
             ->qmf_buf_imag[ptr_sbr_dec->str_codec_qmf_bank.num_time_slots][0],
        (op_delay + SBR_HF_ADJ_OFFSET + codec_x_delay) * sizeof(FLOAT32) * 64);

    memmove(&ptr_sbr_dec->sbr_qmf_out_real[0][0],
            &ptr_sbr_dec->sbr_qmf_out_real[ptr_sbr_dec->str_codec_qmf_bank
                                               .num_time_slots][0],
            (op_delay + SBR_HF_ADJ_OFFSET) * sizeof(FLOAT32) * 64);
    memmove(&ptr_sbr_dec->sbr_qmf_out_imag[0][0],
            &ptr_sbr_dec->sbr_qmf_out_imag[ptr_sbr_dec->str_codec_qmf_bank
                                               .num_time_slots][0],
            (op_delay + SBR_HF_ADJ_OFFSET) * sizeof(FLOAT32) * 64);

    if (hbe_flag) {
      memmove(&ptr_sbr_dec->ph_vocod_qmf_real[0][0],
              &ptr_sbr_dec->ph_vocod_qmf_real[ptr_sbr_dec->str_codec_qmf_bank
                                                  .num_time_slots][0],
              64 * sizeof(FLOAT32) * (op_delay + SBR_HF_ADJ_OFFSET));
      memmove(ptr_sbr_dec->ph_vocod_qmf_imag,
              ptr_sbr_dec->ph_vocod_qmf_imag +
                  ptr_sbr_dec->str_codec_qmf_bank.num_time_slots,
              64 * sizeof(FLOAT32) * (op_delay + SBR_HF_ADJ_OFFSET));
    }
  }

  ixheaacd_esbr_analysis_filt_block(
      ptr_sbr_dec, ptr_sbr_tables,
      op_delay + codec_x_delay + SBR_HF_ADJ_OFFSET);

  if (hbe_flag) {
    WORD32 err = ixheaacd_qmf_hbe_apply(
        ptr_sbr_dec->p_hbe_txposer,
        ptr_sbr_dec->qmf_buf_real + (op_delay + SBR_HF_ADJ_OFFSET) +
            ESBR_HBE_DELAY_OFFSET,
        ptr_sbr_dec->qmf_buf_imag + (op_delay + SBR_HF_ADJ_OFFSET) +
            ESBR_HBE_DELAY_OFFSET,
        ptr_sbr_dec->str_codec_qmf_bank.num_time_slots,
        ptr_sbr_dec->ph_vocod_qmf_real + (op_delay + SBR_HF_ADJ_OFFSET),
        ptr_sbr_dec->ph_vocod_qmf_imag + (op_delay + SBR_HF_ADJ_OFFSET),
        ptr_frame_data->pitch_in_bins);
    if (err) return err;

    if (upsample_ratio_idx == SBR_UPSAMPLE_IDX_4_1) {
      ixheaacd_hbe_repl_spec(
          &ptr_sbr_dec->p_hbe_txposer->x_over_qmf[0],
          ptr_sbr_dec->ph_vocod_qmf_real + (op_delay + SBR_HF_ADJ_OFFSET),
          ptr_sbr_dec->ph_vocod_qmf_imag + (op_delay + SBR_HF_ADJ_OFFSET),
          ptr_sbr_dec->str_codec_qmf_bank.num_time_slots,
          ptr_sbr_dec->p_hbe_txposer->max_stretch);
    }
  }
  ixheaacd_qmf_enrg_calc(ptr_sbr_dec, upsample_ratio_idx, low_pow_flag);

  for (i = 0; i < 64; i++) {
    memset(ptr_sbr_dec->sbr_qmf_out_real[i], 0, 64 * sizeof(FLOAT32));
    memset(ptr_sbr_dec->sbr_qmf_out_imag[i], 0, 64 * sizeof(FLOAT32));
  }

  ixheaacd_esbr_synthesis_filt_block(
      ptr_sbr_dec, ptr_header_data, ptr_frame_data, apply_processing,
      pp_qmf_buf_real, pp_qmf_buf_imag, stereo_config_idx, ptr_sbr_tables,
      mps_sbr_flag, ch_fac);

  ptr_frame_data->prev_sbr_mode = sbr_mode;
  return 0;
}

WORD32 ixheaacd_sbr_dec_from_mps(FLOAT32 *p_mps_qmf_output, VOID *p_sbr_dec,
                                 VOID *p_sbr_frame, VOID *p_sbr_header) {
  WORD32 i, k;
  ia_sbr_frame_info_data_struct *ptr_frame_data =
      (ia_sbr_frame_info_data_struct *)p_sbr_frame;
  ia_sbr_header_data_struct *ptr_header_data =
      (ia_sbr_header_data_struct *)p_sbr_header;
  ia_sbr_dec_struct *ptr_sbr_dec = (ia_sbr_dec_struct *)p_sbr_dec;
  ia_frame_info_struct *p_frame_info = &ptr_frame_data->str_frame_info_details;
  WORD32 no_bins;
  WORD32 upsample_ratio_idx = ptr_header_data->sbr_ratio_idx;
  WORD32 op_delay = 6 + SBR_HF_ADJ_OFFSET;
  WORD32 num_anal_bands = 40;
  WORD32 mps_sbr_flag = ptr_frame_data->mps_sbr_flag;
  WORD32 err = 0;

  if (ptr_header_data->is_usf_4) {
    op_delay += 6;
  }

  num_anal_bands = num_anal_bands - (upsample_ratio_idx << 3);

  if (!mps_sbr_flag) {
    return 0;
  }

  no_bins = ptr_header_data->output_framesize / 64;

  for (i = 0; i < no_bins; i++) {
    FLOAT32 *p_loc_mps_qmf_output =
        p_mps_qmf_output + i * (MAX_NUM_QMF_BANDS_ESBR * 2);
    for (k = 0; k < ptr_header_data->pstr_freq_band_data->sub_band_start; k++) {
      ptr_sbr_dec->mps_qmf_buf_real[op_delay + i][k] = *p_loc_mps_qmf_output++;
      ptr_sbr_dec->mps_qmf_buf_imag[op_delay + i][k] = *p_loc_mps_qmf_output++;

      ptr_sbr_dec->mps_sbr_qmf_buf_real[SBR_HF_ADJ_OFFSET + i][k] =
          ptr_sbr_dec->qmf_buf_real[SBR_HF_ADJ_OFFSET + i][k];
      ptr_sbr_dec->mps_sbr_qmf_buf_imag[SBR_HF_ADJ_OFFSET + i][k] =
          ptr_sbr_dec->qmf_buf_real[SBR_HF_ADJ_OFFSET + i][k];
    }
  }

  if (ptr_frame_data->reset_flag) {
    WORD32 l;
    WORD32 start_band = ptr_header_data->pstr_freq_band_data->qmf_sb_prev;
    WORD32 end_band = num_anal_bands;
    WORD32 start_slot =
        SBR_HF_ADJ_OFFSET + ptr_frame_data->rate * p_frame_info->border_vec[0];

    for (l = start_slot; l < op_delay; l++) {
      for (k = start_band; k < end_band; k++) {
        ptr_sbr_dec->mps_qmf_buf_real[l][k] = 0.0;
        ptr_sbr_dec->mps_qmf_buf_imag[l][k] = 0.0;
      }
    }

    for (l = 0; l < SBR_HF_ADJ_OFFSET; l++) {
      for (k = start_band; k < end_band; k++) {
        ptr_sbr_dec->mps_qmf_buf_real[l][k] = 0.0;
        ptr_sbr_dec->mps_qmf_buf_imag[l][k] = 0.0;
      }
    }
  }
  ptr_header_data->pstr_freq_band_data->qmf_sb_prev =
      ptr_header_data->pstr_freq_band_data->sub_band_start;

  err = ixheaacd_generate_hf(
      ptr_sbr_dec->mps_qmf_buf_real + SBR_HF_ADJ_OFFSET,
      ptr_sbr_dec->mps_qmf_buf_imag + SBR_HF_ADJ_OFFSET, NULL, NULL,
      ptr_sbr_dec->mps_sbr_qmf_buf_real + SBR_HF_ADJ_OFFSET,
      ptr_sbr_dec->mps_sbr_qmf_buf_imag + SBR_HF_ADJ_OFFSET, ptr_frame_data,
      ptr_header_data);
  if (err) return err;

  ptr_frame_data->pstr_sbr_header = ptr_header_data;
  ptr_frame_data->sbr_mode = ORIG_SBR;
  ptr_frame_data->prev_sbr_mode = ORIG_SBR;
  err = ixheaacd_sbr_env_calc(
      ptr_frame_data, ptr_sbr_dec->mps_sbr_qmf_buf_real + SBR_HF_ADJ_OFFSET,
      ptr_sbr_dec->mps_sbr_qmf_buf_imag + SBR_HF_ADJ_OFFSET,
      ptr_sbr_dec->mps_qmf_buf_real + SBR_HF_ADJ_OFFSET,
      ptr_sbr_dec->mps_qmf_buf_imag + SBR_HF_ADJ_OFFSET,
      (ptr_header_data->hbe_flag == 0) ? NULL
                                       : ptr_sbr_dec->p_hbe_txposer->x_over_qmf,
      ptr_sbr_dec->scratch_buff, NULL);

  if (err) return err;
  for (i = 0; i < no_bins; i++) {
    FLOAT32 *p_loc_mps_qmf_output =
        p_mps_qmf_output + i * (MAX_NUM_QMF_BANDS_ESBR * 2);
    for (k = 0; k < ptr_header_data->pstr_freq_band_data->sub_band_start; k++) {
      *p_loc_mps_qmf_output++ =
          ptr_sbr_dec->mps_qmf_buf_real[SBR_HF_ADJ_OFFSET + i][k];
      *p_loc_mps_qmf_output++ =
          ptr_sbr_dec->mps_qmf_buf_imag[SBR_HF_ADJ_OFFSET + i][k];
    }
    for (k = ptr_header_data->pstr_freq_band_data->sub_band_start; k < 64;
         k++) {
      *p_loc_mps_qmf_output++ =
          ptr_sbr_dec->mps_sbr_qmf_buf_real[SBR_HF_ADJ_OFFSET + i][k];
      *p_loc_mps_qmf_output++ =
          ptr_sbr_dec->mps_sbr_qmf_buf_imag[SBR_HF_ADJ_OFFSET + i][k];
    }
  }

  for (i = 0; i < op_delay; i++) {
    memmove(ptr_sbr_dec->mps_qmf_buf_real[i],
            ptr_sbr_dec->mps_qmf_buf_real[no_bins + i], 64 * sizeof(FLOAT32));

    memmove(ptr_sbr_dec->mps_qmf_buf_imag[i],
            ptr_sbr_dec->mps_qmf_buf_imag[no_bins + i], 64 * sizeof(FLOAT32));

    memmove(ptr_sbr_dec->mps_sbr_qmf_buf_real[i],
            ptr_sbr_dec->mps_sbr_qmf_buf_real[no_bins + i],
            64 * sizeof(FLOAT32));

    memmove(ptr_sbr_dec->mps_sbr_qmf_buf_imag[i],
            ptr_sbr_dec->mps_sbr_qmf_buf_imag[no_bins + i],
            64 * sizeof(FLOAT32));
  }

  ptr_frame_data->reset_flag = 0;
  return err;
}
