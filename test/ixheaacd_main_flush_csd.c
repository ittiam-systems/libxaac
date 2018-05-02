/*****************************************************************************/
/*                                                                           */
/*                        Enhanced aacPlus Decoder                           */
/*                                                                           */
/*                   ITTIAM SYSTEMS PVT LTD, BANGALORE                       */
/*                          COPYRIGHT(C) 2004                                */
/*                                                                           */
/*  This program is proprietary to Ittiam Systems Pvt. Ltd. and is protected */
/*  under Indian Copyright Act as an unpublished work.Its use and disclosure */
/*  is  limited by  the terms and conditions of a license  agreement. It may */
/*  be copied or  otherwise reproduced or  disclosed  to persons outside the */
/*  licensee 's  organization  except  in  accordance  with  the  terms  and */
/*  conditions of  such an agreement. All  copies and reproductions shall be */
/*  the  property  of Ittiam Systems Pvt.  Ltd. and  must  bear  this notice */
/*  in its entirety.                                                         */
/*                                                                           */
/*****************************************************************************/

/*
   Main for enhanced aacPlus decoding
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define AOSP_CHANGE

#include "ixheaacd_fileifc.h"
#include <ixheaacd_type_def.h>
#include "ixheaacd_error_standards.h"
#include "ixheaacd_error_handler.h"
#include "ixheaacd_apicmd_standards.h"
#include "ixheaacd_memory_standards.h"
#include "ixheaacd_aac_config.h"
#include "ixheaacd_metadata_read.h"

IA_ERRORCODE ixheaacd_dec_api(pVOID p_ia_module_obj, WORD32 i_cmd, WORD32 i_idx,
                              pVOID pv_value);

VOID ixheaacd_error_handler_init();
VOID ia_testbench_error_handler_init();

extern ia_error_info_struct ixheaacd_ia_testbench_error_info;
extern ia_error_info_struct ixheaacd_error_info;
// extern int num_of_output_ch;

/*****************************************************************************/
/* Process select hash defines                                               */
/*****************************************************************************/
#define WAV_HEADER
#define DISPLAY_MESSAGE
//#define REINIT_FOR_ERROR
//#define ERROR_PATTERN_READ
//#define TEST_INSUFFICIENT_INPUT 1
//#define TEST_FRAMEWISE_INPUT

#ifdef TEST_FRAMEWISE_INPUT
// Include header file for array of frame sizes : Audio_FrameSize
//#include "0_3gp_4.h"
#endif

/*****************************************************************************/
/* Constant hash defines                                                     */
/*****************************************************************************/
#define MAX_STACK_PROC 10
#define MAX_MEM_ALLOCS 100
#define IA_MAX_CMD_LINE_LENGTH 300
#define IA_MAX_ARGS 20
#define IA_SCREEN_WIDTH 80
#define PARAMFILE "paramfilesimple.txt"

/*****************************************************************************/
/* Error codes for the testbench                                             */
/*****************************************************************************/
#define IA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED 0xFFFF8000
#define IA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED 0xFFFF8001

#define IA_HE_AAC_DEC_TABLE_RELOCATABLE_ENABLE 0

#ifdef ARM_PROFILE
#include "armtimer.h"
#endif

#ifdef ARM_PROFILE_HW
#include <sys/time.h>
#define CLK_FREQ_BOARD_MHZ 716  // a9 omap4430 board
//#define CLK_FREQ_BOARD_MHZ 1555 //Nexus6P
//#define CLK_FREQ_BOARD_MHZ 2035 //Tegra Board
//#define CLK_FREQ_BOARD_MHZ 550 //a8  board
//#define CLK_FREQ_BOARD_MHZ 297 //dm365 board
//#define CLK_FREQ_BOARD_MHZ 1209.6 //a5 board
#endif
#ifdef ARM_PROFILE_HW
long long itGetMs(void) {
  struct timeval t;
  long long currTime;

  if (gettimeofday(&t, NULL) == -1) {
    printf("Error in gettimeofday. It has returned -1. \n");
  }
  currTime = ((t.tv_sec * 1000 * 1000) + (t.tv_usec));
  return currTime;
}
#endif
/*****************************************************************************/
/* Global variables                                                          */
/*****************************************************************************/
pVOID g_pv_arr_alloc_memory[MAX_MEM_ALLOCS];
WORD g_w_malloc_count;
FILE *g_pf_out;
FileWrapperPtr g_pf_inp; /* file pointer to bitstream file (mp4) */

metadata_info meta_info;  // metadata pointer;
WORD32 ixheaacd_i_bytes_to_read;
FILE *g_pf_meta;

WORD32 raw_testing = 0;

#define _IA_PRINT_ERROR(p_mod_err_info, context, e)           \
  if ((e) != IA_NO_ERROR) {                                   \
    ixheaacd_error_handler((p_mod_err_info), (context), (e)); \
  }

#ifdef ERROR_PATTERN_READ
FILE *g_pf_err = NULL; /* file pointer to error pattern file */
#endif

//#define CHECK_STACK_USAGE
#ifdef CHECK_STACK_USAGE
int *stack_corrupt, stack_used;
void stack_corrupt_func() {
  int i, stack_val2;
  stack_corrupt = &stack_val2;
  for (i = 10; i < 2048; i++) {
    stack_corrupt[-(i)] = 0xcfadbe3d;
  }
}
#endif

#ifndef WIN32
#ifdef ITTIAM_BIG_ENDIAN

int ia_fwrite(pVOID buffer[], int size, int nwords, FILE *fp) {
  int i, k;
  pWORD8 pb_buf = (pWORD8)buffer;

  for (i = 0; i < nwords; i++) {
    for (k = size - 1; k >= 0; k--) {
      putc(pb_buf[i * size + k], fp);
    }
  }
  return 1;
}

#else
int ia_fwrite(pVOID buffer[], int size, int nwords, FILE *fp) {
  int i, j;
  pWORD8 pb_buf = (pWORD8)buffer;

  for (i = 0; i < nwords; i++) {
    for (j = 0; j < size; j++) {
      putc(pb_buf[i * size + j], fp);
    }
  }
  return 1;
}

#endif
#endif
#ifdef ERROR_PATTERN_READ
/*****************************************************************************/
/*                                                                           */
/*  Function name : ReadErrorPatternFile                                     */
/*                                                                           */
/*  Description   : The function reads the frameError flag from a file.      */
/*                                                                           */
/*  Inputs        : FILE *epf (Input file to read from)                      */
/*                                                                           */
/*  Globals       : none                                                     */
/*                                                                           */
/*  Outputs       : none                                                     */
/*                                                                           */
/*  Returns       : 1 if the frame is ok, else 0                             */
/*                                                                           */
/*  Issues        : none                                                     */
/*                                                                           */
/*  Revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*        29 07 2005       Ittiam                Created                     */
/*                                                                           */
/*****************************************************************************/

static char ReadErrorPatternFile(FILE *epf) {
  char tmp;
  int readOk;

  if (!epf) return 1;

  readOk = fscanf(epf, "%c\n", &tmp);
  if (readOk != 1) {
    rewind(epf);
    fscanf(epf, "%c\n", &tmp);
  }
  if (tmp == '0')
    return 1;
  else
    return 0;
}
#endif

#ifdef WAV_HEADER
#ifndef ARM_PROFILE_BOARD
/*****************************************************************************/
/*                                                                           */
/*  Function name : ixheaacd_write16_bits_lh */
/*                                                                           */
/*  Description   : write 16 bits low high (always little endian)            */
/*                                                                           */
/*  Inputs        : none                                                     */
/*                                                                           */
/*  Globals       : pVOID g_pv_arr_alloc_memory[MAX_MEM_ALLOCS];             */
/*                  WORD  g_w_malloc_count;                                  */
/*                  FILE *g_pf_inp, *g_pf_out;                               */
/*                                                                           */
/*  Processing    : write 16 bits low high (always little endian)            */
/*                                                                           */
/*  Outputs       : none                                                     */
/*                                                                           */
/*  Returns       : none                                                     */
/*                                                                           */
/*  Issues        : none                                                     */
/*                                                                           */
/*  Revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*        29 07 2005       Ittiam                Created                     */
/*                                                                           */
/*****************************************************************************/

static VOID ixheaacd_write16_bits_lh(FILE *fp, WORD32 i) {
  putc(i & 0xff, fp);
  putc((i >> 8) & 0xff, fp);
}

/*****************************************************************************/
/*                                                                           */
/*  Function name : ixheaacd_write32_bits_lh */
/*                                                                           */
/*  Description   : write 32 bits low high (always little endian)            */
/*                                                                           */
/*  Inputs        : none                                                     */
/*                                                                           */
/*  Globals       : FILE* fp (file to write)                                 */
/*                  WORD32 i (value to write)                                */
/*                                                                           */
/*  Processing    : write 32 bits low high (always little endian)            */
/*                                                                           */
/*  Outputs       : none                                                     */
/*                                                                           */
/*  Returns       : none                                                     */
/*                                                                           */
/*  Issues        : none                                                     */
/*                                                                           */
/*  Revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*        29 07 2005       Ittiam                Created                     */
/*                                                                           */
/*****************************************************************************/

static VOID ixheaacd_write32_bits_lh(FILE *fp, WORD32 i) {
  ixheaacd_write16_bits_lh(fp, (WORD32)(i & 0xffffL));
  ixheaacd_write16_bits_lh(fp, (WORD32)((i >> 16) & 0xffffL));
}

/*****************************************************************************/
/*                                                                           */
/*  Function name : write_wav_header                                         */
/*                                                                           */
/*  Description   : Write wav header to a wav file                           */
/*                                                                           */
/*  Inputs        : none                                                     */
/*                                                                           */
/*  Globals       : FILE* fp (file to write)                                 */
/*                  WORD32 pcmbytes (total bytes in wav file)                */
/*                  WORD32 freq (sampling freq)                              */
/*                  WORD32 channels (output channels)                        */
/*                  WORD32 bits (bits per sample)                            */
/*                                                                           */
/*  Processing    : Write wav header                                         */
/*                                                                           */
/*  Outputs       : none                                                     */
/*                                                                           */
/*  Returns       : none                                                     */
/*                                                                           */
/*  Issues        : none                                                     */
/*                                                                           */
/*  Revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*        29 07 2005       Ittiam                Created                     */
/*                                                                           */
/*****************************************************************************/

