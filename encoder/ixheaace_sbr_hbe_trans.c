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

#include "ixheaace_sbr_hbe.h"
#include "ixheaace_sbr_hbe_fft.h"
#include "ixheaac_error_standards.h"
#include "ixheaace_error_codes.h"
#include "ixheaac_constants.h"

VOID ixheaace_hbe_repl_spec(WORD32 x_over_qmf[IXHEAACE_MAX_NUM_PATCHES],
                            FLOAT32 qmf_buf_real[][64], FLOAT32 qmf_buf_imag[][64],
                            WORD32 no_bins, WORD32 max_stretch) {
  WORD32 patch_bands;
  WORD32 patch, band, col, target, source_bands, i;
  WORD32 num_patches = 0;

  for (i = 1; i < IXHEAACE_MAX_NUM_PATCHES; i++) {
    if (x_over_qmf[i] != 0) {
      num_patches++;
    }
  }

  for (patch = (max_stretch - 1); patch < num_patches; patch++) {
    patch_bands = x_over_qmf[patch + 1] - x_over_qmf[patch];
    target = x_over_qmf[patch];
    source_bands = x_over_qmf[max_stretch - 1] - x_over_qmf[max_stretch - 2];

    while (patch_bands > 0) {
      WORD32 ixheaace_num_bands = source_bands;
      WORD32 start_band = x_over_qmf[max_stretch - 1] - 1;
      if (target + ixheaace_num_bands >= x_over_qmf[patch + 1]) {
        ixheaace_num_bands = x_over_qmf[patch + 1] - target;
      }
      if ((((target + ixheaace_num_bands - 1) & 1) + ((x_over_qmf[max_stretch - 1] - 1) & 1)) &
          1) {
        if (ixheaace_num_bands == source_bands) {
          ixheaace_num_bands--;
        } else {
          start_band--;
        }
      }

      if (!ixheaace_num_bands) {
        break;
      }

      for (col = 0; col < no_bins; col++) {
        band = target + ixheaace_num_bands - 1;
        if (64 <= band) {
          band = 63;
        }
        if (x_over_qmf[patch + 1] <= band) {
          band = x_over_qmf[patch + 1] - 1;
        }
        for (i = 0; i < ixheaace_num_bands; i++, band--) {
          qmf_buf_real[col][band] = qmf_buf_real[col][start_band - i];
          qmf_buf_imag[col][band] = qmf_buf_imag[col][start_band - i];
        }
      }
      target += ixheaace_num_bands;
      patch_bands -= ixheaace_num_bands;
    }
  }
}
