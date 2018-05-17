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
#include "impd_type_def.h"
#include "impd_memory_standards.h"
#include "impd_drc_bitbuffer.h"
#include "impd_drc_extr_delta_coded_info.h"
#include "impd_drc_common.h"
#include "impd_drc_struct.h"
#include "impd_drc_interface.h"

#include "impd_drc_bitstream_dec_api.h"
#include "impd_drc_gain_dec.h"
#include "impd_drc_filter_bank.h"
#include "impd_drc_multi_band.h"
#include "impd_drc_process_audio.h"
#include "impd_parametric_drc_dec.h"
#include "impd_drc_eq.h"
#include "impd_drc_gain_decoder.h"
#include "impd_drc_selection_process.h"

#include "impd_drc_peak_limiter.h"
#include "impd_drc_api_struct_def.h"

#define BITSTREAM_FILE_FORMAT_SPLIT 1

static WORD32 impd_down_mix ( ia_drc_sel_proc_output_struct *uni_drc_sel_proc_output, FLOAT32** input_audio, WORD32 frame_len)
{
    WORD32  num_base_ch   = uni_drc_sel_proc_output->base_channel_count;
    WORD32  num_target_ch = uni_drc_sel_proc_output->target_channel_count;
    WORD32  i, i_ch, o_ch;
    FLOAT32 tmp_out[MAX_CHANNEL_COUNT];
    
    if (uni_drc_sel_proc_output->downmix_matrix_present == 0)
        return 0;  
    
    if (input_audio == 0)
        return 0;
    
    if (num_target_ch > MAX_CHANNEL_COUNT)
        return -1;
    
    if (num_target_ch > num_base_ch)
        return -1;
    
    for (i=0; i<frame_len; i++) {
        for (o_ch=0; o_ch<num_target_ch; o_ch++) 
		{
            tmp_out[o_ch] = 0.0f;
            for (i_ch=0; i_ch<num_base_ch; i_ch++) 
			{
                tmp_out[o_ch] += input_audio[i_ch][i] * uni_drc_sel_proc_output->downmix_matrix[i_ch][o_ch];
            }
        }
        for (o_ch=0; o_ch<num_target_ch; o_ch++) {
            input_audio[o_ch][i] = tmp_out[o_ch];
        }
        for ( ; o_ch<num_base_ch; o_ch++) {
            input_audio[o_ch][i] = 0.0f;
        }
    }
    
    
    return 0;
}

WORD32 impd_init_process_audio_main_qmf (ia_drc_api_struct *p_obj_drc)

