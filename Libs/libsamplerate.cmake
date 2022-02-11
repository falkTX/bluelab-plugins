set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_libsamplerate INTERFACE)
set(LIBSAMPLERATE_SRC "${BLUELAB_DEPS}/libsamplerate/src/")
set(_libsamplerate_src
  common.h
  config.h
  fastest_coeffs.h
  float_cast.h
  high_qual_coeffs.h
  mid_qual_coeffs.h
  samplerate.c
  samplerate.h
  src_linear.c
  src_sinc.c
  src_zoh.c
  )
list(TRANSFORM _libsamplerate_src PREPEND "${LIBSAMPLERATE_SRC}")
iplug_target_add(_libsamplerate INTERFACE
  INCLUDE ${BLUELAB_DEPS}/libsamplerate/src
  SOURCE ${_libsamplerate_src}
)
