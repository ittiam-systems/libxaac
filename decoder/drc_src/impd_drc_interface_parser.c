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
#include <string.h>
#include <math.h>

#include "impd_type_def.h"
#include "impd_drc_bitbuffer.h"
#include "impd_drc_common.h"
#include "impd_drc_interface.h"
#include "impd_drc_extr_delta_coded_info.h"
#include "impd_drc_struct.h"

WORD32 impd_unidrc_interface_signature_read(ia_bit_buf_struct* it_bit_buff,
                              ia_drc_uni_interface_signat_struct* drc_uni_interface_signature)
{
    //WORD32 err = 0
	WORD32 interface_signat_data_len = 0, i, tmp;

	tmp = impd_read_bits_buf(it_bit_buff, 16);
	if(it_bit_buff->error)
			return it_bit_buff->error;

	drc_uni_interface_signature->interface_signat_type = (tmp>>8)&0xff;
	drc_uni_interface_signature->interface_signat_data_len = tmp&0xff;
    
    interface_signat_data_len = drc_uni_interface_signature->interface_signat_data_len + 1;
    for (i=0; i<interface_signat_data_len; i++) {
		tmp = impd_read_bits_buf(it_bit_buff, 8);
		if(it_bit_buff->error)
			return it_bit_buff->error;
        drc_uni_interface_signature->interface_signat_data[i] = (UWORD32)tmp;
    }
    
    return(0);
}
WORD32 impd_sys_interface_read(ia_bit_buf_struct* it_bit_buff,
                     ia_system_interface_struct* system_interface)
{
    //WORD32 err = 0; 
	WORD32 i = 0, tmp = 0;
    
	system_interface->target_config_request_type = impd_read_bits_buf(it_bit_buff, 2);
	if(it_bit_buff->error)
			return it_bit_buff->error;
    
    switch (system_interface->target_config_request_type) {
        case 0:
			system_interface->num_downmix_id_requests = impd_read_bits_buf(it_bit_buff, 4);
			if(it_bit_buff->error)
			return it_bit_buff->error;
            
            if (system_interface->num_downmix_id_requests == 0) {
                system_interface->num_downmix_id_requests = 1;
                system_interface->requested_dwnmix_id[0] = 0;
                break;
            }
            for (i=0; i<system_interface->num_downmix_id_requests; i++) {
				system_interface->requested_dwnmix_id[i] = impd_read_bits_buf(it_bit_buff, 7);
				if(it_bit_buff->error)
					return it_bit_buff->error;
            }
            break;
        case 1:
			system_interface->requested_target_layout = impd_read_bits_buf(it_bit_buff, 8);
			if(it_bit_buff->error)
				return it_bit_buff->error;
            break;
        case 2:
			tmp = impd_read_bits_buf(it_bit_buff, 7);
			if(it_bit_buff->error)
				return it_bit_buff->error;
            system_interface->requested_target_ch_count = tmp + 1;
            break;
        default:
            return(1);
            break;
    }
    return(0);
}

WORD32 impd_loudness_norm_control_interface_read(ia_bit_buf_struct* it_bit_buff,
                                           ia_loudness_norm_ctrl_interface_struct* loudness_norm_ctrl_interface)
{
    //WORD32 err = 0;
	WORD32 tmp = 0;
    
	loudness_norm_ctrl_interface->loudness_normalization_on = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    
    if (loudness_norm_ctrl_interface->loudness_normalization_on == 1) {
		tmp = impd_read_bits_buf(it_bit_buff, 12);
		if(it_bit_buff->error)
			return it_bit_buff->error;
        loudness_norm_ctrl_interface->target_loudness = - tmp * 0.03125f;
    }
    return(0);
}

