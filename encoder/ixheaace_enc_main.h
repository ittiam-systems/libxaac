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
/* here we distinguish between stereo and the mono only encoder */
#define IXHEAACE_MAX_CH_IN_BS_ELE (2)

#define MAXIMUM_BS_ELE \
  8 /* 1 <SCE> 2 <CPE> 3<CPE> 4<CPE> 5<LFE> 6<SCE> - 8.1 channel + 2 cc channels*/

#define FRAME_LEN_1024 1024
#define FRAME_LEN_512 512
#define FRAME_LEN_480 480
#define FRAME_LEN_960 960

#define AACENC_TRANS_FAC 8   /* encoder WORD16 long ratio */
#define AACENC_PCM_LEVEL 1.0 /* encoder pcm 0db refernence */

#define MAX_INPUT_CHAN (IXHEAACE_MAX_CH_IN_BS_ELE)

#define MAX_FRAME_LEN (1024)

// Change to accommodate 4:1 resampler - input = 4096 samples per channel
#define MAX_INPUT_SAMPLES (MAX_FRAME_LEN * MAX_INPUT_CHAN * 4)

#define NUM_CHANS_MONO (1)
#define NUM_CHANS_STEREO (2)
#define MAX_NUM_CHANNELS (6)
#define MIN_NUM_CHANNELS (1)
/*-------------------------- defines --------------------------------------*/

#define BUFFERSIZE 1024 /* anc data */

/*-------------------- structure definitions ------------------------------*/

typedef struct {
  WORD32 sample_rate;              /* mapped input sample rate */
  WORD32 core_sample_rate;         /* core coder processing sample rate */
  WORD32 native_sample_rate;       /* audio file sample rate */
  WORD32 bit_rate;                 /* encoder bit rate in bits/sec */
  WORD32 num_in_channels;          /* number of channels on input (1,2) */
  WORD32 num_out_channels;         /* number of channels on output (1,2) */
  WORD32 band_width;               /* targeted audio bandwidth in Hz */
  WORD32 dual_mono;                /* flag: make 2 SCEs for stereo input files */
  WORD32 use_tns;                  /* flag: use temporal noise shaping */
  WORD32 use_adts;                 /* flag: use ADTS instead of ADIF */
  WORD32 calc_crc;                 /* flag: write CRC checks */
  WORD32 private_bit;              /* private bit of MPEG Header */
  WORD32 copyright_bit;            /* copyright bit of MPEG Header */
  WORD32 original_copy_bit;        /* original bit of MPEG Header */
  WORD32 num_stereo_preprocessing; /* forbid usage of stereo prerpocessing */
  WORD32 inv_quant;                /* improve distortion by inverse quantization */
  WORD32 full_bandwidth;           /* improve distortion by inverse quantization */
  WORD32 flag_framelength_small;   /* indicates frame size. 0 -> 512, 1 -> 480
                                   indicates frame size. 0 -> 1024, 1 -> 960 */
  WORD32 bitreservoir_size;        /* size of bit reservoir (default:0; max 6144)*/
} iaace_config;

typedef struct {
  WORD32 *shared_buffer1;
  WORD32 *shared_buffer_2;
  WORD32 *shared_buffer3;
  WORD8 *shared_buffer5;
} iaace_scratch;

/* pstr_ancillary configuration struct */
typedef struct {
  WORD32 anc_flag;
  WORD32 anc_mode;
  WORD32 anc_rate;
} ixheaace_config_ancillary;

VOID ia_enhaacplus_enc_aac_init_default_config(iaace_config *config, WORD32 aot);

WORD32 ia_enhaacplus_enc_aac_enc_pers_size(WORD32 num_aac_chan, WORD32 aot);
WORD32 ia_enhaacplus_enc_aac_enc_scr_size(VOID);

VOID ia_enhaacplus_enc_init_aac_tabs(ixheaace_aac_tables *pstr_aac_tabs);
