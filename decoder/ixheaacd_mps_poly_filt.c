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
#include <string.h>
#include <math.h>
#include <ixheaacd_type_def.h>
#include <ixheaacd_interface.h>
#include "ixheaacd_mps_polyphase.h"
#include <ixheaacd_type_def.h>
#include "ixheaacd_bitbuffer.h"
#include "ixheaacd_config.h"
#include "ixheaacd_mps_dec.h"
#include "ixheaacd_mps_interface.h"
#include "ixheaacd_constants.h"
#include <ixheaacd_basic_ops32.h>
#include "ixheaacd_function_selector.h"

extern const WORD32
    ixheaacd_mps_polyphase_filter_coeff_fix[10 * MAX_NUM_QMF_BANDS_SAC / 2];
extern WORD32 ixheaacd_mps_pre_re[64];
extern WORD32 ixheaacd_mps_pre_im[64];
extern WORD32 ixheaacd_mps_post_re[128];
extern WORD32 ixheaacd_mps_post_im[128];

static PLATFORM_INLINE WORD32 ixheaacd_mult32(WORD32 a, WORD32 b) {
  WORD32 result;
  WORD64 temp_result;

  temp_result = (WORD64)a * (WORD64)b;
  result = (WORD32)(temp_result >> 31);

  return (result);
}

VOID ixheaacd_mps_synt_create(ia_mps_poly_phase_struct *kernel,
                              WORD32 resolution) {
  kernel->resolution = resolution;
}

VOID ixheaacd_mps_synt_init(ia_mps_poly_phase_synth_struct *self) {
  memset(self->state, 0, sizeof(WORD32) * 64 * 20);
}

static VOID ixheaacd_float_to_int32(FLOAT32 *in, WORD32 *out, WORD32 q_factor,
                                    WORD32 sample) {
  WORD32 loop;
  UWORD32 temp = (1 << q_factor);

  for (loop = 0; loop < sample; loop++) out[loop] = (WORD32)(in[loop] * temp);
}

VOID ixheaacd_mps_synt_pre_twiddle_dec(WORD32 *ptr_in, WORD32 *table_re,
                                       WORD32 *table_im, WORD32 resolution) {
  WORD32 tmp, k;
  for (k = 0; k < 2 * resolution; k += 2) {
    tmp = ixheaacd_add32_sat(ixheaacd_mult32(ptr_in[k], table_re[k >> 1]),
                             ixheaacd_mult32(ptr_in[k + 1], table_im[k >> 1]));
    ptr_in[k + 1] = ixheaacd_add32_sat(
        ixheaacd_mult32(ixheaacd_negate32_sat(ptr_in[k]), table_im[k >> 1]),
        ixheaacd_mult32(ptr_in[k + 1], table_re[k >> 1]));

    ptr_in[k] = tmp;
  }
}

VOID ixheaacd_mps_synt_post_twiddle_dec(WORD32 *ptr_in, WORD32 *table_re,
                                        WORD32 *table_im, WORD32 resolution) {
  WORD32 tmp, k;
  for (k = 0; k < 2 * resolution; k += 2) {
    tmp = ixheaacd_add32_sat(ixheaacd_mult32(ptr_in[k], table_re[k]),
                             ixheaacd_mult32(ptr_in[k + 1], table_im[k]));

    ptr_in[k + 1] =
        ixheaacd_add32_sat(ixheaacd_mult32(-ptr_in[k], table_im[k]),
                           ixheaacd_mult32(ptr_in[k + 1], table_re[k]));

    ptr_in[k] = tmp;
  }
}

VOID ixheaacd_mps_synt_post_fft_twiddle_dec(WORD32 resolution, WORD32 *fin_re,
                                            WORD32 *fin_im, WORD32 *table_re,
                                            WORD32 *table_im, WORD32 *state) {
  WORD32 l;
  for (l = 0; l < 2 * resolution; l++) {
    state[2 * resolution - l - 1] =
        ixheaacd_add32_sat(ixheaacd_mult32(fin_re[l], table_re[l]),
                           ixheaacd_mult32(fin_im[l], table_im[l]));
  }
}

VOID ixheaacd_mps_synt_out_calc_dec(WORD32 resolution, WORD32 *out,
                                    WORD32 *state, const WORD32 *filter_coeff) {
  WORD32 l, k;
  WORD32 *out1, *out2, *state1, *state2;
  out1 = out;
  out2 = out + resolution;
  state1 = state;
  state2 = state + (3 * resolution);

  for (k = 0; k < 5; k++) {
    for (l = 0; l < resolution; l++) {
      *out1++ = (WORD32)(((WORD64)(*state1++) * (*filter_coeff++)) >> 31);
      *out2++ = (WORD32)(((WORD64)(*state2++) * (*filter_coeff++)) >> 31);
    }
    out1 += resolution;
    out2 += resolution;
    state1 += (3 * resolution);
    state2 += (3 * resolution);
  }
}

VOID ixheaacd_mps_synt_calc(ia_mps_dec_state_struct *self) {
  WORD32 k, l, ts, ch;
  WORD64 acc;
  WORD32 ptr_in[128];
  WORD32 fin_re[128];
  WORD32 fin_im[128];
  FLOAT32 temp;
  WORD32 *state, *tmp_state, *out;
  const WORD32 *filt_coeff;
  WORD32 *tmp_buf = self->tmp_buf;

  ia_mps_poly_phase_struct kernel = self->poly_phase_filt_kernel;
  WORD32 resolution = kernel.resolution;
  for (ch = 0; ch < self->out_ch_count; ch++) {
    tmp_state = (&self->qmf_filt_state[ch])->state;
    state = &tmp_buf[self->time_slots * 2 * resolution];
    memcpy(state, tmp_state, sizeof(WORD32) * 20 * resolution);
    out = &tmp_buf[74 * MAX_NUM_QMF_BANDS_SAC];

    for (ts = 0; ts < self->time_slots; ts++) {
      ixheaacd_float_to_int32(&self->qmf_out_dir[ch][ts][0].re, ptr_in, 10,
                              resolution * 2);

      filt_coeff = ixheaacd_mps_polyphase_filter_coeff_fix;

      state -= (2 * resolution);
      (*ixheaacd_mps_synt_pre_twiddle)(ptr_in, ixheaacd_mps_pre_re,
                                       ixheaacd_mps_pre_im, resolution);

      (*ixheaacd_mps_complex_fft_64)(ptr_in, fin_re, fin_im, resolution);

      (*ixheaacd_mps_synt_post_twiddle)(ptr_in, ixheaacd_mps_post_re,
                                        ixheaacd_mps_post_im, resolution);

      (*ixheaacd_mps_complex_fft_64)(ptr_in, &fin_re[1], &fin_im[1],
                                     resolution);

      (*ixheaacd_mps_synt_post_fft_twiddle)(resolution, fin_re, fin_im,
                                            ixheaacd_mps_post_re,
                                            ixheaacd_mps_post_im, state);
      (*ixheaacd_mps_synt_out_calc)(resolution, out, state, filt_coeff);

      for (k = 0; k < resolution; k++) {
        acc = 0;
        for (l = 0; l < 10; l++) {
          acc = acc + out[resolution * l + k];
        }
        if (acc >= 2147483647)
          temp = 1.0;
        else if (acc <= -2147483647 - 1)
          temp = -1.0f;
        else
          temp = (FLOAT32)((WORD32)acc) / ((FLOAT32)(1 << 10));

        self->output_buffer[ch][self->qmf_band_count * ts + k] = (FLOAT32)temp;
      }
    }

    memcpy(tmp_state, state, sizeof(WORD32) * 20 * resolution);
  }
}
