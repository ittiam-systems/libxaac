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
/*****************************************************************************/
/* Constant hashdefines                                                      */
/*****************************************************************************/

/*****************************************************************************/
/* Ittiam enhaacplus_enc ErrorCode Definitions                             */
/*****************************************************************************/

/*****************************************************************************/
/* Class 0: API Errors                                                       */
/*****************************************************************************/
/* Non Fatal Errors */

// AAC Profiles

// MPS

// USAC

// DRC

/* Fatal Errors */

// AAC Profiles
#define IA_EXHEAACE_API_FATAL_MEM_ALLOC (0xFFFF8000)
#define IA_EXHEAACE_API_FATAL_UNSUPPORTED_AOT (0xFFFF8001)
// MPS

// USAC

// DRC

/*****************************************************************************/
/* Class 1: Configuration Errors                                             */
/*****************************************************************************/
/* Non Fatal Errors */

// AAC profiles
#define IA_EXHEAACE_CONFIG_NONFATAL_INVALID_CONFIG (0x00000800)
#define IA_EXHEAACE_CONFIG_NONFATAL_BITRES_SIZE_TOO_SMALL (0x00000801)

// MPS
#define IA_EXHEAACE_CONFIG_NONFATAL_MPS_INVALID_CONFIG (0x00000900)
#define IA_EXHEAACE_CONFIG_NONFATAL_MPS_PARAM_ERROR (0x00000901)

// USAC

// DRC

/* Fatal Errors */

// AAC profiles
#define IA_EXHEAACE_CONFIG_FATAL_SAMP_FREQ (0xFFFF8800)
#define IA_EXHEAACE_CONFIG_FATAL_NUM_CHANNELS (0xFFFF8801)
#define IA_EXHEAACE_CONFIG_FATAL_USE_STEREO_PRE_PROC (0xFFFF8802)
#define IA_EXHEAACE_CONFIG_FATAL_QUALITY_LEVEL (0xFFFF8803)
#define IA_EXHEAACE_CONFIG_FATAL_PCM_WDSZ (0xFFFF8804)
#define IA_EXHEAACE_CONFIG_FATAL_AAC_CLASSIC_WITH_PS (0xFFFF8805)
#define IA_EXHEAACE_CONFIG_FATAL_AAC_CLASSIC (0xFFFF8806)
#define IA_EXHEAACE_CONFIG_FATAL_USE_TNS (0xFFFF8807)
#define IA_EXHEAACE_CONFIG_FATAL_CHANNELS_MASK (0xFFFF8808)
#define IA_EXHEAACE_CONFIG_FATAL_WRITE_PCE (0xFFFF8809)
#define IA_EXHEAACE_CONFIG_FATAL_USE_FULL_BANDWIDTH (0xFFFF880A)
#define IA_EXHEAACE_CONFIG_FATAL_USE_SPEECH_CONF (0xFFFF880B)

// MPS

// USAC

// DRC

/*****************************************************************************/
/* Class 2: Initialization Errors                                             */
/*****************************************************************************/
/* Non Fatal Errors */

/* Fatal Errors */

// AAC Profiles
#define IA_EXHEAACE_INIT_FATAL_RESAMPLER_INIT_FAILED (0xFFFF9000)
#define IA_EXHEAACE_INIT_FATAL_AAC_INIT_FAILED (0xFFFF9001)
#define IA_EXHEAACE_INIT_FATAL_AACPLUS_NOT_AVAIL (0xFFFF9002)
#define IA_EXHEAACE_INIT_FATAL_BITRATE_NOT_SUPPORTED (0xFFFF9003)
#define IA_EXHEAACE_INIT_FATAL_INVALID_TNS_PARAM (0xFFFF9004)
#define IA_EXHEAACE_INIT_FATAL_SCALE_FACTOR_BAND_NOT_SUPPORTED (0xFFFF9005)
#define IA_EXHEAACE_INIT_FATAL_INVALID_CORE_SAMPLE_RATE (0xFFFF9006)
#define IA_EXHEAACE_INIT_FATAL_INVALID_BIT_RATE (0xFFFF9007)
#define IA_EXHEAACE_INIT_FATAL_INVALID_ELEMENT_TYPE (0xFFFF9008)
#define IA_EXHEAACE_INIT_FATAL_NUM_CHANNELS_NOT_SUPPORTED (0xFFFF9009)
#define IA_EXHEAACE_INIT_FATAL_INVALID_NUM_CHANNELS_IN_ELE (0xFFFF900A)
#define IA_EXHEAACE_INIT_FATAL_SFB_TABLE_INIT_FAILED (0xFFFF900B)

// MPS
#define IA_EXHEAACE_INIT_FATAL_MPS_INIT_FAILED (0xFFFF9100)
// USAC

// DRC

// SBR
#define IA_EXHEAACE_INIT_FATAL_SBR_INVALID_NUM_CHANNELS (0xFFFF9400)
#define IA_EXHEAACE_INIT_FATAL_SBR_INVALID_SAMPLERATE_MODE (0xFFFF9401)
#define IA_EXHEAACE_INIT_FATAL_SBR_INVALID_FREQ_COEFFS (0xFFFF9402)
#define IA_EXHEAACE_INIT_FATAL_SBR_INVALID_NUM_BANDS (0xFFFF9403)
#define IA_EXHEAACE_INIT_FATAL_SBR_INVALID_BUFFER_LENGTH (0xFFFF9404)
#define IA_EXEHAACE_INIT_FATAL_SBR_NOISE_BAND_NOT_SUPPORTED (0xFFFF9405)
/*****************************************************************************/
/* Class 3: Execution Errors                                                 */
/*****************************************************************************/
/* Non Fatal Errors */