WORD32 impd_loudness_norm_param_interface_read(ia_bit_buf_struct* it_bit_buff,
                                             ia_loudness_norm_parameter_interface_struct* loudness_norm_param_interface)
{
    //WORD32 err = 0;
	WORD32 tmp = 0;
    
	tmp = impd_read_bits_buf(it_bit_buff, 3);
	if(it_bit_buff->error)
			return it_bit_buff->error;
	
	loudness_norm_param_interface->album_mode = (tmp>>2)&1;
	loudness_norm_param_interface->peak_limiter = (tmp>>1)&1;
	loudness_norm_param_interface->change_loudness_deviation_max = tmp&1;

    if (loudness_norm_param_interface->change_loudness_deviation_max == 1) {
		loudness_norm_param_interface->loudness_deviation_max = impd_read_bits_buf(it_bit_buff, 6);
		if(it_bit_buff->error)
			return it_bit_buff->error;
    }
    
	loudness_norm_param_interface->change_loudness_measur_method = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (loudness_norm_param_interface->change_loudness_measur_method == 1) {
		loudness_norm_param_interface->loudness_measurement_method = impd_read_bits_buf(it_bit_buff, 3);
		if(it_bit_buff->error)
			return it_bit_buff->error;
    }
    
	loudness_norm_param_interface->change_loudness_measur_system = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (loudness_norm_param_interface->change_loudness_measur_system == 1) {
		loudness_norm_param_interface->loudness_measurement_system = impd_read_bits_buf(it_bit_buff, 4);
		if(it_bit_buff->error)
			return it_bit_buff->error;
    }
    
	loudness_norm_param_interface->change_loudness_measur_pre_proc = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (loudness_norm_param_interface->change_loudness_measur_pre_proc == 1) {
		loudness_norm_param_interface->loudness_measurement_pre_proc = impd_read_bits_buf(it_bit_buff, 2);
		if(it_bit_buff->error)
			return it_bit_buff->error;
    }
    
	loudness_norm_param_interface->change_device_cut_off_freq = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (loudness_norm_param_interface->change_device_cut_off_freq == 1) {
		tmp = impd_read_bits_buf(it_bit_buff, 6);
		if(it_bit_buff->error)
			return it_bit_buff->error;
        loudness_norm_param_interface->device_cut_off_frequency = max(min(tmp*10,500),20);
    }
    
	loudness_norm_param_interface->change_loudness_norm_gain_db_max = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (loudness_norm_param_interface->change_loudness_norm_gain_db_max == 1) {
		tmp = impd_read_bits_buf(it_bit_buff, 6);
		if(it_bit_buff->error)
			return it_bit_buff->error;
        if (tmp<63) {
            loudness_norm_param_interface->loudness_norm_gain_db_max = tmp*0.5f;
        } else {
            loudness_norm_param_interface->loudness_norm_gain_db_max = LOUDNESS_NORMALIZATION_GAIN_MAX_DEFAULT;
        }
    }
    
	loudness_norm_param_interface->change_loudness_norm_gain_modification_db = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (loudness_norm_param_interface->change_loudness_norm_gain_modification_db == 1) {
		tmp = impd_read_bits_buf(it_bit_buff, 10);
		if(it_bit_buff->error)
			return it_bit_buff->error;
        loudness_norm_param_interface->loudness_norm_gain_modification_db = -16+tmp*0.03125f;
    }
    
	loudness_norm_param_interface->change_output_peak_level_max = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (loudness_norm_param_interface->change_output_peak_level_max == 1) {
		tmp = impd_read_bits_buf(it_bit_buff, 6);
		if(it_bit_buff->error)
			return it_bit_buff->error;
        loudness_norm_param_interface->output_peak_level_max = tmp*0.5f;
    }
    
    return(0);
}

