# include old library and common
include("${XAAC_ROOT}/decoder/libxaacdec.cmake")
include("${XAAC_ROOT}/encoder/libxaacenc.cmake")
include("${XAAC_ROOT}/common/common.cmake")

# generate object files without linking
add_library(xaacobjlib OBJECT ${LIBXAACENC_SRCS} ${LIBXAAC_COMMON_SRCS} ${LIBXAACDEC_SRCS} ${LIBXAACCDEC_ASMS})

# PIC for shared libraries
set_property(TARGET xaacobjlib PROPERTY POSITION_INDEPENDENT_CODE 1)

# build from same object files
add_library(xaac SHARED $<TARGET_OBJECTS:xaacobjlib>)
add_library(xaac_static STATIC $<TARGET_OBJECTS:xaacobjlib>)