list(APPEND XAACDEC_SRCS
    "${XAAC_ROOT}/test/ixheaacd_error.c"
    "${XAAC_ROOT}/test/ixheaacd_fileifc.c"
    "${XAAC_ROOT}/test/ixheaacd_main.c"
    "${XAAC_ROOT}/test/ixheaacd_metadata_read.c")

set(LIBXAACDEC_INCLUDES ${XAAC_ROOT}/decoder ${XAAC_ROOT}/test ${XAAC_ROOT}/decoder/drc_src)

include_directories(${LIBXAACDEC_INCLUDES})

add_executable(xaacdec ${XAACDEC_SRCS})

if(MSVC)
  target_link_libraries(xaacdec LINK_PUBLIC libxaacdec)
else()
  target_link_libraries(xaacdec LINK_PUBLIC libxaacdec m)
endif()

set_target_properties(
  xaacdec
  PROPERTIES
  COMPILE_FLAGS
  "-UARM_PROFILE_HW -UARM_PROFILE_BOARD -DDRC_ENABLE -DMULTICHANNEL_ENABLE -DECLIPSE -DWIN32"
)