WORD32 impd_drc_interface_read(ia_bit_buf_struct* it_bit_buff,
                                  ia_dyn_rng_ctrl_interface_struct* drc_ctrl_interface)
{
    //WORD32 err = 0;
	WORD32 i = 0, j = 0, tmp = 0;
    
	drc_ctrl_interface->dynamic_range_control_on = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    
    if (drc_ctrl_interface->dynamic_range_control_on == 1) {
		drc_ctrl_interface->num_drc_feature_requests = impd_read_bits_buf(it_bit_buff, 3);
		if(it_bit_buff->error)
			return it_bit_buff->error;

        for (i=0; i<drc_ctrl_interface->num_drc_feature_requests; i++) {
			drc_ctrl_interface->drc_feature_req_type[i] = impd_read_bits_buf(it_bit_buff, 2);
			if(it_bit_buff->error)
				return it_bit_buff->error;
            
            switch (drc_ctrl_interface->drc_feature_req_type[i]) {
                case 0:
					tmp = impd_read_bits_buf(it_bit_buff, 8);
					if(it_bit_buff->error)
						return it_bit_buff->error;

					drc_ctrl_interface->requested_num_drc_effects[i] = (tmp>>4)&0xf;
					drc_ctrl_interface->desired_num_drc_effects_of_requested[i] = tmp&0xf;

                    for (j=0; j<drc_ctrl_interface->requested_num_drc_effects[i]; j++) 
					{
						drc_ctrl_interface->requested_drc_effect_type[i][j] = impd_read_bits_buf(it_bit_buff, 4);
						if(it_bit_buff->error)
							return it_bit_buff->error;
                    }
                    break;
                case 1:
					tmp = impd_read_bits_buf(it_bit_buff, 3);
					if(it_bit_buff->error)
						return it_bit_buff->error;

					drc_ctrl_interface->requested_dyn_rng_measurement_type[i] = (tmp>>1)&3;
					drc_ctrl_interface->requested_dyn_range_is_single_val_flag[i] = tmp&1;

                    if (drc_ctrl_interface->requested_dyn_range_is_single_val_flag[i] == 0) {
						tmp = impd_read_bits_buf(it_bit_buff, 8);
						if(it_bit_buff->error)
							return it_bit_buff->error;
                        if (tmp == 0)
                            drc_ctrl_interface->requested_dyn_range_value[i] = 0.0f;
                        else if(tmp <= 128)
                            drc_ctrl_interface->requested_dyn_range_value[i] = tmp * 0.25f;
                        else if(tmp <= 204)
                            drc_ctrl_interface->requested_dyn_range_value[i] = 0.5f * tmp - 32.0f;
                        else
                            drc_ctrl_interface->requested_dyn_range_value[i] = tmp - 134.0f;
                    } else {
						tmp = impd_read_bits_buf(it_bit_buff, 8);
						if(it_bit_buff->error)
							return it_bit_buff->error;
                        if (tmp == 0)
                            drc_ctrl_interface->requested_dyn_range_min_val[i] = 0.0f;
                        else if(tmp <= 128)
                            drc_ctrl_interface->requested_dyn_range_min_val[i] = tmp * 0.25f;
                        else if(tmp <= 204)
                            drc_ctrl_interface->requested_dyn_range_min_val[i] = 0.5f * tmp - 32.0f;
                        else
                            drc_ctrl_interface->requested_dyn_range_min_val[i] = tmp - 134.0f;
						tmp = impd_read_bits_buf(it_bit_buff, 8);
						if(it_bit_buff->error)
							return it_bit_buff->error;	
                        if (tmp == 0)
                            drc_ctrl_interface->requested_dyn_range_max_val[i] = 0.0f;
                        else if(tmp <= 128)
                            drc_ctrl_interface->requested_dyn_range_max_val[i] = tmp * 0.25f;
                        else if(tmp <= 204)
                            drc_ctrl_interface->requested_dyn_range_max_val[i] = 0.5f * tmp - 32.0f;
                        else
                            drc_ctrl_interface->requested_dyn_range_max_val[i] = tmp - 134.0f;
                    }
                    break;
                case 2:
					drc_ctrl_interface->requested_drc_characteristic[i] = impd_read_bits_buf(it_bit_buff, 7);
					if(it_bit_buff->error)
						return it_bit_buff->error;
                    break;
                default:
                    return(1);
                    break;
            }
        }
    }
    return(0);
}

