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
/****************************************************************************/
/*     types               type define    prefix		examples
 * bytes */
/************************  ***********    ******    ****************  ***** */

#define SIZE_T size_t

typedef char CHAR8;   /* c       CHAR8    c_name     1   */
typedef char *pCHAR8; /* pc      pCHAR8   pc_nmae    1   */

typedef signed char WORD8;   /* b       WORD8    b_name     1   */
typedef signed char *pWORD8; /* pb      pWORD8   pb_nmae    1   */

typedef unsigned char UWORD8;   /*	ub		UWORD8	 ub_count	1	*/
typedef unsigned char *pUWORD8; /*	pub		pUWORD8	 pub_count	1	*/

typedef signed short WORD16;      /* s       WORD16   s_count    2   */
typedef signed short *pWORD16;    /* ps      pWORD16  ps_count   2   */
typedef unsigned short UWORD16;   /*	us		UWORD16	 us_count	2	*/
typedef unsigned short *pUWORD16; /*	pus		pUWORD16 pus_count	2	*/

typedef signed int WORD24;      /* k       WORD24   k_count    3   */
typedef signed int *pWORD24;    /* pk      pWORD24  pk_count   3   */
typedef unsigned int UWORD24;   /*	uk		UWORD24	 uk_count	3	*/
typedef unsigned int *pUWORD24; /*	puk		pUWORD24 puk_count	3	*/

typedef signed int WORD32;      /* i       WORD32   i_count    4   */
typedef signed int *pWORD32;    /* pi      pWORD32  pi_count   4   */
typedef unsigned int UWORD32;   /*	ui		UWORD32	 ui_count	4	*/
typedef unsigned int *pUWORD32; /*	pui		pUWORD32 pui_count	4	*/

typedef signed long long WORD40;      /*	m		WORD40	 m_count	5	*/
typedef signed long long *pWORD40;    /*	pm		pWORD40	 pm_count	5	*/
typedef unsigned long long UWORD40;   /*	um		UWORD40	 um_count	5	*/
typedef unsigned long long *pUWORD40; /*	pum		pUWORD40 pum_count	5	*/

typedef signed long long WORD64;      /*	h		WORD64	 h_count	8	*/
typedef signed long long *pWORD64;    /*	ph		pWORD64	 ph_count	8	*/
typedef unsigned long long UWORD64;   /*	uh		UWORD64	 uh_count	8	*/
typedef unsigned long long *pUWORD64; /*	puh		pUWORD64 puh_count	8	*/

typedef float FLOAT32;    /*	f		FLOAT32	 f_count	4
                           */
typedef float *pFLOAT32;  /* pf      pFLOAT32 pf_count   4   */
typedef double FLOAT64;   /*	d		UFLOAT64 d_count	8
                           */
typedef double *pFlOAT64; /* pd      pFLOAT64 pd_count   8   */

typedef void VOID;   /*	v		VOID	 v_flag		4	*/
typedef void *pVOID; /*	pv		pVOID	 pv_flag	4	*/

/* variable size types: platform optimized implementation */
typedef signed int BOOL;       /* bool    BOOL     bool_true      */
typedef unsigned int UBOOL;    /*	ubool	BOOL	 ubool_true		*/
typedef signed int FLAG;       /* flag    FLAG     flag_false     */
typedef unsigned int UFLAG;    /* uflag	FLAG	 uflag_false	*/
typedef signed int LOOPIDX;    /* lp      LOOPIDX  lp_index       */
typedef unsigned int ULOOPIDX; /*	ulp		SLOOPIDX ulp_index		*/
typedef signed int WORD;       /* lp      LOOPIDX  lp_index       */
typedef unsigned int UWORD;    /*	ulp		SLOOPIDX ulp_index		*/

typedef LOOPIDX LOOPINDEX;   /* lp    LOOPIDX  lp_index       */
typedef ULOOPIDX ULOOPINDEX; /* ulp   SLOOPIDX ulp_index      */

typedef signed int IA_ERRORCODE;

#define PLATFORM_INLINE __inline