{
    WORD32 error=0, i, j, num_samples_per_channel;
    FLOAT32 *input_buffer;
    FLOAT32 *output_buffer;
    FLOAT32 *audio_io_buf_real[10];
    FLOAT32 *audio_io_buf_imag[10];
    FLOAT32 *scratch_buffer;
	WORD32 last_frame=0;
    scratch_buffer= (FLOAT32*)p_obj_drc->pp_mem[1];
    input_buffer  = (FLOAT32*)p_obj_drc->pp_mem[2];
    output_buffer = (FLOAT32*)p_obj_drc->pp_mem[3];
 
    if(p_obj_drc->p_state->ui_in_bytes<=0)
	{
           p_obj_drc->p_state->ui_out_bytes=0;
		return 0;
    }

	 if((p_obj_drc->p_state->ui_in_bytes/p_obj_drc->str_config.num_ch_in/(p_obj_drc->str_config.pcm_size>>3)) < (UWORD32)p_obj_drc->str_config.frame_size)
    last_frame=1;
    for(i=0;i<p_obj_drc->str_config.num_ch_in;i++){
    audio_io_buf_real[i]=scratch_buffer+i*(p_obj_drc->str_config.frame_size+32);
	audio_io_buf_imag[i]=scratch_buffer+p_obj_drc->str_config.num_ch_in*p_obj_drc->str_config.frame_size+p_obj_drc->str_config.num_ch_in*64+i*(p_obj_drc->str_config.frame_size+64);
      for(j=0;j<p_obj_drc->str_config.frame_size;j++){
      audio_io_buf_real[i][j]=input_buffer[j*p_obj_drc->str_config.num_ch_in + i];
      audio_io_buf_imag[i][j]=input_buffer[p_obj_drc->str_config.num_ch_in*p_obj_drc->str_config.frame_size+j*p_obj_drc->str_config.num_ch_in + i];
      }
    }
    
    error = impd_process_drc_bitstream_dec_gain(p_obj_drc->str_payload.pstr_bitstream_dec,
		                                         p_obj_drc->pstr_bit_buf,
                                                 p_obj_drc->str_payload.pstr_drc_config,
                                                 p_obj_drc->str_payload.pstr_drc_gain,
                                                 &p_obj_drc->str_bit_handler.it_bit_buf[p_obj_drc->str_bit_handler.byte_index_bs],
                                                 p_obj_drc->str_bit_handler.num_bytes_bs,
                                                 p_obj_drc->str_bit_handler.num_bits_offset_bs,
                                                 &p_obj_drc->str_bit_handler.num_bits_read_bs);



    if (error > PROC_COMPLETE) return -1;
    
    p_obj_drc->str_bit_handler.num_bytes_read_bs  = (p_obj_drc->str_bit_handler.num_bits_read_bs >> 3);
    p_obj_drc->str_bit_handler.num_bits_offset_bs = (p_obj_drc->str_bit_handler.num_bits_read_bs  & 7);
    p_obj_drc->str_bit_handler.byte_index_bs   += p_obj_drc->str_bit_handler.num_bytes_read_bs;
	if(p_obj_drc->str_bit_handler.gain_stream_flag==0)	//ITTIAM: Flag for applying gain frame by frame
	{
		p_obj_drc->str_bit_handler.num_bytes_bs      -= p_obj_drc->str_bit_handler.num_bytes_read_bs;   
	}
    
    
    
    
    
    
	
    if (p_obj_drc->str_config.bitstream_file_format == BITSTREAM_FILE_FORMAT_SPLIT) {
        if (p_obj_drc->str_bit_handler.num_bits_offset_bs != 0)
        {
            p_obj_drc->str_bit_handler.num_bits_read_bs   = p_obj_drc->str_bit_handler.num_bits_read_bs + 8 - p_obj_drc->str_bit_handler.num_bits_offset_bs;
            p_obj_drc->str_bit_handler.num_bytes_read_bs  = p_obj_drc->str_bit_handler.num_bytes_read_bs + 1;
            p_obj_drc->str_bit_handler.num_bits_offset_bs = 0;
            p_obj_drc->str_bit_handler.byte_index_bs   = p_obj_drc->str_bit_handler.byte_index_bs + 1;
			if(p_obj_drc->str_bit_handler.gain_stream_flag==0)	//ITTIAM: Flag for applying gain frame by frame
			{
				p_obj_drc->str_bit_handler.num_bytes_bs      = p_obj_drc->str_bit_handler.num_bytes_bs - 1;
			}
        }
    }  

            error = impd_drc_process_freq_domain(p_obj_drc->str_payload.pstr_gain_dec[0],
                                   p_obj_drc->str_payload.pstr_drc_config,
                                   p_obj_drc->str_payload.pstr_drc_gain,
                                   audio_io_buf_real,
                                   audio_io_buf_imag,
                                   p_obj_drc->str_payload.pstr_drc_sel_proc_output->loudness_normalization_gain_db,
                                   p_obj_drc->str_payload.pstr_drc_sel_proc_output->boost,
                                   p_obj_drc->str_payload.pstr_drc_sel_proc_output->compress,
                                   p_obj_drc->str_payload.pstr_drc_sel_proc_output->drc_characteristic_target);
            if (error) return -1;
            
            error = impd_down_mix(p_obj_drc->str_payload.pstr_drc_sel_proc_output,
                                 audio_io_buf_real,
                                  p_obj_drc->str_config.frame_size);
            if (error) return -1;
            
            error = impd_down_mix(p_obj_drc->str_payload.pstr_drc_sel_proc_output,
                                 audio_io_buf_imag,
                                  p_obj_drc->str_config.frame_size);
            if (error) return -1;
            
            error = impd_drc_process_freq_domain(p_obj_drc->str_payload.pstr_gain_dec[1],
                                   p_obj_drc->str_payload.pstr_drc_config,
                                   p_obj_drc->str_payload.pstr_drc_gain,
                                   audio_io_buf_real,
                                   audio_io_buf_imag,
                                   p_obj_drc->str_payload.pstr_drc_sel_proc_output->loudness_normalization_gain_db,
                                   p_obj_drc->str_payload.pstr_drc_sel_proc_output->boost,
                                   p_obj_drc->str_payload.pstr_drc_sel_proc_output->compress,
                                   p_obj_drc->str_payload.pstr_drc_sel_proc_output->drc_characteristic_target);
            if (error) return -1;

        if (p_obj_drc->str_payload.pstr_drc_sel_proc_output->loudness_normalization_gain_db != 0.0f)
        {
            FLOAT32 loudness_normalization_gain = (FLOAT32)pow(10.0,p_obj_drc->str_payload.pstr_drc_sel_proc_output->loudness_normalization_gain_db/20.0);
            for (i=0; i < p_obj_drc->str_config.num_ch_out; i++) {
                for (j=0; j <p_obj_drc->str_config. frame_size; j++) {
                          audio_io_buf_real[i][j] *= loudness_normalization_gain;
                          audio_io_buf_imag[i][j] *= loudness_normalization_gain;
            
                }
            }
        }
        num_samples_per_channel = p_obj_drc->str_config.frame_size;

        for (i=0; i < p_obj_drc->str_config.num_ch_out; i++) {
            for (j=0; j < p_obj_drc->str_config.frame_size; j++) {
               output_buffer[j*p_obj_drc->str_config.num_ch_out + i]                                                             = audio_io_buf_real[i][j];
			   output_buffer[p_obj_drc->str_config.frame_size*p_obj_drc->str_config.num_ch_in+j*p_obj_drc->str_config.num_ch_out + i] = audio_io_buf_imag[i][j];

            }
        }
  p_obj_drc->p_state->ui_out_bytes=p_obj_drc->str_config.num_ch_out*(p_obj_drc->str_config.frame_size)*4;     
  p_obj_drc->p_state->ui_out_bytes=p_obj_drc->str_config.num_ch_out*(p_obj_drc->p_state->ui_in_bytes/p_obj_drc->str_config.num_ch_in);  

	if(last_frame==0){

        if (p_obj_drc->str_config.bitstream_file_format != BITSTREAM_FILE_FORMAT_SPLIT) {
            error = impd_process_drc_bitstream_dec(p_obj_drc->str_payload.pstr_bitstream_dec,
	          	                                     p_obj_drc->pstr_bit_buf,
                                                     p_obj_drc->str_payload.pstr_drc_config,
                                                     p_obj_drc->str_payload.pstr_loudness_info,
                                                     &p_obj_drc->str_bit_handler.it_bit_buf[p_obj_drc->str_bit_handler.byte_index_bs],
                                                     p_obj_drc->str_bit_handler.num_bytes_bs,
                                                     p_obj_drc->str_bit_handler.num_bits_offset_bs,
                                                     &p_obj_drc->str_bit_handler.num_bits_read_bs);
                                                     
            if (error > PROC_COMPLETE) 
            return -1;
            
            p_obj_drc->str_bit_handler.num_bytes_read_bs  = (p_obj_drc->str_bit_handler.num_bits_read_bs >> 3);
            p_obj_drc->str_bit_handler.num_bits_offset_bs = (p_obj_drc->str_bit_handler.num_bits_read_bs  & 7);
            p_obj_drc->str_bit_handler.byte_index_bs   += p_obj_drc->str_bit_handler.num_bytes_read_bs;
            p_obj_drc->str_bit_handler.num_bytes_bs      -= p_obj_drc->str_bit_handler.num_bytes_read_bs;
            
        }

	}
 return error;       
    
}

