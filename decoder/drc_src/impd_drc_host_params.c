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
#include "impd_type_def.h"
#include "impd_drc_extr_delta_coded_info.h"
#include "impd_drc_common.h"
#include "impd_drc_struct.h"
#include "impd_drc_interface.h"
#include "impd_drc_selection_process.h"
#include "impd_drc_filter_bank.h"
#include "impd_drc_rom.h"

 
WORD32 impd_set_default_params_selection_process(ia_drc_sel_proc_params_struct *pstr_drc_sel_proc_params)
{
    pstr_drc_sel_proc_params->base_channel_count = -1;
    pstr_drc_sel_proc_params->base_layout = -1;
    pstr_drc_sel_proc_params->target_config_request_type = 0;
    pstr_drc_sel_proc_params->num_downmix_id_requests = 0;
    
    pstr_drc_sel_proc_params->album_mode = 0;

    pstr_drc_sel_proc_params->peak_limiter = 1;

    pstr_drc_sel_proc_params->loudness_normalization_on = 0;
    pstr_drc_sel_proc_params->target_loudness = -24.0f;

    pstr_drc_sel_proc_params->loudness_deviation_max = LOUDNESS_DEVIATION_MAX_DEFAULT;

    pstr_drc_sel_proc_params->loudness_measurement_method = USER_METHOD_DEFINITION_DEFAULT;
    pstr_drc_sel_proc_params->loudness_measurement_system = USER_MEASUREMENT_SYSTEM_DEFAULT;
    pstr_drc_sel_proc_params->loudness_measurement_pre_proc = USER_LOUDNESS_PREPROCESSING_DEFAULT;
    pstr_drc_sel_proc_params->device_cut_off_frequency = 500;
    pstr_drc_sel_proc_params->loudness_norm_gain_db_max = LOUDNESS_NORMALIZATION_GAIN_MAX_DEFAULT; 
    pstr_drc_sel_proc_params->loudness_norm_gain_modification_db = 0.0f;
    pstr_drc_sel_proc_params->output_peak_level_max = 0.0f;
    if (pstr_drc_sel_proc_params->peak_limiter == 1) {

        pstr_drc_sel_proc_params->output_peak_level_max = 6.0f;

    }
    
    pstr_drc_sel_proc_params->dynamic_range_control_on = 1;
    pstr_drc_sel_proc_params->num_bands_supported = 4; 
    pstr_drc_sel_proc_params->num_drc_feature_requests = 0;
    

    pstr_drc_sel_proc_params->boost = 1.f;
    pstr_drc_sel_proc_params->compress = 1.f;
    pstr_drc_sel_proc_params->drc_characteristic_target = 0;
    
    return 0;
}
WORD32 impd_set_custom_params(const WORD32 param_set_idx,
                                 ia_drc_sel_proc_params_struct* pstr_drc_sel_proc_params)
{
    WORD32 i, k;
    
    const ia_loc_sys_interface_struct* system_interface = &(loc_sys_interface[param_set_idx-1]);
    
    const ia_loc_loudness_norm_ctrl_interface_struct* loudness_norm_ctrl_interface = &(loc_loudness_norm_ctrl_interface[param_set_idx-1]);
    const ia_loc_loudness_norm_param_interface_struct* loudness_norm_param_interface = &(loc_loudness_norm_param_interface[param_set_idx-1]);
    
    const ia_loc_drc_interface_struct* drc_ctrl_interface = &(loc_dyn_range_ctrl_interface[param_set_idx-1]);
    const ia_loc_requested_drc_effect_struct* requested_drc_effect_type = &(loc_requested_drc_effect_type_str[param_set_idx-1]);
    const ia_loc_drc_parameter_interface_struct* drc_parameter_interface = &(loc_drc_parameter_interface[param_set_idx-1]);

    pstr_drc_sel_proc_params->target_config_request_type = system_interface->target_config_request_type;
    switch (system_interface->target_config_request_type) {
        case 1:
            pstr_drc_sel_proc_params->requested_target_layout = system_interface->requested_target_layout;
            break;
        case 2:
            pstr_drc_sel_proc_params->requested_target_ch_count = system_interface->requested_target_ch_count;
            break;
        case 0:
        default:
            pstr_drc_sel_proc_params->num_downmix_id_requests = system_interface->num_downmix_id_requests;
            for (i=0; i<system_interface->num_downmix_id_requests; i++) {
                pstr_drc_sel_proc_params->requested_dwnmix_id[i] = system_interface->requested_dwnmix_id[i];
            }
            break;
    }
    
    pstr_drc_sel_proc_params->loudness_normalization_on = loudness_norm_ctrl_interface->loudness_normalization_on;
    pstr_drc_sel_proc_params->target_loudness = loudness_norm_ctrl_interface->target_loudness;
    
    pstr_drc_sel_proc_params->album_mode = loudness_norm_param_interface->album_mode;
    pstr_drc_sel_proc_params->peak_limiter = loudness_norm_param_interface->peak_limiter;
    pstr_drc_sel_proc_params->loudness_deviation_max = loudness_norm_param_interface->loudness_deviation_max;
    pstr_drc_sel_proc_params->loudness_measurement_method = loudness_norm_param_interface->loudness_measurement_method;
    pstr_drc_sel_proc_params->loudness_measurement_system = loudness_norm_param_interface->loudness_measurement_system;
    pstr_drc_sel_proc_params->loudness_measurement_pre_proc = loudness_norm_param_interface->loudness_measurement_pre_proc;
    pstr_drc_sel_proc_params->device_cut_off_frequency= loudness_norm_param_interface->device_cut_off_frequency;
    pstr_drc_sel_proc_params->loudness_norm_gain_db_max = loudness_norm_param_interface->loudness_norm_gain_db_max;
    pstr_drc_sel_proc_params->loudness_norm_gain_modification_db = loudness_norm_param_interface->loudness_norm_gain_modification_db;
    pstr_drc_sel_proc_params->output_peak_level_max = loudness_norm_param_interface->output_peak_level_max;

    pstr_drc_sel_proc_params->dynamic_range_control_on = drc_ctrl_interface->dynamic_range_control_on;
    pstr_drc_sel_proc_params->num_drc_feature_requests = drc_ctrl_interface->num_drc_feature_requests;
    for (i=0; i<drc_ctrl_interface->num_drc_feature_requests; i++)
    {
        pstr_drc_sel_proc_params->drc_feature_req_type[i] = drc_ctrl_interface->drc_feature_req_type[i];
        switch (drc_ctrl_interface->drc_feature_req_type[i])
        {
            case MATCH_EFFECT_TYPE:
                pstr_drc_sel_proc_params->requested_num_drc_effects[i] = requested_drc_effect_type->requested_num_drc_effects;
                pstr_drc_sel_proc_params->desired_num_drc_effects_of_requested[i] = requested_drc_effect_type->desired_num_drc_effects_of_requested;
                for (k=0; k<requested_drc_effect_type->requested_num_drc_effects; k++)
                {
                    pstr_drc_sel_proc_params->requested_drc_effect_type[i][k] = requested_drc_effect_type->requested_drc_effect_type[k];
                }
                break;
            case MATCH_DYNAMIC_RANGE:
                pstr_drc_sel_proc_params->requested_dyn_range_measur_type[i] = drc_ctrl_interface->requested_dyn_rng_measurement_type;
                pstr_drc_sel_proc_params->requested_dyn_range_range_flag[i] = drc_ctrl_interface->requested_dyn_range_is_single_val_flag;
                pstr_drc_sel_proc_params->requested_dyn_range_value[i] = drc_ctrl_interface->requested_dyn_range_value;
                pstr_drc_sel_proc_params->requested_dyn_range_min_val[i] = drc_ctrl_interface->requested_dyn_range_min_val;
                pstr_drc_sel_proc_params->requested_dyn_range_max_val[i] = drc_ctrl_interface->requested_dyn_range_max_val;
                break;
            case MATCH_DRC_CHARACTERISTIC:
                pstr_drc_sel_proc_params->requested_drc_characteristic[i] = drc_ctrl_interface->requested_drc_characteristic;
                break;
            default:
                return (UNEXPECTED_ERROR);
        }
    }
    
    pstr_drc_sel_proc_params->boost    = drc_parameter_interface->boost; 
    pstr_drc_sel_proc_params->compress = drc_parameter_interface->compress; 
    pstr_drc_sel_proc_params->drc_characteristic_target = drc_parameter_interface->drc_characteristic_target;
    
    return (0);
}
WORD32 impd_eval_custom_params_selection_process(ia_drc_sel_proc_params_struct* pstr_drc_sel_proc_params)
{
    pstr_drc_sel_proc_params->loudness_norm_gain_db_max = max (0.0f, pstr_drc_sel_proc_params->loudness_norm_gain_db_max);
    pstr_drc_sel_proc_params->loudness_deviation_max = max (0, pstr_drc_sel_proc_params->loudness_deviation_max);
    
    return (0);
}
    

