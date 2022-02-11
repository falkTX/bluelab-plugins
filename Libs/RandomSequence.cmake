set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_RandomSequence INTERFACE)
set(RANDOMSEQUENCE_SRC "${BLUELAB_DEPS}/RandomSequence/")
set(_RandomSequence_src
  randomsequence.h
  )
list(TRANSFORM _RandomSequence_src PREPEND "${RANDOMSEQUENCE_SRC}")
iplug_target_add(_RandomSequence INTERFACE
  INCLUDE ${BLUELAB_DEPS}/RandomSequence
  SOURCE ${_RandomSequence_src}
)
