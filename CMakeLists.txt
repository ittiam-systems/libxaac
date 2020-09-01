cmake_minimum_required(VERSION 3.5)

set(XAAC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(XAAC_CONFIG_DIR "${CMAKE_CURRENT_BINARY_DIR}")

if("${XAAC_ROOT}" STREQUAL "${XAAC_CONFIG_DIR}")
  message(
    FATAL_ERROR
      "Building from within the libxaac source tree is not supported.\n"
      "Hint: Run these commands\n"
      "$ rm -rf CMakeCache.txt CMakeFiles\n"
      "$ mkdir -p ./build\n"
      "$ cd ./build\n"
      "And re-run CMake from the build directory.")
endif()

set(CMAKE_STATIC_LIBRARY_PREFIX "")

if(SANITIZE)
  string(TOLOWER ${SANITIZE} SANITIZE)

  set(CMAKE_SANITIZER_C_FLAGS "-fno-omit-frame-pointer -fsanitize=${SANITIZE}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_SANITIZER_C_FLAGS}")
endif()

list(
  APPEND
    LIBXAACDEC_SRCS
    "${XAAC_ROOT}/decoder/ixheaacd_aacdecoder.c"
    "${XAAC_ROOT}/decoder/ixheaacd_aacpluscheck.c"
    "${XAAC_ROOT}/decoder/ixheaacd_aac_imdct.c"
    "${XAAC_ROOT}/decoder/ixheaacd_aac_rom.c"
    "${XAAC_ROOT}/decoder/ixheaacd_aac_tns.c"
    "${XAAC_ROOT}/decoder/ixheaacd_acelp_bitparse.c"
    "${XAAC_ROOT}/decoder/ixheaacd_acelp_decode.c"
    "${XAAC_ROOT}/decoder/ixheaacd_acelp_mdct.c"
    "${XAAC_ROOT}/decoder/ixheaacd_acelp_tools.c"
    "${XAAC_ROOT}/decoder/ixheaacd_adts_crc_check.c"
    "${XAAC_ROOT}/decoder/ixheaacd_api.c"
    "${XAAC_ROOT}/decoder/ixheaacd_arith_dec.c"
    "${XAAC_ROOT}/decoder/ixheaacd_avq_dec.c"
    "${XAAC_ROOT}/decoder/ixheaacd_avq_rom.c"
    "${XAAC_ROOT}/decoder/ixheaacd_basic_ops.c"
    "${XAAC_ROOT}/decoder/ixheaacd_bitbuffer.c"
    "${XAAC_ROOT}/decoder/ixheaacd_block.c"
    "${XAAC_ROOT}/decoder/ixheaacd_channel.c"
    "${XAAC_ROOT}/decoder/ixheaacd_common_initfuncs.c"
    "${XAAC_ROOT}/decoder/ixheaacd_common_lpfuncs.c"
    "${XAAC_ROOT}/decoder/ixheaacd_common_rom.c"
    "${XAAC_ROOT}/decoder/ixheaacd_create.c"
    "${XAAC_ROOT}/decoder/ixheaacd_decode_main.c"
    "${XAAC_ROOT}/decoder/ixheaacd_dsp_fft32x32s.c"
    "${XAAC_ROOT}/decoder/ixheaacd_env_calc.c"
    "${XAAC_ROOT}/decoder/ixheaacd_env_dec.c"
    "${XAAC_ROOT}/decoder/ixheaacd_env_extr.c"
    "${XAAC_ROOT}/decoder/ixheaacd_esbr_envcal.c"
    "${XAAC_ROOT}/decoder/ixheaacd_esbr_polyphase.c"
    "${XAAC_ROOT}/decoder/ixheaacd_esbr_rom.c"
    "${XAAC_ROOT}/decoder/ixheaacd_esbr_fft.c"
    "${XAAC_ROOT}/decoder/ixheaacd_ext_ch_ele.c"
    "${XAAC_ROOT}/decoder/ixheaacd_fft.c"
    "${XAAC_ROOT}/decoder/ixheaacd_freq_sca.c"
    "${XAAC_ROOT}/decoder/ixheaacd_fwd_alias_cnx.c"
    "${XAAC_ROOT}/decoder/ixheaacd_hbe_trans.c"
    "${XAAC_ROOT}/decoder/ixheaacd_headerdecode.c"
    "${XAAC_ROOT}/decoder/ixheaacd_hufftables.c"
    "${XAAC_ROOT}/decoder/ixheaacd_huff_tools.c"
    "${XAAC_ROOT}/decoder/ixheaacd_hybrid.c"
    "${XAAC_ROOT}/decoder/ixheaacd_imdct.c"
    "${XAAC_ROOT}/decoder/ixheaacd_initfuncs.c"
    "${XAAC_ROOT}/decoder/ixheaacd_init_config.c"
    "${XAAC_ROOT}/decoder/ixheaacd_longblock.c"
    "${XAAC_ROOT}/decoder/ixheaacd_lpc.c"
    "${XAAC_ROOT}/decoder/ixheaacd_lpc_dec.c"
    "${XAAC_ROOT}/decoder/ixheaacd_lpfuncs.c"
    "${XAAC_ROOT}/decoder/ixheaacd_lpp_tran.c"
    "${XAAC_ROOT}/decoder/ixheaacd_lt_predict.c"
    "${XAAC_ROOT}/decoder/ixheaacd_mps_dec.c"
    "${XAAC_ROOT}/decoder/ixheaacd_mps_decorr.c"
    "${XAAC_ROOT}/decoder/ixheaacd_mps_hybrid_filt.c"
    "${XAAC_ROOT}/decoder/ixheaacd_mps_parse.c"
    "${XAAC_ROOT}/decoder/ixheaacd_mps_pre_mix.c"
    "${XAAC_ROOT}/decoder/ixheaacd_mps_rom.c"
    "${XAAC_ROOT}/decoder/ixheaacd_mps_smoothing.c"
    "${XAAC_ROOT}/decoder/ixheaacd_mps_temp_process.c"
    "${XAAC_ROOT}/decoder/ixheaacd_mps_temp_reshape.c"
    "${XAAC_ROOT}/decoder/ixheaacd_pns_js_thumb.c"
    "${XAAC_ROOT}/decoder/ixheaacd_pred_vec_block.c"
    "${XAAC_ROOT}/decoder/ixheaacd_process.c"
    "${XAAC_ROOT}/decoder/ixheaacd_ps_bitdec.c"
    "${XAAC_ROOT}/decoder/ixheaacd_ps_dec.c"
    "${XAAC_ROOT}/decoder/ixheaacd_pvc_rom.c"
    "${XAAC_ROOT}/decoder/ixheaacd_rom.c"
    "${XAAC_ROOT}/decoder/ixheaacd_sbrdecoder.c"
    "${XAAC_ROOT}/decoder/ixheaacd_sbrdec_initfuncs.c"
    "${XAAC_ROOT}/decoder/ixheaacd_sbrdec_lpfuncs.c"
    "${XAAC_ROOT}/decoder/ixheaacd_sbr_crc.c"
    "${XAAC_ROOT}/decoder/ixheaacd_sbr_dec.c"
    "${XAAC_ROOT}/decoder/ixheaacd_sbr_rom.c"
    "${XAAC_ROOT}/decoder/ixheaacd_spectrum_dec.c"
    "${XAAC_ROOT}/decoder/ixheaacd_stereo.c"
    "${XAAC_ROOT}/decoder/ixheaacd_tcx_fwd_alcnx.c"
    "${XAAC_ROOT}/decoder/ixheaacd_tcx_fwd_mdct.c"
    "${XAAC_ROOT}/decoder/ixheaacd_thumb_ps_dec.c"
    "${XAAC_ROOT}/decoder/ixheaacd_tns.c"
    "${XAAC_ROOT}/decoder/ixheaacd_basic_funcs.c"
    "${XAAC_ROOT}/decoder/ixheaacd_Windowing.c"
    "${XAAC_ROOT}/decoder/ixheaacd_latmdemux.c"
    "${XAAC_ROOT}/decoder/ixheaacd_multichannel.c"
    "${XAAC_ROOT}/decoder/ixheaacd_drc_freq_dec.c"
    "${XAAC_ROOT}/decoder/ixheaacd_mps_poly_filt.c"
    "${XAAC_ROOT}/decoder/ixheaacd_huff_code_reorder.c"
    "${XAAC_ROOT}/decoder/ixheaacd_rev_vlc.c"
    "${XAAC_ROOT}/decoder/ixheaacd_qmf_dec.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_api.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_bitbuffer.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_dec.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_dynamic_payload.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_eq.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_extr_delta_coded_info.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_filter_bank.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_gain_dec.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_gain_decoder.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_init.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_interface_decoder.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_loudness_control.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_main_td_process.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_multiband.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_parametric_dec.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_peak_limiter.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_process.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_rom.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_selection_process.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_selection_process_drcset_selection.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_selection_process_init.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_shape_filter.c"
    "${XAAC_ROOT}/decoder/drc_src/impd_drc_static_payload.c")

list(APPEND LIBXAACDEC_X86_SRCS
            "${XAAC_ROOT}/decoder/generic/ixheaacd_qmf_dec_generic.c"
            "${XAAC_ROOT}/decoder/x86/ixheaacd_function_selector_x86.c")

set(LIBXAACDEC_INCLUDES ${XAAC_ROOT}/decoder ${XAAC_ROOT}/decoder/drc_src)

set(LIBXAACDEC_C_FLAGS "${LIBXAACDEC_X86_C_FLAGS}")

include_directories(${LIBXAACDEC_INCLUDES} ${LIBXAACDEC_X86_INCLUDES})
add_library(libxaacdec ${LIBXAACDEC_SRCS} ${LIBXAACDEC_X86_SRCS})
set_target_properties(libxaacdec
                      PROPERTIES COMPILE_FLAGS "${LIBXAACDEC_C_FLAGS}")

list(APPEND XAACDEC_SRCS
            "${XAAC_ROOT}/test/ixheaacd_error.c"
            "${XAAC_ROOT}/test/ixheaacd_fileifc.c"
            "${XAAC_ROOT}/test/ixheaacd_main.c"
            "${XAAC_ROOT}/test/ixheaacd_metadata_read.c")

add_executable(xaacdec ${XAACDEC_SRCS})

target_link_libraries(xaacdec libxaacdec m)
set_target_properties(
  xaacdec
  PROPERTIES
    COMPILE_FLAGS
    "-UARM_PROFILE_HW -UARM_PROFILE_BOARD -DECLIPSE -DDRC_ENABLE -DMULTICHANNEL_ENABLE -DENABLE_DRC"
  )
