# Introduction libxaac encoder APIs

# Encoder APIs

| **API Call** | **Description** |
|------|------|
|ixheaace_get_lib_id_strings| Gets the encoder library name and version number details |
|ixheaace_create| Sets the encoder configuration parameters, gets the memory requirements and allocates required memory |
|ixheaace_process| Encodes the input frame data |
|ixheaace_delete| Frees the allocated memories for the encoder |

## Flowchart of calling sequence
![API Flowchart](docs/Api_flowchart_enc.png)

# Audio Object Types(AOT)
| **AOT** | **Description** |
|------|------|
|2|AAC-LC|
|5|HE-AACv1|
|23|AAC-LD|
|29|HE-AACv2|
|39|AAC-ELD|
|42|USAC (support will be added soon)|

# Running the libxaac encoder
The libxaac encoder can be run by providing command-line parameters(CLI options) directly or by providing a parameter file as a command line argument.
The reference paramfile is placed in `encoder\test` directory(paramfilesimple.txt)

# Command line usage :
```
<exceutable> -ifile:<input_file> -ofile:<out_file> [options]
(or)
<executable> -paramfile:<paramfile>

[options] can be,
[-br:<bitrate>]
[-mps:<use_mps>]
[-adts:<use_adts_flag>]
[-tns:<use_tns_flag>]
[-framesize:<framesize_to_be_used>]
[-aot:<audio_object_type>]
[-esbr:<esbr_flag>]
[-full_bandwidth:<Enable use of full bandwidth of input>]
[-max_out_buffer_per_ch:<bitreservoir_size>]
[-tree_cfg:<tree_config>]

where,
  <paramfile> is the parameter file with multiple commands
  <inputfile> is the input 16-bit WAV or PCM file name
  <outputfile> is the output ADTS/ES file name
  <bitrate> is the bit-rate in bits per second. Default value is 48000.
  <use_mps> Valid values are 0 (disable MPS) and 1 (enable MPS). Default is 0.
  <use_adts_flag> Valid values are 0 ( No ADTS header) and 1 ( generate ADTS header). Default is 0.
  <use_tns_flag> Valid values are 0 (disable TNS) and 1 (enable TNS). Default is 1.
  <framesize_to_be_used> is the framesize to be used.
        For AOT 23, 39 (LD core coder profiles) valid values are 480 and 512. Default is 512.
        For AOT 2, 5, 29 (LC core coder profiles) valid values are 960 and 1024. Default is 1024.
  <audio_object_type> is the Audio object type
        2 for AAC-LC
        5 for HE-AACv1(Legacy SBR)
        23 for AAC-LD
        29 for HE-AACv2
        39 for AAC-ELD
        Default is 2 for AAC-LC.
  <esbr_flag> Valid values are 0 (disable eSBR) and 1 (enable eSBR in HE-AACv1 encoding). Default is 0.
  <full_bandwidth> Enable use of full bandwidth of input. Valid values are 0(disable full bandwidth) and 1(enable full bandwidth). Default is 0.
  <bitreservoir_size> is the maximum size of bit reservoir to be used.
        Valid values are from -1 to 6144. -1 will omit use of bit reservoir. Default is 384.
  <tree_config> MPS tree config
        0 for '212'
        1 for '5151'
        2 for '5152'
        3 for '525'
        Default '212' for stereo input and '5151' for 6ch input.
```
Sample CLI:
```
-ifile:input_file.wav -ofile:out_file.aac -br:<bit_rate> –aot:<audio profile>  
```


#  Additional Documents
Brief description about documents present in  `docs` folder
* [`LIBXAAC-Enc-API.pdf`](docs/LIBXAAC-Enc-API.pdf) - Describes Application Program Interface for libxaac encoder.
* [`LIBXAAC-Enc-GSG.pdf`](docs/LIBXAAC-Enc-GSG.pdf) - Getting Started Guide for the libxaac encoder.

# Validating the libxaac encoder
Testing for libxaac encoder is done using listening test for different AOT(Audio object types)
