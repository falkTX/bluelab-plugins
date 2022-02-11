set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_dsp_cpp_filters INTERFACE)
set(DSP_CPP_FILTERS_SRC "${BLUELAB_DEPS}/DSP-Cpp-filters/src/")
set(_dsp_cpp_filters_src
  filter_common.h
  filter_includes.h
  fo_apf.cpp
  fo_apf.h
  fo_hpf.cpp
  fo_hpf.h
  fo_lpf.cpp
  fo_lpf.h
  fo_shelving_high.cpp
  fo_shelving_high.h
  fo_shelving_low.cpp
  fo_shelving_low.h
  so_apf2.cpp
  so_apf2.h
  so_apf.cpp
  so_apf.h
  so_bpf.cpp
  so_bpf.h
  so_bsf.cpp
  so_bsf.h
  so_butterworth_bpf.cpp
  so_butterworth_bpf.h
  so_butterworth_bsf.cpp
  so_butterworth_bsf.h
  so_butterworth_hpf.cpp
  so_butterworth_hpf.h
  so_butterworth_lpf.cpp
  so_butterworth_lpf.h
  so_hpf.cpp
  so_hpf.h
  so_linkwitz_riley_hpf.cpp
  so_linkwitz_riley_hpf.h
  so_linkwitz_riley_lpf.cpp
  so_linkwitz_riley_lpf.h
  so_lpf.cpp
  so_lpf.h
  so_parametric_cq_boost.cpp
  so_parametric_cq_boost.h
  so_parametric_cq_cut.cpp
  so_parametric_cq_cut.h
  so_parametric_ncq.cpp
  so_parametric_ncq.h
  )
list(TRANSFORM _dsp_cpp_filters_src PREPEND "${DSP_CPP_FILTERS_SRC}")
iplug_target_add(_dsp_cpp_filters INTERFACE
  INCLUDE ${BLUELAB_DEPS}/DSP-Cpp-filters/src
  SOURCE ${_dsp_cpp_filters_src}
)
