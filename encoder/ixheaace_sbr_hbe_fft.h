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

VOID ixheaace_cmplx_anal_fft_p2(FLOAT32 *ptr_x, FLOAT32 *ptr_y, WORD32 npoints);

VOID ixheaace_cmplx_anal_fft_p3(FLOAT32 *ptr_x_in, FLOAT32 *ptr_x_out, WORD32 npoints);

VOID ixheaace_real_synth_fft_p2(FLOAT32 *ptr_x, FLOAT32 *ptr_y, WORD32 npoints);

VOID ixheaace_real_synth_fft_p3(FLOAT32 *ptr_x_in, FLOAT32 *ptr_x_out, WORD32 npoints);