WORD32 write_wav_header(FILE *fp, WORD32 pcmbytes, WORD32 freq, WORD32 channels,
                        WORD32 bits, WORD32 i_channel_mask) {
  if (channels > 2) {
    WORD32 bytes = (bits + 7) / 8;
    fwrite("RIFF", 1, 4, fp); /* label */
    ixheaacd_write32_bits_lh(
        fp, pcmbytes + 44 - 8);   /* length in bytes without header */
    fwrite("WAVEfmt ", 2, 4, fp); /* 2 labels */
    /* tag for WAVE_FORMAT_EXTENSIBLE */
    if (channels > 2) {
      ixheaacd_write16_bits_lh(fp, 0x28);
      ixheaacd_write16_bits_lh(fp, 0x00);
      ixheaacd_write16_bits_lh(fp, 0xfffe);
    } else {
      ixheaacd_write32_bits_lh(
          fp, 2 + 2 + 4 + 4 + 2 + 2);  /* length of PCM format decl area */
      ixheaacd_write16_bits_lh(fp, 1); /* is pcm? */
    }

    ixheaacd_write16_bits_lh(fp, channels);
    ixheaacd_write32_bits_lh(fp, freq);
    ixheaacd_write32_bits_lh(fp, freq * channels * bytes); /* bps */
    ixheaacd_write16_bits_lh(fp, channels * bytes);
    ixheaacd_write16_bits_lh(fp, bits);

    /* tag for WAVE_FORMAT_EXTENSIBLE */
    if (channels > 2) {
      ixheaacd_write16_bits_lh(fp, 0x16);
      ixheaacd_write16_bits_lh(fp, 0x10);           /*Samples.wReserved*/
      ixheaacd_write32_bits_lh(fp, i_channel_mask); /* dwChannelMask */

      ixheaacd_write32_bits_lh(fp, 0x0001); /* SubFormat.Data1 */
      ixheaacd_write32_bits_lh(
          fp, 0x00100000); /* SubFormat.Data2 and SubFormat.Data3 */

      ixheaacd_write16_bits_lh(fp, 0x0080);
      ixheaacd_write16_bits_lh(fp, 0xAA00);

      ixheaacd_write16_bits_lh(fp, 0x3800);
      ixheaacd_write16_bits_lh(fp, 0x719b);
                        /*
			ixheaacd_write16_bits_lh(fp, 0x6166);
			ixheaacd_write16_bits_lh(fp, 0x7463);

			  ixheaacd_write16_bits_lh(fp, 0x0004);
			  ixheaacd_write16_bits_lh(fp, 0x0000);

				ixheaacd_write16_bits_lh(fp, 0x5800);
				ixheaacd_write16_bits_lh(fp, 0x0028);
			*/		}

                        fwrite("data", 1, 4, fp);
                        ixheaacd_write32_bits_lh(fp, pcmbytes);

                        return (ferror(fp) ? -1 : 0);

  } else {
    WORD32 bytes = (bits + 7) / 8;
    fwrite("RIFF", 1, 4, fp); /* label */
    ixheaacd_write32_bits_lh(
        fp, pcmbytes + 44 - 8);   /* length in bytes without header */
    fwrite("WAVEfmt ", 2, 4, fp); /* 2 labels */
    ixheaacd_write32_bits_lh(
        fp, 2 + 2 + 4 + 4 + 2 + 2);  /* length of PCM format decl area */
    ixheaacd_write16_bits_lh(fp, 1); /* is pcm? */
    ixheaacd_write16_bits_lh(fp, channels);
    ixheaacd_write32_bits_lh(fp, freq);
    ixheaacd_write32_bits_lh(fp, freq * channels * bytes); /* bps */
    ixheaacd_write16_bits_lh(fp, channels * bytes);
    ixheaacd_write16_bits_lh(fp, bits);
    fwrite("data", 1, 4, fp);
    ixheaacd_write32_bits_lh(fp, pcmbytes);

    return (ferror(fp) ? -1 : 0);
  }
}
#endif /* WAV_HEADER */
#endif /*ARM_PROFILE_BOARD*/

#ifdef DISPLAY_MESSAGE

/*****************************************************************************/
/*                                                                           */
/*  Function name : ia_display_id_message                                    */
/*                                                                           */
/*  Description   : Display the ID message of the process                    */
/*                                                                           */
/*  Inputs        : WORD8 lib_name[] (library name)                          */
/*                  WORD8 lib_version[] (library version)                    */
/*                  WORD8 api_version[] (API version)                        */
/*                                                                           */
/*  Globals       : none                                                     */
/*                                                                           */
/*  Processing    : Display all the information about the process            */
/*                                                                           */
/*  Outputs       : none                                                     */
/*                                                                           */
/*  Returns       : none                                                     */
/*                                                                           */
/*  Issues        : none                                                     */
/*                                                                           */
/*  Revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*        29 07 2005       Tejaswi/Vishal        Created                     */
/*                                                                           */
/*****************************************************************************/

VOID ia_display_id_message(WORD8 lib_name[], WORD8 lib_version[]) {
  WORD8 str[4][IA_SCREEN_WIDTH] = {"ITTIAM SYSTEMS PVT LTD, BANGALORE\n",
                                   "http:\\\\www.ittiam.com\n", "", ""};
  WORD8 spaces[IA_SCREEN_WIDTH / 2 + 1];
  WORD32 i, spclen;

  strcpy((pCHAR8)str[2], (pCHAR8)lib_name);
  strcat((pCHAR8)str[2], (pCHAR8)lib_version);
  strcat((pCHAR8)str[2], "\n");
  strcat((pCHAR8)str[4 - 1], "\n");

  for (i = 0; i < IA_SCREEN_WIDTH / 2 + 1; i++) {
    spaces[i] = ' ';
  }

  for (i = 0; i < 4; i++) {
    spclen = IA_SCREEN_WIDTH / 2 - strlen((pCHAR8)str[i]) / 2;
    spaces[spclen] = '\0';
    printf("%s", (pCHAR8)spaces);
    spaces[spclen] = ' ';
    printf("%s", (pCHAR8)str[i]);
  }
}
#endif /* DISPLAY_MESSAGE */

/*****************************************************************************/
/*                                                                           */
/*  Function name : ixheaacd_set_config_param                       */
/*                                                                           */
/*  Description   : Set config parameters                                    */
/*                                                                           */
/*  Inputs        : pVOID p_ia_process_api_obj (process API obj)             */
/*                  WORD32 argc (Arguments count)                            */
/*                  pWORD8 argv[] (Argument strings)                         */
/*                                                                           */
/*  Globals       : none                                                     */
/*                                                                           */
/*  Processing    : Set config params inside API                             */
/*                                                                           */
/*  Outputs       : none                                                     */
/*                                                                           */
/*  Returns       : IA_ERRORCODE error_value  (Error value)                  */
/*                                                                           */
/*  Issues        : none                                                     */
/*                                                                           */
/*  Revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*        29 07 2005       Ittiam                Created                     */
/*                                                                           */
/*****************************************************************************/

IA_ERRORCODE ixheaacd_set_config_param(WORD32 argc, pWORD8 argv[],
                                       pVOID p_ia_process_api_obj) {
  LOOPIDX i;
  IA_ERRORCODE err_code = IA_NO_ERROR;
  /* the process API function */
  IA_ERRORCODE(*p_ia_process_api)
  (pVOID p_ia_process_api_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) =
      ixheaacd_dec_api;
  ia_error_info_struct *p_proc_err_info = &ixheaacd_error_info;

  for (i = 0; i < argc; i++) {
    /* PCM Word Size (For single input file) */
    if (!strncmp((pCHAR8)argv[i], "-pcmsz:", 7)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 7);
      UWORD32 ui_pcm_wd_sz = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_PCM_WDSZ, &ui_pcm_wd_sz);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
    /* Down-mix stereo to mono. */
    if (!strncmp((pCHAR8)argv[i], "-dmix:", 6)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 6);
      UWORD32 ui_down_mix = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_DOWNMIX, &ui_down_mix);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
#ifdef RESAMPLE_SUPPORT
    /* Resample the output to 8 kHz. */
    if (!strncmp((pCHAR8)argv[i], "-f08:", 5)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 5);
      UWORD32 ui_08khz_out = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_OUT08KHZ, &ui_08khz_out);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
    /* Resample the output to 16 kHz. */
    if (!strncmp((pCHAR8)argv[i], "-f16:", 5)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 5);
      UWORD32 ui_16khz_out = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_OUT16KHZ, &ui_16khz_out);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
#endif
    /* Interleave mono output to stereo */
    if (!strncmp((pCHAR8)argv[i], "-tostereo:", 10)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 10);
      UWORD32 ui_to_stereo = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_TOSTEREO, &ui_to_stereo);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
    /* Downsampled synthesis to be used */
    if (!strncmp((pCHAR8)argv[i], "-dsample:", 9)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 9);
      UWORD32 ui_dsample = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_DSAMPLE, &ui_dsample);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
    /* To indicate if its a MP4 file or not. */
    if (!strncmp((pCHAR8)argv[i], "-mp4:", 5)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 5);
      UWORD32 ui_mp4_flag = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_ISMP4, &ui_mp4_flag);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }

#ifdef HEAACV2_AS_AACLC
    /* To indicate if its a MP4 file or not. */
    if (!strncmp((pCHAR8)argv[i], "-aac_lc_only:", 13)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 13);
      UWORD32 ui_aac_lc_only = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_AAC_ONLY, &ui_aac_lc_only);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
#endif

#ifdef LATM_LOAS
    /* To indicate if its a LOAS file or not. */
    if (!strncmp((pCHAR8)argv[i], "-isLOAS:", 8)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 8);
      UWORD32 ui_loas_flag = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_ISLOAS, &ui_loas_flag);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }

#endif

#ifdef DRC_ENABLE
    if (!strncmp((pCHAR8)argv[i], "-drc:", 5)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 5);
      UWORD32 ui_drc_enable = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_DRC_ENABLE, &ui_drc_enable);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
    if (!strncmp((pCHAR8)argv[i], "-drc_cut_fac:", 13)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 13);
      UWORD32 ui_drc_enable = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_DRC_CUT, &ui_drc_enable);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
    if (!strncmp((pCHAR8)argv[i], "-drc_boost_fac:", 15)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 15);
      UWORD32 ui_drc_enable = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_DRC_BOOST, &ui_drc_enable);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
    if (!strncmp((pCHAR8)argv[i], "-drc_target_level:", 18)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 18);
      UWORD32 ui_drc_enable = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_DRC_TARGET_LEVEL, &ui_drc_enable);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
