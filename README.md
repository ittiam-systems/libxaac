# Introduction of the Ex-HEAAC Decoder


Ex-HEAAC (Extended HE AAC), the latest upgrade to the MPEG AAC codec family,
is the audio codec of choice for digital radio and low bit rate
streaming applications. Selected as the mandatory audio codec for DRM
(Digital Radio Mondiale), Ex-HEAAC bridges the gap between speech and
audio coding, and provides consistent high quality audio for all signal 
types, such as speech, music or mixed content.

Another important feature is the codec’s coding efficiency. The audio 
codec produces excellent sound for both music and speech, delivering 
high quality performance at bit rates starting as low as 6 kbit/s for 
mono and 12 kbit/s for stereo services. Thus Ex-HEAAC streaming apps and
streaming radio players may switch to very low bit rate streams and 
offer a continuous playback even while the network is congested. Once 
more bandwidth becomes available on the network again, the Ex-HEAAC
client can request a higher bitrate version and seamless switch over 
the full range of bitrates. Audio bitrate that’s being saved due to the 
improved coding efficiency can be used to improve video quality. Ex-HEAAC
supports AAC and HEAAC v2 as well.

AAC is a popular audio coding technique recommended by MPEG committee. 
The codec handles audio signals sampled in the range of 8 kHz to 96 
kHz. It operates on a frame of 1024 samples. The bit-rates supported 
are in the range of 8 kbps to 576 kbps per channel.

SBR and PS are the tools used in combination with the AAC general audio 
codec resulting in HEAAC v2 (also known as Enhanced AAC Plus). It
provides significant increase in coding gain. In SBR, the high-band, 
i.e. the high frequency part of the spectrum is replicated using the 
low-band. In PS, channel redundancy is exploited and parameters are 
extracted from a down-mixed channel.The bit-rate is by far below the 
bit-rate required when using conventional AAC coding. This translates 
into better quality at lower bit-rates.


# Introduction to Ex-HEAAC Decoder APIs

## Files to be included are
* [`ixheaacd_apicmd_standards.h`](decoder/ixheaacd_apicmd_standards.h)
* [`ixheaacd_type_def.h`](decoder/ixheaacd_type_def.h)
* [`ixheaacd_memory_standards.h`](decoder/ixheaacd_memory_standards.h)
* [`ixheaacd_error_standards.h`](decoder/ixheaacd_error_standards.h)
* [`ixheaacd_error_handler.h`](decoder/ixheaacd_error_handler.h)
* [`ixheaacd_aac_config.h`](decoder/ixheaacd_aac_config.h)

## Decoder APIs

A single API is used to get and set configurations and execute the decode thread, based on command index passed.
* ia_xheaacd_dec_api