WORD32 impd_drc_param_interface_read(ia_bit_buf_struct* it_bit_buff,
                                           ia_drc_parameter_interface_struct* drc_parameter_interface)
{
    //WORD32 err = 0;
	WORD32 tmp = 0;

	tmp = impd_read_bits_buf(it_bit_buff, 2);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    
	drc_parameter_interface->change_compress = (tmp>>1)&1;
	drc_parameter_interface->change_boost = tmp&1;
    
    if (drc_parameter_interface->change_compress == 1) {
		tmp = impd_read_bits_buf(it_bit_buff, 8);
		if(it_bit_buff->error)
			return it_bit_buff->error;
        if (tmp<255) {
            drc_parameter_interface->compress = 1 - tmp * 0.00390625f;
        } else {
            drc_parameter_interface->compress = 0.f;
        }
    }
    
    if (drc_parameter_interface->change_boost == 1) {
		tmp = impd_read_bits_buf(it_bit_buff, 8);
		if(it_bit_buff->error)
			return it_bit_buff->error;
        if (tmp<255) {
            drc_parameter_interface->boost = 1 - tmp * 0.00390625f;
        } else {
            drc_parameter_interface->boost = 0.f;
        }
    }
    
	drc_parameter_interface->change_drc_characteristic_target = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    
    if (drc_parameter_interface->change_drc_characteristic_target == 1) {
		drc_parameter_interface->drc_characteristic_target = impd_read_bits_buf(it_bit_buff, 8);
		if(it_bit_buff->error)
			return it_bit_buff->error;
    }
    return(0);
}

static WORD32 impd_parse_loud_eq_param_interface(ia_bit_buf_struct* it_bit_buff,
                                  ia_loudness_eq_parameter_interface_struct* loudness_eq_parameter_interface)
{
    //WORD32 err = 0;
    WORD32 bsSensitivity, bsPlaybackGain;
    
	loudness_eq_parameter_interface->loudness_eq_request_flag = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (loudness_eq_parameter_interface->loudness_eq_request_flag) {
		loudness_eq_parameter_interface->loudness_eq_request = impd_read_bits_buf(it_bit_buff, 2);
		if(it_bit_buff->error)
			return it_bit_buff->error;
    }
	loudness_eq_parameter_interface->sensitivity_flag = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (loudness_eq_parameter_interface->sensitivity_flag) {
		bsSensitivity = impd_read_bits_buf(it_bit_buff, 7);
		if(it_bit_buff->error)
			return it_bit_buff->error;
        loudness_eq_parameter_interface->sensitivity = bsSensitivity + 23.0f;
    }
	loudness_eq_parameter_interface->playback_gain_flag = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (loudness_eq_parameter_interface->playback_gain_flag) {
		bsPlaybackGain = impd_read_bits_buf(it_bit_buff, 7);
		if(it_bit_buff->error)
			return it_bit_buff->error;
        loudness_eq_parameter_interface->playback_gain = (FLOAT32) - bsPlaybackGain;
    }
    return (0);
}