// AAC Profiles

// MPS
#define IA_EXHEAACE_EXE_NONFATAL_MPS_ENCODE_ERROR (0x00001900)
#define IA_EXHEAACE_EXE_NONFATAL_MPS_INVALID_DATA_BANDS (0x00001901)

// USAC

// DRC

// ESBR
#define IA_EXHEAACE_EXE_NONFATAL_ESBR_INVALID_BANDWIDTH_INDEX (0x0001C00)
#define IA_EXHEAACE_EXE_NONFATAL_ESBR_INVALID_NUM_PATCH (0x0001C01)
#define IA_EXHEAACE_EXE_NONFATAL_ESBR_INVALID_VOCOD_BUF (0x0001C02)
#define IA_EXHEAACE_EXE_NONFATAL_ESBR_INVALID_PVC_MODE (0x0001C03)
#define IA_EXHEAACE_EXE_NONFATAL_ESBR_INVALID_FFT (0x0001C04)
#define IA_EXHEAACE_EXE_NONFATAL_ESBR_INVALID_START_BAND (0x0001C05)
#define IA_EXHEAACE_EXE_NONFATAL_ESBR_INVALID_VALUE (0x0001C06)

/* Fatal Errors */

// AAC Profiles
#define IA_EXHEAACE_EXE_FATAL_SBR_INVALID_TIME_SLOTS (0xFFFF9800)
#define IA_EXHEAACE_EXE_FATAL_SBR_INVALID_IN_CHANNELS (0xFFFF9801)
#define IA_EXHEAACE_EXE_FATAL_PS_INVALID_HYBRID_RES_VAL (0xFFFF9802)
#define IA_EXHEAACE_EXE_FATAL_UNSUPPORTED_AOT (0xFFFF9803)
#define IA_EXHEAACE_EXE_FATAL_INVALID_BLOCK_TYPE (0xFFFF9804)
#define IA_EXHEAACE_EXE_FATAL_INVALID_SBR_FRAME_TYPE (0xFFFF9805)
#define IA_EXHEAACE_EXE_FATAL_INVALID_SBR_NUM_ENVELOPES (0xFFFF9806)
#define IA_EXHEAACE_EXE_FATAL_SBR_INVALID_NUM_BANDS (0xFFFF9807)
#define IA_EXHEAACE_EXE_FATAL_SBR_INVALID_BS (0xFFFF9808)
#define IA_EXHEAACE_EXE_FATAL_SBR_INVALID_CODEBOOK (0xFFFF9809)
#define IA_EXHEAACE_EXE_FATAL_INVALID_SCALE_FACTOR_GAIN (0xFFFF980A)
#define IA_EXHEAACE_EXE_FATAL_INVALID_BIT_RES_LEVEL (0xFFFF980B)
#define IA_EXHEAACE_EXE_FATAL_INVALID_BIT_CONSUMPTION (0xFFFF980C)
#define IA_EXHEAACE_EXE_FATAL_INVALID_SIDE_INFO_BITS (0xFFFF980D)
#define IA_EXHEAACE_EXE_FATAL_INVALID_HUFFMAN_BITS (0xFFFF980E)
#define IA_EXHEAACE_EXE_FATAL_INVALID_SCALE_FACTOR_BITS (0xFFFF980F)
#define IA_EXHAACE_EXE_FATAL_SBR_INVALID_AMP_RES (0xFFFF9810)
#define IA_EXHEAACE_EXE_FATAL_INVALID_OUT_BYTES (0xFFFF9811)
#define IA_EXHEAACE_EXE_FATAL_INVALID_TNS_FILT_ORDER (0xFFFF9812)
#define IA_EXHEAACE_EXE_FATAL_SBR_INVALID_SAMP_FREQ (0xFFFF9813)

// MPS
#define IA_EXHEAACE_EXE_FATAL_MPS_NULL_DATA_HANDLE (0xFFFF9900)
#define IA_EXHEAACE_EXE_FATAL_MPS_INVALID_HUFF_DATA_TYPE (0xFFFF9901)
#define IA_EXHEAACE_EXE_FATAL_MPS_INVALID_NUM_PARAM_SETS (0xFFFF9902)
#define IA_EXHEAACE_EXE_FATAL_MPS_UNSUPPORTED_GUIDED_ENV_SHAPE (0xFFFF9903)
#define IA_EXHEAACE_EXE_FATAL_MPS_3D_STEREO_MODE_NOT_SUPPORTED (0xFFFF9904)
#define IA_EXHEAACE_EXE_FATAL_MPS_UNSUPPORTED_RESIDUAL_CODING (0xFFFF9905)
#define IA_EXHEAACE_EXE_FATAL_MPS_UNSUPPORTED_ARBITARY_DOWNMIX_CODING (0xFFFF9906)
#define IA_EXHEAACE_EXE_FATAL_MPS_ARBITARY_TREE_NOT_SUPPORTED (0xFFFF9907)
#define IA_EXHEAACE_EXE_FATAL_MPS_INVALID_QUANT_COARSE (0xFFFF9908)
#define IA_EXHEAACE_EXE_FATAL_MPS_INVALID_RES_STRIDE (0xFFFF9909)
#define IA_EXHEAACE_EXE_FATAL_MPS_INVALID_LEVELS (0xFFFF990A)
#define IA_EXHEAACE_EXE_FATAL_MPS_CFFT_PROCESS (0xFFFF990B)

// USAC

// DRC