#endif
    /* To indicate if its a MP4 file or not. */
    if (!strncmp((pCHAR8)argv[i], "-nosync:", 8)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 8);
      UWORD32 ui_disable_sync = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_DISABLE_SYNC, &ui_disable_sync);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
    /* To indicate SBR upsampling. */
    if (!strncmp((pCHAR8)argv[i], "-sbrup:", 7)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 7);
      UWORD32 ui_auto_sbr_upsample = atoi(pb_arg_val);
      err_code =
          (*p_ia_process_api)(p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
                              IA_ENHAACPLUS_DEC_CONFIG_PARAM_AUTO_SBR_UPSAMPLE,
                              &ui_auto_sbr_upsample);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
    /* To indicate sample rate for a RAW bit-stream. */
    if (!strncmp((pCHAR8)argv[i], "-fs:", 4)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 4);
      UWORD32 ui_samp_freq = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_SAMP_FREQ, &ui_samp_freq);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
    /* To indicate the number of maximum channels */
    if (!strncmp((pCHAR8)argv[i], "-maxchannel:", 12)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 12);
      UWORD32 ui_max_channel = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_MAX_CHANNEL, &ui_max_channel);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }

    /* To indicate the number of coupling channels to be used for coupling */
    if (!strncmp((pCHAR8)argv[i], "-coupchannel:", 13)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 13);
      UWORD32 ui_coupling_channel = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_COUP_CHANNEL, &ui_coupling_channel);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }

    /* Down-mix N.1 to stereo */
    if (!strncmp((pCHAR8)argv[i], "-downmix:", 9)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 9);
      UWORD32 ui_downmix = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_DOWNMIX_STEREO, &ui_downmix);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }

    /* For LD files, to indicate  */
    if (!strncmp((pCHAR8)argv[i], "-fs480:", 7)) {
      pCHAR8 pb_arg_val = (pCHAR8)(argv[i] + 7);
      UWORD32 ui_fs480 = atoi(pb_arg_val);
      err_code = (*p_ia_process_api)(
          p_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_FRAMESIZE, &ui_fs480);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
  }

  return IA_NO_ERROR;
}

/*****************************************************************************/
/*                                                                           */
/*  Function name : ixheaacd_get_config_param                       */
/*                                                                           */
/*  Description   : Get config parameters                                    */
/*                                                                           */
/*  Inputs        : pVOID p_ia_process_api_obj (process API obj)             */
/*                  pWORD32 pi_samp_freq (Ptr for samp freq param)           */
/*                  pWORD32 pi_num_chan (Ptr for num chan param)             */
/*                  pWORD32 pi_pcm_wd_sz (Ptr for PCM Word size param)       */
/*                                                                           */
/*  Globals       : none                                                     */
/*                                                                           */
/*  Processing    : Get config params from API                               */
/*                                                                           */
/*  Outputs       : none                                                     */
/*                                                                           */
/*  Returns       : IA_ERRORCODE error_value  (Error value)                  */
/*                                                                           */
/*  Issues        : none                                                     */
/*                                                                           */
/*  Revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*        29 07 2005       Ittiam                Created                     */
/*                                                                           */
/*****************************************************************************/

IA_ERRORCODE ixheaacd_get_config_param(pVOID p_ia_process_api_obj,
                                       pWORD32 pi_samp_freq,
                                       pWORD32 pi_num_chan,
                                       pWORD32 pi_pcm_wd_sz,
                                       pWORD32 pi_channel_mask) {
  IA_ERRORCODE err_code = IA_NO_ERROR;
  /* the process API function */
  IA_ERRORCODE(*p_ia_process_api)
  (pVOID p_ia_process_api_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value) =
      ixheaacd_dec_api;
  ia_error_info_struct *p_proc_err_info = &ixheaacd_error_info;

  /* Sampling frequency */
  {
    err_code = (*p_ia_process_api)(
        p_ia_process_api_obj, IA_API_CMD_GET_CONFIG_PARAM,
        IA_ENHAACPLUS_DEC_CONFIG_PARAM_SAMP_FREQ, pi_samp_freq);
    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
  }
  /* Total Number of Channels */
  {
    err_code = (*p_ia_process_api)(
        p_ia_process_api_obj, IA_API_CMD_GET_CONFIG_PARAM,
        IA_ENHAACPLUS_DEC_CONFIG_PARAM_NUM_CHANNELS, pi_num_chan);
    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
  }
  /* PCM word size */
  {
    err_code = (*p_ia_process_api)(
        p_ia_process_api_obj, IA_API_CMD_GET_CONFIG_PARAM,
        IA_ENHAACPLUS_DEC_CONFIG_PARAM_PCM_WDSZ, pi_pcm_wd_sz);
    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
  }
  /* channel mask to tell the arrangement of channels in bit stream */
  {
    err_code = (*p_ia_process_api)(
        p_ia_process_api_obj, IA_API_CMD_GET_CONFIG_PARAM,
        IA_ENHAACPLUS_DEC_CONFIG_PARAM_CHANNEL_MASK, pi_channel_mask);
    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
  }

  /* Channel mode to tell MONO/STEREO/DUAL-MONO/NONE_OF_THESE */
  {
    UWORD32 ui_channel_mode;
    err_code = (*p_ia_process_api)(
        p_ia_process_api_obj, IA_API_CMD_GET_CONFIG_PARAM,
        IA_ENHAACPLUS_DEC_CONFIG_PARAM_CHANNEL_MODE, &ui_channel_mode);
    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    if (ui_channel_mode == 0)
      printf("Channel Mode: MONO_OR_PS\n");
    else if (ui_channel_mode == 1)
      printf("Channel Mode: STEREO\n");
    else if (ui_channel_mode == 2)
      printf("Channel Mode: DUAL-MONO\n");
    else
      printf("Channel Mode: NONE_OF_THESE or MULTICHANNEL\n");
  }

  /* Channel mode to tell SBR PRESENT/NOT_PRESENT */
  {
    UWORD32 ui_sbr_mode;
    err_code = (*p_ia_process_api)(
        p_ia_process_api_obj, IA_API_CMD_GET_CONFIG_PARAM,
        IA_ENHAACPLUS_DEC_CONFIG_PARAM_SBR_MODE, &ui_sbr_mode);
    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    if (ui_sbr_mode == 0)
      printf("SBR Mode: NOT_PRESENT\n");
    else if (ui_sbr_mode == 1)
      printf("SBR Mode: PRESENT\n");
    else
      printf("SBR Mode: ILLEGAL\n");
  }
  return IA_NO_ERROR;
}

/*****************************************************************************/
/*                                                                           */
/*  Function name : ixheaacd_main_process                           */
/*                                                                           */
/*  Description   : Stacked processing with function pointer selection       */
/*                                                                           */
/*  Inputs        : WORD32 argc (Arguments count)                            */
/*                  pWORD8 argv[] (Argument strings)                         */
/*                                                                           */
/*  Globals       : pVOID g_pv_arr_alloc_memory[MAX_MEM_ALLOCS];             */
/*                  WORD  g_w_malloc_count;                                  */
/*                  FILE *g_pf_inp, *g_pf_out;                               */
/*                                                                           */
/*  Processing    : Stacked processing of multiple components                */
/*                  Loop1: Set params + Mem alloc                            */
/*                  Loop2: Set params + Init process + Get params            */
/*                  Loop3: Execute                                           */
/*                                                                           */
/*  Outputs       : None                                                     */
/*                                                                           */
/*  Returns       : IA_ERRORCODE error_value  (Error value)                  */
/*                                                                           */
/*  Issues        : none                                                     */
/*                                                                           */
/*  Revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*        29 07 2005       Tejaswi/Vishal        Created                     */
/*                                                                           */
/*****************************************************************************/

#ifdef REINIT_FOR_ERROR
int ixheaacd_main_process(WORD32 argc, pWORD8 argv[],
                          pWORD8 pb_output_file_name)
