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
/* Constant hash defines                                                     */
/*****************************************************************************/
/* A constant to let API copy small strings to buffers outside */
#define IA_APIVERSION_MAJOR 1
#define IA_APIVERSION_MINOR 10

/* last compatible version */

#define IA_LASTCOMP_APIVERSION_MAJOR 1
#define IA_LASTCOMP_APIVERSION_MINOR 10

#define IA_STR(str) #str
#define IA_MAKE_VERSION_STR(maj, min) IA_STR(maj) "." IA_STR(min)
#define IA_APIVERSION IA_MAKE_VERSION_STR(IA_APIVERSION_MAJOR, IA_APIVERSION_MINOR)

#define IA_LAST_COMP_APIVERSION \
  IA_MAKE_VERSION_STR(IA_LASTCOMP_APIVERSION_MAJOR, IA_LASTCOMP_APIVERSION_MINOR)

VOID ia_enhaacplus_enc_set_shared_bufs(iaace_scratch *scr, WORD32 **shared_buf1,
                                       WORD32 **shared_buf2, WORD32 **shared_buf3,
                                       WORD8 **shared_buf5);
