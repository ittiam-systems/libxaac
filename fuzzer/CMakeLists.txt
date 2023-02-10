list(APPEND XAACDEC_SRCS
    "${XAAC_ROOT}/fuzzer/xaac_dec_fuzzer.cpp")

set(LIBXAACDEC_INCLUDES ${XAAC_ROOT}/decoder ${XAAC_ROOT}/test ${XAAC_ROOT}/decoder/drc_src)

include_directories(${LIBXAACDEC_INCLUDES})

add_executable(xaac_dec_fuzzer ${XAACDEC_SRCS})

if(MSVC)
  target_link_libraries(xaac_dec_fuzzer LINK_PUBLIC libxaacdec)
else()
  target_link_libraries(xaac_dec_fuzzer LINK_PUBLIC libxaacdec m)
endif()

set_target_properties(
  xaac_dec_fuzzer
  PROPERTIES
  LINK_FLAGS
  -fsanitize=fuzzer,${SANITIZE}
)