#else
int ixheaacd_main_process(WORD32 argc, pWORD8 argv[])
#endif
{
  LOOPIDX i;
  WORD frame_counter = 0;
#ifdef DISPLAY_MESSAGE
  /* Library Info and Identification strings */
  WORD8 pb_process_name[IA_SCREEN_WIDTH] = "";
  WORD8 pb_lib_version[IA_SCREEN_WIDTH] = "";
#endif

  /* Error code */
  IA_ERRORCODE err_code = IA_NO_ERROR;
  IA_ERRORCODE err_code_reinit = IA_NO_ERROR;

  /* API obj */
  pVOID pv_ia_process_api_obj;
  /* First part                                        */
  /* Error Handler Init                                */
  /* Get Library Name, Library Version and API Version */
  /* Initialize API structure + Default config set     */
  /* Set config params from user                       */
  /* Initialize memory tables                          */
  /* Get memory information and allocate memory        */

  /* Memory variables */
  UWORD32 n_mems, ui_rem;
  UWORD32 ui_proc_mem_tabs_size;
  /* API size */
  UWORD32 pui_ap_isize;
  /* Process initing done query variable */
  UWORD32 ui_init_done, ui_exec_done;
  pWORD8 pb_inp_buf = 0, pb_out_buf = 0;
  pWORD8 pb_inp_buf1 = 0;
  // pWORD16 litt2big;

  UWORD32 ui_inp_size = 0;
  WORD32 i_bytes_consumed, i_bytes_read;
  WORD32 i_buff_size;
  WORD32 prev_sampling_rate = 0;
  WORD32 skip_samples = 0;
  WORD32 total_samples = 0;
  WORD32 write_flag = 1;
  WORD32 bytes_to_write = 0;
  WORD32 ixheaacd_drc_offset = 0;
  WORD32 current_samples = 0;
  WORD32 samples_written = 0;
  WORD32 init_iteration = 1;
#ifdef REINIT_FOR_ERROR
  WORD32 i_persist_size;
  WORD32 i_process_err = 0, i_op_file_cnt = 0;
  WORD32 i_error_in_init = 0;
  WORD32 i_count_init_errors = 0;

  pVOID pv_persist_ptr;
#endif

#ifdef ARM_PROFILE_HW
  int frame_count_b = 0;
  long long cycles_b = 0;
  long long start1_b, stop1_b;
  double Curr_b, Ave_b = 0, Sum_b = 0;
  double Peak_b = 0;
  WORD32 Peak_frame_b = 0;
#endif
#ifdef TEST_INSUFFICIENT_INPUT
  WORD32 buff_pos, read_bytes = 330, input_buff_size;
#endif
  WORD32 i_out_bytes, i_total_bytes = 0;
  WORD32 i_samp_freq, i_num_chan, i_pcm_wd_sz, i_channel_mask;

  /* The process API function */
  IA_ERRORCODE(*p_ia_process_api)
  (pVOID p_ia_process_api_obj, WORD32 i_cmd, WORD32 i_idx, pVOID pv_value);

  /* The set config from argc argv */
  IA_ERRORCODE(*p_set_config_param)
  (WORD32 argc, pWORD8 argv[], pVOID p_ia_process_api_obj);

  /* The get config from API */
  IA_ERRORCODE(*p_get_config_param)
  (pVOID p_ia_process_api_obj, pWORD32 pi_samp_freq, pWORD32 pi_num_chan,
   pWORD32 pi_pcm_wd_sz, pWORD32 pi_channel_mask);

#ifdef TEST_FRAMEWISE_INPUT
  // Open the file with config header data
  FILE *fp_hdr = fopen("0_3gp_4.hdr", "rb");
#endif
  /* The error init function */
  VOID (*p_error_init)();

  /* The process error info structure */
  ia_error_info_struct *p_proc_err_info;

#ifdef ARM_PROFILE

  for (profile_index = 0; profile_index < MAX_MODULE; profile_index++) {
    profile_instance[profile_index].peak = 0;
    profile_instance[profile_index].average = 0;
    profile_instance[profile_index].cycles = 0;
    profile_instance[profile_index].sum = 0;
    profile_instance[profile_index].peak_frame = 0;
    // profile_instance[profile_index].info=profile_info[profile_index];
  }

#endif

  /* Process struct initing */
  p_ia_process_api = ixheaacd_dec_api;
  p_set_config_param = ixheaacd_set_config_param;
  p_get_config_param = ixheaacd_get_config_param;
  p_error_init = ixheaacd_error_handler_init;
  p_proc_err_info = &ixheaacd_error_info;
  /* Process struct initing end */

  /* ******************************************************************/
  /* Initialize the error handler                                     */
  /* ******************************************************************/
  (*p_error_init)();

/* ******************************************************************/
/* Get the library name, library version and API version            */
/* ******************************************************************/

#ifdef DISPLAY_MESSAGE
  /* Get the library name string */
  err_code = (*p_ia_process_api)(NULL, IA_API_CMD_GET_LIB_ID_STRINGS,
                                 IA_CMD_TYPE_LIB_NAME, pb_process_name);

  _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

  /* Get the library version string */
  err_code = (*p_ia_process_api)(NULL, IA_API_CMD_GET_LIB_ID_STRINGS,
                                 IA_CMD_TYPE_LIB_VERSION, pb_lib_version);

  _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

  /* Display the ittiam identification message */
  ia_display_id_message(pb_process_name, pb_lib_version);
#endif

  /* ******************************************************************/
  /* Initialize API structure and set config params to default        */
  /* ******************************************************************/

  /* Get the API size */
  err_code =
      (*p_ia_process_api)(NULL, IA_API_CMD_GET_API_SIZE, 0, &pui_ap_isize);
#ifdef MEM_PROFILE
  {
    float temp = 0;
    temp = (float)((float)pui_ap_isize / 1024);
    printf("Get the API size %d = %f \n", pui_ap_isize, temp);  // added by siva
  }
#endif
  _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

  /* Allocate memory for API */
  g_pv_arr_alloc_memory[g_w_malloc_count] = malloc(pui_ap_isize + 4);

  if (g_pv_arr_alloc_memory[g_w_malloc_count] == NULL) {
    err_code = IA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED;
    _IA_HANDLE_ERROR(&ixheaacd_ia_testbench_error_info,
                     (pWORD8) "API struct alloc", err_code);
  }

  /* API object requires 4 bytes (WORD32) alignment */
  ui_rem = ((WORD32)g_pv_arr_alloc_memory[g_w_malloc_count] & 3);
  /* Set API object with the memory allocated */
  pv_ia_process_api_obj =
      (pVOID)((WORD8 *)g_pv_arr_alloc_memory[g_w_malloc_count] + 4 - ui_rem);

  g_w_malloc_count++;

  /* Set the config params to default values */
  err_code = (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_INIT,
                                 IA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS, NULL);

  _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

  /* ******************************************************************/
  /* Set config parameters got from the user present in argc argv     */
  /* ******************************************************************/

  err_code = (*p_set_config_param)(argc, argv, pv_ia_process_api_obj);
  _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

/* ******************************************************************/
/* Table Relocatibility                                             */
/* ******************************************************************/
#if IA_HE_AAC_DEC_TABLE_RELOCATABLE_ENABLE

  /* Get number of tables required */
  err_code = (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_GET_N_TABLES,
                                 0, &n_mems);
#ifdef MEM_PROFILE
  printf("Get number of tables required: %d \n", n_mems);  // added by siva
#endif
  _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

  for (i = 0; i < (WORD32)n_mems; i++) {
    int ui_size, ui_alignment;
    pVOID pv_alloc_ptr = NULL, pv_curr_ptr = NULL;
    LOOPIDX k;

    /* Get table size */
    err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                   IA_API_CMD_GET_TABLE_INFO_SIZE, i, &ui_size);
#ifdef MEM_PROFILE
    {
      float temp = 0;
      temp = (float)((float)ui_size / 1024);
      printf("get table size: %d =%f\n", ui_size, temp);  // added by siva
    }
#endif
    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

    /* Get table alignment */
    err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                   IA_API_CMD_GET_TABLE_INFO_ALIGNMENT, i,
                                   &ui_alignment);

    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

    g_pv_arr_alloc_memory[g_w_malloc_count] = malloc(ui_size + ui_alignment);

    if (g_pv_arr_alloc_memory[g_w_malloc_count] == NULL) {
      _IA_HANDLE_ERROR(&ixheaacd_ia_testbench_error_info,
                       (pWORD8) "Mem for table relocation alloc",
                       IA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED);
    }

    ui_rem = ((WORD32)g_pv_arr_alloc_memory[g_w_malloc_count] % ui_alignment);
    pv_alloc_ptr = (pVOID)((WORD8 *)g_pv_arr_alloc_memory[g_w_malloc_count] +
                           ui_alignment - ui_rem);

    g_w_malloc_count++;

    /* Get the current table pointer */
    err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                   IA_API_CMD_GET_TABLE_PTR, i, &pv_curr_ptr);

    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

    for (k = 0; k < ui_size; k++) {
      ((pWORD8)pv_alloc_ptr)[k] = ((pWORD8)pv_curr_ptr)[k];
      /* Disabled for multiple files running
((pWORD8)pv_curr_ptr)[k] = (WORD8)0xab; */
    }

    /* Set the relocated table pointer */
    err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                   IA_API_CMD_SET_TABLE_PTR, i, pv_alloc_ptr);

    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
  }
#endif
  /* ******************************************************************/
  /* Initialize Memory info tables                                    */
  /* ******************************************************************/

  /* Get memory info tables size */
  err_code =
      (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_GET_MEMTABS_SIZE, 0,
                          &ui_proc_mem_tabs_size);
#ifdef MEM_PROFILE
  printf("memory info table size: %d \n",
         ui_proc_mem_tabs_size);  // added by siva
#endif
  _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

  g_pv_arr_alloc_memory[g_w_malloc_count] = malloc(ui_proc_mem_tabs_size + 4);

  if (g_pv_arr_alloc_memory[g_w_malloc_count] == NULL) {
    err_code = IA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED;
    _IA_HANDLE_ERROR(&ixheaacd_ia_testbench_error_info,
                     (pWORD8) "Mem tables alloc", err_code);
  }

  /* API object requires 4 bytes (WORD32) alignment */
  ui_rem = ((WORD32)g_pv_arr_alloc_memory[g_w_malloc_count] & 3);

  /* Set pointer for process memory tables	*/
  err_code = (*p_ia_process_api)(
      pv_ia_process_api_obj, IA_API_CMD_SET_MEMTABS_PTR, 0,
      (pVOID)((WORD8 *)g_pv_arr_alloc_memory[g_w_malloc_count] + 4 - ui_rem));

  _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

  g_w_malloc_count++;

  /* initialize the API, post config, fill memory tables	*/
  err_code = (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_INIT,
                                 IA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS, NULL);

  _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

  /* ******************************************************************/
  /* Allocate Memory with info from library                           */
  /* ******************************************************************/

  /* Get number of memory tables required */
  err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                 IA_API_CMD_GET_N_MEMTABS, 0, &n_mems);
#ifdef MEM_PROFILE
  printf("Get number of memory tables required %d \n", n_mems);  // added by
                                                                 // siva
#endif

  _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

  for (i = 0; i < (WORD32)n_mems; i++) {
    int ui_size, ui_alignment, ui_type;
    pVOID pv_alloc_ptr;

    /* Get memory size */
    err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                   IA_API_CMD_GET_MEM_INFO_SIZE, i, &ui_size);
#ifdef MEM_PROFILE
    {
      float temp = 0;
      printf("memory size: %d =%f \n", ui_size,
             temp = (float)((float)ui_size / 1024));  // added by siva
    }
#endif
    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

    /* Get memory alignment */
    err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                   IA_API_CMD_GET_MEM_INFO_ALIGNMENT, i,
                                   &ui_alignment);

    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

    /* Get memory type */
    err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                   IA_API_CMD_GET_MEM_INFO_TYPE, i, &ui_type);

    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

    err_code =
        (*p_get_config_param)(pv_ia_process_api_obj, &i_samp_freq, &i_num_chan,
                              &i_pcm_wd_sz, &i_channel_mask);
    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

    //  ui_size += 20*1024*sizeof(WORD8);

    if (ui_type == IA_MEMTYPE_OUTPUT) {
      //		ui_size = 8192;
      if (i_pcm_wd_sz == 16)
        ui_size = 16384 * sizeof(WORD16);  // refer SAMPLE_BUF_SIZE value in
                                           // audio.c file //Ramesh
      else
        ui_size =
            16384 * 3 *
            sizeof(
                WORD8);  // refer SAMPLE_BUF_SIZE value in audio.c file //Ramesh
    }

    g_pv_arr_alloc_memory[g_w_malloc_count] = malloc(ui_size + ui_alignment);

    if (g_pv_arr_alloc_memory[g_w_malloc_count] == NULL) {
      err_code = IA_TESTBENCH_MFMAN_FATAL_MEM_ALLOC_FAILED;
      _IA_HANDLE_ERROR(&ixheaacd_ia_testbench_error_info,
                       (pWORD8) "Mem tables alloc", err_code);
    }

    ui_rem = ((WORD32)g_pv_arr_alloc_memory[g_w_malloc_count] % ui_alignment);
    pv_alloc_ptr = (pVOID)((WORD8 *)g_pv_arr_alloc_memory[g_w_malloc_count] +
                           ui_alignment - ui_rem);

    g_w_malloc_count++;

    /* Set the buffer pointer */
    err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                   IA_API_CMD_SET_MEM_PTR, i, pv_alloc_ptr);

    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    if (ui_type == IA_MEMTYPE_INPUT) {
      pb_inp_buf = pv_alloc_ptr;
      ui_inp_size = ui_size;
#ifdef TEST_INSUFFICIENT_INPUT
      input_buff_size = ui_size;
#endif
    }
#ifdef REINIT_FOR_ERROR
    if (ui_type == IA_MEMTYPE_PERSIST) {
      i_persist_size = ui_size;
      pv_persist_ptr = pv_alloc_ptr;
    }
#endif
    if (ui_type == IA_MEMTYPE_OUTPUT) {
      pb_out_buf = pv_alloc_ptr;
    }
  }

/* End first part */

/* Second part        */
/* Initialize process */
/* Get config params  */

/* ******************************************************************/
/* Initialize process in a loop (to handle junk data at beginning)  */
/* ******************************************************************/

#ifndef TEST_FRAMEWISE_INPUT
  i_bytes_consumed = ui_inp_size;
  i_buff_size = ui_inp_size;

#else

