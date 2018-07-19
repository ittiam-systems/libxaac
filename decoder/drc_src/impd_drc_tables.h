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
#ifndef IMPD_DRC_TABLES_H
#define IMPD_DRC_TABLES_H

const ia_filter_bank_params_struct
    normal_cross_freq[FILTER_BANK_PARAMETER_COUNT] = {
        {2.0f / 1024.0f, 0.0000373252f, 0.9913600345f},
        {3.0f / 1024.0f, 0.0000836207f, 0.9870680830f},
        {4.0f / 1024.0f, 0.0001480220f, 0.9827947083f},
        {5.0f / 1024.0f, 0.0002302960f, 0.9785398263f},
        {6.0f / 1024.0f, 0.0003302134f, 0.9743033527f},
        {2.0f / 256.0f, 0.0005820761f, 0.9658852897f},
        {3.0f / 256.0f, 0.0012877837f, 0.9492662926f},
        {2.0f / 128.0f, 0.0022515827f, 0.9329321561f},
        {3.0f / 128.0f, 0.0049030350f, 0.9010958535f},
        {2.0f / 64.0f, 0.0084426929f, 0.8703307793f},
        {3.0f / 64.0f, 0.0178631928f, 0.8118317459f},
        {2.0f / 32.0f, 0.0299545822f, 0.7570763753f},
        {3.0f / 32.0f, 0.0604985076f, 0.6574551915f},
        {2.0f / 16.0f, 0.0976310729f, 0.5690355937f},
        {3.0f / 16.0f, 0.1866943331f, 0.4181633458f},
        {2.0f / 8.0f, 0.2928932188f, 0.2928932188f},
};

#endif
