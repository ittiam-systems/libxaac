include(CheckCXXCompilerFlag)
set(CMAKE_C_STANDARD 99)
# Adds compiler options for all targets
function(libxaac_add_compile_options)
  if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    add_compile_options(-std=gnu99 -march=armv8-a)
  elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch32")
    add_compile_options(-O3 -Wall -std=c99 -mcpu=cortex-a8 -march=armv7-a -mfloat-abi=softfp -mfpu=neon)
  elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i686" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    add_compile_options(-O3 -Wall -Wsequence-point -fwrapv)
  endif()

  set(CMAKE_REQUIRED_FLAGS -fsanitize=fuzzer-no-link)
  check_cxx_compiler_flag(-fsanitize=fuzzer-no-link
                          COMPILER_HAS_SANITIZE_FUZZER)
  unset(CMAKE_REQUIRED_FLAGS)

  if(DEFINED SANITIZE)
    set(CMAKE_REQUIRED_FLAGS -fsanitize=${SANITIZE})
    check_cxx_compiler_flag(-fsanitize=${SANITIZE} COMPILER_HAS_SANITIZER)
    unset(CMAKE_REQUIRED_FLAGS)

    if(NOT COMPILER_HAS_SANITIZER)
      message(
        FATAL_ERROR "ERROR: Compiler doesn't support -fsanitize=${SANITIZE}")
      return()
    endif()
    add_compile_options(-fno-omit-frame-pointer -fsanitize=${SANITIZE})
  endif()

endfunction()

# Adds defintions for all targets
function(libxaac_add_definitions)
  if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    add_definitions(-DARMV8 -DDEFAULT_ARCH=D_ARCH_ARMV8_GENERIC)
  elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch32")
    add_definitions(-DARMV7 -DDEFAULT_ARCH=D_ARCH_ARM_A9Q)
  elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "i686")
    add_definitions(-DX86 -DX86_LINUX=1 -DDISABLE_AVX2 -DDEFAULT_ARCH=D_ARCH_X86_SSE42)
  endif()
endfunction()

# Adds libraries needed for executables
function(libxaac_set_link_libraries)
  if(NOT MSVC)
    link_libraries(Threads::Threads --static)
  endif()
endfunction()

#Adds libraries for fuzzer
function(libxaac_fuzzer_set_link_libraries)
  link_libraries(Threads::Threads m)
endfunction()