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

/* standard memory table descriptor for libraries */
typedef struct {
  UWORD32 ui_size;         /* size of the memory in bytes	*/
  UWORD32 ui_alignment;    /* alignment in bytes 			*/
  UWORD32 ui_type;         /* type of memory 				*/
  UWORD32 ui_placement[2]; /* 64 bit placement info		*/
  UWORD32 ui_priority;     /* the importance for placement	*/
  UWORD32 ui_placed[2];    /* the o_red location for placement	*/
} ixheaace_mem_info_struct;

typedef struct {
  UWORD32 ui_size;
  UWORD32 ui_alignment;
  UWORD32 ui_type;
  pVOID mem_ptr;
} ixheaace_mem_info_table;

typedef struct {
  WORD32 sample_rate;               /* audio file sample rate */
  WORD32 bitrate;                   /* encoder bit rate in bits/sec */
  WORD32 num_channels_in;           /* number of channels on input (1,2) */
  WORD32 num_channels_out;          /* number of channels on output (1,2) */
  WORD32 bandwidth;                 /* targeted audio bandwidth in Hz */
  WORD32 dual_mono;                 /* flag: make 2 SCEs for stereo input files */
  WORD32 use_tns;                   /* flag: use temporal noise shaping */
  WORD32 noise_filling;             /* flag: use noise filling */
  WORD32 use_adts;                  /* flag: use ADTS instead of ADIF */
  WORD32 private_bit;               /* private bit of MPEG Header */
  WORD32 copyright_bit;             /* copyright bit of MPEG Header */
  WORD32 original_copy_bit;         /* original bit of MPEG Header */
  WORD32 f_no_stereo_preprocessing; /* forbid usage of stereo prerpocessing */
  WORD32 inv_quant;                 /* improve distortion by inverse quantization */
  WORD32 full_bandwidth;            /* improve distortion by inverse quantization */
  WORD32 bitreservoir_size;         /* size of bit reservoir (default:0; max 6144)*/
  WORD32 length;
} ixheaace_aac_enc_config;

typedef struct {
  UWORD32 ui_pcm_wd_sz;
  WORD32 i_bitrate;
  WORD32 frame_length;
  WORD32 frame_cmd_flag;
  WORD32 out_bytes_flag;
  WORD32 user_tns_flag;
  WORD32 aot;
  WORD32 i_mps_tree_config;
  WORD32 esbr_flag;
  WORD32 i_channels;
  WORD32 i_samp_freq;
  WORD32 i_native_samp_freq;
  WORD32 i_channels_mask;
  WORD32 i_num_coupling_chan;
  /* Add config params here */
  WORD32 i_use_mps;
  WORD32 i_use_adts;
  WORD32 i_use_es;

  FLAG write_program_config_element;

  ixheaace_aac_enc_config aac_config;

} ixheaace_input_config;

typedef struct {
  WORD8 *p_lib_name;
  WORD8 *p_version_num;

} ixheaace_version;
typedef struct {
  WORD32 i_out_bytes;
  WORD32 i_bytes_consumed;
  UWORD32 ui_inp_buf_size;

  UWORD32 malloc_count;
  UWORD32 ui_rem;
  UWORD32 ui_proc_mem_tabs_size;

  pVOID pv_ia_process_api_obj;
  pVOID arr_alloc_memory[100];

  pVOID (*malloc_xheaace)(UWORD32, UWORD32);
  VOID (*free_xheaace)(pVOID);
  ixheaace_version version;
  ixheaace_mem_info_table mem_info_table[4];
  WORD32 input_size;
  WORD32 samp_freq;
  WORD32 header_samp_freq;
  WORD32 audio_profile;
  FLOAT32 down_sampling_ratio;
  pWORD32 pb_inp_buf_32;
} ixheaace_output_config;

typedef struct {
  ixheaace_input_config input_config;
  ixheaace_output_config output_config;
} ixheaace_user_config_struct;

/*****************************************************************************/
/* Constant hash defines                                                     */
/*****************************************************************************/

/* ittiam standard memory types to be used inter frames */
#define IA_MEMTYPE_PERSIST 0x00
/* read write, to be used intra frames */
#define IA_MEMTYPE_SCRATCH 0x01
/* read only memory, intra frame */
#define IA_MEMTYPE_INPUT 0x02
/* read-write memory, for usable output, intra frame */
#define IA_MEMTYPE_OUTPUT 0x03

/* ittiam standard memory priorities */
#define IA_MEMPRIORITY_ANYWHERE 0x00
#define IA_MEMPRIORITY_HIGH 0x05

/* ittiam standard memory placements */
/* placement is defined by 64 bits */

#define IA_MEMPLACE_EXT_RAM_0 0x010000

/* the simple common PC RAM */

typedef struct {
  WORD32 codec_mode;
} ia_input_config;
