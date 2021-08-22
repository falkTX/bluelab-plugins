set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_hungarian INTERFACE)
set(HUNGARIAN_SRC "${BLUELAB_DEPS}/hungarian/")
set(_hungarian_src
  Hungarian.h
  Hungarian.cpp
  )
list(TRANSFORM _hungarian_src PREPEND "${HUNGARIAN_SRC}")
iplug_target_add(_hungarian INTERFACE
  INCLUDE ${BLUELAB_DEPS}/hungarian
  SOURCE ${_hungarian_src}
)
