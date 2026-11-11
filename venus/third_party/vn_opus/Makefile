TOP_DIR:=$(TOP_DIR)
ROOT_DIR:=$(TOP_DIR)/../..
MAKEFILE_PATH:=$(subst $(strip \),/,$(abspath $(lastword $(MAKEFILE_LIST))))

OPUS_KERNEL:=src/opus_kenerl
SRC_FILES+= \
  $(OPUS_KERNEL)/celt/bands.c \
  $(OPUS_KERNEL)/celt/celt.c \
  $(OPUS_KERNEL)/celt/celt_encoder.c \
  $(OPUS_KERNEL)/celt/celt_decoder.c \
  $(OPUS_KERNEL)/celt/cwrs.c \
  $(OPUS_KERNEL)/celt/entcode.c \
  $(OPUS_KERNEL)/celt/entdec.c \
  $(OPUS_KERNEL)/celt/entenc.c \
  $(OPUS_KERNEL)/celt/kiss_fft.c \
  $(OPUS_KERNEL)/celt/laplace.c \
  $(OPUS_KERNEL)/celt/mathops.c \
  $(OPUS_KERNEL)/celt/mdct.c \
  $(OPUS_KERNEL)/celt/modes.c \
  $(OPUS_KERNEL)/celt/pitch.c \
  $(OPUS_KERNEL)/celt/celt_lpc.c \
  $(OPUS_KERNEL)/celt/quant_bands.c \
  $(OPUS_KERNEL)/celt/rate.c \
  $(OPUS_KERNEL)/celt/vq.c \
  $(OPUS_KERNEL)/src/analysis.c \
  $(OPUS_KERNEL)/src/mapping_matrix.c \
  $(OPUS_KERNEL)/src/mlp.c \
  $(OPUS_KERNEL)/src/mlp_data.c \
  $(OPUS_KERNEL)/src/opus.c \
  $(OPUS_KERNEL)/src/opus_decoder.c \
  $(OPUS_KERNEL)/src/opus_encoder.c \
  $(OPUS_KERNEL)/src/opus_multistream.c \
  $(OPUS_KERNEL)/src/opus_multistream_encoder.c \
  $(OPUS_KERNEL)/src/opus_multistream_decoder.c \
  $(OPUS_KERNEL)/src/opus_projection_encoder.c \
  $(OPUS_KERNEL)/src/opus_projection_decoder.c \
  $(OPUS_KERNEL)/src/repacketizer.c

SILK_COMMON:=CNG code_signs init_decoder decode_core decode_frame decode_parameters \
  decode_indices decode_pulses decoder_set_fs dec_API enc_API encode_indices \
  encode_pulses gain_quant interpolate LP_variable_cutoff NLSF_decode NSQ \
  NSQ_del_dec PLC shell_coder tables_gain tables_LTP tables_NLSF_CB_NB_MB \
  tables_NLSF_CB_WB tables_other tables_pitch_lag tables_pulses_per_block VAD \
  control_audio_bandwidth quant_LTP_gains VQ_WMat_EC HP_variable_cutoff NLSF_encode \
  NLSF_VQ NLSF_unpack NLSF_del_dec_quant process_NLSFs stereo_LR_to_MS \
  stereo_MS_to_LR check_control_input control_SNR init_encoder control_codec \
  A2NLSF ana_filt_bank_1 biquad_alt bwexpander_32 bwexpander debug decode_pitch \
  inner_prod_aligned lin2log log2lin LPC_analysis_filter LPC_inv_pred_gain \
  table_LSF_cos NLSF2A NLSF_stabilize NLSF_VQ_weights_laroia pitch_est_tables \
  resampler resampler_down2_3 resampler_down2 resampler_private_AR2 \
  resampler_private_down_FIR resampler_private_IIR_FIR resampler_private_up2_HQ \
  resampler_rom sigm_Q15 sort sum_sqr_shift stereo_decode_pred stereo_encode_pred \
  stereo_find_predictor stereo_quant_pred LPC_fit
SRC_FILES+=$(foreach f,$(SILK_COMMON),$(OPUS_KERNEL)/silk/$(f).c)

SILK_FIXED:=LTP_analysis_filter_FIX LTP_scale_ctrl_FIX corrMatrix_FIX \
  encode_frame_FIX find_LPC_FIX find_LTP_FIX find_pitch_lags_FIX find_pred_coefs_FIX \
  noise_shape_analysis_FIX process_gains_FIX regularize_correlations_FIX \
  residual_energy16_FIX residual_energy_FIX warped_autocorrelation_FIX \
  apply_sine_window_FIX autocorr_FIX burg_modified_FIX k2a_FIX k2a_Q16_FIX \
  pitch_analysis_core_FIX vector_ops_FIX schur64_FIX schur_FIX
SRC_FILES+=$(foreach f,$(SILK_FIXED),$(OPUS_KERNEL)/silk/fixed/$(f).c)

INC_DIRS+= \
  -I${TOP_DIR}/venus/third_party/vn_opus/inc \
  -I${TOP_DIR}/venus/third_party/vn_opus/$(OPUS_KERNEL)/src \
  -I${TOP_DIR}/venus/third_party/vn_opus/$(OPUS_KERNEL)/celt \
  -I${TOP_DIR}/venus/third_party/vn_opus/$(OPUS_KERNEL)/silk \
  -I${TOP_DIR}/venus/third_party/vn_opus/$(OPUS_KERNEL)/silk/fixed \
  -I${TOP_DIR}/venus/third_party/vn_opus/$(OPUS_KERNEL)/silk/float

DFLAGS+=-DOPUS_BUILD -DFIXED_POINT -DUSE_ALLOCA -DHAVE_ALLOCA_H -DHAVE_LRINT -DHAVE_LRINTF
U_FLAGS:=
U_CPPFLAGS:=
U_LIBS:=

include ${TOP_DIR}/config/common/makefile.mk
