# src files
list(
  APPEND
  LIBXAACDEC_SRCS
  "${XAAC_ROOT}/decoder/ixheaacd_aacdecoder.c"
  "${XAAC_ROOT}/decoder/ixheaacd_aacpluscheck.c"
  "${XAAC_ROOT}/decoder/ixheaacd_aac_ec.c"
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
  "${XAAC_ROOT}/decoder/ixheaacd_basic_funcs.c"
  "${XAAC_ROOT}/decoder/ixheaacd_basic_ops.c"
  "${XAAC_ROOT}/decoder/ixheaacd_bitbuffer.c"
  "${XAAC_ROOT}/decoder/ixheaacd_block.c"
  "${XAAC_ROOT}/decoder/ixheaacd_channel.c"
  "${XAAC_ROOT}/decoder/ixheaacd_common_initfuncs.c"
  "${XAAC_ROOT}/decoder/ixheaacd_common_lpfuncs.c"
  "${XAAC_ROOT}/decoder/ixheaacd_common_rom.c"
  "${XAAC_ROOT}/decoder/ixheaacd_create.c"
  "${XAAC_ROOT}/decoder/ixheaacd_decode_main.c"
  "${XAAC_ROOT}/decoder/ixheaacd_drc_freq_dec.c"
  "${XAAC_ROOT}/decoder/ixheaacd_dsp_fft32x32s.c"
  "${XAAC_ROOT}/decoder/ixheaacd_ec_rom.c"
  "${XAAC_ROOT}/decoder/ixheaacd_env_calc.c"
  "${XAAC_ROOT}/decoder/ixheaacd_env_dec.c"
  "${XAAC_ROOT}/decoder/ixheaacd_env_extr.c"
  "${XAAC_ROOT}/decoder/ixheaacd_esbr_envcal.c"
  "${XAAC_ROOT}/decoder/ixheaacd_esbr_polyphase.c"
  "${XAAC_ROOT}/decoder/ixheaacd_ext_ch_ele.c"
  "${XAAC_ROOT}/decoder/ixheaacd_fft.c"
  "${XAAC_ROOT}/decoder/ixheaacd_fft_ifft_32x32.c"
  "${XAAC_ROOT}/decoder/ixheaacd_freq_sca.c"
  "${XAAC_ROOT}/decoder/ixheaacd_fwd_alias_cnx.c"
  "${XAAC_ROOT}/decoder/ixheaacd_hbe_dft_trans.c"
  "${XAAC_ROOT}/decoder/ixheaacd_hbe_trans.c"
  "${XAAC_ROOT}/decoder/ixheaacd_headerdecode.c"
  "${XAAC_ROOT}/decoder/ixheaacd_hufftables.c"
  "${XAAC_ROOT}/decoder/ixheaacd_huff_code_reorder.c"
  "${XAAC_ROOT}/decoder/ixheaacd_huff_tools.c"
  "${XAAC_ROOT}/decoder/ixheaacd_hybrid.c"
  "${XAAC_ROOT}/decoder/ixheaacd_imdct.c"
  "${XAAC_ROOT}/decoder/ixheaacd_initfuncs.c"
  "${XAAC_ROOT}/decoder/ixheaacd_init_config.c"
  "${XAAC_ROOT}/decoder/ixheaacd_latmdemux.c"
  "${XAAC_ROOT}/decoder/ixheaacd_ld_mps_config.c"
  "${XAAC_ROOT}/decoder/ixheaacd_ld_mps_dec.c"
  "${XAAC_ROOT}/decoder/ixheaacd_longblock.c"
  "${XAAC_ROOT}/decoder/ixheaacd_lpc.c"
  "${XAAC_ROOT}/decoder/ixheaacd_lpc_dec.c"
  "${XAAC_ROOT}/decoder/ixheaacd_lpfuncs.c"
  "${XAAC_ROOT}/decoder/ixheaacd_lpp_tran.c"
  "${XAAC_ROOT}/decoder/ixheaacd_lt_predict.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_apply_common.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_apply_m1.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_apply_m2.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_bitdec.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_blind.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_calc_m1m2_common.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_calc_m1m2_emm.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_calc_m1m2_tree_515x.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_calc_m1m2_tree_51sx.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_calc_m1m2_tree_52xx.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_calc_m1m2_tree_727x.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_calc_m1m2_tree_757x.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_dec.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_decorr.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_get_index.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_hybrid_filt.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_initfuncs.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_m1m2_common.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_mdct_2_qmf.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_parse.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_polyphase.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_poly_filt.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_pre_mix.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_process.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_reshape_bb_env.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_res_block.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_res_channel.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_res_channel_info.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_res_longblock.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_res_pns_js_thumb.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_res_pulsedata.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_res_tns.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_rom.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_smoothing.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_temp_process.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_temp_reshape.c"
  "${XAAC_ROOT}/decoder/ixheaacd_mps_tonality.c"
  "${XAAC_ROOT}/decoder/ixheaacd_multichannel.c"
  "${XAAC_ROOT}/decoder/ixheaacd_peak_limiter.c"
  "${XAAC_ROOT}/decoder/ixheaacd_pns_js_thumb.c"
  "${XAAC_ROOT}/decoder/ixheaacd_pred_vec_block.c"
  "${XAAC_ROOT}/decoder/ixheaacd_process.c"
  "${XAAC_ROOT}/decoder/ixheaacd_ps_bitdec.c"
  "${XAAC_ROOT}/decoder/ixheaacd_ps_dec.c"
  "${XAAC_ROOT}/decoder/ixheaacd_ps_dec_flt.c"
  "${XAAC_ROOT}/decoder/ixheaacd_pvc_rom.c"
  "${XAAC_ROOT}/decoder/ixheaacd_qmf_dec.c"
  "${XAAC_ROOT}/decoder/ixheaacd_rev_vlc.c"
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
  "${XAAC_ROOT}/decoder/ixheaacd_usac_ec.c"
  "${XAAC_ROOT}/decoder/ixheaacd_Windowing.c")

set(LIBXAACDEC_INCLUDES ${XAAC_ROOT}/decoder ${XAAC_ROOT}/decoder/drc_src)
include_directories(${LIBXAACDEC_INCLUDES})

include("${XAAC_ROOT}/decoder/drc_src/libxaacdec_drc.cmake")

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch32")
  include("${XAAC_ROOT}/decoder/armv7/libxaacdec_armv7.cmake")
elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
  include("${XAAC_ROOT}/decoder/armv8/libxaacdec_armv8.cmake")
else()
  include("${XAAC_ROOT}/decoder/x86/libxaacdec_x86.cmake")
endif()

