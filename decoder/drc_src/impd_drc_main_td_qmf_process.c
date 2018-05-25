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
#include "impd_drc_rom.h"


VOID process_qmf_syn_filt_bank(ia_drc_qmf_filt_struct *qmf_filt,
                          FLOAT64 *buff,
                          FLOAT32 *input_real,
                          FLOAT32 *input_imag,
                          FLOAT32 *output)
{
  WORD32    i,j;
  FLOAT64   U[10 * QMF_NUM_FILT_BANDS];
  FLOAT64   W[10 * QMF_NUM_FILT_BANDS];

  FLOAT64   tmp;

  for ( i=20*QMF_FILT_RESOLUTION-1; i>=2*QMF_FILT_RESOLUTION; i-- ) {
    buff[i] = buff[i-2*QMF_FILT_RESOLUTION];
  }


  for ( i=0; i<2*QMF_FILT_RESOLUTION; i++ ) {
    tmp = 0.0;
    for ( j=0; j<QMF_FILT_RESOLUTION; j++ ) {
      tmp = tmp
            + input_real[j] * qmf_filt->syn_tab_real[i][j]
            - input_imag[j] * qmf_filt->syn_tab_imag[i][j];

    }
    buff[i] = tmp;
  }


  for ( i=0; i<5; i++ ) {
    for ( j=0; j<QMF_FILT_RESOLUTION; j++ ) {
      U[2*QMF_FILT_RESOLUTION*i+j]            = buff[4*QMF_FILT_RESOLUTION*i+j];
      U[2*QMF_FILT_RESOLUTION*i+QMF_FILT_RESOLUTION+j] = buff[4*QMF_FILT_RESOLUTION*i+3*QMF_FILT_RESOLUTION+j];
    }
  }


  for ( i=0; i<10*QMF_FILT_RESOLUTION; i++ ) {

        W[i] = U[i] * qmf_filter_coeff[i];
  }


  for ( i=0; i<QMF_FILT_RESOLUTION; i++ ) {
    tmp = 0.0;
    for ( j=0; j<10; j++ ) {
      tmp = tmp + W[QMF_FILT_RESOLUTION*j+i];
    }
    output[i] = (FLOAT32) tmp;
  }
}


VOID process_qmf_ana_filt_bank(ia_drc_qmf_filt_struct *qmf_filt,
                          FLOAT64 *buff,
                          FLOAT32 *input,
                          FLOAT32 *output_real,
                          FLOAT32 *output_imag)
{
  WORD32 i,j;
  FLOAT32   Z[10 * QMF_NUM_FILT_BANDS];
  FLOAT32   Y[2 * QMF_NUM_FILT_BANDS];

  for ( i=10*QMF_FILT_RESOLUTION-1; i>=QMF_FILT_RESOLUTION; i-- ) {
    buff[i] = buff[i-QMF_FILT_RESOLUTION];
  }


  for ( i=QMF_FILT_RESOLUTION-1; i>=0; i-- ) {
    buff[i] = input[QMF_FILT_RESOLUTION-1-i];
  }


  for ( i=0; i<10*QMF_FILT_RESOLUTION; i++ ) {

        Z[i] = (FLOAT32) (buff[i] * qmf_filter_coeff[i]);

  }

  for ( i=0; i<2*QMF_FILT_RESOLUTION; i++ ) {
    Y[i] = 0.0f;
    for ( j=0; j<5; j++ ) {
      Y[i] += Z[i + j * 2 * QMF_FILT_RESOLUTION];
    }
  }

  for ( i=0; i<QMF_FILT_RESOLUTION; i++ ) {
    output_real[i] = 0.0f;
    output_imag[i] = 0.0f;
    for ( j=0; j<2*QMF_FILT_RESOLUTION; j++ ) {
      output_real[i] += (FLOAT32) (Y[j] * qmf_filt->ana_tab_real[i][j]);
      output_imag[i] += (FLOAT32) (Y[j] * qmf_filt->ana_tab_imag[i][j]);
    }
  }

}


