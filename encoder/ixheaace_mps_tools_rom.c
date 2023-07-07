/******************************************************************************
 *                                                                            *
 * Copyright (C) 2023 The Android Open Source Project
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

#include <stddef.h>
#include "ixheaac_type_def.h"
#include "ixheaace_error_codes.h"
#include "ixheaace_api.h"
#include "ixheaace_mps_common_fix.h"
#include "ixheaace_mps_defines.h"
#include "ixheaace_mps_common_define.h"
#include "ixheaace_aac_constants.h"

#include "ixheaace_mps_buf.h"
#include "ixheaace_mps_lib.h"
#include "ixheaace_mps_main_structure.h"

const ixheaace_cmplx_str sine_table_1024[] = {
    {0.999969f, 0.000000f}, {0.999969f, 0.001526f}, {0.999969f, 0.003082f},
    {0.999969f, 0.004608f}, {0.999969f, 0.006134f}, {0.999969f, 0.007660f},
    {0.999969f, 0.009216f}, {0.999939f, 0.010742f}, {0.999939f, 0.012268f},
    {0.999908f, 0.013794f}, {0.999878f, 0.015350f}, {0.999847f, 0.016876f},
    {0.999817f, 0.018402f}, {0.999786f, 0.019928f}, {0.999756f, 0.021484f},
    {0.999725f, 0.023010f}, {0.999695f, 0.024536f}, {0.999664f, 0.026062f},
    {0.999634f, 0.027618f}, {0.999573f, 0.029144f}, {0.999542f, 0.030670f},
    {0.999481f, 0.032196f}, {0.999420f, 0.033752f}, {0.999390f, 0.035278f},
    {0.999329f, 0.036804f}, {0.999268f, 0.038330f}, {0.999207f, 0.039886f},
    {0.999146f, 0.041412f}, {0.999084f, 0.042938f}, {0.999023f, 0.044464f},
    {0.998932f, 0.045990f}, {0.998871f, 0.047546f}, {0.998810f, 0.049072f},
    {0.998718f, 0.050598f}, {0.998627f, 0.052124f}, {0.998566f, 0.053650f},
    {0.998474f, 0.055206f}, {0.998383f, 0.056732f}, {0.998291f, 0.058258f},
    {0.998199f, 0.059784f}, {0.998108f, 0.061310f}, {0.998016f, 0.062866f},
    {0.997925f, 0.064392f}, {0.997833f, 0.065918f}, {0.997711f, 0.067444f},
    {0.997620f, 0.068970f}, {0.997498f, 0.070496f}, {0.997406f, 0.072021f},
    {0.997284f, 0.073578f}, {0.997162f, 0.075104f}, {0.997070f, 0.076630f},
    {0.996948f, 0.078156f}, {0.996826f, 0.079681f}, {0.996704f, 0.081207f},
    {0.996582f, 0.082733f}, {0.996429f, 0.084259f}, {0.996307f, 0.085785f},
    {0.996185f, 0.087311f}, {0.996033f, 0.088867f}, {0.995911f, 0.090393f},
    {0.995758f, 0.091919f}, {0.995636f, 0.093445f}, {0.995483f, 0.094971f},
    {0.995331f, 0.096497f}, {0.995178f, 0.098022f}, {0.995026f, 0.099548f},
    {0.994873f, 0.101074f}, {0.994720f, 0.102600f}, {0.994568f, 0.104126f},
    {0.994415f, 0.105652f}, {0.994232f, 0.107178f}, {0.994080f, 0.108704f},
    {0.993896f, 0.110229f}, {0.993744f, 0.111755f}, {0.993561f, 0.113281f},
    {0.993378f, 0.114807f}, {0.993225f, 0.116333f}, {0.993042f, 0.117828f},
    {0.992859f, 0.119354f}, {0.992676f, 0.120880f}, {0.992493f, 0.122406f},
    {0.992279f, 0.123932f}, {0.992096f, 0.125458f}, {0.991913f, 0.126984f},
    {0.991699f, 0.128510f}, {0.991516f, 0.130005f}, {0.991302f, 0.131531f},
    {0.991119f, 0.133057f}, {0.990906f, 0.134583f}, {0.990692f, 0.136108f},
    {0.990479f, 0.137634f}, {0.990265f, 0.139130f}, {0.990051f, 0.140656f},
    {0.989838f, 0.142181f}, {0.989624f, 0.143707f}, {0.989410f, 0.145203f},
    {0.989166f, 0.146729f}, {0.988953f, 0.148254f}, {0.988708f, 0.149750f},
    {0.988495f, 0.151276f}, {0.988251f, 0.152802f}, {0.988037f, 0.154327f},
    {0.987793f, 0.155823f}, {0.987549f, 0.157349f}, {0.987305f, 0.158844f},
    {0.987061f, 0.160370f}, {0.986816f, 0.161896f}, {0.986572f, 0.163391f},
    {0.986298f, 0.164917f}, {0.986053f, 0.166412f}, {0.985809f, 0.167938f},
    {0.985535f, 0.169464f}, {0.985291f, 0.170959f}, {0.985016f, 0.172485f},
    {0.984741f, 0.173981f}, {0.984467f, 0.175507f}, {0.984222f, 0.177002f},
    {0.983948f, 0.178528f}, {0.983673f, 0.180023f}, {0.983398f, 0.181519f},
    {0.983093f, 0.183044f}, {0.982819f, 0.184540f}, {0.982544f, 0.186066f},
    {0.982239f, 0.187561f}, {0.981964f, 0.189056f}, {0.981659f, 0.190582f},
    {0.981384f, 0.192078f}, {0.981079f, 0.193573f}, {0.980774f, 0.195099f},
    {0.980499f, 0.196594f}, {0.980194f, 0.198090f}, {0.979889f, 0.199615f},
    {0.979584f, 0.201111f}, {0.979248f, 0.202606f}, {0.978943f, 0.204102f},
    {0.978638f, 0.205597f}, {0.978333f, 0.207123f}, {0.977997f, 0.208618f},
    {0.977692f, 0.210114f}, {0.977356f, 0.211609f}, {0.977020f, 0.213104f},
    {0.976715f, 0.214600f}, {0.976379f, 0.216095f}, {0.976044f, 0.217590f},
    {0.975708f, 0.219116f}, {0.975372f, 0.220612f}, {0.975037f, 0.222107f},
    {0.974670f, 0.223602f}, {0.974335f, 0.225098f}, {0.973999f, 0.226593f},
    {0.973633f, 0.228058f}, {0.973297f, 0.229553f}, {0.972931f, 0.231049f},
    {0.972595f, 0.232544f}, {0.972229f, 0.234039f}, {0.971863f, 0.235535f},
    {0.971497f, 0.237030f}, {0.971130f, 0.238525f}, {0.970764f, 0.239990f},
    {0.970398f, 0.241486f}, {0.970032f, 0.242981f}, {0.969666f, 0.244476f},
    {0.969269f, 0.245941f}, {0.968903f, 0.247437f}, {0.968536f, 0.248932f},
    {0.968140f, 0.250427f}, {0.967743f, 0.251892f}, {0.967377f, 0.253387f},
    {0.966980f, 0.254852f}, {0.966583f, 0.256348f}, {0.966187f, 0.257843f},
    {0.965790f, 0.259308f}, {0.965393f, 0.260803f}, {0.964996f, 0.262268f},
    {0.964600f, 0.263763f}, {0.964172f, 0.265228f}, {0.963776f, 0.266724f},
    {0.963379f, 0.268188f}, {0.962952f, 0.269653f}, {0.962524f, 0.271149f},
    {0.962128f, 0.272614f}, {0.961700f, 0.274109f}, {0.961273f, 0.275574f},
    {0.960846f, 0.277039f}, {0.960419f, 0.278534f}, {0.959991f, 0.279999f},
    {0.959564f, 0.281464f}, {0.959137f, 0.282928f}, {0.958710f, 0.284393f},
    {0.958252f, 0.285889f}, {0.957825f, 0.287354f}, {0.957397f, 0.288818f},
    {0.956940f, 0.290283f}, {0.956482f, 0.291748f}, {0.956055f, 0.293213f},
    {0.955597f, 0.294678f}, {0.955139f, 0.296143f}, {0.954681f, 0.297607f},
    {0.954224f, 0.299072f}, {0.953766f, 0.300537f}, {0.953308f, 0.302002f},
    {0.952850f, 0.303467f}, {0.952362f, 0.304932f}, {0.951904f, 0.306396f},
    {0.951447f, 0.307861f}, {0.950958f, 0.309296f}, {0.950500f, 0.310760f},
    {0.950012f, 0.312225f}, {0.949524f, 0.313690f}, {0.949036f, 0.315125f},
    {0.948547f, 0.316589f}, {0.948090f, 0.318054f}, {0.947571f, 0.319489f},
    {0.947083f, 0.320953f}, {0.946594f, 0.322418f}, {0.946106f, 0.323853f},
    {0.945618f, 0.325317f}, {0.945099f, 0.326752f}, {0.944611f, 0.328217f},
    {0.944092f, 0.329651f}, {0.943604f, 0.331116f}, {0.943085f, 0.332550f},
    {0.942566f, 0.334015f}, {0.942047f, 0.335449f}, {0.941559f, 0.336884f},
    {0.941040f, 0.338348f}, {0.940521f, 0.339783f}, {0.939972f, 0.341217f},
    {0.939453f, 0.342651f}, {0.938934f, 0.344116f}, {0.938416f, 0.345551f},
    {0.937866f, 0.346985f}, {0.937347f, 0.348419f}, {0.936798f, 0.349854f},
    {0.936279f, 0.351288f}, {0.935730f, 0.352722f}, {0.935181f, 0.354156f},
    {0.934631f, 0.355591f}, {0.934082f, 0.357025f}, {0.933533f, 0.358459f},
    {0.932983f, 0.359894f}, {0.932434f, 0.361328f}, {0.931885f, 0.362762f},
    {0.931335f, 0.364197f}, {0.930756f, 0.365601f}, {0.930206f, 0.367035f},
    {0.929626f, 0.368469f}, {0.929077f, 0.369904f}, {0.928497f, 0.371307f},
    {0.927948f, 0.372742f}, {0.927368f, 0.374176f}, {0.926788f, 0.375580f},
    {0.926208f, 0.377014f}, {0.925629f, 0.378418f}, {0.925049f, 0.379852f},
    {0.924469f, 0.381256f}, {0.923889f, 0.382690f}, {0.923279f, 0.384094f},
    {0.922699f, 0.385529f}, {0.922119f, 0.386932f}, {0.921509f, 0.388336f},
    {0.920929f, 0.389771f}, {0.920319f, 0.391174f}, {0.919708f, 0.392578f},
    {0.919128f, 0.393982f}, {0.918518f, 0.395416f}, {0.917908f, 0.396820f},
    {0.917297f, 0.398224f}, {0.916687f, 0.399628f}, {0.916077f, 0.401031f},
    {0.915436f, 0.402435f}, {0.914825f, 0.403839f}, {0.914215f, 0.405243f},
    {0.913574f, 0.406647f}, {0.912964f, 0.408051f}, {0.912323f, 0.409454f},
    {0.911713f, 0.410858f}, {0.911072f, 0.412231f}, {0.910431f, 0.413635f},
    {0.909821f, 0.415039f}, {0.909180f, 0.416443f}, {0.908539f, 0.417816f},
    {0.907898f, 0.419220f}, {0.907257f, 0.420624f}, {0.906586f, 0.421997f},
    {0.905945f, 0.423401f}, {0.905304f, 0.424774f}, {0.904633f, 0.426178f},
    {0.903992f, 0.427551f}, {0.903320f, 0.428955f}, {0.902679f, 0.430328f},
    {0.902008f, 0.431702f}, {0.901337f, 0.433105f}, {0.900696f, 0.434479f},
    {0.900024f, 0.435852f}, {0.899353f, 0.437225f}, {0.898682f, 0.438629f},
    {0.898010f, 0.440002f}, {0.897339f, 0.441376f}, {0.896637f, 0.442749f},
    {0.895966f, 0.444122f}, {0.895294f, 0.445496f}, {0.894592f, 0.446869f},
    {0.893921f, 0.448242f}, {0.893219f, 0.449615f}, {0.892548f, 0.450989f},
    {0.891846f, 0.452362f}, {0.891144f, 0.453705f}, {0.890442f, 0.455078f},
    {0.889740f, 0.456451f}, {0.889038f, 0.457825f}, {0.888336f, 0.459167f},
    {0.887634f, 0.460541f}, {0.886932f, 0.461914f}, {0.886230f, 0.463257f},
    {0.885498f, 0.464630f}, {0.884796f, 0.465973f}, {0.884094f, 0.467346f},
    {0.883362f, 0.468689f}, {0.882629f, 0.470032f}, {0.881927f, 0.471405f},
    {0.881195f, 0.472748f}, {0.880463f, 0.474091f}, {0.879730f, 0.475464f},
    {0.878998f, 0.476807f}, {0.878265f, 0.478149f}, {0.877533f, 0.479492f},
    {0.876801f, 0.480835f}, {0.876068f, 0.482178f}, {0.875336f, 0.483521f},
    {0.874573f, 0.484863f}, {0.873840f, 0.486206f}, {0.873108f, 0.487549f},
    {0.872345f, 0.488892f}, {0.871582f, 0.490234f}, {0.870850f, 0.491577f},
    {0.870087f, 0.492889f}, {0.869324f, 0.494232f}, {0.868561f, 0.495575f},
    {0.867798f, 0.496887f}, {0.867035f, 0.498230f}, {0.866272f, 0.499542f},
    {0.865509f, 0.500885f}, {0.864746f, 0.502197f}, {0.863983f, 0.503540f},
    {0.863190f, 0.504852f}, {0.862427f, 0.506195f}, {0.861633f, 0.507507f},
    {0.860870f, 0.508820f}, {0.860077f, 0.510162f}, {0.859314f, 0.511475f},
    {0.858521f, 0.512787f}, {0.857727f, 0.514099f}, {0.856934f, 0.515411f},
    {0.856140f, 0.516724f}, {0.855347f, 0.518036f}, {0.854553f, 0.519348f},
    {0.853760f, 0.520660f}, {0.852966f, 0.521973f}, {0.852173f, 0.523285f},
    {0.851349f, 0.524597f}, {0.850555f, 0.525909f}, {0.849731f, 0.527191f},
    {0.848938f, 0.528503f}, {0.848114f, 0.529816f}, {0.847321f, 0.531097f},
    {0.846497f, 0.532410f}, {0.845673f, 0.533691f}, {0.844849f, 0.535004f},
    {0.844025f, 0.536285f}, {0.843201f, 0.537598f}, {0.842377f, 0.538879f},
    {0.841553f, 0.540161f}, {0.840729f, 0.541473f}, {0.839905f, 0.542755f},
    {0.839050f, 0.544037f}, {0.838226f, 0.545319f}, {0.837402f, 0.546600f},
    {0.836548f, 0.547882f}, {0.835693f, 0.549164f}, {0.834869f, 0.550446f},
    {0.834015f, 0.551727f}, {0.833160f, 0.553009f}, {0.832306f, 0.554291f},
    {0.831482f, 0.555573f}, {0.830627f, 0.556854f}, {0.829773f, 0.558105f},
    {0.828918f, 0.559387f}, {0.828033f, 0.560669f}, {0.827179f, 0.561920f},
    {0.826324f, 0.563202f}, {0.825470f, 0.564453f}, {0.824585f, 0.565735f},
    {0.823730f, 0.566986f}, {0.822845f, 0.568268f}, {0.821991f, 0.569519f},
    {0.821106f, 0.570770f}, {0.820221f, 0.572052f}, {0.819336f, 0.573303f},
    {0.818481f, 0.574554f}, {0.817596f, 0.575806f}, {0.816711f, 0.577057f},
    {0.815826f, 0.578308f}, {0.814941f, 0.579559f}, {0.814026f, 0.580811f},
    {0.813141f, 0.582062f}, {0.812256f, 0.583313f}, {0.811340f, 0.584564f},
    {0.810455f, 0.585785f}, {0.809570f, 0.587036f}, {0.808655f, 0.588287f},
    {0.807739f, 0.589508f}, {0.806854f, 0.590759f}, {0.805939f, 0.592010f},
    {0.805023f, 0.593231f}, {0.804108f, 0.594452f}, {0.803223f, 0.595703f},
    {0.802307f, 0.596924f}, {0.801361f, 0.598175f}, {0.800446f, 0.599396f},
    {0.799530f, 0.600616f}, {0.798615f, 0.601837f}, {0.797699f, 0.603058f},
    {0.796753f, 0.604279f}, {0.795837f, 0.605499f}, {0.794922f, 0.606720f},
    {0.793976f, 0.607941f}, {0.793030f, 0.609161f}, {0.792114f, 0.610382f},
    {0.791168f, 0.611603f}, {0.790222f, 0.612823f}, {0.789276f, 0.614014f},
    {0.788361f, 0.615234f}, {0.787415f, 0.616455f}, {0.786469f, 0.617645f},
    {0.785492f, 0.618866f}, {0.784546f, 0.620056f}, {0.783600f, 0.621246f},
    {0.782654f, 0.622467f}, {0.781708f, 0.623657f}, {0.780731f, 0.624847f},
    {0.779785f, 0.626068f}, {0.778809f, 0.627258f}, {0.777863f, 0.628448f},
    {0.776886f, 0.629639f}, {0.775909f, 0.630829f}, {0.774963f, 0.632019f},
    {0.773987f, 0.633209f}, {0.773010f, 0.634399f}, {0.772034f, 0.635590f},
    {0.771057f, 0.636749f}, {0.770081f, 0.637939f}, {0.769104f, 0.639130f},
    {0.768127f, 0.640289f}, {0.767151f, 0.641479f}, {0.766144f, 0.642670f},
    {0.765167f, 0.643829f}, {0.764191f, 0.645020f}, {0.763184f, 0.646179f},
    {0.762207f, 0.647339f}, {0.761200f, 0.648529f}, {0.760193f, 0.649689f},
    {0.759216f, 0.650848f}, {0.758209f, 0.652008f}, {0.757202f, 0.653168f},
    {0.756195f, 0.654327f}, {0.755188f, 0.655487f}, {0.754181f, 0.656647f},
    {0.753174f, 0.657806f}, {0.752167f, 0.658966f}, {0.751160f, 0.660126f},
    {0.750153f, 0.661255f}, {0.749146f, 0.662415f}, {0.748108f, 0.663574f},
    {0.747101f, 0.664703f}, {0.746094f, 0.665863f}, {0.745056f, 0.666992f},
    {0.744019f, 0.668152f}, {0.743011f, 0.669281f}, {0.741974f, 0.670410f},
    {0.740936f, 0.671570f}, {0.739929f, 0.672699f}, {0.738892f, 0.673828f},
    {0.737854f, 0.674957f}, {0.736816f, 0.676086f}, {0.735779f, 0.677216f},
    {0.734741f, 0.678345f}, {0.733704f, 0.679474f}, {0.732666f, 0.680603f},
    {0.731598f, 0.681732f}, {0.730560f, 0.682831f}, {0.729523f, 0.683960f},
    {0.728455f, 0.685089f}, {0.727417f, 0.686188f}, {0.726349f, 0.687317f},
    {0.725311f, 0.688416f}, {0.724243f, 0.689545f}, {0.723175f, 0.690643f},
    {0.722137f, 0.691772f}, {0.721069f, 0.692871f}, {0.720001f, 0.693970f},
    {0.718933f, 0.695068f}, {0.717865f, 0.696167f}, {0.716797f, 0.697266f},
    {0.715729f, 0.698364f}, {0.714661f, 0.699463f}, {0.713593f, 0.700562f},
    {0.712524f, 0.701660f}, {0.711426f, 0.702759f}, {0.710358f, 0.703857f},
    {0.709259f, 0.704926f}, {0.708191f, 0.706024f}, {0.707092f, 0.707092f}};

const ixheaace_cmplx_str sine_window_32[] = {
    {0.999695f, 0.024536f}, {0.997284f, 0.073578f}, {0.992493f, 0.122406f},
    {0.985291f, 0.170959f}, {0.975708f, 0.219116f}, {0.963776f, 0.266724f},
    {0.949524f, 0.313690f}, {0.932983f, 0.359894f}, {0.914215f, 0.405243f},
    {0.893219f, 0.449615f}, {0.870087f, 0.492889f}, {0.844849f, 0.535004f},
    {0.817596f, 0.575806f}, {0.788361f, 0.615234f}, {0.757202f, 0.653168f},
    {0.724243f, 0.689545f}};

const ixheaace_cmplx_str sine_window_64[] = {
    {0.999939f, 0.012268f}, {0.999329f, 0.036804f}, {0.998108f, 0.061310f},
    {0.996307f, 0.085785f}, {0.993896f, 0.110229f}, {0.990906f, 0.134583f},
    {0.987305f, 0.158844f}, {0.983093f, 0.183044f}, {0.978333f, 0.207123f},
    {0.972931f, 0.231049f}, {0.966980f, 0.254852f}, {0.960419f, 0.278534f},
    {0.953308f, 0.302002f}, {0.945618f, 0.325317f}, {0.937347f, 0.348419f},
    {0.928497f, 0.371307f}, {0.919128f, 0.393982f}, {0.909180f, 0.416443f},
    {0.898682f, 0.438629f}, {0.887634f, 0.460541f}, {0.876068f, 0.482178f},
    {0.863983f, 0.503540f}, {0.851349f, 0.524597f}, {0.838226f, 0.545319f},
    {0.824585f, 0.565735f}, {0.810455f, 0.585785f}, {0.795837f, 0.605499f},
    {0.780731f, 0.624847f}, {0.765167f, 0.643829f}, {0.749146f, 0.662415f},
    {0.732666f, 0.680603f}, {0.715729f, 0.698364f}};

const FLOAT32 qmf_mps_ld_fb_320[QMF320_MPSLDFB_PFT_TABLE_SIZE] = {
    (1.0777725402e-004f),  (-9.4703806099e-004f), (6.1286436394e-003f),  (-9.0161964297e-002f),
    (5.5554401875e-001f),  (1.2731316383e-004f),  (-1.2311334722e-003f), (4.9468209036e-003f),
    (-1.1305026710e-001f), (5.2990418673e-001f),  (1.1927412561e-004f),  (-1.5128203668e-003f),
    (3.5794533323e-003f),  (-1.3681203127e-001f), (5.0423312187e-001f),  (1.0006380762e-004f),
    (-1.7925058492e-003f), (2.0164034795e-003f),  (-1.6139641404e-001f), (4.7861024737e-001f),
    (7.2826202086e-005f),  (-2.0697340369e-003f), (2.4838969694e-004f),  (-1.8674756587e-001f),
    (4.5311337709e-001f),  (3.8808015233e-005f),  (-2.3429044522e-003f), (-1.7331546405e-003f),
    (-2.1280488372e-001f), (4.2781800032e-001f),  (-5.4359588830e-007f), (-2.6112669148e-003f),
    (-3.9357249625e-003f), (-2.3950359225e-001f), (4.0279802680e-001f),  (-4.3614549213e-005f),
    (-2.8741455171e-003f), (-6.3655078411e-003f), (-2.6677471399e-001f), (3.7812507153e-001f),
    (-8.9040157036e-005f), (-3.1308881007e-003f), (-9.0275555849e-003f), (-2.9454550147e-001f),
    (3.5386830568e-001f),  (-1.3519046479e-004f), (-3.3808732405e-003f), (-1.1925406754e-002f),
    (-3.2273942232e-001f), (3.3009397984e-001f),  (-1.8045579782e-004f), (-3.6236830056e-003f),
    (-1.5061311424e-002f), (-3.5127705336e-001f), (3.0686509609e-001f),  (-2.2396800341e-004f),
    (-3.8587960880e-003f), (-1.8435835838e-002f), (-3.8007527590e-001f), (2.8424069285e-001f),
    (-2.6416976471e-004f), (-4.0859002620e-003f), (-2.2048022598e-002f), (-4.0904915333e-001f),
    (2.6227575541e-001f),  (-3.0001887353e-004f), (-4.3045589700e-003f), (-2.5894984603e-002f),
    (-4.3811064959e-001f), (2.4102044106e-001f),  (-3.3083156450e-004f), (-4.5145484619e-003f),
    (-2.9972121119e-002f), (-4.6717000008e-001f), (2.2052007914e-001f),  (-3.5614447552e-004f),
    (-4.7155953944e-003f), (-3.4272894263e-002f), (-4.9613577127e-001f), (2.0081442595e-001f),
    (-3.7579826312e-004f), (-4.9072988331e-003f), (-3.8788780570e-002f), (-5.2491527796e-001f),
    (1.8193808198e-001f),  (-3.8993739872e-004f), (-5.0893351436e-003f), (-4.3509010226e-002f),
    (-5.5341482162e-001f), (1.6391974688e-001f),  (-3.9912899956e-004f), (-5.2615385503e-003f),
    (-4.8421185464e-002f), (-5.8154034615e-001f), (1.4678207040e-001f),  (-4.0421969607e-004f),
    (-5.4236799479e-003f), (-5.3510606289e-002f), (-6.0919785500e-001f), (1.3054165244e-001f),
    (-4.0645478293e-004f), (-5.5756671354e-003f), (-5.8760054410e-002f), (-6.3629388809e-001f),
    (1.1520925164e-001f),  (-4.0720938705e-004f), (-5.7173836976e-003f), (-6.4149998128e-002f),
    (-6.6273581982e-001f), (1.0078965127e-001f),  (-4.0812738007e-004f), (-5.8488911018e-003f),
    (-6.9658569992e-002f), (-6.8843221664e-001f), (8.7281554937e-002f),  (-4.1120912647e-004f),
    (-5.9703430161e-003f), (-7.5261354446e-002f), (-7.1329379082e-001f), (7.4678033590e-002f),
    (-4.1838851757e-004f), (-6.0821287334e-003f), (-8.0931767821e-002f), (-7.3723363876e-001f),
    (6.2966249883e-002f),  (-4.3148122495e-004f), (-6.1847940087e-003f), (-8.6640790105e-002f),
    (-7.6016783714e-001f), (5.2128262818e-002f),  (-4.5229538227e-004f), (-6.2791546807e-003f),
    (-9.2357128859e-002f), (-7.8201586008e-001f), (4.2139917612e-002f),  (-4.8211280955e-004f),
    (-6.3661932945e-003f), (-9.8047181964e-002f), (-8.0270123482e-001f), (3.2972395420e-002f),
    (-5.2196672186e-004f), (-6.4471233636e-003f), (-1.0367526114e-001f), (-8.2215231657e-001f),
    (2.4589803070e-002f),  (-5.7247944642e-004f), (-6.5232971683e-003f), (-1.0920339823e-001f),
    (-8.4030228853e-001f), (1.6952158883e-002f),  (-6.3343788497e-004f), (-6.5963375382e-003f),
    (-1.1459194124e-001f), (-8.5709118843e-001f), (1.0006074794e-002f),  (-7.0449430496e-004f),
    (-6.6681848839e-003f), (-1.1979964375e-001f), (-8.7246519327e-001f), (3.6968050990e-003f),
    (-7.9609593377e-004f), (-6.7403013818e-003f), (-1.2478165329e-001f), (-8.8632321358e-001f),
    (-1.6344460892e-003f), (-9.0200459817e-004f), (-6.8151149899e-003f), (-1.2949258089e-001f),
    (-8.9860773087e-001f), (-5.9283543378e-003f), (-1.0116943158e-003f), (-6.8955891766e-003f),
    (-1.3388808072e-001f), (-9.0933418274e-001f), (-9.6466485411e-003f), (-1.1244935449e-003f),
    (-6.9835213944e-003f), (-1.3791990280e-001f), (-9.1846722364e-001f), (-1.2838950381e-002f),
    (-1.2393904617e-003f), (-7.0809246972e-003f), (-1.4153905213e-001f), (-9.2597639561e-001f),
    (-1.5539921820e-002f), (-1.3542033266e-003f), (-7.1895248257e-003f), (-1.4469626546e-001f),
    (-9.3183851242e-001f), (-1.7783239484e-002f), (-1.4669501688e-003f), (-7.3110014200e-003f),
    (-1.4734169841e-001f), (-9.3603670597e-001f), (-1.9597738981e-002f), (-1.5753224725e-003f),
    (-7.4466220103e-003f), (-1.4942565560e-001f), (-9.3856132030e-001f), (-2.1011535078e-002f),
    (-1.6771152150e-003f), (-7.5972955674e-003f), (-1.5089863539e-001f), (-9.3940949440e-001f),
    (-2.2049814463e-002f), (-1.7698677257e-003f), (-7.7634919435e-003f), (-1.5171185136e-001f),
    (-9.3858534098e-001f), (-2.2738276049e-002f), (-1.8512960523e-003f), (-7.9450644553e-003f),
    (-1.5181747079e-001f), (-9.3610012531e-001f), (-2.3101080209e-002f), (-1.9192657201e-003f),
    (-8.1413704902e-003f), (-1.5116891265e-001f), (-9.3197190762e-001f), (-2.3163486272e-002f),
    (-1.9716904499e-003f), (-8.3509404212e-003f), (-1.4972095191e-001f), (-9.2622530460e-001f),
    (-2.2950030863e-002f), (-2.0066620782e-003f), (-8.5715763271e-003f), (-1.4743055403e-001f),
    (-9.1889131069e-001f), (-2.2486699745e-002f), (-2.0227057394e-003f), (-8.8005559519e-003f),
    (-1.4425669611e-001f), (-9.1000711918e-001f), (-2.1799135953e-002f), (-2.0185527392e-003f),
    (-9.0341167524e-003f), (-1.4016106725e-001f), (-8.9961612225e-001f), (-2.0914383233e-002f),
    (-1.9932338037e-003f), (-9.2674419284e-003f), (-1.3510815799e-001f), (-8.8776648045e-001f),
    (-1.9859094173e-002f), (-1.9461065531e-003f), (-9.4948727638e-003f), (-1.2906542420e-001f),
    (-8.7451159954e-001f), (-1.8660902977e-002f), (-1.8770052120e-003f), (-9.7100129351e-003f),
    (-1.2200380862e-001f), (-8.5991013050e-001f), (-1.7346922308e-002f), (-1.7859865911e-003f),
    (-9.9056493491e-003f), (-1.1389782280e-001f), (-8.4402561188e-001f), (-1.5944939107e-002f),
    (-1.6734169330e-003f), (-1.0073989630e-002f), (-1.0472598672e-001f), (-8.2692527771e-001f),
    (-1.4481747523e-002f), (-1.5399802942e-003f), (-1.0205906816e-002f), (-9.4470888376e-002f),
    (-8.0868041515e-001f), (-1.2984249741e-002f), (-1.3865872752e-003f), (-1.0291703977e-002f),
    (-8.3119556308e-002f), (-7.8936588764e-001f), (-1.1477986351e-002f), (-1.2144348584e-003f),
    (-1.0320962407e-002f), (-7.0663399994e-002f), (-7.6905936003e-001f), (-9.9884867668e-003f),
    (-1.0248266626e-003f), (-1.0282764211e-002f), (-5.7098604739e-002f), (-7.4784147739e-001f),
    (-8.5393209010e-003f), (-8.1919803051e-004f), (-1.0165717453e-002f), (-4.2426198721e-002f),
    (-7.2579479218e-001f), (-7.1533406153e-003f), (-5.9914286248e-004f), (-9.9579729140e-003f),
    (-2.6652012020e-002f), (-7.0300412178e-001f), (-5.8508114889e-003f), (-3.6626873771e-004f),
    (-9.6475090832e-003f), (-9.7871217877e-003f), (-6.7955517769e-001f), (-4.6512838453e-003f),
    (-1.2227181287e-004f), (-9.2221321538e-003f), (8.1523396075e-003f),  (-6.5553492308e-001f),
    (-3.5699680448e-003f), (1.3090072025e-004f),  (-8.6695179343e-003f), (2.7145106345e-002f),
    (-6.3103044033e-001f), (-2.6181070134e-003f), (3.9128778735e-004f),  (-7.9773496836e-003f),
    (4.7164849937e-002f),  (-6.0613000393e-001f), (-1.7908872105e-003f), (6.5761915175e-004f),
    (-7.1337916888e-003f), (6.8181537092e-002f),  (-5.8092808723e-001f), (-1.0135001503e-003f)};

const FLOAT32 qmf_mps_ld_fb_640[QMF640_MPSLDFB_PFT_TABLE_SIZE] = {
    (9.3863010989e-005f),  (-8.7536586216e-004f), (6.4016343094e-003f),  (-8.4552817047e-002f),
    (5.6194400787e-001f),  (1.2169149704e-004f),  (-1.0187102016e-003f), (5.8556534350e-003f),
    (-9.5771118999e-002f), (5.4914402962e-001f),  (1.2793767382e-004f),  (-1.1605311884e-003f),
    (5.2649765275e-003f),  (-1.0721673071e-001f), (5.3632181883e-001f),  (1.2668863928e-004f),
    (-1.3017356396e-003f), (4.6286652796e-003f),  (-1.1888379604e-001f), (5.2348655462e-001f),
    (1.2296593923e-004f),  (-1.4426353155e-003f), (3.9453012869e-003f),  (-1.3076621294e-001f),
    (5.1064836979e-001f),  (1.1558231199e-004f),  (-1.5830053017e-003f), (3.2136053778e-003f),
    (-1.4285783470e-001f), (4.9781781435e-001f),  (1.0582985124e-004f),  (-1.7228506040e-003f),
    (2.4323666003e-003f),  (-1.5515175462e-001f), (4.8500382900e-001f),  (9.4297764008e-005f),
    (-1.8621610943e-003f), (1.6004402423e-003f),  (-1.6764105856e-001f), (4.7221666574e-001f),
    (8.0514568253e-005f),  (-2.0008818246e-003f), (7.1672687773e-004f),  (-1.8031860888e-001f),
    (4.5946595073e-001f),  (6.5137835918e-005f),  (-2.1385864820e-003f), (-2.1994746930e-004f),
    (-1.9317652285e-001f), (4.4676083326e-001f),  (4.8101064749e-005f),  (-2.2751907818e-003f),
    (-1.2104592752e-003f), (-2.0620720088e-001f), (4.3411090970e-001f),  (2.9514967537e-005f),
    (-2.4106178898e-003f), (-2.2558500059e-003f), (-2.1940255165e-001f), (4.2152509093e-001f),
    (9.8814107332e-006f),  (-2.5448307861e-003f), (-3.3569468651e-003f), (-2.3275400698e-001f),
    (4.0901294351e-001f),  (-1.0968602510e-005f), (-2.6777030434e-003f), (-4.5145032927e-003f),
    (-2.4625316262e-001f), (3.9658311009e-001f),  (-3.2559255487e-005f), (-2.8091520071e-003f),
    (-5.7292259298e-003f), (-2.5989097357e-001f), (3.8424444199e-001f),  (-5.4669842939e-005f),
    (-2.9391390271e-003f), (-7.0017897524e-003f), (-2.7365845442e-001f), (3.7200567126e-001f),
    (-7.7506563684e-005f), (-3.0675258022e-003f), (-8.3327051252e-003f), (-2.8754624724e-001f),
    (3.5987523198e-001f),  (-1.0057374311e-004f), (-3.1942503992e-003f), (-9.7224051133e-003f),
    (-3.0154475570e-001f), (3.4786140919e-001f),  (-1.2368557509e-004f), (-3.3192564733e-003f),
    (-1.1171258055e-002f), (-3.1564420462e-001f), (3.3597227931e-001f),  (-1.4669535449e-004f),
    (-3.4424900077e-003f), (-1.2679555453e-002f), (-3.2983466983e-001f), (3.2421571016e-001f),
    (-1.6928518016e-004f), (-3.5639149137e-003f), (-1.4247507788e-002f), (-3.4410607815e-001f),
    (3.1259948015e-001f),  (-1.9162640092e-004f), (-3.6834510975e-003f), (-1.5875114128e-002f),
    (-3.5844799876e-001f), (3.0113074183e-001f),  (-2.1345751884e-004f), (-3.8009947166e-003f),
    (-1.7562393099e-002f), (-3.7284970284e-001f), (2.8981682658e-001f),  (-2.3447850253e-004f),
    (-3.9165974595e-003f), (-1.9309276715e-002f), (-3.8730087876e-001f), (2.7866455913e-001f),
    (-2.5462667691e-004f), (-4.0301652625e-003f), (-2.1115457639e-002f), (-4.0179058909e-001f),
    (2.6768052578e-001f),  (-2.7371285250e-004f), (-4.1416347958e-003f), (-2.2980585694e-002f),
    (-4.1630774736e-001f), (2.5687095523e-001f),  (-2.9165804153e-004f), (-4.2509674095e-003f),
    (-2.4904217571e-002f), (-4.3084129691e-001f), (2.4624188244e-001f),  (-3.0837973463e-004f),
    (-4.3581505306e-003f), (-2.6885753497e-002f), (-4.4538003206e-001f), (2.3579898477e-001f),
    (-3.2378203468e-004f), (-4.4631510973e-003f), (-2.8924530372e-002f), (-4.5991250873e-001f),
    (2.2554755211e-001f),  (-3.3788106521e-004f), (-4.5659458265e-003f), (-3.1019711867e-002f),
    (-4.7442746162e-001f), (2.1549259126e-001f),  (-3.5053401371e-004f), (-4.6664695255e-003f),
    (-3.3170353621e-002f), (-4.8891320825e-001f), (2.0563863218e-001f),  (-3.6175493733e-004f),
    (-4.7647207975e-003f), (-3.5375438631e-002f), (-5.0335830450e-001f), (1.9599021971e-001f),
    (-3.7159718340e-004f), (-4.8605888151e-003f), (-3.7633713335e-002f), (-5.1775097847e-001f),
    (1.8655113876e-001f),  (-3.7999937194e-004f), (-4.9540083855e-003f), (-3.9943847805e-002f),
    (-5.3207957745e-001f), (1.7732504010e-001f),  (-3.8705617771e-004f), (-5.0450465642e-003f),
    (-4.2304381728e-002f), (-5.4633224010e-001f), (1.6831515729e-001f),  (-3.9281861973e-004f),
    (-5.1336232573e-003f), (-4.4713638723e-002f), (-5.6049734354e-001f), (1.5952435136e-001f),
    (-3.9737694897e-004f), (-5.2197398618e-003f), (-4.7170232981e-002f), (-5.7456302643e-001f),
    (1.5095503628e-001f),  (-4.0088107926e-004f), (-5.3033372387e-003f), (-4.9672137946e-002f),
    (-5.8851766586e-001f), (1.4260910451e-001f),  (-4.0338383405e-004f), (-5.3843962960e-003f),
    (-5.2217379212e-002f), (-6.0234934092e-001f), (1.3448855281e-001f),  (-4.0505555808e-004f),
    (-5.4629631341e-003f), (-5.4803829640e-002f), (-6.1604642868e-001f), (1.2659475207e-001f),
    (-4.0614881436e-004f), (-5.5389581248e-003f), (-5.7429198176e-002f), (-6.2959736586e-001f),
    (1.1892842501e-001f),  (-4.0676075150e-004f), (-5.6123761460e-003f), (-6.0090914369e-002f),
    (-6.4299046993e-001f), (1.1149007827e-001f),  (-4.0709332097e-004f), (-5.6832311675e-003f),
    (-6.2786586583e-002f), (-6.5621429682e-001f), (1.0428040475e-001f),  (-4.0732545312e-004f),
    (-5.7515366934e-003f), (-6.5513409674e-002f), (-6.6925734282e-001f), (9.7298897803e-002f),
    (-4.0770808118e-004f), (-5.8172862045e-003f), (-6.8268470466e-002f), (-6.8210834265e-001f),
    (9.0545162559e-002f),  (-4.0854664985e-004f), (-5.8804959990e-003f), (-7.1048669517e-002f),
    (-6.9475615025e-001f), (8.4017947316e-002f),  (-4.1002241778e-004f), (-5.9412117116e-003f),
    (-7.3850922287e-002f), (-7.0718955994e-001f), (7.7716566622e-002f),  (-4.1239586426e-004f),
    (-5.9994738549e-003f), (-7.6671779156e-002f), (-7.1939796209e-001f), (7.1639508009e-002f),
    (-4.1594370850e-004f), (-6.0553550720e-003f), (-7.9507902265e-002f), (-7.3137050867e-001f),
    (6.5784148872e-002f),  (-4.2083335575e-004f), (-6.1089023948e-003f), (-8.2355625927e-002f),
    (-7.4309676886e-001f), (6.0148354620e-002f),  (-4.2732476140e-004f), (-6.1602159403e-003f),
    (-8.5211075842e-002f), (-7.5456637144e-001f), (5.4730266333e-002f),  (-4.3563771760e-004f),
    (-6.2093720771e-003f), (-8.8070511818e-002f), (-7.6576924324e-001f), (4.9526259303e-002f),
    (-4.4600359979e-004f), (-6.2565426342e-003f), (-9.0929701924e-002f), (-7.7669566870e-001f),
    (4.4533081353e-002f),  (-4.5858716476e-004f), (-6.3017667271e-003f), (-9.3784548342e-002f),
    (-7.8733605146e-001f), (3.9746750146e-002f),  (-4.7345875646e-004f), (-6.3452622853e-003f),
    (-9.6630692482e-002f), (-7.9768097401e-001f), (3.5163912922e-002f),  (-4.9076689174e-004f),
    (-6.3871243037e-003f), (-9.9463671446e-002f), (-8.0772149563e-001f), (3.0780877918e-002f),
    (-5.1067111781e-004f), (-6.4275567420e-003f), (-1.0227891803e-001f), (-8.1744915247e-001f),
    (2.6590615511e-002f),  (-5.3326232592e-004f), (-6.4666904509e-003f), (-1.0507161170e-001f),
    (-8.2685548067e-001f), (2.2588992491e-002f),  (-5.5855646497e-004f), (-6.5047293901e-003f),
    (-1.0783691704e-001f), (-8.3593225479e-001f), (1.8772648647e-002f),  (-5.8640236966e-004f),
    (-6.5418654121e-003f), (-1.1056987941e-001f), (-8.4467232227e-001f), (1.5131668188e-002f),
    (-6.1692652525e-004f), (-6.5783206373e-003f), (-1.1326543987e-001f), (-8.5306841135e-001f),
    (1.1661184952e-002f),  (-6.4994930290e-004f), (-6.6143544391e-003f), (-1.1591844261e-001f),
    (-8.6111402512e-001f), (8.3509646356e-003f),  (-6.8494328298e-004f), (-6.6502285190e-003f),
    (-1.1852371693e-001f), (-8.6880439520e-001f), (5.1832948811e-003f),  (-7.2404538514e-004f),
    (-6.6861407831e-003f), (-1.2107557058e-001f), (-8.7612599134e-001f), (2.2103153169e-003f),
    (-7.7061145566e-004f), (-6.7221261561e-003f), (-1.2356808037e-001f), (-8.8305824995e-001f),
    (-4.6855807886e-004f), (-8.2158041187e-004f), (-6.7584766075e-003f), (-1.2599521875e-001f),
    (-8.8958823681e-001f), (-2.8003340121e-003f), (-8.7498105131e-004f), (-6.7957863212e-003f),
    (-1.2835204601e-001f), (-8.9572954178e-001f), (-4.9293786287e-003f), (-9.2902814504e-004f),
    (-6.8344431929e-003f), (-1.3063311577e-001f), (-9.0148586035e-001f), (-6.9273295812e-003f),
    (-9.8383461591e-004f), (-6.8746237084e-003f), (-1.3283239305e-001f), (-9.0685033798e-001f),
    (-8.7857460603e-003f), (-1.0395538993e-003f), (-6.9165546447e-003f), (-1.3494376838e-001f),
    (-9.1181802750e-001f), (-1.0507551953e-002f), (-1.0959620122e-003f), (-6.9604511373e-003f),
    (-1.3696120679e-001f), (-9.1638565063e-001f), (-1.2103702873e-002f), (-1.1530250777e-003f),
    (-7.0065916516e-003f), (-1.3887859881e-001f), (-9.2054879665e-001f), (-1.3574197888e-002f),
    (-1.2105966453e-003f), (-7.0552495308e-003f), (-1.4068968594e-001f), (-9.2430406809e-001f),
    (-1.4923358336e-002f), (-1.2681842782e-003f), (-7.1066003293e-003f), (-1.4238841832e-001f),
    (-9.2764878273e-001f), (-1.6156485304e-002f), (-1.3256429229e-003f), (-7.1608433500e-003f),
    (-1.4396859705e-001f), (-9.3058031797e-001f), (-1.7277117819e-002f), (-1.3827638468e-003f),
    (-7.2182063013e-003f), (-1.4542391896e-001f), (-9.3309664726e-001f), (-1.8289361149e-002f),
    (-1.4391905861e-003f), (-7.2789187543e-003f), (-1.4674818516e-001f), (-9.3519610167e-001f),
    (-1.9195662811e-002f), (-1.4947097516e-003f), (-7.3430840857e-003f), (-1.4793521166e-001f),
    (-9.3687731028e-001f), (-1.9999813288e-002f), (-1.5489540529e-003f), (-7.4108825065e-003f),
    (-1.4897871017e-001f), (-9.3813979626e-001f), (-2.0706148818e-002f), (-1.6016908921e-003f),
    (-7.4823615141e-003f), (-1.4987260103e-001f), (-9.3898290396e-001f), (-2.1316919476e-002f),
    (-1.6526894178e-003f), (-7.5576924719e-003f), (-1.5061059594e-001f), (-9.3940681219e-001f),
    (-2.1835187450e-002f), (-1.7015410122e-003f), (-7.6368991286e-003f), (-1.5118667483e-001f),
    (-9.3941211700e-001f), (-2.2264443338e-002f), (-1.7479787348e-003f), (-7.7200052328e-003f),
    (-1.5159477293e-001f), (-9.3899971247e-001f), (-2.2607907653e-002f), (-1.7917567166e-003f),
    (-7.8069791198e-003f), (-1.5182891488e-001f), (-9.3817096949e-001f), (-2.2868644446e-002f),
    (-1.8325200072e-003f), (-7.8977877274e-003f), (-1.5188319981e-001f), (-9.3692785501e-001f),
    (-2.3049183190e-002f), (-1.8700722139e-003f), (-7.9923402518e-003f), (-1.5175175667e-001f),
    (-9.3527245522e-001f), (-2.3152977228e-002f), (-1.9041235792e-003f), (-8.0905584618e-003f),
    (-1.5142890811e-001f), (-9.3320751190e-001f), (-2.3183524609e-002f), (-1.9344078610e-003f),
    (-8.1921815872e-003f), (-1.5090890229e-001f), (-9.3073624372e-001f), (-2.3143447936e-002f),
    (-1.9606938586e-003f), (-8.2970457152e-003f), (-1.5018628538e-001f), (-9.2786192894e-001f),
    (-2.3035895079e-002f), (-1.9826870412e-003f), (-8.4048351273e-003f), (-1.4925561845e-001f),
    (-9.2458862066e-001f), (-2.2864164785e-002f), (-2.0002126694e-003f), (-8.5152359679e-003f),
    (-1.4811170101e-001f), (-9.2092043161e-001f), (-2.2631708533e-002f), (-2.0131117199e-003f),
    (-8.6279176176e-003f), (-1.4674940705e-001f), (-9.1686213017e-001f), (-2.2341690958e-002f),
    (-2.0211567171e-003f), (-8.7425475940e-003f), (-1.4516362548e-001f), (-9.1241872311e-001f),
    (-2.1996961907e-002f), (-2.0242547616e-003f), (-8.8585643098e-003f), (-1.4334976673e-001f),
    (-9.0759557486e-001f), (-2.1601308137e-002f), (-2.0221893210e-003f), (-8.9755039662e-003f),
    (-1.4130303264e-001f), (-9.0239852667e-001f), (-2.1158147603e-002f), (-2.0149163902e-003f),
    (-9.0927295387e-003f), (-1.3901908696e-001f), (-8.9683371782e-001f), (-2.0670616999e-002f),
    (-2.0022888202e-003f), (-9.2095714062e-003f), (-1.3649365306e-001f), (-8.9090716839e-001f),
    (-2.0142132416e-002f), (-1.9841785543e-003f), (-9.3253115192e-003f), (-1.3372266293e-001f),
    (-8.8462579250e-001f), (-1.9576057792e-002f), (-1.9606270362e-003f), (-9.4392402098e-003f),
    (-1.3070219755e-001f), (-8.7799650431e-001f), (-1.8976125866e-002f), (-1.9315859536e-003f),
    (-9.5505062491e-003f), (-1.2742865086e-001f), (-8.7102663517e-001f), (-1.8345680088e-002f),
    (-1.8970289966e-003f), (-9.6583357081e-003f), (-1.2389861047e-001f), (-8.6372399330e-001f),
    (-1.7687706277e-002f), (-1.8569815438e-003f), (-9.7616901621e-003f), (-1.2010899931e-001f),
    (-8.5609632730e-001f), (-1.7006140202e-002f), (-1.8114587292e-003f), (-9.8597351462e-003f),
    (-1.1605655402e-001f), (-8.4815198183e-001f), (-1.6304368153e-002f), (-1.7605143366e-003f),
    (-9.9515644833e-003f), (-1.1173909158e-001f), (-8.3989918232e-001f), (-1.5585509129e-002f),
    (-1.7042002873e-003f), (-1.0036026128e-002f), (-1.0715358704e-001f), (-8.3134686947e-001f),
    (-1.4853162691e-002f), (-1.6426335787e-003f), (-1.0111952201e-002f), (-1.0229838639e-001f),
    (-8.2250368595e-001f), (-1.4110331424e-002f), (-1.5758809168e-003f), (-1.0178210214e-002f),
    (-9.7171187401e-002f), (-8.1337898970e-001f), (-1.3360806741e-002f), (-1.5040797880e-003f),
    (-1.0233603418e-002f), (-9.1770596802e-002f), (-8.0398184061e-001f), (-1.2607692741e-002f),
    (-1.4273397392e-003f), (-1.0276827961e-002f), (-8.6095176637e-002f), (-7.9432225227e-001f),
    (-1.1853585951e-002f), (-1.3458349276e-003f), (-1.0306579992e-002f), (-8.0143928528e-002f),
    (-7.8440952301e-001f), (-1.1102385819e-002f), (-1.2597256573e-003f), (-1.0321546346e-002f),
    (-7.3915921152e-002f), (-7.7425378561e-001f), (-1.0356968269e-002f), (-1.1691439431e-003f),
    (-1.0320378467e-002f), (-6.7410878837e-002f), (-7.6386493444e-001f), (-9.6200043336e-003f),
    (-1.0743001476e-003f), (-1.0301630013e-002f), (-6.0628447682e-002f), (-7.5325345993e-001f),
    (-8.8949296623e-003f), (-9.7535311943e-004f), (-1.0263898410e-002f), (-5.3568758070e-002f),
    (-7.4242949486e-001f), (-8.1837112084e-003f), (-8.7248592172e-004f), (-1.0205759667e-002f),
    (-4.6232450753e-002f), (-7.3140352964e-001f), (-7.4901022017e-003f), (-7.6591013931e-004f),
    (-1.0125675239e-002f), (-3.8619950414e-002f), (-7.2018599510e-001f), (-6.8165790290e-003f),
    (-6.5580842784e-004f), (-1.0022218339e-002f), (-3.0732547864e-002f), (-7.0878815651e-001f),
    (-6.1642420478e-003f), (-5.4247735534e-004f), (-9.8937284201e-003f), (-2.2571478039e-002f),
    (-6.9722014666e-001f), (-5.5373813957e-003f), (-4.2596619460e-004f), (-9.7389295697e-003f),
    (-1.4138570987e-002f), (-6.8549299240e-001f), (-4.9372608773e-003f), (-3.0657128082e-004f),
    (-9.5560895279e-003f), (-5.4356725886e-003f), (-6.7361742258e-001f), (-4.3653072789e-003f),
    (-1.8451632059e-004f), (-9.3438196927e-003f), (3.5346730147e-003f),  (-6.6160440445e-001f),
    (-3.8251809310e-003f), (-6.0027297877e-005f), (-9.1004446149e-003f), (1.2770005502e-002f),
    (-6.4946544170e-001f), (-3.3147553913e-003f), (6.6618180426e-005f),  (-8.8245263323e-003f),
    (2.2267201915e-002f),  (-6.3721030951e-001f), (-2.8387091588e-003f), (1.9518326735e-004f),
    (-8.5145104676e-003f), (3.2023012638e-002f),  (-6.2485051155e-001f), (-2.3975048680e-003f),
    (3.2545044087e-004f),  (-8.1687811762e-003f), (4.2033810169e-002f),  (-6.1239802837e-001f),
    (-1.9807203207e-003f), (4.5712510473e-004f),  (-7.7859172598e-003f), (5.2295893431e-002f),
    (-5.9986191988e-001f), (-1.6010539839e-003f), (5.9015140869e-004f),  (-7.3645371012e-003f),
    (6.2805138528e-002f),  (-5.8725595474e-001f), (-1.2320743408e-003f), (7.2508689482e-004f),
    (-6.9030462764e-003f), (7.3557935655e-002f),  (-5.7460016012e-001f), (-7.9492607620e-004f)};

const ixheaace_mps_config_table mps_config_tab[21] = {
    {AOT_AAC_ELD, 0, 16000, 16000, 39999},  {AOT_AAC_ELD, 0, 22050, 16000, 49999},
    {AOT_AAC_ELD, 0, 24000, 16000, 61999},  {AOT_AAC_ELD, 0, 32000, 20000, 84999},
    {AOT_AAC_ELD, 0, 44100, 50000, 192000}, {AOT_AAC_ELD, 0, 48000, 62000, 192000},

    {AOT_AAC_ELD, 1, 16000, 18000, 31999},  {AOT_AAC_ELD, 1, 22050, 18000, 31999},
    {AOT_AAC_ELD, 1, 24000, 20000, 64000},

    {AOT_AAC_ELD, 2, 32000, 18000, 64000},  {AOT_AAC_ELD, 2, 44100, 21000, 64000},
    {AOT_AAC_ELD, 2, 48000, 26000, 64000},

    {AOT_USAC, 2, 32000, 18000, 64000},     {AOT_USAC, 2, 44100, 21000, 64000},
    {AOT_USAC, 2, 48000, 26000, 64000},

    {AOT_AAC_ELD, 4, 32000, 18000, 64000},  {AOT_AAC_ELD, 4, 44100, 21000, 64000},
    {AOT_AAC_ELD, 4, 48000, 26000, 64000},

    {AOT_USAC, 4, 32000, 18000, 64000},     {AOT_USAC, 4, 44100, 21000, 64000},
    {AOT_USAC, 4, 48000, 26000, 64000}

};