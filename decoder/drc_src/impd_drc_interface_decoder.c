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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "impd_type_def.h"

#include "impd_drc_bitbuffer.h"
#include "impd_drc_common.h"
#include "impd_drc_interface.h"
#include "impd_drc_parser_interface.h"

WORD32
impd_drc_dec_interface_process(
							  ia_bit_buf_struct* it_bit_buff,
                              ia_drc_interface_struct* pstr_drc_interface,
                              UWORD8* it_bit_buf,
                              WORD32 num_bit_stream_bits,
                              WORD32* num_bits_read)
{
    WORD32 err = 0;
    
    if (it_bit_buff != NULL && num_bit_stream_bits) {
		it_bit_buff = impd_create_init_bit_buf(it_bit_buff, it_bit_buf, num_bit_stream_bits/8);

    } else {
        return -1;
    }
    
    err = impd_unidrc_interface_read(it_bit_buff, pstr_drc_interface);
    if (err) return(err);
    
    *num_bits_read = (it_bit_buff->size ) - it_bit_buff->cnt_bits;
    
    return err;
    
}