WORD32
impd_unidrc_interface_extension_read(ia_bit_buf_struct* it_bit_buff,
                              ia_drc_interface_struct* impd_drc_uni_interface,
                              ia_drc_uni_interface_ext_struct* drc_uni_interface_ext)
{
    WORD32 err = 0, i = 0, tmp = 0, dummy;
    WORD32 uni_drc_interface_ext_type;
    ia_specific_interface_extension_struct* specific_interface_ext;
    
	uni_drc_interface_ext_type = impd_read_bits_buf(it_bit_buff, 4);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    
    while (uni_drc_interface_ext_type != UNIDRCINTERFACEEXT_TERM) {
        specific_interface_ext = &(drc_uni_interface_ext->specific_interface_ext[i]);
        specific_interface_ext->uni_drc_interface_ext_type = uni_drc_interface_ext_type;
		tmp = impd_read_bits_buf(it_bit_buff, 4);
		if(it_bit_buff->error)
			return it_bit_buff->error;
        specific_interface_ext->ext_size_bits = tmp+4;
        
		tmp = impd_read_bits_buf(it_bit_buff, specific_interface_ext->ext_size_bits);
		if(it_bit_buff->error)
			return it_bit_buff->error;

        specific_interface_ext->ext_bit_size  = tmp+1;
        
        switch (uni_drc_interface_ext_type) {
            case UNIDRCINTERFACEEXT_EQ:
				impd_drc_uni_interface->drc_uni_interface_ext.loudness_eq_parameter_interface_flag = impd_read_bits_buf(it_bit_buff, 1);
				if(it_bit_buff->error)
					return it_bit_buff->error;
                if (impd_drc_uni_interface->drc_uni_interface_ext.loudness_eq_parameter_interface_flag) {
                    err = impd_parse_loud_eq_param_interface(it_bit_buff, &(impd_drc_uni_interface->drc_uni_interface_ext.loudness_eq_parameter_interface));
                    if (err) return(err);
                }
				impd_drc_uni_interface->drc_uni_interface_ext.eq_ctrl_interface_flag = impd_read_bits_buf(it_bit_buff, 1);
				if(it_bit_buff->error)
					return it_bit_buff->error;
                if (impd_drc_uni_interface->drc_uni_interface_ext.eq_ctrl_interface_flag) {
					impd_drc_uni_interface->drc_uni_interface_ext.eq_ctrl_interface.eq_set_purpose_request = impd_read_bits_buf(it_bit_buff, 16);
					if(it_bit_buff->error)
						return it_bit_buff->error;
                }
                break;
            default:
				dummy = impd_read_bits_buf(it_bit_buff, specific_interface_ext->ext_bit_size);
				if(it_bit_buff->error)
					return it_bit_buff->error;
                break;
        }
        
        i++;
        if (i==EXT_COUNT_MAX) 
		{
            return(1);
        }
		uni_drc_interface_ext_type = impd_read_bits_buf(it_bit_buff, 4);
		if(it_bit_buff->error)
			return it_bit_buff->error;
    }
    drc_uni_interface_ext->interface_ext_count = i;
    return(0);
}
WORD32 impd_unidrc_interface_read(ia_bit_buf_struct* it_bit_buff,
                     ia_drc_interface_struct* uniDrcInterface)
{
    WORD32 err = 0;
    
	uniDrcInterface->interface_signat_flag = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (uniDrcInterface->interface_signat_flag == 1) {
        err = impd_unidrc_interface_signature_read(it_bit_buff, &(uniDrcInterface->drc_uni_interface_signature));
        if (err) return(err);
    }
    
	uniDrcInterface->system_interface_flag = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (uniDrcInterface->system_interface_flag == 1) {
        err = impd_sys_interface_read(it_bit_buff, &(uniDrcInterface->system_interface));
        if (err) return(err);
    }
    
	uniDrcInterface->loudness_norm_ctrl_interface_flag = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (uniDrcInterface->loudness_norm_ctrl_interface_flag == 1) {
        err = impd_loudness_norm_control_interface_read(it_bit_buff, &(uniDrcInterface->loudness_norm_ctrl_interface));
        if (err) return(err);
    }
    
	uniDrcInterface->loudness_norm_parameter_interface_flag = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (uniDrcInterface->loudness_norm_parameter_interface_flag == 1) {
        err = impd_loudness_norm_param_interface_read(it_bit_buff, &(uniDrcInterface->loudness_norm_param_interface));
        if (err) return(err);
    }
    
	uniDrcInterface->drc_interface_flag = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (uniDrcInterface->drc_interface_flag == 1) {
        err = impd_drc_interface_read(it_bit_buff, &(uniDrcInterface->drc_ctrl_interface));
        if (err) return(err);
    }
    
	uniDrcInterface->drc_parameter_interface_flag = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (uniDrcInterface->drc_parameter_interface_flag == 1) {
        err = impd_drc_param_interface_read(it_bit_buff, &(uniDrcInterface->drc_parameter_interface));
        if (err) return(err);
    }
    
	uniDrcInterface->drc_uni_interface_ext_flag = impd_read_bits_buf(it_bit_buff, 1);
	if(it_bit_buff->error)
		return it_bit_buff->error;
    if (uniDrcInterface->drc_uni_interface_ext_flag == 1) {
        err = impd_unidrc_interface_extension_read(it_bit_buff, uniDrcInterface, &(uniDrcInterface->drc_uni_interface_ext));
        if (err) return(err);
    }
    
    return(0);
}