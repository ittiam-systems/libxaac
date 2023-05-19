# Introduction Extended HE-AAC Encoder APIs

# Encoder APIs

| **API Call** | **Description** |
|------|------|
|ixheaace_get_lib_id_strings| Gets the encoder library name and version number details |
|ixheaace_create| Sets the encoder configuration parameters, gets the memory requirements and allocate required memory |
|ixheaace_process| Encodes the input frame data |
|ixheaace_delete| Frees the allocated memories for the encoder |

## Flowchart of calling sequence
![API Flowchart](docs/Api_flowchart_enc.png)

# Audio Object Types(AOT)
| **AOT** | **Description** |
|------|------|
|2|AAC LC|
|5|HEAACv1|
|23|AAC LD|
|29|HEAACv2|
|39|AAC ELD|
|42|USAC (support will be added soon)|

# Running the Ex-HEAAC Encoder
The Ex-HEAAC Encoder can be run by providing command-line parameters(CLI options) directly or by providing a parameter file as a command line argument.
The reference paramfile is placed in smoke_test_suite(paramfilesimple.txt)

# Command line usage :
```
<exceutable> -ifile:<input_file> -imeta:<meta_data_file> -ofile:<out_file> [options]

<executable> -paramfile:<paramfile>
[options] can be,
[-br:<bitrate>]
[-mps:<use_mps>]
[-adts:<use_adts_flag (0/1)>]
[-tns:<use_tns_flag>]
[-framesize:<framesize_to_be_used>]
[-aot:<audio_object_type>]
[-esbr:<esbr_flag (0/1)>]
[-full_bandwidth:<Enable use of full bandwidth of input>]
[-max_out_buffer_per_ch:<bitreservoir_size>]
[-tree_cfg:<tree_config>]


where, 
 <paramfile>            is the parameter file with multiple commands
 <inputfile>            is the input 16-bit WAV or PCM file name
 <outputfile>           is the output ADTS/ES file name
 <bitrate>              is the bit-rate in bits per second. Valid values are Plain AAC: 8000-576000 bps per channel
 <use_mps>              When set to 1 MPS is enable. Default 0.
 <use_adts_flag>        when set to 1 ADTS header generated. Default is 0
 <use_tns_flag>         controls usage of TNS in encoding. Default is 1.
 <framesize_to_be_used> is the framesize to be used For AOT 23, 39 (LD core coder profiles) valid values are 480 and 512 .Default is 512
                        For AOT 2, 5, 29 (LC core coder profiles) valid values are 960 and 1024 .Default is 1024
 <audio_object_type>    is the Audio object type
                        2 for AAC LC
                        5 for HEAACv1(Legacy SBR)
                        23 for AAC LD
                        29 for HEAACv2
                        39 for AAC ELD 
                        Default is 2 for AAC LC
 <esbr_flag>            when set to 1 enables eSBR in HEAACv1 encoding Default is 0
 <full_bandwidth>       Enable use of full bandwidth of input. Valid values are 0(disable) and 1(enable). Default is 0.
 <bitreservoir_size>    is the maximum size of bit reservoir to be used. Valid values are from -1 to 6144. -1 to omit use of bit reservoir. Default is 384.
 <tree_config>          MPS tree config
                        0 for '212'
                        1 for '5151'
                        2 for '5152'
                        3 for '525'
                        Default '212' for stereo input '515' for 6 ch input
```
Sample CLI:
```
-ifile:input_file.wav -ofile:out_file.aac -br:<bit_rate> –aot:<audio profile>  
```


#  Additional Documents
Brief description about documents present in  `docs` folder
* [`IA-XHEAAC-Enc-API.pdf`](docs/IA-XHEAAC-Enc-API.pdf) - Describes Application Program Interface for Ex-HEAAC Encoder.
* [`IA-XHEAAC-Enc-GSG.pdf`](docs/IA-XHEAAC-Enc-GSG.pdf) - Getting Started Guide for the Ex-HEAAC Encoder.

# Validating the Ex-HEAAC Encoder
Testing for Ex-HEAAC Encoder done using listening test for different AOT(Audio object types)
