/******************************************************************************
 *
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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "impd_type_def.h"
#include "impd_drc_extr_delta_coded_info.h"
#include "impd_drc_common.h"
#include "impd_drc_struct.h"
#include "impd_drc_filter_bank.h"
#include "impd_drc_multi_band.h"
#include "impd_drc_gain_dec.h"
#include "impd_drc_process_audio.h"

WORD32 impd_apply_gains_and_add(
    ia_drc_instructions_struct* pstr_drc_instruction_arr,
    const WORD32 drc_instructions_index,
    ia_drc_params_struct* ia_drc_params_struct,
    ia_gain_buffer_struct* pstr_gain_buf,
    shape_filter_block shape_filter_block[], FLOAT32* deinterleaved_audio[],

    FLOAT32* channel_audio[], WORD32 impd_apply_gains) {
  WORD32 c, b, g, i;
  WORD32 offset = 0, signalIndex = 0;
  WORD32 gainIndexForGroup[CHANNEL_GROUP_COUNT_MAX];
  WORD32 signalIndexForChannel[MAX_CHANNEL_COUNT];
  FLOAT32* lpcm_gains;
  FLOAT32 sum;
  FLOAT32 drc_gain_last, gainThr;
  WORD32 iEnd, iStart;
  ia_drc_instructions_struct* str_drc_instruction_str =
      &(pstr_drc_instruction_arr[drc_instructions_index]);

  if (drc_instructions_index >= 0) {
    str_drc_instruction_str =
        &(pstr_drc_instruction_arr[drc_instructions_index]);
    {
      if (str_drc_instruction_str->drc_set_id > 0) {
        if (ia_drc_params_struct->delay_mode == DELAY_MODE_LOW_DELAY) {
          offset = ia_drc_params_struct->drc_frame_size;
        }
        gainIndexForGroup[0] = 0;
        for (g = 0; g < str_drc_instruction_str->num_drc_ch_groups - 1; g++) {
          gainIndexForGroup[g + 1] =
              gainIndexForGroup[g] +
              str_drc_instruction_str->band_count_of_ch_group[g];
        }
        signalIndexForChannel[0] = 0;
        for (c = 0; c < str_drc_instruction_str->audio_num_chan - 1; c++) {
          if (str_drc_instruction_str->channel_group_of_ch[c] >= 0) {
            signalIndexForChannel[c + 1] =
                signalIndexForChannel[c] +
                str_drc_instruction_str->band_count_of_ch_group
                    [str_drc_instruction_str->channel_group_of_ch[c]];
          } else {
            signalIndexForChannel[c + 1] = signalIndexForChannel[c] + 1;
          }
        }

        for (g = 0; g < str_drc_instruction_str->num_drc_ch_groups; g++) {
          for (b = 0; b < str_drc_instruction_str->band_count_of_ch_group[g];
               b++) {
            if (str_drc_instruction_str->ch_group_parametric_drc_flag[g] == 0) {
              lpcm_gains =
                  pstr_gain_buf->buf_interpolation[gainIndexForGroup[g] + b]
                      .lpcm_gains +
                  MAX_SIGNAL_DELAY - ia_drc_params_struct->gain_delay_samples -
                  ia_drc_params_struct->audio_delay_samples + offset;
            } else {
              lpcm_gains =
                  pstr_gain_buf->buf_interpolation[gainIndexForGroup[g] + b]
                      .lpcm_gains +
                  MAX_SIGNAL_DELAY +
                  str_drc_instruction_str
                      ->parametric_drc_look_ahead_samples[g] -
                  ia_drc_params_struct->audio_delay_samples;
            }
            iEnd = 0;
            iStart = 0;
            while (iEnd < ia_drc_params_struct->drc_frame_size) {
              if (shape_filter_block[g].shape_flter_block_flag) {
                drc_gain_last = shape_filter_block[g].drc_gain_last;
                gainThr = 0.0001f * drc_gain_last;
                while ((iEnd < ia_drc_params_struct->drc_frame_size) &&
                       (fabs(lpcm_gains[iEnd] - drc_gain_last) <= gainThr))
                  iEnd++;
              } else {
                iEnd = ia_drc_params_struct->drc_frame_size;
              }

              for (c = 0; c < str_drc_instruction_str->audio_num_chan; c++)

              {
                if (g == str_drc_instruction_str->channel_group_of_ch[c]) {
                  signalIndex = signalIndexForChannel[c] + b;

                  if (impd_apply_gains == 1) {
                    impd_shape_filt_block_time_process(
                        &shape_filter_block[g], &lpcm_gains[0], signalIndex,
                        &deinterleaved_audio[signalIndex][0], iStart, iEnd);

                  } else {
                    for (i = iStart; i < iEnd; i++) {
                      deinterleaved_audio[signalIndex][i] = lpcm_gains[i];
                    }
                  }
                }
              }
              if ((iEnd < ia_drc_params_struct->drc_frame_size) &&
                  (shape_filter_block[g].shape_flter_block_flag)) {
                impd_shape_filt_block_adapt(lpcm_gains[iEnd],
                                            &shape_filter_block[g]);
              }
              iStart = iEnd;
            }
          }
        }
      }
    }
  }

  signalIndex = 0;

  if (str_drc_instruction_str->drc_set_id > 0) {
    for (c = 0; c < str_drc_instruction_str->audio_num_chan; c++)

    {
      g = str_drc_instruction_str->channel_group_of_ch[c];
      if (g >= 0) {
        for (i = 0; i < ia_drc_params_struct->drc_frame_size; i++) {
          sum = 0.0f;
          for (b = 0; b < str_drc_instruction_str->band_count_of_ch_group[g];
               b++) {
            sum += deinterleaved_audio[signalIndex + b][i];
          }

          channel_audio[c][i] = sum;
        }
        signalIndex += str_drc_instruction_str->band_count_of_ch_group[g];
      } else {
        for (i = 0; i < ia_drc_params_struct->drc_frame_size; i++) {
          channel_audio[c][i] = deinterleaved_audio[signalIndex][i];
        }
        signalIndex++;
      }
    }
  } else {
    for (c = 0; c < str_drc_instruction_str->audio_num_chan; c++)

    {
      for (i = 0; i < ia_drc_params_struct->drc_frame_size; i++) {
        channel_audio[c][i] = deinterleaved_audio[c][i];
      }
    }
  }

  return (0);
}

/* subband-domain DRC: in-place application of DRC gains to audio frame */
WORD32
impd_apply_gains_subband(ia_drc_instructions_struct* pstr_drc_instruction_arr,
                         const WORD32 drc_instructions_index,
                         ia_drc_params_struct* ia_drc_params_struct,
                         ia_gain_buffer_struct* pstr_gain_buf,
                         ia_overlap_params_struct* pstr_overlap_params,
                         FLOAT32* deinterleaved_audio_delayed_re[],
                         FLOAT32* deinterleaved_audio_delayed_im[],
                         FLOAT32* deinterleaved_audio_re[],
                         FLOAT32* deinterleaved_audio_im[]) {
  WORD32 c, b, g, m, s;
  WORD32 gainIndexForGroup[CHANNEL_GROUP_COUNT_MAX];
  FLOAT32* lpcm_gains;
  FLOAT32 gainSb, gainLr;
  ia_drc_instructions_struct* str_drc_instruction_str;
  WORD32 offset = 0, signalIndex = 0;
  WORD32 drc_frame_sizeSb = 0;
  WORD32 nDecoderSubbands = 0;
  WORD32 L = 0; /* L: downsampling factor */
  WORD32 analysisDelay = 0;
  switch (ia_drc_params_struct->sub_band_domain_mode) {
    case SUBBAND_DOMAIN_MODE_QMF64:
      nDecoderSubbands = AUDIO_CODEC_SUBBAND_COUNT_QMF64;
      L = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF64;
      analysisDelay = AUDIO_CODEC_SUBBAND_ANALYSE_DELAY_QMF64;
      break;
    case SUBBAND_DOMAIN_MODE_QMF71:
      nDecoderSubbands = AUDIO_CODEC_SUBBAND_COUNT_QMF71;
      L = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF71;
      analysisDelay = AUDIO_CODEC_SUBBAND_ANALYSE_DELAY_QMF71;
      break;
    case SUBBAND_DOMAIN_MODE_STFT256:
      nDecoderSubbands = AUDIO_CODEC_SUBBAND_COUNT_STFT256;
      L = AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_STFT256;
      analysisDelay = AUDIO_CODEC_SUBBAND_ANALYSE_DELAY_STFT256;
      break;
    default:
      return -1;
      break;
  }
  drc_frame_sizeSb = ia_drc_params_struct->drc_frame_size / L;

  if (drc_instructions_index >= 0) {
    str_drc_instruction_str =
        &(pstr_drc_instruction_arr[drc_instructions_index]);
    {
      if (str_drc_instruction_str->drc_set_id > 0) {
        if (ia_drc_params_struct->delay_mode == DELAY_MODE_LOW_DELAY) {
          offset = ia_drc_params_struct->drc_frame_size;
        }
        gainIndexForGroup[0] = 0;
        for (g = 0; g < str_drc_instruction_str->num_drc_ch_groups - 1; g++) {
          gainIndexForGroup[g + 1] =
              gainIndexForGroup[g] +
              str_drc_instruction_str
                  ->band_count_of_ch_group[g]; /* index of first gain sequence
                                                  in channel group */
        }

        for (c = 0; c < str_drc_instruction_str->audio_num_chan; c++)

        {
          g = str_drc_instruction_str->channel_group_of_ch[c];
          if (g >= 0) {
            for (m = 0; m < drc_frame_sizeSb; m++) {
              if (str_drc_instruction_str->band_count_of_ch_group[g] >
                  1) { /* multiband DRC */
                for (s = 0; s < nDecoderSubbands; s++) {
                  gainSb = 0.0f;
                  for (b = 0;
                       b < str_drc_instruction_str->band_count_of_ch_group[g];
                       b++) {
                    if (str_drc_instruction_str
                            ->ch_group_parametric_drc_flag[g] == 0) {
                      lpcm_gains =
                          pstr_gain_buf
                              ->buf_interpolation[gainIndexForGroup[g] + b]
                              .lpcm_gains +
                          MAX_SIGNAL_DELAY -
                          ia_drc_params_struct->gain_delay_samples -
                          ia_drc_params_struct->audio_delay_samples + offset;
                    } else {
                      lpcm_gains =
                          pstr_gain_buf
                              ->buf_interpolation[gainIndexForGroup[g] + b]
                              .lpcm_gains +
                          MAX_SIGNAL_DELAY +
                          str_drc_instruction_str
                              ->parametric_drc_look_ahead_samples[g] -
                          ia_drc_params_struct->audio_delay_samples +
                          analysisDelay;
                    }
                    /* get gain for this timeslot by downsampling */
                    gainLr = lpcm_gains[(m * L + (L - 1) / 2)];
                    gainSb += pstr_overlap_params->str_group_overlap_params[g]
                                  .str_band_overlap_params[b]
                                  .overlap_weight[s] *
                              gainLr;
                  }
                  deinterleaved_audio_re[signalIndex][m * nDecoderSubbands +
                                                      s] =
                      gainSb *
                      deinterleaved_audio_delayed_re[signalIndex]
                                                    [m * nDecoderSubbands + s];
                  if (ia_drc_params_struct->sub_band_domain_mode ==
                      SUBBAND_DOMAIN_MODE_STFT256) { /* For STFT filterbank, the
                                                        real value of the
                                                        nyquist band is stored
                                                        at the imag value of the
                                                        first band */
                    if (s != 0)
                      deinterleaved_audio_im[signalIndex][m * nDecoderSubbands +
                                                          s] =
                          gainSb *
                          deinterleaved_audio_delayed_im[signalIndex]
                                                        [m * nDecoderSubbands +
                                                         s];
                    if (s == (nDecoderSubbands - 1))
                      deinterleaved_audio_im[signalIndex][m * nDecoderSubbands +
                                                          0] =
                          gainSb *
                          deinterleaved_audio_delayed_im[signalIndex]
                                                        [m * nDecoderSubbands +
                                                         0];
                  } else {
                    deinterleaved_audio_im[signalIndex][m * nDecoderSubbands +
                                                        s] =
                        gainSb *
                        deinterleaved_audio_delayed_im[signalIndex]
                                                      [m * nDecoderSubbands +
                                                       s];
                  }
                }
              } else { /* single-band DRC */
                if (str_drc_instruction_str->ch_group_parametric_drc_flag[g] ==
                    0) {
                  lpcm_gains =
                      pstr_gain_buf->buf_interpolation[gainIndexForGroup[g]]
                          .lpcm_gains +
                      MAX_SIGNAL_DELAY -
                      ia_drc_params_struct->gain_delay_samples -
                      ia_drc_params_struct->audio_delay_samples + offset;
                } else {
                  lpcm_gains =
                      pstr_gain_buf->buf_interpolation[gainIndexForGroup[g]]
                          .lpcm_gains +
                      MAX_SIGNAL_DELAY +
                      str_drc_instruction_str
                          ->parametric_drc_look_ahead_samples[g] -
                      ia_drc_params_struct->audio_delay_samples + analysisDelay;
                }
                /* get gain for this timeslot by downsampling */
                gainSb = lpcm_gains[(m * L + (L - 1) / 2)];
                for (s = 0; s < nDecoderSubbands; s++) {
                  deinterleaved_audio_re[signalIndex][m * nDecoderSubbands +
                                                      s] =
                      gainSb *
                      deinterleaved_audio_delayed_re[signalIndex]
                                                    [m * nDecoderSubbands + s];
                  deinterleaved_audio_im[signalIndex][m * nDecoderSubbands +
                                                      s] =
                      gainSb *
                      deinterleaved_audio_delayed_im[signalIndex]
                                                    [m * nDecoderSubbands + s];
                }
              }
            }
          }
          signalIndex++;
        }
      }
    }
  }
  return (0);
}