static  WORD32 impd_down_mix ( ia_drc_sel_proc_output_struct *uni_drc_sel_proc_output, FLOAT32** input_audio, WORD32 frame_len)
{
    WORD32  num_base_ch   = uni_drc_sel_proc_output->base_channel_count;
    WORD32  num_target_ch = uni_drc_sel_proc_output->target_channel_count;
    WORD32  i, i_ch, o_ch;
    FLOAT32 tmp_out[MAX_CHANNEL_COUNT];


    if (num_target_ch > MAX_CHANNEL_COUNT)
        return -1;

    if (num_target_ch > num_base_ch)
        return -1;

    for (i=0; i<frame_len; i++) {
        for (o_ch=0; o_ch<num_target_ch; o_ch++) {
            tmp_out[o_ch] = 0.0f;
            for (i_ch=0; i_ch<num_base_ch; i_ch++) {
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





WORD32 impd_init_process_audio_main_td_qmf (ia_drc_api_struct *p_obj_drc)

{
    WORD32 error, i, j, num_samples_per_channel;
    FLOAT32 *input_buffer;
    WORD16 *input_buffer16, *output_buffer16;
    FLOAT32 *output_buffer;
    FLOAT32 *audio_io_buf_real[10];
    FLOAT32 *audio_io_buf_imag[10];
    FLOAT32 *audio_in_out_buf[10];
    FLOAT32 *scratch_buffer;
    WORD32 last_frame=0;
    error=0;
    scratch_buffer= (FLOAT32*)p_obj_drc->pp_mem[1];
    input_buffer  = (FLOAT32*)p_obj_drc->pp_mem[2];
    output_buffer = (FLOAT32*)p_obj_drc->pp_mem[3];

    input_buffer16  = (WORD16*)p_obj_drc->pp_mem[2];
    output_buffer16 = (WORD16*)p_obj_drc->pp_mem[3];


    if(p_obj_drc->p_state->ui_in_bytes<=0){
           p_obj_drc->p_state->ui_out_bytes=0;
        return 0;
    }

     if((p_obj_drc->p_state->ui_in_bytes/p_obj_drc->str_config.num_ch_in/(p_obj_drc->str_config.pcm_size>>3)) < (UWORD32)p_obj_drc->str_config.frame_size)
    last_frame=1;


    for(i=0;i<p_obj_drc->str_config.num_ch_in;i++){
    audio_in_out_buf[i]=scratch_buffer;
    scratch_buffer=scratch_buffer+(p_obj_drc->str_config.frame_size+32);
    audio_io_buf_real[i]=scratch_buffer+(p_obj_drc->str_config.frame_size*p_obj_drc->str_config.num_ch_in+512);
    audio_io_buf_imag[i]=scratch_buffer+2*(p_obj_drc->str_config.frame_size*p_obj_drc->str_config.num_ch_in+512);;
      for(j=0;j<p_obj_drc->str_config.frame_size;j++){
      if(p_obj_drc->str_config.pcm_size==16){
      audio_in_out_buf[i][j]=((FLOAT32)input_buffer16[j*p_obj_drc->str_config.num_ch_in + i])/32767.0f;
      }
      else {
      audio_in_out_buf[i][j]=input_buffer[j*p_obj_drc->str_config.num_ch_in + i];
      }
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
    if(p_obj_drc->str_bit_handler.gain_stream_flag==0)  //ITTIAM: Flag for applying gain frame by frame
    {
        p_obj_drc->str_bit_handler.num_bytes_bs      -= p_obj_drc->str_bit_handler.num_bytes_read_bs;
    }
    if (p_obj_drc->str_config.bitstream_file_format == BITSTREAM_FILE_FORMAT_SPLIT) {
        /* shift over fill-bits for frame byte alignment */
        if (p_obj_drc->str_bit_handler.num_bits_offset_bs != 0)
        {
            p_obj_drc->str_bit_handler.num_bits_read_bs   = p_obj_drc->str_bit_handler.num_bits_read_bs + 8 - p_obj_drc->str_bit_handler.num_bits_offset_bs;
            p_obj_drc->str_bit_handler.num_bytes_read_bs  = p_obj_drc->str_bit_handler.num_bytes_read_bs + 1;
            p_obj_drc->str_bit_handler.num_bits_offset_bs = 0;
            p_obj_drc->str_bit_handler.byte_index_bs   = p_obj_drc->str_bit_handler.byte_index_bs + 1;
            if(p_obj_drc->str_bit_handler.gain_stream_flag==0)  //ITTIAM: Flag for applying gain frame by frame
            {
                p_obj_drc->str_bit_handler.num_bytes_bs      = p_obj_drc->str_bit_handler.num_bytes_bs - 1;
            }
        }
    }

                for (i=0; i < p_obj_drc->str_config.num_ch_in; i++) {
                    for (j=0; j < p_obj_drc->str_config.frame_size; j += 64) {

                        process_qmf_ana_filt_bank(p_obj_drc->str_payload.pstr_qmf_filter,
                                                      p_obj_drc->str_payload.pstr_qmf_filter->ana_buff+i*4*p_obj_drc->str_config.frame_size,
                                                      &(audio_in_out_buf[i][j]),
                                                      &(audio_io_buf_real[i][j]),
                                                      &(audio_io_buf_imag[i][j]));


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


            if (error) return error;

            if(p_obj_drc->str_payload.pstr_drc_sel_proc_output->target_channel_count<p_obj_drc->str_payload.pstr_drc_sel_proc_output->base_channel_count){
            error = impd_down_mix(p_obj_drc->str_payload.pstr_drc_sel_proc_output,
                                 audio_io_buf_real,
                                  p_obj_drc->str_config.frame_size);
            if (error) return error;

            error = impd_down_mix(p_obj_drc->str_payload.pstr_drc_sel_proc_output,
                                 audio_io_buf_imag,
                                  p_obj_drc->str_config.frame_size);
            if (error) return error;
            }

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
                for (i=0; i < p_obj_drc->str_config.num_ch_out; i++) {
                    for (j=0; j < p_obj_drc->str_config.frame_size; j += 64) {

                        process_qmf_syn_filt_bank(p_obj_drc->str_payload.pstr_qmf_filter,
                                                      p_obj_drc->str_payload.pstr_qmf_filter->syn_buff+i*4*p_obj_drc->str_config.frame_size,
                                                      &(audio_io_buf_real[i][j]),
                                                      &(audio_io_buf_imag[i][j]),
                                                      &(audio_in_out_buf[i][j]));


                    }
                }

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
             if(p_obj_drc->str_config.pcm_size==16){
              output_buffer16[j*p_obj_drc->str_config.num_ch_out + i] = (WORD16)(audio_in_out_buf[i][j]*32767.0f);
             }
             else {
               output_buffer[j*p_obj_drc->str_config.num_ch_out + i] = audio_in_out_buf[i][j];

            }
        }
        }
    p_obj_drc->p_state->ui_out_bytes=p_obj_drc->str_config.num_ch_out*(p_obj_drc->p_state->ui_in_bytes/p_obj_drc->str_config.num_ch_in);

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

    p_obj_drc->str_bit_handler.num_bytes_read_bs   = (p_obj_drc->str_bit_handler.num_bits_read_bs >> 3);
    p_obj_drc->str_bit_handler.num_bits_offset_bs  = (p_obj_drc->str_bit_handler.num_bits_read_bs  & 7);
    p_obj_drc->str_bit_handler.byte_index_bs   += p_obj_drc->str_bit_handler.num_bytes_read_bs;
    p_obj_drc->str_bit_handler.num_bytes_bs      -= p_obj_drc->str_bit_handler.num_bytes_read_bs;

}


 return error;
}

