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
#include "ixheaacd_sbr_common.h"
#include <ixheaacd_type_def.h>

#include "ixheaacd_constants.h"
#include <ixheaacd_basic_ops32.h>
#include <ixheaacd_basic_ops16.h>
#include <ixheaacd_basic_ops40.h>
#include "ixheaacd_basic_ops.h"
#include "ixheaacd_common_rom.h"
#include "ixheaacd_dsp_fft32x32s.h"
#include "ixheaacd_intrinsics.h"

#include <ixheaacd_basic_op.h>

VOID ixheaacd_inv_dit_fft_8pt_dec(WORD32 *y, WORD32 *real, WORD32 *imag) {
  WORD32 a0, a1, a2, a3, a00, a10, a20, a30;
  WORD32 vr, vi;

  WORD32 x[16];

  a00 = ixheaacd_add32_sat(y[0], y[8]);
  a0 = ixheaacd_sub32_sat(y[0], y[8]);

  a20 = ixheaacd_add32_sat(y[1], y[9]);
  a3 = ixheaacd_sub32_sat(y[1], y[9]);

  a10 = ixheaacd_add32_sat(y[4], y[12]);
  a2 = ixheaacd_sub32_sat(y[4], y[12]);

  a30 = ixheaacd_add32_sat(y[5], y[13]);
  a1 = ixheaacd_sub32_sat(y[5], y[13]);

  x[0] = ixheaacd_add32_sat(a00, a10);
  x[4] = ixheaacd_sub32_sat(a00, a10);
  x[1] = ixheaacd_add32_sat(a20, a30);
  x[5] = ixheaacd_sub32_sat(a20, a30);

  x[2] = ixheaacd_sub32_sat(a0, a1);
  x[6] = ixheaacd_add32_sat(a0, a1);
  x[3] = ixheaacd_add32_sat(a3, a2);
  x[7] = ixheaacd_sub32_sat(a3, a2);

  a00 = ixheaacd_add32_sat(y[2], y[10]);
  a0 = ixheaacd_sub32_sat(y[2], y[10]);

  a20 = ixheaacd_add32_sat(y[3], y[11]);
  a3 = ixheaacd_sub32_sat(y[3], y[11]);

  a10 = ixheaacd_add32_sat(y[6], y[14]);
  a2 = ixheaacd_sub32_sat(y[6], y[14]);

  a30 = ixheaacd_add32_sat(y[7], y[15]);
  a1 = ixheaacd_sub32_sat(y[7], y[15]);

  x[8] = ixheaacd_add32_sat(a00, a10);
  x[12] = ixheaacd_sub32_sat(a00, a10);
  x[9] = ixheaacd_add32_sat(a20, a30);
  x[13] = ixheaacd_sub32_sat(a20, a30);

  x[10] = ixheaacd_sub32_sat(a0, a1);
  x[14] = ixheaacd_add32_sat(a0, a1);
  x[11] = ixheaacd_add32_sat(a3, a2);
  x[15] = ixheaacd_sub32_sat(a3, a2);

  real[0] = ixheaacd_add32_sat(x[0], x[8]);
  imag[0] = ixheaacd_add32_sat(x[1], x[9]);
  a00 = ixheaacd_sub32_sat(x[0], x[8]);
  a10 = ixheaacd_sub32_sat(x[1], x[9]);

  a0 = ixheaacd_sub32_sat(x[4], x[13]);
  a1 = ixheaacd_add32_sat(x[5], x[12]);

  real[4] = ixheaacd_add32_sat(x[4], x[13]);
  imag[4] = ixheaacd_sub32_sat(x[5], x[12]);

  vr = ixheaacd_mult32x16in32_shl_sat(ixheaacd_sub32_sat(x[10], x[11]), 0x5A82);
  vi = ixheaacd_mult32x16in32_shl_sat(ixheaacd_add32_sat(x[10], x[11]), 0x5A82);

  real[1] = ixheaacd_add32_sat(x[2], vr);
  imag[1] = ixheaacd_add32_sat(x[3], vi);

  a2 = ixheaacd_sub32_sat(x[2], vr);
  a3 = ixheaacd_sub32_sat(x[3], vi);

  real[2] = ixheaacd_add32_sat(a0, a2);
  imag[2] = ixheaacd_add32_sat(a1, a3);
  vr = ixheaacd_mult32x16in32_shl_sat(ixheaacd_add32_sat(x[14], x[15]), 0x5A82);
  vi = ixheaacd_mult32x16in32_shl_sat(ixheaacd_sub32_sat(x[14], x[15]), 0x5A82);

  a20 = ixheaacd_sub32_sat(x[6], vr);
  a30 = ixheaacd_add32_sat(x[7], vi);

  real[3] = ixheaacd_add32_sat(a00, a20);
  imag[3] = ixheaacd_add32_sat(a10, a30);

  real[5] = ixheaacd_add32_sat(x[6], vr);
  imag[5] = ixheaacd_sub32_sat(x[7], vi);
}
