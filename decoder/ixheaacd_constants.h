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
#ifndef IXHEAACD_CONSTANTS_H
#define IXHEAACD_CONSTANTS_H

/*****************************************************************************/
/* constant macros                                                           */
/*****************************************************************************/
#define Q0 1
#define Q1 2
#define Q2 4
#define Q3 8
#define Q4 16
#define Q5 32
#define Q6 64
#define Q7 128
#define Q8 256
#define Q9 512
#define Q10 1024
#define Q11 2048
#define Q14 16384
#define Q15 32768
#define Q16 65536
#define Q18 262144
#define Q19 524288
#define Q20 1048576
#define Q24 16777216
#define Q25 33554432
#define Q26 67108864
#define Q28 268435456
#define Q29 536870912
#define Q30 1073741824
#define Q31 2147483647
#define Q32 4294967296
#define Q35 34359738368
#define Q38 274877906944
#define Q39 549755813887
#define Q40 Q39

#define MAX_64 (WORD64)0x7fffffffffffffff
#define MIN_64 (WORD64)0x8000000000000000

#define MAX_32 (WORD32)0x7fffffffL
#define MIN_32 (WORD32)0x80000000L

#define MAX_16 (WORD16)0x7fff
#define MIN_16 (WORD16)0x8000

#define NULLPTR ((VOID *)0)

#define IT_NULL ((VOID *)0)
/*****************************************************************************/
/* function macros                                                           */
/*****************************************************************************/
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#endif /* IXHEAACD_CONSTANTS_H */
