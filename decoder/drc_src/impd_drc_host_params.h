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
#ifndef IMPD_DRC_HOST_PARAMS_H
#define IMPD_DRC_HOST_PARAMS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

WORD32 impd_set_default_params_selection_process(ia_drc_sel_proc_params_struct* pstr_drc_sel_proc_params);

WORD32 impd_set_custom_params(const WORD32 param_set_idx,
                           ia_drc_sel_proc_params_struct* pstr_drc_sel_proc_params);

WORD32 impd_eval_custom_params_selection_process(ia_drc_sel_proc_params_struct* pstr_drc_sel_proc_params);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
