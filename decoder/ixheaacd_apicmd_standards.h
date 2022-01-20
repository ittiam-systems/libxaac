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
#ifndef IXHEAACD_APICMD_STANDARDS_H
#define IXHEAACD_APICMD_STANDARDS_H

/*****************************************************************************/
/* Ittiam standard API commands                                              */
/*****************************************************************************/
#define IA_API_CMD_GET_LIB_ID_STRINGS 0x0001

#define IA_API_CMD_GET_API_SIZE 0x0002
#define IA_API_CMD_INIT 0x0003

#define IA_API_CMD_SET_CONFIG_PARAM 0x0004
#define IA_API_CMD_GET_CONFIG_PARAM 0x0005

#define IA_API_CMD_GET_MEMTABS_SIZE 0x0006
#define IA_API_CMD_SET_MEMTABS_PTR 0x0007
#define IA_API_CMD_GET_N_MEMTABS 0x0008

#define IA_API_CMD_EXECUTE 0x0009

#define IA_API_CMD_PUT_INPUT_QUERY 0x000A
#define IA_API_CMD_GET_CURIDX_INPUT_BUF 0x000B
#define IA_API_CMD_SET_INPUT_BYTES 0x000C
#define IA_API_CMD_GET_OUTPUT_BYTES 0x000D
#define IA_API_CMD_INPUT_OVER 0x000E
#define IA_API_CMD_INPUT_SEEK 0x000F
#define IA_API_CMD_RESET 0x0010

#define IA_API_CMD_GET_MEM_INFO_SIZE 0x0011
#define IA_API_CMD_GET_MEM_INFO_ALIGNMENT 0x0012
#define IA_API_CMD_GET_MEM_INFO_TYPE 0x0013
#define IA_API_CMD_GET_MEM_INFO_PLACEMENT 0x0014
#define IA_API_CMD_GET_MEM_INFO_PRIORITY 0x0015
#define IA_API_CMD_SET_MEM_PTR 0x0016
#define IA_API_CMD_SET_MEM_INFO_SIZE 0x0017
#define IA_API_CMD_SET_MEM_PLACEMENT 0x0018

#define IA_API_CMD_GET_N_TABLES 0x0019
#define IA_API_CMD_GET_TABLE_INFO_SIZE 0x001A
#define IA_API_CMD_GET_TABLE_INFO_ALIGNMENT 0x001B
#define IA_API_CMD_GET_TABLE_INFO_PRIORITY 0x001C
#define IA_API_CMD_SET_TABLE_PTR 0x001D
#define IA_API_CMD_GET_TABLE_PTR 0x001E
#define IA_API_CMD_GET_LOUDNESS_VAL 0x001F

/*****************************************************************************/
/* Ittiam standard API command indices                                       */
/*****************************************************************************/
/* IA_API_CMD_GET_LIB_ID_STRINGS indices */
#define IA_CMD_TYPE_LIB_NAME 0x0100
#define IA_CMD_TYPE_LIB_VERSION 0x0200
#define IA_CMD_TYPE_API_VERSION 0x0300

/* IA_API_CMD_INIT indices */
#define IA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS 0x0100
#define IA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS 0x0200
#define IA_CMD_TYPE_INIT_PROCESS 0x0300
#define IA_CMD_TYPE_INIT_DONE_QUERY 0x0400
#define IA_CMD_TYPE_GA_HDR 0x0800
#define IA_CMD_TYPE_FLUSH_MEM 0x1000

/* IA_API_CMD_EXECUTE indices */
#define IA_CMD_TYPE_DO_EXECUTE 0x0100
#define IA_CMD_TYPE_DONE_QUERY 0x0200

#endif /* IXHEAACD_APICMD_STANDARDS_H */