| **API Command** | **API Sub Command** | **Description** |
|------|------|------|
|IA_API_CMD_GET_LIB_ID_STRINGS | IA_CMD_TYPE_LIB_NAME | Gets the decoder library name |
|IA_API_CMD_GET_LIB_ID_STRINGS | IA_CMD_TYPE_LIB_VERSION | Gets the decoder version |
|IA_API_CMD_GET_API_SIZE | 0 | Gets the memory requirements size of the API |
|IA_API_CMD_INIT | IA_CMD_TYPE_INIT_API_PRE_CONFIG_PARAMS | Sets the configuration parameters of the Ex-HEAAC v2 Decoder to default values |
|IA_API_CMD_INIT | IA_CMD_TYPE_INIT_API_POST_CONFIG_PARAMS | Sets the attributes(size, priority, alignment) of all memory types required by the application onto the memory structure |
|IA_API_CMD_SET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_SAMP_FREQ	| Sets the core AAC sampling frequency for RAW header decoding | 
|IA_API_CMD_SET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PCM_WDSZ | Sets the bit width of the output pcm samples.The value has to be 16 |
|IA_API_CMD_SET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_DOWNMIX | Sets the parameter whether the output needs to be down-mix to mono(1) or not(0) |
|IA_API_CMD_SET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_TOSTEREO	| Sets the flag to disable interleave mono to stereo |
|IA_API_CMD_SET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_DSAMPLE	| Sets the parameter whether the output needs to be downsampled(1) or not(0).This is valid only for Ex-HEAAC v2 build | 
|IA_API_CMD_SET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_FRAMEOK	| Sets the flag to 0 or 1 to indicate whether the current frame is valid(1) or not(0) |
|IA_API_CMD_SET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_ISMP4	| Sets the flag to 0 or 1 to indicate whether given test vector is an mp4 file or not |
|IA_API_CMD_SET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_DISABLE_SYNC	| Sets the flag of ADTS syncing or not ADTS syncing as 0 or 1 |
|IA_API_CMD_SET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_AUTO_SBR_UPSAMPLE	| Sets the parameter auto SBR upsample to 0 or 1.Used in case of stream changing from SBR present to SBR not present.This is valid only for Ex-HEAAC v2 build |
|IA_API_CMD_SET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_MAX_CHANNEL	| Sets the maximum number of channels present.Its maximum value is 2 for stereo library and 8 for multichannel library |
|IA_API_CMD_SET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_COUP_CHANNEL	| Sets the number of coupling channels to be used for coupling.It can take values from 0 to 16.This command is supported only if the library has multichannel support |
|IA_API_CMD_SET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_DOWNMIX_STEREO	| Sets the flag of downmixing n number of channels to stereo.Can be 0 or 1. This command is supported only if the library has multichannel support |
|IA_API_CMD_SET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_ISLOAS	| Sets the flag indicating that the input file is a .loas file.Can be 0 or 1 |
|IA_API_CMD_GET_N_MEMTABS	| 0 | Gets the number of memory types |
|IA_API_CMD_GET_N_TABLES	| 0 | Gets the number of tables |
|IA_API_CMD_GET_MEM_INFO_SIZE	| 0 | Gets the size of the memory type being referred to by the index |
|IA_API_CMD_GET_MEM_INFO_ALIGNMENT	| 0 | Gets the alignment information of the memory-type being referred to by the index |
|IA_API_CMD_GET_MEM_INFO_TYPE	| 0 | Gets the type of memory being referred to by the index |
|IA_API_CMD_SET_MEM_PTR	| 0 | Sets the pointer to the memory being referred to by the index to the input value |
|IA_API_CMD_GET_TABLE_INFO_SIZE	| 0 | Gets the size of the memory type being referred to by the index |
|IA_API_CMD_GET_TABLE_INFO_ALIGNMENT	| 0 | Gets the alignment information of the memory-type being referred to by the index |
|IA_API_CMD_GET_TABLE_PTR	| 0 | Gets the address of the current location of the table |
|IA_API_CMD_SET_TABLE_PTR	| 0 | Sets the relocated table address |
|IA_API_CMD_INPUT_OVER	| 0 | Signals the end of bit-stream to the library |
|IA_API_CMD_SET_INPUT_BYTES	| 0 | Sets the number of bytes available in the input buffer for initialization |
|IA_API_CMD_INIT | IA_CMD_TYPE_INIT_PROCESS	| Search for the valid header, does header decode to get the parameters and initializes state and configuration structure |
|IA_API_CMD_INIT | IA_CMD_TYPE_INIT_DONE_QUERY	| Checks if the initialization process has completed |
|IA_API_CMD_GET_CURIDX_INPUT_BUF | 0 | Gets the number of input buffer bytes consumed by the last initialization |
|IA_API_CMD_GET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_SAMP_FREQ | Gets the sampling frequency | 
|IA_API_CMD_GET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_NUM_CHANNELS | Gets the output number of channels |
|IA_API_CMD_GET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_PCM_WDSZ | Gets the output PCM word size |
|IA_API_CMD_GET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_CHANNEL_MODE | Gets the channel mode. (Mono or PS/Stereo/Dual-mono) |
|IA_API_CMD_GET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_CHANNEL_MASK	| Gets the channel mask |
|IA_API_CMD_GET_CONFIG_PARAM | IA_ENHAACPLUS_DEC_CONFIG_PARAM_SBR_MODE | Gets the SBR mode (Present/ Not Present).This is valid only for Ex-HEAAC v2 build |
|IA_API_CMD_EXECUTE | IA_CMD_TYPE_DO_EXECUTE	| Executes the decode thread |
|IA_API_CMD_EXECUTE | IA_CMD_TYPE_DONE_QUERY	| Checks if the end of decode has been reached |
|IA_API_CMD_GET_OUTPUT_BYTES | 0 | Gets the number of bytes output by the decoder in the last frame |

