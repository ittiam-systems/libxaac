list(APPEND XAACDEC_SRCS "${XAAC_ROOT}/test/decoder/ixheaacd_error.c"
     "${XAAC_ROOT}/test/decoder/ixheaacd_fileifc.c" "${XAAC_ROOT}/test/decoder/ixheaacd_main.c"
     "${XAAC_ROOT}/test/decoder/ixheaacd_metadata_read.c")

set(LIBXAACDEC_INCLUDES ${XAAC_ROOT}/decoder ${XAAC_ROOT}/test/decoder/
                        ${XAAC_ROOT}/decoder/drc_src)

if(SUPPORT_MP4)
  set(LIBISOMEDIA_INCLUDES $(XAAC_ROOT)/../isobmff/IsoLib/libisomediafile/src)

  set(LIBISOOSW32_INCLUDES $(XAAC_ROOT)/../isobmff/IsoLib/libisomediafile/w32)

  include_directories(${LIBXAACDEC_INCLUDES} ${LIBISOMEDIA_INCLUDES} ${LIBISOOSW32_INCLUDES})
else()
  include_directories(${LIBXAACDEC_INCLUDES})
endif()

libxaac_add_executable(xaacdec libxaacdec SOURCES ${XAACDEC_SRCS} INCLUDES
                       ${LIBXAACDEC_INCLUDES})

if(SUPPORT_MP4)
  include_external_msproject(libisomediafile
      ${XAAC_ROOT}/../isobmff/build/IsoLib/libisomediafile/libisomediafile.vcxproj)

  add_dependencies(xaacdec libisomediafile)
endif()

set(COMMON_FLAGS "-UARM_PROFILE_HW -UARM_PROFILE_BOARD -DDRC_ENABLE -DMULTICHANNEL_ENABLE -DECLIPSE -DWIN32 -DLOUDNESS_LEVELING_SUPPORT")

if(SUPPORT_MP4)
  set(MP4_FLAG "-DSUPPORT_MP4")
endif()

if(MSVC)
  set_target_properties(
    xaacdec
    PROPERTIES
      COMPILE_FLAGS
      "-UARM_PROFILE_HW -UARM_PROFILE_BOARD -DDRC_ENABLE -DMULTICHANNEL_ENABLE -DECLIPSE -DWIN32 -D_CRT_SECURE_NO_WARNINGS ${MP4_FLAG}"
  )
else()
  set_target_properties(
    xaacdec
    PROPERTIES
      COMPILE_FLAGS
      "-UARM_PROFILE_HW -UARM_PROFILE_BOARD -DDRC_ENABLE -DMULTICHANNEL_ENABLE -DECLIPSE -DWIN32 ${MP4_FLAG}"
  )
endif()
