set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_libebur128 INTERFACE)
set(LIBEBUR128_SRC "${BLUELAB_DEPS}/libebur128/ebur128/")
set(_libebur128_src
  ebur128.c
  ebur128.h
  )
list(TRANSFORM _libebur128_src PREPEND "${LIBEBUR128_SRC}")
iplug_target_add(_libebur128 INTERFACE
  INCLUDE ${BLUELAB_DEPS}/libebur128/ebur128
  ${BLUELAB_DEPS}/libebur128/ebur128/queue/sys
  SOURCE ${_libebur128_src}
)
