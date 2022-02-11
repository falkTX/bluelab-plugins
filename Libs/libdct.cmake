set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_libdct INTERFACE)
set(LIBDCT_SRC "${BLUELAB_DEPS}/libdct/")
set(_libdct_src
  fast-dct-8.c
  fast-dct-8.h
  #fast-dct-fft.c
  #fast-dct-fft.h
  fast-dct-lee.c
  fast-dct-lee.h
  libdct_defs.h
  naive-dct.c
  naive-dct.h
  )
list(TRANSFORM _libdct_src PREPEND "${LIBDCT_SRC}")
iplug_target_add(_libdct INTERFACE
  INCLUDE ${BLUELAB_DEPS}/libdct
  SOURCE ${_libdct_src}
)