## Flowchart of calling sequence

![API Flowchart](docs/Api_flowchart.png)

#  Building the Ex-HEAAC Decoder

## Building for AOSP
* Makefiles for building the Ex-HEAAC decoder library is provided in root(`libxaac/`) folder.
* Makefiles for building the Ex-HEAAC decoder testbench is provided in `test` folder.
* Build the library followed by the application using the below commands:
Go to root directory
```
$ mm
```

## Using CMake
Users can also use cmake to build for `x86`, `x86_64`, and Windows (MSVS project) platforms.

### Building for native platforms
Run the following commands to build the Ex-HEAAC Decoder for native platform:
```
Go to the root directory(libxaac/) of the Ex-HEAAC Decoder.
Create a new folder in the project root directory and move to the newly created folder.

$ cd <path to libxaac>
$ mkdir bin
$ cd bin
$ cmake ..
$ cmake --build .
```

### Creating MSVS project files
To create MSVS project files for the Ex-HEAAC decoder from cmake, run the following commands:
```
Go to the root directory(libxaac/) of the Ex-HEAAC Decoder.
Create a new folder in the project root directory and move to the newly created folder.

$ cd <path to libxaac>
$ mkdir bin
$ cd bin
$ cmake -G "Visual Studio 15 2017" ..
```

The above command creates MSVS 2017 project files. If the version is different, modify the generator name accordingly.
The Ex-HEAAC decoder can be built using these project files.

# Running the Ex-HEAAC Decoder

The Ex-HEAAC Decoder can be run by providing command-line parameters(CLI options) directly or by providing a parameter file as a command line argument.

Command line usage : 
```
<exceutable> -ifile:<input_file> -imeta:<meta_data_file> -ofile:<out_file> [options]

[options] can be,
[-pcmsz:<pcmwordsize>]
[-dmix:<down_mix>]
[-tostereo:<interleave_to_stereo>]
[-dsample:<down_sample_sbr>]
[-fs:<RAW_sample_rate>]
[-nosync:<disable_sync>]
[-sbrup:<auto_sbr_upsample>]
[-maxchannel:<maximum_num_channels>]
[-mp4:<mp4_flag>]

where,
  <inputfile>        	 is the input AAC file name.
  <meta_data_file> 	 	 is a text file which contains metadata for USAC files with an MP4 container.
  <outputfile>       	 is the output file name.
  <pcmwordsize>      	 is the bits per sample info. Only 16 is valid
  <down_mix> 		 	 is to enable/disable always mono output. Default 0.
  <interleave_to_stereo> is to enable/disable always interleaved to stereo output. Default 1.
  <down_sample_sbr> 	 is to enable/disable down-sampled SBR output. Default auto identification from header.
  <RAW_sample_rate> 	 is to indicate the core AAC sample rate for a RAW stream. If this is specified no other file format headers are searched for.
  <disable_sync> 		 is to disable the ADTS/ADIF sync search i.e when enabled the decoder expects the header to be at the start of input buffer. Default 0.
  <auto_sbr_upsample> 	 is to enable(1) or disable(0) auto SBR upsample in case of stream changing from SBR present to SBR not present. Default 1.
  <maximum_num_channels> is the number of maxiumum channels the input may have. Default is 6 (5.1).
  <mp4_flag> 			 is a flag that should be set to 1 when passing raw stream along with meta data text file.

```					 
					 
Sample CLI:
```
<xaac_dec_exe> -ifile:in_file.aac -ofile:out_file.wav -pcmsz:16 					 
```

# Validating the Ex-HEAAC Decoder

Conformance testing for AAC/HEAAC v1/HEAAC v2 mainly involves comparing 
decoder under test output with the ISO and 3GPP reference decoded output.

Testing for USAC is done using encoded streams generated using ISO USAC 
reference encoder. The output generated by ITTIAM USAC decoder is 
compared against the output generated by ISO USAC decoder for 16-bit 
conformance on the respective(ARMv7, ARMv8, X86_32, X86_64) platforms.