WORD32
impd_filter_banks_process(ia_drc_instructions_struct* pstr_drc_instruction_arr,
                          const WORD32 drc_instructions_index,
                          ia_drc_params_struct* ia_drc_params_struct,
                          FLOAT32* audio_io_buf[],
                          ia_audio_band_buffer_struct* audio_band_buffer,
                          ia_filter_banks_struct* ia_filter_banks_struct,
                          const WORD32 passThru) {
  WORD32 c, g, e, i, num_bands;
  // WORD32 err = 0;
  FLOAT32* audio_in;
  FLOAT32** audio_out;
  ia_drc_filter_bank_struct* str_drc_filter_bank;
  ia_drc_instructions_struct* str_drc_instruction_str;
  WORD32 drc_frame_size = ia_drc_params_struct->drc_frame_size;

  if (drc_instructions_index >= 0) {
    str_drc_instruction_str =
        &(pstr_drc_instruction_arr[drc_instructions_index]);
  } else {
    return -1;
  }

  e = 0;

  for (c = 0; c < str_drc_instruction_str->audio_num_chan; c++)

  {
    str_drc_filter_bank = NULL;

    audio_in = audio_io_buf[c];

    audio_out = &(audio_band_buffer->non_interleaved_audio[e]);
    if ((passThru == 0) && (drc_instructions_index >= 0)) {
      if (str_drc_instruction_str->drc_set_id < 0) {
        num_bands = 1;
      } else {
        g = str_drc_instruction_str->channel_group_of_ch[c];
        if (g == -1) {
          num_bands = 1;
          // if (ia_filter_banks_struct->str_drc_filter_bank != NULL)
          //{
          str_drc_filter_bank =
              &(ia_filter_banks_struct->str_drc_filter_bank
                    [str_drc_instruction_str->num_drc_ch_groups]);
          //}
        } else {
          num_bands = str_drc_instruction_str->band_count_of_ch_group[g];
          // if (ia_filter_banks_struct->str_drc_filter_bank != NULL)
          //{
          str_drc_filter_bank =
              &(ia_filter_banks_struct->str_drc_filter_bank[g]);
          //}
        }
        // if (ia_filter_banks_struct->str_drc_filter_bank != NULL)
        //{
        // if (&str_drc_filter_bank->str_all_pass_cascade != NULL)
        //{
        impd_all_pass_cascade_process(
            &str_drc_filter_bank->str_all_pass_cascade, c, drc_frame_size,
            audio_in);
        //}
        //}
      }
    } else {
      num_bands = 1;
    }
    switch (num_bands) {
      case 1:
        for (i = 0; i < drc_frame_size; i++) {
          audio_out[0][i] = audio_in[i];
        }
        e++;
        break;
      case 2:
        impd_two_band_filter_process(&str_drc_filter_bank->str_two_band_bank, c,
                                     drc_frame_size, audio_in, audio_out);
        e += 2;
        break;
      case 3:
        impd_three_band_filter_process(
            &str_drc_filter_bank->str_three_band_bank, c, drc_frame_size,
            audio_in, audio_out);
        e += 3;
        break;
      case 4:
        impd_four_band_filter_process(&str_drc_filter_bank->str_four_band_bank,
                                      c, drc_frame_size, audio_in, audio_out);
        e += 4;
        break;
      default:
        return (PARAM_ERROR);
        break;
    }
  }

  return (0);
}

