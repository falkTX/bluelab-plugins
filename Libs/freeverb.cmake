set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_freeverb INTERFACE)
set(FREEVERB_SRC "${BLUELAB_DEPS}/freeverb/")
set(_freeverb_src
  allpass.hpp
  comb.hpp
  denormals.h
  Makefile
  readme.txt
  revmodel.cpp
  revmodel.hpp
  tuning.h
  )
list(TRANSFORM _freeverb_src PREPEND "${FREEVERB_SRC}")
iplug_target_add(_freeverb INTERFACE
  INCLUDE ${BLUELAB_DEPS}/freeverb
  SOURCE ${_freeverb_src}
)
