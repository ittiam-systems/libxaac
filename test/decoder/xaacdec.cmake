list(APPEND XAACDEC_SRCS
  "${XAAC_ROOT}/test/decoder/ixheaacd_error.c"
  "${XAAC_ROOT}/test/decoder/ixheaacd_fileifc.c"
  "${XAAC_ROOT}/test/decoder/ixheaacd_main.c"
  "${XAAC_ROOT}/test/decoder/ixheaacd_metadata_read.c"
)

set(LIBXAACDEC_INCLUDES
  ${XAAC_ROOT}/decoder
  ${XAAC_ROOT}/test/decoder/
  ${XAAC_ROOT}/decoder/drc_src
)

if(SUPPORT_MP4)
  set(LIBISOMEDIA_INCLUDES "${XAAC_ROOT}/../isobmff/IsoLib/libisomediafile/src")

  # Platform-specific includes
  if(MSVC)
    set(LIBISOOSW32_INCLUDES "${XAAC_ROOT}/../isobmff/IsoLib/libisomediafile/w32")
    include_directories(${LIBXAACDEC_INCLUDES} ${LIBISOMEDIA_INCLUDES} ${LIBISOOSW32_INCLUDES})
  else()
    set(LIBISOMEDIA_PLATFORM_INCLUDES ${XAAC_ROOT}/../isobmff/IsoLib/libisomediafile/linux)
    include_directories(${LIBXAACDEC_INCLUDES} ${LIBISOMEDIA_INCLUDES} ${LIBISOMEDIA_PLATFORM_INCLUDES})
  endif()
else()
  include_directories(${LIBXAACDEC_INCLUDES})
endif()

# Add xaacdec executable
libxaac_add_executable(xaacdec libxaacdec SOURCES ${XAACDEC_SRCS} INCLUDES ${LIBXAACDEC_INCLUDES})

# Platform-specific linking of MP4 library
if(SUPPORT_MP4)
  if(MSVC)
    # Use Visual Studio project on Windows
    include_external_msproject(libisomediafile
        ${XAAC_ROOT}/../isobmff/build/IsoLib/libisomediafile/libisomediafile.vcxproj)

    add_dependencies(xaacdec libisomediafile)

  else()
    # Linux: link prebuilt static .a library
    set(LIBISOMEDIA_LIB_PATH "${XAAC_ROOT}/../isobmff/lib/liblibisomediafile.a")
    target_link_libraries(xaacdec ${LIBISOMEDIA_LIB_PATH})
  endif()
endif()

# Common compile flags
set(COMMON_FLAGS "-UARM_PROFILE_HW -UARM_PROFILE_BOARD -DDRC_ENABLE -DMULTICHANNEL_ENABLE -DECLIPSE -DWIN32")

if(SUPPORT_MP4)
  set(MP4_FLAG "-DSUPPORT_MP4")
endif()

# Apply compile flags per platform
if(MSVC)
  set_target_properties(
    xaacdec
    PROPERTIES
      COMPILE_FLAGS
      "${COMMON_FLAGS} -D_CRT_SECURE_NO_WARNINGS ${MP4_FLAG}"
  )
else()
  set_target_properties(
    xaacdec
    PROPERTIES
      COMPILE_FLAGS
      "${COMMON_FLAGS} ${MP4_FLAG}"
  )
endif()