WORD32
impd_store_audio_io_buffer_time(FLOAT32* audio_in_out_buf[],
                                ia_audio_in_out_buf* audio_io_buf_internal) {
  WORD32 i, j;

  if (audio_io_buf_internal->audio_delay_samples) {
    for (i = 0; i < audio_io_buf_internal->audio_num_chan; i++) {
      for (j = 0; j < audio_io_buf_internal->frame_size; j++) {
        audio_io_buf_internal->audio_io_buffer_delayed
            [i][audio_io_buf_internal->audio_delay_samples + j] =
            audio_in_out_buf[i][j];
      }
    }
  } else {
    audio_io_buf_internal->audio_io_buffer_delayed = audio_in_out_buf;
    audio_io_buf_internal->audio_in_out_buf = audio_in_out_buf;
  }

  return 0;
}

WORD32
impd_store_audio_io_buffer_freq(FLOAT32* audio_real_buff[],
                                FLOAT32* audio_imag_buff[],
                                ia_audio_in_out_buf* audio_io_buf_internal) {
  WORD32 i, j;

  if (audio_io_buf_internal->audio_delay_sub_band_samples) {
    for (i = 0; i < audio_io_buf_internal->audio_num_chan; i++) {
      for (j = 0; j < audio_io_buf_internal->audio_sub_band_frame_size *
                          audio_io_buf_internal->audio_sub_band_count;
           j++) {
        audio_io_buf_internal->audio_buffer_delayed_real
            [i][audio_io_buf_internal->audio_delay_sub_band_samples *
                    audio_io_buf_internal->audio_sub_band_count +
                j] = audio_real_buff[i][j];
        audio_io_buf_internal->audio_buffer_delayed_imag
            [i][audio_io_buf_internal->audio_delay_sub_band_samples *
                    audio_io_buf_internal->audio_sub_band_count +
                j] = audio_imag_buff[i][j];
      }
    }
  } else {
    audio_io_buf_internal->audio_buffer_delayed_real = audio_real_buff;
    audio_io_buf_internal->audio_buffer_delayed_imag = audio_imag_buff;
    audio_io_buf_internal->audio_real_buff = audio_real_buff;
    audio_io_buf_internal->audio_imag_buff = audio_imag_buff;
  }

  return 0;
}

