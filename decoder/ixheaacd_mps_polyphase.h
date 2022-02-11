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
#ifndef IXHEAACD_MPS_POLYPHASE_H
#define IXHEAACD_MPS_POLYPHASE_H

#define MAX_NUM_QMF_BANDS_SAC (128)
#define POLY_PHASE_SYNTH_SIZE (1152)

typedef struct ia_mps_poly_phase_synth_struct {
  FLOAT32 state[POLY_PHASE_SYNTH_SIZE];
} ia_mps_poly_phase_synth_struct;

typedef struct ia_mps_poly_phase_struct {
  int resolution;
} ia_mps_poly_phase_struct;

VOID ixheaacd_mps_synt_create(ia_mps_poly_phase_struct *kernel,
                              WORD32 resolution);

VOID ixheaacd_mps_synt_init(ia_mps_poly_phase_synth_struct *self);

#endif
