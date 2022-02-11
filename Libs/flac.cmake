set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_flac INTERFACE)
#set(FLAC_SRC "${BLUELAB_DEPS}/flac/src")
#set(_flac_src)
#list(TRANSFORM _dsp_cpp_filters_src PREPEND "${DSP_CPP_FILTERS_SRC}")
iplug_target_add(_flac INTERFACE
  INCLUDE ${BLUELAB_DEPS}/flac/include
  #SOURCE ${_flac_src}
  LINK ${BLUELAB_DEPS}/flac/lib/linux/libFLAC-static.a
)
