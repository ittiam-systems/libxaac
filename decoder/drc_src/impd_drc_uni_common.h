/******************************************************************************
 *
 * Copyright (C) 2015 The Android Open Source Project
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
#ifndef IMPD_DRC_UNI_COMMON_H
#define IMPD_DRC_UNI_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#define AMD1_SYNTAX 0
#define MPEG_D_DRC_EXTENSION_V1 0
#define AMD1_PARAMETRIC_LIMITER 0

#define MPEG_H_SYNTAX 0

#define EQ_IS_SUPPORTED 1

#define MEASURE_AVERAGE_BITRATE 0
#define DEBUG_NODES 0

#define DRC_GAIN_DEBUG_FILE 0
#define DEBUG_BITSTREAM 0
#define DEBUG_DRC_SELECTION 0
#define DEBUG_WARNINGS 0
#define ENABLE_ADDITIONAL_TESTS 1

#define SPEAKER_POS_COUNT_MAX 128
#define DOWNMIX_COEFF_COUNT_MAX 32 * 32
#define MAX_CHANNEL_COUNT 128
#define BAND_COUNT_MAX 8
#define SEQUENCE_COUNT_MAX 24
#define GAIN_SET_COUNT_MAX SEQUENCE_COUNT_MAX
#define MEASUREMENT_COUNT_MAX 16
#define DOWNMIX_INSTRUCTION_COUNT_MAX 16
#define DRC_COEFF_COUNT_MAX 8
#define DRC_INSTRUCTIONS_COUNT_MAX (DOWNMIX_INSTRUCTION_COUNT_MAX + 20)
#define LOUDNESS_INFO_COUNT_MAX (DOWNMIX_INSTRUCTION_COUNT_MAX + 20)
#define AUDIO_CODEC_FRAME_SIZE_MAX 4096
#define DRC_CODEC_FRAME_SIZE_MAX (AUDIO_CODEC_FRAME_SIZE_MAX / 8)
#define NODE_COUNT_MAX DRC_CODEC_FRAME_SIZE_MAX
#define CHANNEL_GROUP_COUNT_MAX SEQUENCE_COUNT_MAX
#define SUB_DRC_COUNT 4
#define SEL_DRC_COUNT 3
#define DOWNMIX_ID_COUNT_MAX 8
#define MAX_SIGNAL_DELAY 4500

#define DELAY_MODE_REGULAR_DELAY 0
#define DELAY_MODE_LOW_DELAY 1
#define DELAY_MODE_DEFAULT DELAY_MODE_REGULAR_DELAY

#define FEATURE_REQUEST_COUNT_MAX 10
#define EFFECT_TYPE_REQUEST_COUNT_MAX 10

#define SELECTION_CANDIDATE_COUNT_MAX 32

#define PROC_COMPLETE 1
#define UNEXPECTED_ERROR 2
#define PARAM_ERROR 3
#define EXTERNAL_ERROR 4
#define ERRORHANDLING 5
#define BITSTREAM_ERROR 6

#define UNDEFINED_LOUDNESS_VALUE 1000.0f
#define ID_FOR_BASE_LAYOUT 0x0
#define ID_FOR_ANY_DOWNMIX 0x7F
#define ID_FOR_NO_DRC 0x0
#define ID_FOR_ANY_DRC 0x3F
#define ID_FOR_ANY_EQ 0x3F

#define LOCATION_MP4_INSTREAM_UNIDRC 0x1
#define LOCATION_MP4_DYN_RANGE_INFO 0x2
#define LOCATION_MP4_COMPRESSION_VALUE 0x3
#define LOCATION_SELECTED LOCATION_MP4_INSTREAM_UNIDRC

#define AUDIO_CODEC_SUBBAND_COUNT_MAX 256

#define SUBBAND_DOMAIN_MODE_OFF 0
#define SUBBAND_DOMAIN_MODE_QMF64 1
#define SUBBAND_DOMAIN_MODE_QMF71 2
#define SUBBAND_DOMAIN_MODE_STFT256 3

#define AUDIO_CODEC_SUBBAND_COUNT_QMF64 64
#define AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF64 64
#define AUDIO_CODEC_SUBBAND_ANALYSE_DELAY_QMF64 320

#define AUDIO_CODEC_SUBBAND_COUNT_QMF71 71
#define AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_QMF71 64
#define AUDIO_CODEC_SUBBAND_ANALYSE_DELAY_QMF71 320 + 384

#define AUDIO_CODEC_SUBBAND_COUNT_STFT256 256
#define AUDIO_CODEC_SUBBAND_DOWNSAMPLING_FACTOR_STFT256 256
#define AUDIO_CODEC_SUBBAND_ANALYSE_DELAY_STFT256 256

#define MAX_NUM_DOWNMIX_ID_REQUESTS 15
#define MAX_NUM_DRC_FEATURE_REQUESTS 7
#define MAX_NUM_DRC_EFFECT_TYPE_REQUESTS 15
#define MAX_SIGNATURE_DATA_LENGTH_PLUS_ONE 256

#define EXT_COUNT_MAX 2
#define UNIDRCGAINEXT_TERM 0x0
#define UNIDRCLOUDEXT_TERM 0x0
#define UNIDRCCONFEXT_TERM 0x0
#define UNIDRCINTERFACEEXT_TERM 0x0

#define MAXPACKETLOSSTIME 2.5f

#define SLOPE_FACTOR_DB_TO_LINEAR 0.1151f

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef bool
#define bool WORD32
#endif

typedef struct ia_drc_sel_proc_output_struct {
  FLOAT32 output_peak_level_db;
  FLOAT32 loudness_normalization_gain_db;
  FLOAT32 output_loudness;

  WORD32 sel_drc_set_ids[SUB_DRC_COUNT];
  WORD32 sel_downmix_ids[SUB_DRC_COUNT];
  WORD32 num_sel_drc_sets;

  WORD32 active_downmix_id;
  WORD32 base_channel_count;
  WORD32 target_channel_count;
  WORD32 target_layout;
  WORD32 downmix_matrix_present;
  FLOAT32 downmix_matrix[MAX_CHANNEL_COUNT][MAX_CHANNEL_COUNT];

  FLOAT32 boost;
  FLOAT32 compress;
  WORD32 drc_characteristic_target;

} ia_drc_sel_proc_output_struct;

#ifdef __cplusplus
}
#endif
#endif
