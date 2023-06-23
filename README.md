# Introduction to Ex-HEAAC (libxaac)

Ex-HEAAC, the latest innovation member of the MPEG AAC codec family, is ideally suited for adaptive bit rate streaming and digital radio applications. Ex-HEAAC bridges the gap between speech and audio coding and ensures consistent high-quality audio for all signal types, including speech, music, and mixed material. It is the required audio codec for DRM (Digital Radio Mondiale). When it comes to coding, the codec is incredibly effective, generating high-quality audio for music and speech at bitrates as low as 6 kbit/s for mono and 12 kbit/s for stereo services. By switching to extremely low bitrate streams, Ex-HEAAC streaming apps and streaming radio players can provide uninterrupted playback even during very congested network conditions.

As the Extended High Efficiency AAC Profile is a logical evolution of the MPEG Audio's popular AAC Family profiles, the codec supports AAC-LC, HE-AACv1 (AAC+) and HE-AACv2 (eAAC+) audio object type encoding. The bitrate that was saved with AAC family tools can be used to enhance video quality. Ex-HEAAC is a well-liked option for a number of applications since it is a strong and effective audio codec that provides high-quality audio at low bitrates.

![Architecture](docs/Exheaac_Block_Diagram.jpg)

One of the key features of Ittiam's Ex-HEAAC (refer to above image) is that it has support for AAC-LD (Low Delay), AAC-ELD (Enhanced Low Delay), and AAC-ELDv2 (Enhanced Low Delay version 2) modes. AAC-LD mode provides low latency encoding, making it suitable for applications such as interactive communication and live audio streaming. It helps to reduce the delay in the encoding process to improve the real-time performance of the system. AAC-ELD mode improves the low-delay performance of HE-AAC by reducing the coding delay while maintaining high audio quality. It was observed that minimum delay it can achieve is 15ms. In order to achieve low delay coding scheme and low bitrate, it uses the Low Delay SBR tool. AAC-ELDv2 is the most advanced version of AAC-based low delay coding. It provides an enhanced version of AAC-ELD, which provides even lower coding delay and higher audio quality.

Overall, Ittiam's Ex-HEAAC, with support for AAC-LD, AAC-ELD, and AAC-ELDv2 modes, is a versatile audio coding technology that can be used for a wide range of applications, such as broadcasting, streaming, and teleconferencing which requires high-quality audio compression with minimal delay. 

#### Note
* MPEG-D USAC (Unified Speech and Audio Coding) support for Ittiam's Ex-HEAAC Encoder will be coming soon, which will help in improved audio quality at reduced bitrates. USAC technology will provide efficient and high quality compression of speech and audio signals, making it a valuable addition to our Ex-HEAAC capabilities and features.
* Further Quality enhancements for AAC-ELD and AAC-ELDv2 modes may be pushed as quality assessment is in progress.

#  Building the Ex-HEAAC Decoder and Encoder

## Building for AOSP
* Makefile for building the Ex-HEAAC decoder and encoder library is provided in root(`libxaac/`) folder.
* Makefile for building the Ex-HEAAC decoder and encoder testbench is provided in `test` folder.
* Build the library followed by the application using the below commands:
Go to root directory
```
$ mm
```

## Using CMake
Users can also use cmake to build for `x86`, `x86_64`, `armv7`, `armv8` and Windows (MSVS project) platforms.

### Creating MSVS project files
To create MSVS project files for the Ex-HEAAC decoder and encoder from cmake, run the following commands:
```
Go to the root directory(libxaac/).
Create a new folder in the project root directory and move to the newly created folder.

$ cd <path to libxaac>
$ mkdir bin
$ cd bin
$ cmake -G "Visual Studio 15 2017" ..
```

Above command will create Win32 version of MSVS workspace 
To create MSVS project files for Win64 version from cmake, run the following commands:
```
$ mkdir cmake_build
$ cd cmake_build
$ cmake -G "Visual Studio 15 2017 Win64" ..
```
The above command creates MSVS 2017 project files. If the version is different, modify the generator name accordingly.

### Building for native platforms
Run the following commands to build the Ex-HEAAC decoder and encoder for native platform:
```
Go to the root directory(libxaac/).
Create a new folder in the project root directory and move to the newly created folder.

$ cd <path to libxaac>
$ mkdir bin
$ cd bin
$ cmake ..
$ make
```

### Cross-compiler based builds
Ensure to edit the file cmake/toolchains/*_toolchain.cmake to set proper paths in host for corresponding platforms.

### Building for x86_32 on a x86_64 Linux machine
```
$ cd <path to libxaac>
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/x86_toolchain.cmake
$ make
```

### Building for aarch32/aarch64
Update 'CMAKE_C_COMPILER', 'CMAKE_CXX_COMPILER', 'CMAKE_C_COMPILER_AR', and 'CMAKE_CXX_COMPILER_AR' in CMAKE_TOOLCHAIN_FILE passed below
```
$ cd <path to libxaac>
$ mkdir build
$ cd build
```

### For aarch64
```
$ cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/aarch64_toolchain.cmake
$ make
```

### For aarch32
```
$ cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/aarch32_toolchain.cmake
$ make
```

### For API and testbench usage of decoder, please refer [`README_dec.md`](README_dec.md)
### For API and testbench usage of encoder, please refer [`README_enc.md`](README_enc.md)
