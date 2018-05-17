/******************************************************************************
 *                                                                             
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
#include <math.h>
#include <string.h>
#include "impd_type_def.h"
#include "impd_error_standards.h"
#include "impd_memory_standards.h"
#include "impd_drc_peak_limiter.h"
#include "impd_drc_extr_delta_coded_info.h"
#include "impd_drc_common.h"
#include "impd_drc_struct.h"
#include "impd_drc_interface.h"
#include "impd_drc_bitbuffer.h"
#include "impd_drc_bitstream_dec_api.h"
#include "impd_drc_gain_dec.h"
#include "impd_drc_filter_bank.h"
#include "impd_drc_multi_band.h"
#include "impd_drc_process_audio.h"
#include "impd_parametric_drc_dec.h"
#include "impd_drc_eq.h"
#include "impd_drc_gain_decoder.h"
#include "impd_drc_selection_process.h"
#include "impd_drc_api_struct_def.h"
#include "impd_drc_hashdefines.h"
#include "impd_drc_peak_limiter.h"

static IA_ERRORCODE impd_down_mix ( ia_drc_sel_proc_output_struct *uni_drc_sel_proc_output, FLOAT32** input_audio, WORD32 frame_len)
{
    WORD32  num_base_ch   = uni_drc_sel_proc_output->base_channel_count;
    WORD32  num_target_ch = uni_drc_sel_proc_output->target_channel_count;
    WORD32  i, i_ch, o_ch;
    FLOAT32 tmp_out[MAX_CHANNEL_COUNT];
    

    if (num_target_ch > MAX_CHANNEL_COUNT)
        return -1;
    
    if (num_target_ch > num_base_ch)
        return -1;
    
    for (i=0; i<frame_len; i++) 
	{
        for (o_ch=0; o_ch<num_target_ch; o_ch++) 
		{
            tmp_out[o_ch] = 0.0f;
            for (i_ch=0; i_ch<num_base_ch; i_ch++) 
			{
                tmp_out[o_ch] += input_audio[i_ch][i] * uni_drc_sel_proc_output->downmix_matrix[i_ch][o_ch];
            }
        }
        for (o_ch=0; o_ch<num_target_ch; o_ch++) 
		{
            input_audio[o_ch][i] = tmp_out[o_ch];
        }
        for ( ; o_ch<num_base_ch; o_ch++) 
		{
            input_audio[o_ch][i] = 0.0f;
        }
    }
    
    
    return IA_NO_ERROR;
}

IA_ERRORCODE impd_process_time_domain(ia_drc_api_struct *p_obj_drc)
{
    IA_ERRORCODE err_code= IA_NO_ERROR;
    WORD32   i, j;
    FLOAT32  *input_buffer;
    WORD16   *input_buffer16, *output_buffer16;
    FLOAT32  *output_buffer;
    FLOAT32  *audio_buff[10];
    FLOAT32  *scratch_buffer;
	WORD32   last_frame=0;
	WORD32   num_sample_to_process;
    scratch_buffer  = (FLOAT32*)p_obj_drc->pp_mem[1];
    input_buffer    = (FLOAT32*)p_obj_drc->pp_mem[2];
    output_buffer   = (FLOAT32*)p_obj_drc->pp_mem[3];
    input_buffer16  = (WORD16*)p_obj_drc->pp_mem[2];
    output_buffer16 = (WORD16*)p_obj_drc->pp_mem[3];
    
    if(p_obj_drc->p_state->ui_in_bytes<=0){
        p_obj_drc->p_state->ui_out_bytes=0;
		return IA_NO_ERROR;
    }

    err_code = impd_process_drc_bitstream_dec_gain(p_obj_drc->str_payload.pstr_bitstream_dec,
											 p_obj_drc->pstr_bit_buf,
                                             p_obj_drc->str_payload.pstr_drc_config,
                                             p_obj_drc->str_payload.pstr_drc_gain,
                                             &p_obj_drc->str_bit_handler.it_bit_buf[p_obj_drc->str_bit_handler.byte_index_bs],
                                             p_obj_drc->str_bit_handler.num_bytes_bs,
                                             p_obj_drc->str_bit_handler.num_bits_offset_bs,
                                             &p_obj_drc->str_bit_handler.num_bits_read_bs);

    if (err_code > PROC_COMPLETE) 
    return -1;
    
    p_obj_drc->str_bit_handler.num_bytes_read_bs  = (p_obj_drc->str_bit_handler.num_bits_read_bs >> 3);
    p_obj_drc->str_bit_handler.num_bits_offset_bs = (p_obj_drc->str_bit_handler.num_bits_read_bs  & 7);
    p_obj_drc->str_bit_handler.byte_index_bs     += p_obj_drc->str_bit_handler.num_bytes_read_bs;
	if(p_obj_drc->str_bit_handler.gain_stream_flag==0)	//ITTIAM: Flag for applying gain frame by frame
	{
		p_obj_drc->str_bit_handler.num_bytes_bs      -= p_obj_drc->str_bit_handler.num_bytes_read_bs; 
	}

    if (p_obj_drc->str_config.bitstream_file_format == BITSTREAM_FILE_FORMAT_SPLIT) {
        if (p_obj_drc->str_bit_handler.num_bits_offset_bs != 0)
        {
            p_obj_drc->str_bit_handler.num_bits_read_bs   = p_obj_drc->str_bit_handler.num_bits_read_bs + 8 
																- p_obj_drc->str_bit_handler.num_bits_offset_bs;
            p_obj_drc->str_bit_handler.num_bytes_read_bs  = p_obj_drc->str_bit_handler.num_bytes_read_bs + 1;
            p_obj_drc->str_bit_handler.num_bits_offset_bs = 0;
            p_obj_drc->str_bit_handler.byte_index_bs      = p_obj_drc->str_bit_handler.byte_index_bs + 1;
			if(p_obj_drc->str_bit_handler.gain_stream_flag==0)
			{
				p_obj_drc->str_bit_handler.num_bytes_bs       = p_obj_drc->str_bit_handler.num_bytes_bs - 1;
			}
        }
    }  

	num_sample_to_process=(p_obj_drc->p_state->ui_in_bytes/p_obj_drc->str_config.num_ch_in/(p_obj_drc->str_config.pcm_size>>3));

	if(num_sample_to_process<p_obj_drc->str_config.frame_size)
       last_frame=1;
			
    if(p_obj_drc->str_config.pcm_size==16)
	{
		for(i=0;i<p_obj_drc->str_config.num_ch_in;i++)
		{
			audio_buff[i]=scratch_buffer+i*(p_obj_drc->str_config.frame_size+32);
			
			for(j=0;j<num_sample_to_process;j++)
			{
				
				audio_buff[i][j]=((FLOAT32)input_buffer16[j*p_obj_drc->str_config.num_ch_in + i])/32767.0f;
			}
		}
	}
	else 
	{
		for(i=0;i<p_obj_drc->str_config.num_ch_in;i++)
		{
			audio_buff[i]=scratch_buffer+i*(p_obj_drc->str_config.frame_size+32);
			
			for(j=0;j<num_sample_to_process;j++)
			{
				audio_buff[i][j]=input_buffer[j*p_obj_drc->str_config.num_ch_in + i];
			}
		}
    }
    
    err_code = impd_drc_process_time_domain(p_obj_drc->str_payload.pstr_gain_dec[0],
                           p_obj_drc->str_payload.pstr_drc_config,
                           p_obj_drc->str_payload.pstr_drc_gain,
                           audio_buff,
                           p_obj_drc->str_payload.pstr_drc_sel_proc_output->loudness_normalization_gain_db,
                           p_obj_drc->str_payload.pstr_drc_sel_proc_output->boost,
                           p_obj_drc->str_payload.pstr_drc_sel_proc_output->compress,
                           p_obj_drc->str_payload.pstr_drc_sel_proc_output->drc_characteristic_target);
                           
    if (err_code!=IA_NO_ERROR) 
        return err_code;
    
    if(p_obj_drc->str_payload.pstr_drc_sel_proc_output->downmix_matrix_present!=0)
        err_code = impd_down_mix(p_obj_drc->str_payload.pstr_drc_sel_proc_output,
                                audio_buff,
                                p_obj_drc->str_config.frame_size);
                                
    if (err_code!=IA_NO_ERROR) 
        return err_code;
    
    err_code = impd_drc_process_time_domain(p_obj_drc->str_payload.pstr_gain_dec[1],
                           p_obj_drc->str_payload.pstr_drc_config,
                           p_obj_drc->str_payload.pstr_drc_gain,
                           audio_buff,
                           p_obj_drc->str_payload.pstr_drc_sel_proc_output->loudness_normalization_gain_db,
                           p_obj_drc->str_payload.pstr_drc_sel_proc_output->boost,
                           p_obj_drc->str_payload.pstr_drc_sel_proc_output->compress,
                           p_obj_drc->str_payload.pstr_drc_sel_proc_output->drc_characteristic_target);
                           
    if (err_code!=IA_NO_ERROR) 
        return err_code;
    
    if (p_obj_drc->str_payload.pstr_drc_sel_proc_output->loudness_normalization_gain_db != 0.0f)
    {
        FLOAT32 gain_value = (FLOAT32)pow(10.0,p_obj_drc->str_payload.pstr_drc_sel_proc_output->loudness_normalization_gain_db/20.0);           
        for (i=0; i < p_obj_drc->str_config.num_ch_out; i++) {
            for (j=0; j < p_obj_drc->str_config.frame_size; j++) {
                    audio_buff[i][j] *= gain_value;               
            }
        }
    }
    
    if (p_obj_drc->str_config.peak_limiter)
    {
        for (i=0; i < p_obj_drc->str_config.num_ch_out; i++) {
            for (j=0; j < p_obj_drc->str_config.frame_size; j++) {
               output_buffer[j*p_obj_drc->str_config.num_ch_out + i] = audio_buff[i][j];
            }
        }
        
		err_code=impd_limiter_process(p_obj_drc->str_payload.pstr_peak_limiter,
                                      output_buffer,
                                      p_obj_drc->str_config.frame_size);


        if (err_code!=IA_NO_ERROR) 
            return err_code;
        
        for (i=0; i < p_obj_drc->str_config.num_ch_out; i++) {
            for (j=0; j < p_obj_drc->str_config.frame_size; j++) {
                audio_buff[i][j] = output_buffer[j*p_obj_drc->str_config.num_ch_out + i];
            }
        }
    }
    
	if(p_obj_drc->str_config.pcm_size==16)
	{
		for (i=0; i < p_obj_drc->str_config.num_ch_out; i++) 
		{
			for (j=0; j < p_obj_drc->str_config.frame_size; j++) 
			{
                if(audio_buff[i][j]<-1.0f)
                    output_buffer16[j*p_obj_drc->str_config.num_ch_out + i] = -32767;

                else if (audio_buff[i][j]>1.0f)
                    output_buffer16[j*p_obj_drc->str_config.num_ch_out + i] = 32767;

                else
                    output_buffer16[j*p_obj_drc->str_config.num_ch_out + i] = (WORD16)(audio_buff[i][j]*32767.0f);
			}
		}
	}
    else 
	{
		for (i=0; i < p_obj_drc->str_config.num_ch_out; i++) 
		{
			for (j=0; j < p_obj_drc->str_config.frame_size; j++) 
			{
                output_buffer[j*p_obj_drc->str_config.num_ch_out + i] = audio_buff[i][j];
            }
        }
    }
    
    p_obj_drc->p_state->ui_out_bytes=p_obj_drc->str_config.num_ch_out*(p_obj_drc->p_state->ui_in_bytes/p_obj_drc->str_config.num_ch_in); 

    if(p_obj_drc->p_state->delay_in_output!=0){
        p_obj_drc->p_state->ui_out_bytes=p_obj_drc->str_config.num_ch_out*(p_obj_drc->str_config.frame_size-p_obj_drc->p_state->delay_in_output)*(p_obj_drc->str_config.pcm_size>>3);
        if(p_obj_drc->str_config.pcm_size==16)
	        memcpy(output_buffer16,(output_buffer16+(p_obj_drc->p_state->delay_in_output*p_obj_drc->str_config.num_ch_out)),p_obj_drc->p_state->ui_out_bytes);
	    else
            memcpy(output_buffer,(output_buffer+(p_obj_drc->p_state->delay_in_output*p_obj_drc->str_config.num_ch_out)),p_obj_drc->p_state->ui_out_bytes);

		p_obj_drc->p_state->delay_adjust_samples = p_obj_drc->p_state->delay_in_output;
	    p_obj_drc->p_state->delay_in_output=0;
		
	}
	if(last_frame==1)
	{
		if((num_sample_to_process+p_obj_drc->p_state->delay_adjust_samples)<=p_obj_drc->str_config.frame_size)
	    	p_obj_drc->p_state->ui_out_bytes = (num_sample_to_process+p_obj_drc->p_state->delay_adjust_samples)*p_obj_drc->str_config.num_ch_out*(p_obj_drc->str_config.pcm_size>>3);
        else
	    	p_obj_drc->p_state->ui_out_bytes = (p_obj_drc->str_config.frame_size)*p_obj_drc->str_config.num_ch_out*(p_obj_drc->str_config.pcm_size>>3);

	}
	
    if(last_frame==0){

        

        if (p_obj_drc->str_config.bitstream_file_format != BITSTREAM_FILE_FORMAT_SPLIT) {
            err_code = impd_process_drc_bitstream_dec(p_obj_drc->str_payload.pstr_bitstream_dec,
											 p_obj_drc->pstr_bit_buf,
                                             p_obj_drc->str_payload.pstr_drc_config,
                                             p_obj_drc->str_payload.pstr_loudness_info,
                                             &p_obj_drc->str_bit_handler.it_bit_buf[p_obj_drc->str_bit_handler.byte_index_bs],
                                             p_obj_drc->str_bit_handler.num_bytes_bs,
                                             p_obj_drc->str_bit_handler.num_bits_offset_bs,
                                             &p_obj_drc->str_bit_handler.num_bits_read_bs);
                                             
             if (err_code > PROC_COMPLETE) 
             return -1;
             
             p_obj_drc->str_bit_handler.num_bytes_read_bs  = (p_obj_drc->str_bit_handler.num_bits_read_bs >> 3);
             p_obj_drc->str_bit_handler.num_bits_offset_bs = (p_obj_drc->str_bit_handler.num_bits_read_bs  & 7);
             p_obj_drc->str_bit_handler.byte_index_bs     += p_obj_drc->str_bit_handler.num_bytes_read_bs;
             p_obj_drc->str_bit_handler.num_bytes_bs      -= p_obj_drc->str_bit_handler.num_bytes_read_bs;
             
        }
	}
     
    return     err_code;
}

