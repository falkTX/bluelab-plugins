set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_sndfilter INTERFACE)
set(SNDFILTER_SRC "${BLUELAB_DEPS}/sndfilter/src/")
set(_sndfilter_src
  biquad.c
  biquad.h
  compressor.c
  compressor.h
  mem.c
  mem.h
  reverb.c
  reverb.h
  snd.c
  sndfilter-types.h
  snd.h
  wav.c
  wav.h
  )
list(TRANSFORM _sndfilter_src PREPEND "${SNDFILTER_SRC}")
iplug_target_add(_sndfilter INTERFACE
  INCLUDE ${BLUELAB_DEPS}/sndfilter/src
  ${BLUELAB_DEPS}/sndfilter/src
  SOURCE ${_sndfilter_src}
)
