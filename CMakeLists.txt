cmake_minimum_required(VERSION 3.5.1)
project(libxaac C CXX)
enable_language(ASM)

set(XAAC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(XAAC_CONFIG_DIR "${CMAKE_CURRENT_BINARY_DIR}")
find_package(Threads REQUIRED)

set(CMAKE_STATIC_LIBRARY_PREFIX "")

include("${XAAC_ROOT}/cmake/utils.cmake")

libxaac_add_compile_options()
libxaac_add_definitions()

if(NOT COMPILER_HAS_SANITIZER)
  libxaac_set_link_libraries()
else()
  libxaac_fuzzer_set_link_libraries()
endif()

add_subdirectory(decoder)

if(NOT COMPILER_HAS_SANITIZER)
  add_subdirectory(test)
else()
  add_subdirectory(fuzzer)
endif()