#ifdef REINIT_FOR_ERROR
INIT_AGAIN:
#endif
  /* Clear input buffer */
  memset(pb_inp_buf, 0, ui_inp_size);
  /* Read the config header */
  i_buff_size = fread((unsigned char *)pb_inp_buf, 1, 5, fp_hdr);
  /* Reset the read pointer of header file - useful in case of errors */
  fseek(fp_hdr, 0, SEEK_SET);

  /* Read this frame data*/
  i_buff_size += fread((unsigned char *)pb_inp_buf + i_buff_size, 1,
                       Audio_FrameSize[frame_counter], g_pf_inp->inputFile);

  i_bytes_consumed = 0;
#endif

#ifdef TEST_INSUFFICIENT_INPUT
  buff_pos = 0;
#endif

  do {
#ifndef TEST_FRAMEWISE_INPUT

#ifdef REINIT_FOR_ERROR
  INIT_AGAIN:
#endif
    i_bytes_read = 0;

#ifndef TEST_INSUFFICIENT_INPUT
    if ((ui_inp_size - (i_buff_size - i_bytes_consumed)) > 0) {
      for (i = 0; i < (i_buff_size - i_bytes_consumed); i++) {
        pb_inp_buf[i] = pb_inp_buf[i + i_bytes_consumed];
      }

      FileWrapper_Read(g_pf_inp, (unsigned char *)(pb_inp_buf + i_buff_size -
                                                   i_bytes_consumed),
                       (ui_inp_size - (i_buff_size - i_bytes_consumed)),
                       (pUWORD32)&i_bytes_read);

      i_buff_size = i_buff_size - (i_bytes_consumed - i_bytes_read);

      /* Tell input is over, if algorithm returns with insufficient input and
         there is no
             more input left in the bitstream*/
      if ((i_buff_size <= 0) ||
          ((err_code_reinit == 0x00001804) && i_bytes_read == 0))

      {
        i_buff_size = 0;
        /* Tell that the input is over in this buffer */
        err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                       IA_API_CMD_INPUT_OVER, 0, NULL);

        _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
      }
    }
#else
    if ((ui_inp_size - (i_buff_size - i_bytes_consumed)) > 0) {
      WORD32 read_bytes_act = read_bytes;

      for (i = 0; i < (i_buff_size - i_bytes_consumed); i++) {
        pb_inp_buf[i] = pb_inp_buf[i + i_bytes_consumed];
      }

      if (buff_pos + read_bytes > input_buff_size)
        read_bytes_act = input_buff_size - buff_pos;

      FileWrapper_Read(g_pf_inp, (unsigned char *)(pb_inp_buf + buff_pos),
                       read_bytes_act, (pUWORD32)&i_bytes_read);

      buff_pos += i_bytes_read;

      i_buff_size = i_buff_size - (i_bytes_consumed - i_bytes_read);

      if (i_buff_size <= 0) {
        i_buff_size = 0;
        /* Tell that the input is over in this buffer */
        err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                       IA_API_CMD_INPUT_OVER, 0, NULL);

        _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
      }
    }
#endif

#else

    if (i_bytes_consumed != 0) {
      for (i = 0; i < (i_buff_size - i_bytes_consumed); i++) {
        pb_inp_buf[i] = pb_inp_buf[i + i_bytes_consumed];
      }
      i_buff_size -= i_bytes_consumed;
    }
#endif
    // if( i_buff_size <= 0)
    if ((i_buff_size <= 0) ||
        ((err_code_reinit == 0x00001804) && i_bytes_read == 0)) {
      i_buff_size = 0;
      /* Tell that the input is over in this buffer */
      err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                     IA_API_CMD_INPUT_OVER, 0, NULL);

      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
#ifdef WAV_HEADER
#ifndef ARM_PROFILE_BOARD
      /* ******************************************************************/
      /* Get config params from API                                       */
      /* ******************************************************************/

      err_code =
          (*p_get_config_param)(pv_ia_process_api_obj, &i_samp_freq,
                                &i_num_chan, &i_pcm_wd_sz, &i_channel_mask);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

      // This is done in those cases, where file decodes ends at init time
      // Since init is incomplete, sampling freq might be zero and will result
      // in
      // writing invalid wave header

      if (i_samp_freq == 0) i_samp_freq = prev_sampling_rate;

      if (!fseek(g_pf_out, 0, SEEK_SET))
        write_wav_header(g_pf_out, i_total_bytes, i_samp_freq, i_num_chan,
                         i_pcm_wd_sz, i_channel_mask);
#endif
#endif
      return 1;
    }

    if (init_iteration == 1) {
      if (raw_testing)
        ixheaacd_i_bytes_to_read = get_metadata_dec_info_init(meta_info);
      else
        ixheaacd_i_bytes_to_read = i_buff_size;

      /* Set number of bytes to be processed */
      err_code =
          (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_SET_INPUT_BYTES,
                              0, &ixheaacd_i_bytes_to_read);
      init_iteration++;

    } else {
      /* Set number of bytes to be processed */
      err_code = (*p_ia_process_api)(
          pv_ia_process_api_obj, IA_API_CMD_SET_INPUT_BYTES, 0, &i_buff_size);
    }

    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

    /* Initialize the process */
    err_code = (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_INIT,
                                   IA_CMD_TYPE_INIT_PROCESS, NULL);
    err_code_reinit = err_code;

#ifdef REINIT_FOR_ERROR

    if (err_code != 0) {
      i_process_err = 1;
      i_error_in_init = 1;
      /* Adding some error code so that the re-init will be
      printed as non-fatal error by testbench */
      err_code = 0x00000000;
      i_count_init_errors++;
    }
    //	_IA_HANDLE_ERROR(p_proc_err_info, (pWORD8)"", err_code);
    if (err_code_reinit ==
        0x00001804 /*IA_ENHAACPLUS_DEC_EXE_FATAL_INSUFFICIENT_INPUT_BYTES*/) {
      _IA_PRINT_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    } else if (err_code_reinit) {
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
#else
    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
#endif

    /* Checking for end of initialization */
    err_code = (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_INIT,
                                   IA_CMD_TYPE_INIT_DONE_QUERY, &ui_init_done);

    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

    /* How much buffer is used in input buffers */
    err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                   IA_API_CMD_GET_CURIDX_INPUT_BUF, 0,
                                   &i_bytes_consumed);

    // printf("%d \n",i_bytes_consumed);

    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
#ifdef REINIT_FOR_ERROR

    if (i_process_err == 1) {
      /* To avoid going into loop in case of initialization errors
         beyond a certain limit */
      if (i_count_init_errors > 6000) {
        ixheaacd_error_handler(p_proc_err_info, (pWORD8) "", err_code_reinit);

#ifdef WAV_HEADER  // removed
#ifndef ARM_PROFILE_BOARD
        if (!fseek(g_pf_out, 0, SEEK_SET))
          write_wav_header(g_pf_out, i_total_bytes, i_samp_freq, i_num_chan,
                           i_pcm_wd_sz, i_channel_mask);
#endif
#endif
        /* Try decoding next file */
        return 1;
      }

//	  	  frame_counter++;
#ifndef ARM_PROFILE_BOARD
//	  fprintf(stderr,"\r[%5d]",frame_counter);
#endif
      goto HANDLE_ERROR_AT_INIT;
    }
#endif

#ifdef TEST_INSUFFICIENT_INPUT
    // shift out consumed data
    buff_pos -= i_bytes_consumed;
#endif

  } while (!ui_init_done);

  /* ******************************************************************/
  /* Get config params from API                                       */
  /* ******************************************************************/

  err_code = (*p_get_config_param)(pv_ia_process_api_obj, &i_samp_freq,
                                   &i_num_chan, &i_pcm_wd_sz, &i_channel_mask);
  _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

  if (raw_testing) {
    skip_samples = get_start_offset_in_samples(meta_info);
    total_samples = get_play_time_in_samples(meta_info);
  }

/* End second part */

#ifdef WAV_HEADER
// This condition is added so as to avoid re-writing wave header in
// middle of wave file in case of errors and when we are not opening
// new file in case of errors.

#ifndef WRITE_NEW_FILE
  if (frame_counter == 0)
#endif
#ifndef ARM_PROFILE_BOARD

    write_wav_header(g_pf_out, 0, i_samp_freq, i_num_chan, i_pcm_wd_sz,
                     i_channel_mask);
#endif
#endif
  prev_sampling_rate = i_samp_freq;

#ifdef TEST_INSUFFICIENT_INPUT
  buff_pos = i_buff_size;
#endif
  do {
#ifndef TEST_FRAMEWISE_INPUT
#ifndef TEST_INSUFFICIENT_INPUT
    if ((ui_inp_size - (i_buff_size - i_bytes_consumed)) > 0) {
      for (i = 0; i < (i_buff_size - i_bytes_consumed); i++) {
        pb_inp_buf[i] = pb_inp_buf[i + i_bytes_consumed];
      }
#ifdef ENABLE_LD_DEC
      if (0 != frame_counter) {
#endif
        FileWrapper_Read(g_pf_inp, (unsigned char *)(pb_inp_buf + i_buff_size -
                                                     i_bytes_consumed),
                         (ui_inp_size - (i_buff_size - i_bytes_consumed)),
                         (pUWORD32)&i_bytes_read);
#ifdef ENABLE_LD_DEC
      } else
        i_bytes_read = 0;
#endif

      if (i_bytes_read == 0) {
        i_bytes_read = i_bytes_read;
      }

      i_buff_size = i_buff_size - (i_bytes_consumed - i_bytes_read);

      if ((i_buff_size <= 0) ||
          ((err_code_reinit == 0x00001804) && i_bytes_read == 0)) {
        i_buff_size = 0;
        raw_testing = 0;
        /* Tell that the input is over in this buffer */
        err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                       IA_API_CMD_INPUT_OVER, 0, NULL);

        _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

        // if(i_buff_size == 0)
        //  break;
      }
    }
#else
    if ((ui_inp_size - (i_buff_size - i_bytes_consumed)) > 0) {
      WORD32 read_bytes_act = read_bytes;

      if (buff_pos + read_bytes > input_buff_size)
        read_bytes_act = input_buff_size - buff_pos;

      for (i = 0; i < (i_buff_size - i_bytes_consumed); i++) {
        pb_inp_buf[i] = pb_inp_buf[i + i_bytes_consumed];
      }

      FileWrapper_Read(g_pf_inp, (unsigned char *)(pb_inp_buf + buff_pos),
                       read_bytes_act, (pUWORD32)&i_bytes_read);

      buff_pos += i_bytes_read;

      i_buff_size = i_buff_size - (i_bytes_consumed - i_bytes_read);

      if (i_buff_size <= 0) {
        i_buff_size = 0;
        raw_testing = 0;
        /* Tell that the input is over in this buffer */
        err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                       IA_API_CMD_INPUT_OVER, 0, NULL);

        _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
      }
    }