WORD32
impd_retrieve_audio_io_buffer_time(FLOAT32* audio_in_out_buf[],
                                   ia_audio_in_out_buf* audio_io_buf_internal) {
  WORD32 i, j;

  if (audio_io_buf_internal->audio_delay_samples) {
    for (i = 0; i < audio_io_buf_internal->audio_num_chan; i++) {
      for (j = 0; j < audio_io_buf_internal->frame_size; j++) {
        audio_in_out_buf[i][j] =
            audio_io_buf_internal->audio_io_buffer_delayed[i][j];
      }
    }
  }

  return 0;
}

WORD32
impd_retrieve_audio_buffer_freq(FLOAT32* audio_real_buff[],
                                FLOAT32* audio_imag_buff[],
                                ia_audio_in_out_buf* audio_io_buf_internal) {
  WORD32 i, j;

  if (audio_io_buf_internal->audio_delay_sub_band_samples) {
    for (i = 0; i < audio_io_buf_internal->audio_num_chan; i++) {
      for (j = 0; j < audio_io_buf_internal->audio_sub_band_frame_size *
                          audio_io_buf_internal->audio_sub_band_count;
           j++) {
        audio_real_buff[i][j] =
            audio_io_buf_internal->audio_buffer_delayed_real
                [i][audio_io_buf_internal->audio_sub_band_count + j];
        audio_imag_buff[i][j] =
            audio_io_buf_internal->audio_buffer_delayed_imag
                [i][audio_io_buf_internal->audio_sub_band_count + j];
      }
    }
  }

  return 0;
}

