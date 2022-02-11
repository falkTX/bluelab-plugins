set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_fmem INTERFACE)
set(FMEM_SRC "${BLUELAB_DEPS}/fmem/src/")
set(_fmem_src
  alloc.h
  alloc.c
  fmem-fopencookie.c
  )
list(TRANSFORM _fmem_src PREPEND "${FMEM_SRC}")
iplug_target_add(_fmem INTERFACE
  INCLUDE ${BLUELAB_DEPS}/fmem/include
  SOURCE ${_fmem_src}
)