#endif
#else  // removed
    if (i_bytes_consumed != 0) {
      /* Clear input buffer*/
      memset(pb_inp_buf, 0, ui_inp_size);

      /* Set frame data to input buffer */
      i_buff_size = fread((unsigned char *)pb_inp_buf, 1,
                          Audio_FrameSize[frame_counter], g_pf_inp->inputFile);

      if ((i_buff_size <= 0) ||
          ((err_code_reinit == 0x00001804) && i_bytes_read == 0)) {
#ifdef WAV_HEADER
#ifndef ARM_PROFILE_BOARD
        if (!fseek(g_pf_out, 0, SEEK_SET))
          write_wav_header(g_pf_out, i_total_bytes, i_samp_freq, i_num_chan,
                           i_pcm_wd_sz, i_channel_mask);
#endif
#endif
        return 0;
      }
    }
#endif

    if (raw_testing) {
      ixheaacd_i_bytes_to_read =
          get_metadata_dec_exec(meta_info, frame_counter);
      err_code =
          (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_SET_INPUT_BYTES,
                              0, &ixheaacd_i_bytes_to_read);
    } else {
      /* Set number of bytes to be processed */
      err_code = (*p_ia_process_api)(
          pv_ia_process_api_obj, IA_API_CMD_SET_INPUT_BYTES, 0, &i_buff_size);
    }

    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

#ifdef ERROR_PATTERN_READ
    {
      /* Reading error pattern from file and set config param */
      UWORD32 frame_status = ReadErrorPatternFile(g_pf_err);

      err_code = (*p_ia_process_api)(
          pv_ia_process_api_obj, IA_API_CMD_SET_CONFIG_PARAM,
          IA_ENHAACPLUS_DEC_CONFIG_PARAM_FRAMEOK, &frame_status);
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
#endif

#ifdef CHECK_STACK_USAGE
    stack_corrupt_func stack_corrupt -= 2048;
#endif

#ifdef ARM_PROFILE_HW
    // start1 = times(&start);
    start1_b = itGetMs();
// printf("start1_b = %lld\t",start1_b);
#endif

#ifdef ARM_PROFILE

    IttiamStartTimer1                  // Initialize Timer
        uiStartTime = IttiamGetTimer1  // Read Start Time
#endif                                 // ARM_PROFILE
                                       /* Execute process */

        if (frame_counter % 10 == 0) {
      do {
        err_code = (*p_ia_process_api)(
            pv_ia_process_api_obj, IA_API_CMD_INIT, IA_CMD_TYPE_FLUSH_MEM,
            NULL);  // api implemented to handle flush call

        err_code =
            (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_INIT,
                                IA_CMD_TYPE_INIT_DONE_QUERY, &ui_init_done);

        if (i_buff_size != 0) {
          err_code =
              (*p_ia_process_api)(pv_ia_process_api_obj,
                                  IA_API_CMD_SET_INPUT_BYTES, 0, &i_buff_size);
        } else {
          err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                         IA_API_CMD_SET_INPUT_BYTES, 0,
                                         &ixheaacd_i_bytes_to_read);
        }

      } while (!ui_init_done);

      err_code =
          (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_SET_INPUT_BYTES,
                              0, &ixheaacd_i_bytes_to_read);
    }

    if (ixheaacd_i_bytes_to_read == 2)  // Check to indicate GA header
    {
      do {
        err_code = (*p_ia_process_api)(
            pv_ia_process_api_obj, IA_API_CMD_INIT, IA_CMD_TYPE_GA_HDR,
            NULL);  // api implemented to handle multiple ga_hdr decode

        err_code =
            (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_INIT,
                                IA_CMD_TYPE_INIT_DONE_QUERY, &ui_init_done);

        err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                       IA_API_CMD_GET_CURIDX_INPUT_BUF, 0,
                                       &i_bytes_consumed);

        if ((ui_inp_size - (i_buff_size - i_bytes_consumed)) > 0) {
          for (i = 0; i < (i_buff_size - i_bytes_consumed); i++) {
            pb_inp_buf[i] = pb_inp_buf[i + i_bytes_consumed];
          }

          FileWrapper_Read(
              g_pf_inp,
              (unsigned char *)(pb_inp_buf + i_buff_size - i_bytes_consumed),
              (ui_inp_size - (i_buff_size - i_bytes_consumed)),
              (pUWORD32)&i_bytes_read);
          i_buff_size = i_buff_size - (i_bytes_consumed - i_bytes_read);
        }

        err_code = (*p_ia_process_api)(
            pv_ia_process_api_obj, IA_API_CMD_SET_INPUT_BYTES, 0, &i_buff_size);

      } while (!ui_init_done);

      // frame_counter++;
    }

    else {
      err_code = (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_EXECUTE,
                                     IA_CMD_TYPE_DO_EXECUTE, NULL);
    }

    err_code_reinit = err_code;

#ifdef ARM_PROFILE

    uiEndTime = IttiamGetTimer1   // Read End Time
                IttiamStopTimer1  // Stop Timer */

                // Compute cycles taken (timer decrement type)
                profile_instance[EAAC_PLUS_DECODER]
                    .cycles = uiStartTime - uiEndTime;

#endif  // ARM_PROFILE

#ifdef ARM_PROFILE_HW
    stop1_b = itGetMs();
    cycles_b = (stop1_b - start1_b);
// printf("stop1_b = %lld\n",stop1_b);
#endif

#ifdef CHECK_STACK_USAGE
    for (i = 5; i < 2048; i++) {
      if ((stack_corrupt[i] != 0xcfadbe3d)) {
        stack_used = (2048 - i) * 4;
        break;
      }
    }
    printf("Stack used bytes = %d\n", stack_used);
#endif

#ifdef REINIT_FOR_ERROR
    if (err_code != 0) {
      i_process_err = 1;
      /* Adding some error code so that the re-init will be
      printed as non-fatal error by testbench */

      err_code = 0x00000000;
    }
    if (err_code_reinit ==
        0x00001804 /*IA_ENHAACPLUS_DEC_EXE_FATAL_INSUFFICIENT_INPUT_BYTES*/) {
      _IA_PRINT_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    } else if (err_code) {
      _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
    }
#else
    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);
#endif

    /* Checking for end of processing */
    err_code = (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_EXECUTE,
                                   IA_CMD_TYPE_DONE_QUERY, &ui_exec_done);

    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

    /* How much buffer is used in input buffers */
    err_code = (*p_ia_process_api)(pv_ia_process_api_obj,
                                   IA_API_CMD_GET_CURIDX_INPUT_BUF, 0,
                                   &i_bytes_consumed);
    // printf("%d \n",i_bytes_consumed);

    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

//	fprintf(stderr,"i_bytes_consumed :: [%5d]\n",i_bytes_consumed);

#ifdef TEST_INSUFFICIENT_INPUT
    // shift out consumed data
    buff_pos -= i_bytes_consumed;
#endif

    /* Get the output bytes */
    err_code = (*p_ia_process_api)(
        pv_ia_process_api_obj, IA_API_CMD_GET_OUTPUT_BYTES, 0, &i_out_bytes);

    _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

    i_total_bytes += i_out_bytes;

    if (total_samples != 0)  // Usac stream
    {
      if (raw_testing) {
        if (i_total_bytes <= skip_samples * i_num_chan * (i_pcm_wd_sz >> 3)) {
          err_code =
              (*p_get_config_param)(pv_ia_process_api_obj, &i_samp_freq,
                                    &i_num_chan, &i_pcm_wd_sz, &i_channel_mask);

          write_flag = 0;
        } else {
          write_flag = 1;
          bytes_to_write =
              i_total_bytes - skip_samples * i_num_chan * (i_pcm_wd_sz >> 3);
          if (bytes_to_write < i_out_bytes) {
            ixheaacd_drc_offset = i_out_bytes - bytes_to_write;
            //	i_total_bytes = i_total_bytes - i_out_bytes + bytes_to_write;
            i_out_bytes = bytes_to_write;
            current_samples =
                bytes_to_write / (i_num_chan * (i_pcm_wd_sz >> 3));
          } else {
            ixheaacd_drc_offset = 0;
            current_samples = i_out_bytes / (i_num_chan * (i_pcm_wd_sz >> 3));
          }
        }
      }

      if (raw_testing) {
        samples_written += current_samples;

        if (samples_written > total_samples) {
          i_out_bytes = (total_samples - (samples_written - current_samples)) *
                        (i_num_chan * (i_pcm_wd_sz >> 3));  // hack
          if (i_out_bytes < 0) i_out_bytes = 0;
        }
      }
    }

    //	printf("i_out_bytes = %d\n",i_out_bytes);

    if (write_flag) {
#ifndef WIN32
#ifndef ARM_PROFILE_BOARD
      ia_fwrite((pVOID)(pb_out_buf + ixheaacd_drc_offset), (i_pcm_wd_sz / 8),
                i_out_bytes / (i_pcm_wd_sz / 8), g_pf_out);
#endif
#else
#ifndef ARM_PROFILE_BOARD
      fwrite(pb_out_buf + ixheaacd_drc_offset, sizeof(WORD8), i_out_bytes,
             g_pf_out);
      fflush(g_pf_out);
#endif
#endif
    }

    if (i_out_bytes) frame_counter++;

    printf("\r frame count =%d", frame_counter);

#ifndef ARM_PROFILE_BOARD
// fprintf(stdout,"\r[%5d]\n  ",frame_counter);
#endif
#ifdef ARM_PROFILE_HW
    if (i_out_bytes != 0) {
      int i_out_samples = i_out_bytes >> 2;
      if (frame_count_b) {
        double i_out_samples_per_ch =
            (i_out_bytes) / ((i_pcm_wd_sz / 8) * i_num_chan);
        Curr_b = (((double)cycles_b / 1000000) * CLK_FREQ_BOARD_MHZ) /
                 (i_out_samples_per_ch / i_samp_freq);
        frame_count_b++;
        // fprintf(stderr, "Microseconds: %d\t", cycles_b);
        // fprintf(stderr, "MCPS: %f\n", Curr_b);
        Sum_b += Curr_b;
        Ave_b = Sum_b / frame_count_b;
        if (Peak_b < Curr_b) {
          Peak_b = Curr_b;
          Peak_frame_b = frame_count_b;
        }
      } else {
        frame_count_b++;
      }

      cycles_b = 0;
    }
#endif

#ifdef ARM_PROFILE
    if (i_out_bytes) {
      WORD32 samples;
      samples = (i_out_bytes >> 1) / i_num_chan;

      if (frame_counter == 1) {
        fprintf(fprofile, "\t");
        fprintf(fprofile, "EnhAACPlusDec\t");
        for (profile_index = 0; profile_index < MAX_MODULE; profile_index++)
          profile_instance[profile_index].cycles = 0;

        fprintf(fprofile, "\n");

      } else {
        fprintf(fprofile, "%d\t", frame_counter);

        for (profile_index = 0; profile_index < MAX_MODULE; profile_index++) {
          curr = ((double)(profile_instance[profile_index].cycles) *
                  TIMER_RESOLUTION_256) /
                 (samples);
          curr = (curr * i_samp_freq) / 1000000.0;
          profile_instance[profile_index].sum += curr;
          profile_instance[profile_index].average =
              profile_instance[profile_index].sum / frame_counter;
          fprintf(fprofile, "%f\t", curr);

          if (profile_instance[profile_index].peak < curr) {
            profile_instance[profile_index].peak = curr;
            profile_instance[profile_index].peak_frame = frame_counter;
          }
          profile_instance[profile_index].cycles = 0;
        }

        fprintf(fprofile, "\n");
      }
    }
