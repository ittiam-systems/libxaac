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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "ixheaac_type_def.h"
#include "ixheaace_bitbuffer.h"
#include "iusace_cnst.h"
#include "ixheaace_sbr_def.h"
#include "ixheaace_resampler.h"
#include "ixheaace_sbr_hbe.h"
#include "ixheaace_sbr_hbe_fft.h"
#include "ixheaace_error_codes.h"
#include "ixheaace_common_rom.h"
#include "ixheaac_error_standards.h"
#include "ixheaac_constants.h"

VOID ixheaace_karth2polar(FLOAT32 *ptr_spectrum, FLOAT32 *ptr_mag, FLOAT32 *ptr_phase,
                          WORD32 fft_size) {
  WORD32 n;

  for (n = 1; n < fft_size / 2; n++) {
    ptr_phase[n] = (FLOAT32)atan2(ptr_spectrum[2 * n + 1], ptr_spectrum[2 * n]);
    ptr_mag[n] = (FLOAT32)sqrt(ptr_spectrum[2 * n] * ptr_spectrum[2 * n] +
                               ptr_spectrum[2 * n + 1] * ptr_spectrum[2 * n + 1]);
  }

  if (ptr_spectrum[0] < 0) {
    ptr_phase[0] = (FLOAT32)acos(-1);
    ptr_mag[0] = -ptr_spectrum[0];
  } else {
    ptr_phase[0] = 0;
    ptr_mag[0] = ptr_spectrum[0];
  }

  if (ptr_spectrum[1] < 0) {
    ptr_phase[fft_size / 2] = (FLOAT32)acos(-1);
    ptr_mag[fft_size / 2] = -ptr_spectrum[1];
  } else {
    ptr_phase[fft_size / 2] = 0;
    ptr_mag[fft_size / 2] = ptr_spectrum[1];
  }
}
