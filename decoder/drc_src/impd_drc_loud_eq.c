/******************************************************************************
 *                                                                             
 * Copyright (C) 2015 The Android Open Source Project                          
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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "impd_type_def.h"
#include "impd_drc_uni_tables.h"
#include "impd_drc_uni_common.h"
#include "impd_drc_uni_interface.h"
#include "impd_drc_struct.h"
#include "impd_drc_uni_gain_dec.h"
#include "impd_drc_uni_loud_eq.h"
#include "impd_parametric_drc_dec.h"
#include "impd_drc_uni_multi_band.h"
#include "impd_drc_filter_bank.h"
#include "impd_drc_uni_process_audio.h"
#include "impd_drc_rom.h"


WORD32 impd_parametric_drc_instance_reset(WORD32  instance_idx,
										  ia_parametric_drc_instance_params_struct* pstr_parametric_drc_instance_params)
{
    WORD32 err = 0;
    
    if (pstr_parametric_drc_instance_params->parametric_drc_type == PARAM_DRC_TYPE_FF) 
	{
        
        ia_parametric_drc_type_ff_params_struct* pstr_parametric_ffwd_type_drc_params = &(pstr_parametric_drc_instance_params->str_parametric_drc_type_ff_params);
        err = impd_parametric_ffwd_type_drc_reset(pstr_parametric_ffwd_type_drc_params);
        if (err) 
			return (err);
        
        
    } 
	else 
	{ 
        return (UNEXPECTED_ERROR);     
    }
	
    return 0;
}

WORD32 impd_add_drc_band_audio(ia_drc_instructions_struct* pstr_drc_instruction_arr,
                const WORD32 drc_instructions_index,
                ia_drc_params_struct* ia_drc_params_struct,
                ia_audio_band_buffer_struct* audio_band_buffer,
                FLOAT32* audio_io_buf[])
{
    WORD32 g, b, i, c;
    FLOAT32 sum;
    WORD32 signalIndex = 0;
    FLOAT32** drcBandAudio;
    FLOAT32** channel_audio;
    ia_drc_instructions_struct* str_drc_instruction_str;
    
    drcBandAudio = audio_band_buffer->non_interleaved_audio;
    channel_audio = audio_io_buf;
    
    if (drc_instructions_index >= 0) {
        str_drc_instruction_str = &(pstr_drc_instruction_arr[drc_instructions_index]);
    } else {
        return -1;
    }

    if (str_drc_instruction_str->drc_set_id > 0)
    {

        for (c=0; c<str_drc_instruction_str->audio_num_chan; c++)

        {
            g = str_drc_instruction_str->channel_group_of_ch[c];
            if (g>=0)
            {
                for (i=0; i<ia_drc_params_struct->drc_frame_size; i++)
                {
                    sum = 0.0f;
                    for (b=0; b<str_drc_instruction_str->band_count_of_ch_group[g]; b++)
                    {
                        sum += drcBandAudio[signalIndex+b][i];
                    }

                    channel_audio[c][i] = sum;

                }
                signalIndex += str_drc_instruction_str->band_count_of_ch_group[g];
            }
            else
            {
                signalIndex++;
            }
        }
    }
    else
    {
        for (c=0; c<str_drc_instruction_str->audio_num_chan; c++)

        {
            for (i=0; i<ia_drc_params_struct->drc_frame_size; i++)
            {
                channel_audio[c][i] = drcBandAudio[c][i];
            }
        }
    }
    return (0);
}

WORD32 removeTables(void)
{

    return(0);
}

const ia_slope_code_table_struct*
impd_get_slope_code_tbl_by_value(void)
{
    return(&(slopeCodeTableEntryByValue[0]));
}

FLOAT32 impd_decode_slope_idx(const WORD32 slopeCodeIndex)
{
    const ia_slope_code_table_struct* slopeCodeTable = impd_get_slope_code_tbl_by_value();
    return slopeCodeTable[slopeCodeIndex].value;
}

FLOAT32 impd_decode_slope_idx_magnitude(const WORD32 slopeCodeIndex)
{
    const ia_slope_code_table_struct* slopeCodeTable = impd_get_slope_code_tbl_by_value();
    return (FLOAT32)fabs((FLOAT64)slopeCodeTable[slopeCodeIndex].value);
}


WORD32 impd_get_multi_band_drc_present(ia_drc_config* pstr_drc_config,
                       WORD32                   numSets[3],
                       WORD32                   multiBandDrcPresent[3])
{
    WORD32 err=0, k, m;
    for(k=0; k<pstr_drc_config->drc_instructions_uni_drc_count; k++)
    {
        if ((pstr_drc_config->str_drc_instruction_str[k].downmix_id[0] == ID_FOR_BASE_LAYOUT) || (pstr_drc_config->str_drc_instruction_str[k].drc_apply_to_dwnmix == 0))
        {
            numSets[0]++;
        }
        else if (pstr_drc_config->str_drc_instruction_str[k].downmix_id[0] == ID_FOR_ANY_DOWNMIX)
        {
            numSets[1]++;
        }
        else
        {
            numSets[2]++;
        }
        for (m=0; m<pstr_drc_config->str_drc_instruction_str[k].num_drc_ch_groups; m++)
        {
            if (pstr_drc_config->str_p_loc_drc_coefficients_uni_drc[0].gain_set_params[pstr_drc_config->str_drc_instruction_str->gain_set_index_for_channel_group[m]].band_count > 1)
            {
                if ((pstr_drc_config->str_drc_instruction_str[k].downmix_id[0] == ID_FOR_BASE_LAYOUT) || (pstr_drc_config->str_drc_instruction_str[k].drc_apply_to_dwnmix == 0))
                {
                    multiBandDrcPresent[0] = 1;
                }
                else if (pstr_drc_config->str_drc_instruction_str[k].downmix_id[0] == ID_FOR_ANY_DOWNMIX)
                {
                    multiBandDrcPresent[1] = 1;
                }
                else
                {
                    multiBandDrcPresent[2] = 1;
                }
            }
        }
    }
    return err;
}