#endif

#ifdef REINIT_FOR_ERROR
  HANDLE_ERROR_AT_INIT:
    if (i_process_err == 1) {
      WORD8 pb_file_cnt_arr[9];
      i_process_err = 0;

      ixheaacd_error_handler(p_proc_err_info, (pWORD8) "", err_code_reinit);
      // Do re-init only for fatal errors
      if (err_code_reinit < 0 || i_error_in_init) {
        memset(pv_persist_ptr, 0, i_persist_size);
        /* Set the config params to default values */
        err_code =
            (*p_ia_process_api)(pv_ia_process_api_obj, IA_API_CMD_INIT,
                                IA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS, NULL);

        _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

        /* ******************************************************************/
        /* Set config parameters got from the user present in argc argv     */
        /* ******************************************************************/

        err_code = (*p_set_config_param)(argc, argv, pv_ia_process_api_obj);
        _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

        // removed				/* Open new file only if the
        // error
        // occured
        // during process */
        if (!i_error_in_init) {
//#define WRITE_NEW_FILE
#ifdef WRITE_NEW_FILE
#ifdef WAV_HEADER
#ifndef ARM_PROFILE_BOARD
          if (!fseek(g_pf_out, 0, SEEK_SET))
            write_wav_header(g_pf_out, i_total_bytes, i_samp_freq, i_num_chan,
                             i_pcm_wd_sz, i_channel_mask);
#endif
#endif
          i_op_file_cnt++;
          sprintf((char *)pb_file_cnt_arr, "_%03d.wav", i_op_file_cnt);
          fclose(g_pf_out);
          i_total_bytes = 0;
          if (i_op_file_cnt == 1)
            (strcpy((char *)(pb_output_file_name +
                             strlen((char *)pb_output_file_name) - 4),
                    (const char *)pb_file_cnt_arr),
             "wb");
          else
            (strcpy((char *)(pb_output_file_name +
                             strlen((char *)pb_output_file_name) - 8),
                    (const char *)pb_file_cnt_arr),
             "wb");
          g_pf_out = fopen((const char *)pb_output_file_name, "wb");
          if (g_pf_out == 0) {
            _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "Output file",
                             IA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED);
          }
#endif

        } else {
          i_error_in_init = 0;
        }

        goto INIT_AGAIN;
      }
    }
#endif
    /* Do till the process execution is done */
  } while (!ui_exec_done);

#ifdef ARM_PROFILE_HW
  fprintf(stdout, "\n Peak MCPS = %f\n", Peak_b);
  fprintf(stdout, " Avg MCPS = %f\n", Ave_b);
  fprintf(stdout, " Peak frame = %d\n", Peak_frame_b);
#endif

#ifdef ARM_PROFILE

  fprintf(fprofile, "Enh AAC Plus Decoder\n");
  fprintf(fprofile, "Peak MCPS = %lf\n",
          profile_instance[EAAC_PLUS_DECODER].peak);
  fprintf(fprofile, "Peak frame = %d\n",
          profile_instance[EAAC_PLUS_DECODER].peak_frame);
  fprintf(fprofile, "Avg MCPS = %lf\n\n",
          profile_instance[EAAC_PLUS_DECODER].average);

#endif
  // EXIT:

  fprintf(stderr, "TOTAL FRAMES : [%5d] \n", frame_counter);

  //		i_num_chan = num_of_output_ch;
  err_code = (*p_get_config_param)(pv_ia_process_api_obj, &i_samp_freq,
                                   &i_num_chan, &i_pcm_wd_sz, &i_channel_mask);
  _IA_HANDLE_ERROR(p_proc_err_info, (pWORD8) "", err_code);

#ifdef WAV_HEADER
#ifndef ARM_PROFILE_BOARD
  if (!fseek(g_pf_out, 0, SEEK_SET))
    write_wav_header(g_pf_out, i_total_bytes, i_samp_freq, i_num_chan,
                     i_pcm_wd_sz, i_channel_mask);
#endif
#endif

  return IA_NO_ERROR;
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : ixheaacd_main */
/*                                                                           */
/*  Description   : Main function                                            */
/*                                                                           */
/*  Inputs        : None                                                     */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : Parse the parameter file and run the ixheaacd_main process
 */
/*                                                                           */
/*  Outputs       : None                                                     */
/*                                                                           */
/*  Returns       : 0 on success, -1 on error                                */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*        04 09 2005       Ittiam                Created                     */
/*                                                                           */
/*****************************************************************************/

void print_usage() {
  printf("\n Usage \n");
  printf("\n <exceutable> -ifile:<input_file> -ofile:<out_file> [options]\n");
  printf("\n[options] can be,");
#ifdef ERROR_PATTERN_READ
  printf("\n[-errfile:<error_pattern_file>]");
#endif
  printf("\n[-pcmsz:<pcmwordsize>]");
  printf("\n[-dmix:<down_mix>]");
#ifdef RESAMPLE_SUPPORT
  /* By default not available */
  printf("\n[-f08:<out_08khz>]");
  printf("\n[-f16:<out_16khz>]");
#endif
  printf("\n[-tostereo:<interleave_to_stereo>]");
  printf("\n[-dsample:<down_sample_sbr>]");
  printf("\n[-fs:<RAW_sample_rate>]");
  printf("\n[-nosync:<disable_sync>]");
  printf("\n[-sbrup:<auto_sbr_upsample>]");

  printf("\n[-maxchannel:<maximum_num_channels>]");
#ifdef MULTICHANNEL_ENABLE
  printf("\n[-coupchannel:<coupling_channel>]");
  printf("\n[-downmix:<down_mix_stereo>]");
#endif

  printf("\n\nwhere, \n  <inputfile> is the input AAC file name");
  printf("\n  <outputfile> is the output file name");
#ifdef ERROR_PATTERN_READ
  printf("\n  <error_pattern_file> is the error pattern file name");
#endif
  printf("\n  <pcmwordsize> is the bits per sample info. Only 16 is valid");

  printf("\n  <down_mix> is to enable/disable always mono output. Default 0");
#ifdef RESAMPLE_SUPPORT
  printf("\n  <out_08khz> is to enable/disable 8 kHz output. Default 0 ");
  printf("\n  <out_16khz> is to enable/disable 16 kHz output. Default 0 ");
#endif
  printf("\n  <interleave_to_stereo> is to enable/disable always ");
  printf("\n    interleaved to stereo output. Default 1 ");
  printf("\n  <down_sample_sbr> is to enable/disable down-sampled SBR ");
  printf("\n    output. Default auto identification from header");
  printf("\n  <RAW_sample_rate> is to indicate the core AAC sample rate for");
  printf("\n    a RAW stream. If this is specified no other file format");
  printf("\n    headers are searched for. \n");
  printf("\n  <disable_sync> is to disable the ADTS/ADIF sync search i.e");
  printf("\n    when enabled the decoder expects the header to ");
  printf("\n    be at the start of input buffer. Default 0");
  printf(
      "\n  <auto_sbr_upsample> is to enable(1) or disable(0) auto SBR "
      "upsample ");
  printf(
      "\n    in case of stream changing from SBR present to SBR not present. "
      "Default 1");
  printf("\n  <maximum_num_channels> is the number of maxiumum ");
  printf("\n    channels the input may have. Default is 6 (5.1)");

#ifdef MULTICHANNEL_ENABLE
  printf("\n  <coupling_channel> is element instance tag of ");
  printf("\n    independent coupling channel to be mixed. Default is 0");
  printf("\n  <down_mix_stereo> is flag for Downmix. Give 1 to");
  printf("\n    get stereo (downmix) output. Default is 0");
#endif
}

/*****************************************************************************/
/*                                                                           */
/*  Function Name : ixheaacd_main */
/*                                                                           */
/*  Description   : Main function                                            */
/*                                                                           */
/*  Inputs        : None                                                     */
/*                                                                           */
/*  Globals       : None                                                     */
/*                                                                           */
/*  Processing    : Parse the parameter file and run the ixheaacd_main process
 */
/*                                                                           */
/*  Outputs       : None                                                     */
/*                                                                           */
/*  Returns       : 0 on success, -1 on error                                */
/*                                                                           */
/*  Issues        : None                                                     */
/*                                                                           */
/*  Revision history :                                                       */
/*                                                                           */
/*        DD MM YYYY       Author                Changes                     */
/*        04 09 2005       Ittiam                Created                     */
/*                                                                           */
/*****************************************************************************/

