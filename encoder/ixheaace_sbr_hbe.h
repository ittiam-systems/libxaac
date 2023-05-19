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

#define IXHEAACE_SBR_HF_ADJ_OFFSET (2)
#define IXHEAACE_ESBR_HBE_DELAY_OFFSET (32)
#define IXHEAACE_TIMESLOT_BUFFER_SIZE (78)
#define IXHEAACE_MAX_NUM_PATCHES (6)
#define IXHEAACE_MAX_STRETCH (4)
#define IXHEAACE_MAX_NUM_LIMITERS (12)
#define IXHEAACE_MAX_FREQ_COEFFS (56)
#define IXHEAACE_MAX_NOISE_COEFFS (5)

#define IXHEAACE_HBE_OPER_WIN_LEN (13)
#define IXHEAACE_NUM_QMF_SYNTH_CHANNELS (64)
#define IXHEAACE_TWICE_QMF_SYNTH_CH_NUM (128)
#define IXHEAACE_HBE_ZERO_BAND_IDX (6)
#define IXHEAACE_HBE_OPER_BLK_LEN_2 (10)
#define IXHEAACE_HBE_OPER_BLK_LEN_3 (8)
#define IXHEAACE_HBE_OPER_BLK_LEN_4 (6)
#define IXHEAACE_FD_OVERSAMPLING_FAC (1.5f)

#define IXHEAACE_MAX_NO_COLS_VALUE (64)
#define IXHEAACE_MAX_FRAME_SIZE (1024)
#define IXHEAACE_MAX_NUM_SAMPLES (4096)
#define IXHEAACE_MAX_QMF_X_INBUF_SIZE (IXHEAACE_MAX_NO_COLS_VALUE)
#define IXHEAACE_MAX_QMF_X_OUTBUF_SIZE (2 * IXHEAACE_MAX_QMF_X_INBUF_SIZE)

#define IXHEAACE_MAX_QMF_X_IN_REAL_BUF \
  (IXHEAACE_NUM_QMF_SYNTH_CHANNELS * IXHEAACE_MAX_QMF_X_INBUF_SIZE)
#define IXHEAACE_MAX_QMF_X_IN_IMAG_BUF \
  (IXHEAACE_NUM_QMF_SYNTH_CHANNELS * IXHEAACE_MAX_QMF_X_INBUF_SIZE)

#define IXHEAACE_MAX_QMF_X_OUT_REAL_BUF \
  (IXHEAACE_NUM_QMF_SYNTH_CHANNELS * IXHEAACE_MAX_QMF_X_OUTBUF_SIZE)
#define IXHEAACE_MAX_QMF_X_OUT_IMAG_BUF \
  (IXHEAACE_NUM_QMF_SYNTH_CHANNELS * IXHEAACE_MAX_QMF_X_OUTBUF_SIZE)

#define IXHEAACE_X_INBUF_SIZE (IXHEAACE_MAX_FRAME_SIZE + IXHEAACE_NUM_QMF_SYNTH_CHANNELS)
#define IXHEAACE_X_OUTBUF_SIZE (IXHEAACE_X_INBUF_SIZE * 2)

#define IXHEAACE_MAX_HBE_PERSISTENT_SIZE                                                         \
  (IXHEAACE_MAX_QMF_X_INBUF_SIZE * sizeof(FLOAT32 *) +                                           \
   IXHEAACE_MAX_QMF_X_OUTBUF_SIZE * sizeof(FLOAT32 *) +                                          \
   IXHEAACE_MAX_QMF_X_IN_REAL_BUF * sizeof(FLOAT32) +                                            \
   IXHEAACE_MAX_QMF_X_IN_IMAG_BUF * sizeof(FLOAT32) +                                            \
   IXHEAACE_MAX_QMF_X_OUT_REAL_BUF * sizeof(FLOAT32) +                                           \
   IXHEAACE_MAX_QMF_X_OUT_IMAG_BUF * sizeof(FLOAT32) + IXHEAACE_X_INBUF_SIZE * sizeof(FLOAT32) + \
   IXHEAACE_X_OUTBUF_SIZE * sizeof(FLOAT32))

#define IXHEAACE_LOW (0)
#define IXHEAACE_HIGH (1)

#define IXHEAACE_SBR_CONST_PMIN 1.0f

#define ixheaace_cbrt_calc(a) (pow(a, -0.333333f))

#define IXHEAACE_QMF_FILTER_STATE_ANA_SIZE 320