WORD32
impd_advance_audio_io_buffer_time(ia_audio_in_out_buf* audio_io_buf_internal) {
  WORD32 i;
  if (audio_io_buf_internal->audio_delay_samples) {
    for (i = 0; i < audio_io_buf_internal->audio_num_chan; i++) {
      memmove(
          audio_io_buf_internal->audio_io_buffer_delayed[i],
          &audio_io_buf_internal
               ->audio_io_buffer_delayed[i][audio_io_buf_internal->frame_size],
          sizeof(FLOAT32) * audio_io_buf_internal->audio_delay_samples);
    }
  }

  return 0;
}

WORD32
impd_advance_audio_buff_freq(ia_audio_in_out_buf* audio_io_buf_internal) {
  WORD32 i;
  if (audio_io_buf_internal->audio_delay_sub_band_samples) {
    for (i = 0; i < audio_io_buf_internal->audio_num_chan; i++) {
      memmove(audio_io_buf_internal->audio_buffer_delayed_real[i],
              &audio_io_buf_internal->audio_buffer_delayed_real
                   [i][audio_io_buf_internal->audio_sub_band_frame_size *
                       audio_io_buf_internal->audio_sub_band_count],
              sizeof(FLOAT32) *
                  audio_io_buf_internal->audio_delay_sub_band_samples *
                  audio_io_buf_internal->audio_sub_band_count);
      memmove(audio_io_buf_internal->audio_buffer_delayed_imag[i],
              &audio_io_buf_internal->audio_buffer_delayed_imag
                   [i][audio_io_buf_internal->audio_sub_band_frame_size *
                       audio_io_buf_internal->audio_sub_band_count],
              sizeof(FLOAT32) *
                  audio_io_buf_internal->audio_delay_sub_band_samples *
                  audio_io_buf_internal->audio_sub_band_count);
    }
  }
  return 0;
}