int main(WORD32 argc, pWORD8 argv[]) {
  FILE *param_file_id;

  WORD8 curr_cmd[IA_MAX_CMD_LINE_LENGTH];
  WORD32 fargc, curpos;
  WORD32 processcmd = 0;

  WORD8 fargv[IA_MAX_ARGS][IA_MAX_CMD_LINE_LENGTH];

  pWORD8 pargv[IA_MAX_ARGS];

  WORD8 pb_input_file_path[IA_MAX_CMD_LINE_LENGTH] = "";
  WORD8 pb_output_file_path[IA_MAX_CMD_LINE_LENGTH] = "";
#ifdef REINIT_FOR_ERROR
  WORD8 pb_output_file_name[IA_MAX_CMD_LINE_LENGTH] = "";
#endif

  ia_testbench_error_handler_init();
#ifdef ARM_PROFILE
  fprofile = fopen("profile.txt", "wt");

  if (fprofile == NULL) {
    printf("Unable to open profile file\n");
    exit(0);
  }
#endif

#ifdef RVDS_DEBUG
  argc = 1;
#endif  // siva

  if (argc == 1) {
    param_file_id = fopen(PARAMFILE, "r");
    if (param_file_id == NULL) {
      print_usage();
      return IA_NO_ERROR;
    }

    /* Process one line at a time */
    while (fgets((char *)curr_cmd, IA_MAX_CMD_LINE_LENGTH, param_file_id)) {
      curpos = 0;
      fargc = 0;
      /* if it is not a param_file command and if */
      /* CLP processing is not enabled */
      if (curr_cmd[0] != '@' && !processcmd) { /* skip it */
        continue;
      }

      while (sscanf((char *)curr_cmd + curpos, "%s", fargv[fargc]) != EOF) {
        if (fargv[0][0] == '/' && fargv[0][1] == '/') break;
        if (strcmp((const char *)fargv[0], "@echo") == 0) break;
        if (strcmp((const char *)fargv[fargc], "@New_line") == 0) {
          fgets((char *)curr_cmd + curpos, IA_MAX_CMD_LINE_LENGTH,
                param_file_id);
          continue;
        }
        curpos += strlen((const char *)fargv[fargc]);
        while (*(curr_cmd + curpos) == ' ' || *(curr_cmd + curpos) == '\t')
          curpos++;
        fargc++;
      }

      if (fargc < 1) /* for blank lines etc. */
        continue;

      if (strcmp((const char *)fargv[0], "@Output_path") == 0) {
        if (fargc > 1)
          strcpy((char *)pb_output_file_path, (const char *)fargv[1]);
        else
          strcpy((char *)pb_output_file_path, "");
        continue;
      }

      if (strcmp((const char *)fargv[0], "@Input_path") == 0) {
        if (fargc > 1)
          strcpy((char *)pb_input_file_path, (const char *)fargv[1]);
        else
          strcpy((char *)pb_input_file_path, "");
        continue;
      }

      if (strcmp((const char *)fargv[0], "@Start") == 0) {
        processcmd = 1;
        continue;
      }

      if (strcmp((const char *)fargv[0], "@Stop") == 0) {
        processcmd = 0;
        continue;
      }

      /* otherwise if this a normal command and its enabled for execution */
      if (processcmd) {
        int i;
        int err_code = IA_NO_ERROR;
        int file_count = 0;

        for (i = 0; i < fargc; i++) {
          printf("%s ", fargv[i]);
          pargv[i] = fargv[i];

          if (!strncmp((const char *)fargv[i], "-ifile:", 7)) {
            pWORD8 pb_arg_val = fargv[i] + 7;
            WORD8 pb_input_file_name[IA_MAX_CMD_LINE_LENGTH] = "";

            strcat((char *)pb_input_file_name,
                   (const char *)pb_input_file_path);
            strcat((char *)pb_input_file_name, (const char *)pb_arg_val);

            g_pf_inp = NULL;
            g_pf_inp = FileWrapper_Open((char *)pb_input_file_name);

            if (g_pf_inp == NULL) {
              err_code = IA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED;
              ixheaacd_error_handler(&ixheaacd_ia_testbench_error_info,
                                     (pWORD8) "Input File", err_code);
              exit(1);
            }
            file_count++;
            raw_testing = 0;
          }

          if (!strncmp((const char *)fargv[i], "-imeta:", 7)) {
            pWORD8 pb_arg_val = fargv[i] + 7;
            WORD8 pb_metadata_file_name[IA_MAX_CMD_LINE_LENGTH] = "";

            strcat((char *)pb_metadata_file_name,
                   (const char *)pb_input_file_path);
            strcat((char *)pb_metadata_file_name, (const char *)pb_arg_val);

            g_pf_meta = NULL;
            g_pf_meta = fopen((const char *)pb_metadata_file_name, "r");

            if (g_pf_meta == NULL) {
              err_code = IA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED;
              ixheaacd_error_handler(&ixheaacd_ia_testbench_error_info,
                                     (pWORD8) "Metadata File", err_code);
              exit(1);
            }

            metadata_info_init(&meta_info);
            err_code = ixheaacd_read_metadata_info(g_pf_meta, &meta_info);

            if (err_code == -1) exit(1);

            raw_testing = 1;

            file_count++;
          }

          if (!strncmp((const char *)fargv[i], "-ofile:", 7)) {
            pWORD8 pb_arg_val = fargv[i] + 7;
#ifndef REINIT_FOR_ERROR
            WORD8 pb_output_file_name[IA_MAX_CMD_LINE_LENGTH] = "";
#endif

#ifdef REINIT_FOR_ERROR
            strcpy((char *)pb_output_file_name,
                   (const char *)pb_output_file_path);
#else
            strcat((char *)pb_output_file_name,
                   (const char *)pb_output_file_path);
#endif
            strcat((char *)pb_output_file_name, (const char *)pb_arg_val);

            g_pf_out = NULL;
            g_pf_out = fopen((const char *)pb_output_file_name, "wb");
            if (g_pf_out == NULL) {
              err_code = IA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED;
              ixheaacd_error_handler(&ixheaacd_ia_testbench_error_info,
                                     (pWORD8) "Output File", err_code);
              exit(1);
            }
            file_count++;
          }
#ifdef ARM_PROFILE
          fprintf(fprofile, "%s\n", fargv[i] + 7);
#endif

#ifdef ERROR_PATTERN_READ
          if (!strncmp((const char *)fargv[i], "-errfile:", 9)) {
            pWORD8 pb_arg_val = fargv[i] + 9;
            g_pf_err = fopen((const char *)pb_arg_val, "r");
            if (g_pf_err == NULL) {
              err_code = IA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED;
              ixheaacd_error_handler(&ixheaacd_ia_testbench_error_info,
                                     (pWORD8) "Error Pattern File", err_code);
            }
          }
#endif
        }
        g_w_malloc_count = 0;

        printf("\n");
        if (file_count != 3 && file_count != 2) {
          err_code = IA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED;
          ixheaacd_error_handler(&ixheaacd_ia_testbench_error_info,
                                 (pWORD8) "Input or Output File", err_code);
        }

        if (err_code == IA_NO_ERROR) {
          if (g_pf_inp->isMp4File == 1) {
            strcpy((pCHAR8)fargv[fargc], "-mp4:1");
            pargv[fargc] = fargv[fargc];
            fargc++;
          }

#ifdef REINIT_FOR_ERROR
          ixheaacd_main_process(fargc, pargv, pb_output_file_name);
#else

          ixheaacd_main_process(fargc, pargv);
#endif
        }

        for (i = 0; i < g_w_malloc_count; i++) {
          if (g_pv_arr_alloc_memory[i]) free(g_pv_arr_alloc_memory[i]);
        }
        if (g_pf_out) fclose(g_pf_out);

        if (g_pf_meta) {
          raw_testing = 0;
          fclose(g_pf_meta);
          memset_metadata(meta_info);
        }
        FileWrapper_Close(g_pf_inp);
#ifdef ERROR_PATTERN_READ
        if (g_pf_err) fclose(g_pf_err);
#endif
      }
    }
  } else {
    int i;
    int err_code = IA_NO_ERROR;
    int file_count = 0;

    for (i = 1; i < argc; i++) {
      pargv[i] = fargv[i];
      strcpy((pCHAR8)fargv[i], (pCHAR8)argv[i]);
      printf("%s ", pargv[i]);

      if (!strncmp((const char *)pargv[i], "-ifile:", 7)) {
        pWORD8 pb_arg_val = pargv[i] + 7;
        WORD8 pb_input_file_name[IA_MAX_CMD_LINE_LENGTH] = "";

        err_code = IA_NO_ERROR;
        strcat((char *)pb_input_file_name, (const char *)pb_input_file_path);
        strcat((char *)pb_input_file_name, (const char *)pb_arg_val);

        g_pf_inp = NULL;
        g_pf_inp = FileWrapper_Open((char *)pb_input_file_name);
        if (g_pf_inp == NULL) {
          err_code = IA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED;
          ixheaacd_error_handler(&ixheaacd_ia_testbench_error_info,
                                 (pWORD8) "Input File", err_code);
          exit(1);
        }
        file_count++;
        raw_testing = 0;
      }

      if (!strncmp((const char *)fargv[i], "-imeta:", 7)) {
        pWORD8 pb_arg_val = fargv[i] + 7;
        WORD8 pb_metadata_file_name[IA_MAX_CMD_LINE_LENGTH] = "";

        strcat((char *)pb_metadata_file_name, (const char *)pb_input_file_path);
        strcat((char *)pb_metadata_file_name, (const char *)pb_arg_val);

        g_pf_meta = NULL;
        g_pf_meta = fopen((const char *)pb_metadata_file_name, "r");

        if (g_pf_meta == NULL) {
          err_code = IA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED;
          ixheaacd_error_handler(&ixheaacd_ia_testbench_error_info,
                                 (pWORD8) "Metadata File", err_code);
          exit(1);
        }

        metadata_info_init(&meta_info);
        err_code = ixheaacd_read_metadata_info(g_pf_meta, &meta_info);

        if (err_code == -1) {
          exit(1);
        }

        raw_testing = 1;

        file_count++;
      }

      if (!strncmp((const char *)pargv[i], "-ofile:", 7)) {
        pWORD8 pb_arg_val = pargv[i] + 7;
#ifndef REINIT_FOR_ERROR
        WORD8 pb_output_file_name[IA_MAX_CMD_LINE_LENGTH] = "";
#endif

        strcat((char *)pb_output_file_name, (const char *)pb_output_file_path);
        strcat((char *)pb_output_file_name, (const char *)pb_arg_val);

        g_pf_out = NULL;
        g_pf_out = fopen((const char *)pb_output_file_name, "wb");
        if (g_pf_out == NULL) {
          err_code = IA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED;
          ixheaacd_error_handler(&ixheaacd_ia_testbench_error_info,
                                 (pWORD8) "Output File", err_code);
          exit(1);
        }
        file_count++;
      }

#ifdef ERROR_PATTERN_READ
      if (!strncmp((const char *)pargv[i], "-errfile:", 9)) {
        pWORD8 pb_arg_val = pargv[i] + 9;
        g_pf_err = fopen((const char *)pb_arg_val, "r");
        if (g_pf_err == NULL) {
          err_code = IA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED;
          ixheaacd_error_handler(&ixheaacd_ia_testbench_error_info,
                                 (pWORD8) "Error Pattern File", err_code);
        }
      }
#endif
    }
    g_w_malloc_count = 0;

    printf("\n");
    if (file_count != 2 && file_count != 3) {
      err_code = IA_TESTBENCH_MFMAN_FATAL_FILE_OPEN_FAILED;
      ixheaacd_error_handler(&ixheaacd_ia_testbench_error_info,
                             (pWORD8) "Input or Output File", err_code);
    }

    if (err_code == IA_NO_ERROR) {
      if (g_pf_inp->isMp4File == 1) {
        strcpy((pCHAR8)fargv[argc], "-mp4:1");
        pargv[argc] = fargv[argc];
        argc++;
      }

      ixheaacd_main_process(argc - 1, &pargv[1]
#ifdef REINIT_FOR_ERROR
                            ,
                            pb_output_file_name
#endif
                            );
    }

    for (i = 0; i < g_w_malloc_count; i++) {
      // printf("I am here %d\n",__LINE__);
      if (g_pv_arr_alloc_memory[i]) free(g_pv_arr_alloc_memory[i]);
      // printf("I am here %d\n",__LINE__);
    }
    if (g_pf_out) fclose(g_pf_out);

    if (g_pf_meta) {
      fclose(g_pf_meta);
      memset_metadata(meta_info);
    }
    FileWrapper_Close(g_pf_inp);
#ifdef ERROR_PATTERN_READ
    if (g_pf_err) fclose(g_pf_err);
#endif
  }

#ifdef ARM_PROFILE
  fclose(fprofile);
#endif

  return IA_NO_ERROR;
} /* end ixheaacd_main */
