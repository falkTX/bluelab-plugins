set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_decimator INTERFACE)
set(DECIMATOR_SRC "${BLUELAB_DEPS}/Decimator/")
set(_decimator_src
  Decimator.h
  )
list(TRANSFORM _decimator_src PREPEND "${DECIMATOR_SRC}")
iplug_target_add(_decimator INTERFACE
  INCLUDE ${BLUELAB_DEPS}/Decimator
  SOURCE ${_darknet_src}
)
