set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_fastapprox INTERFACE)
set(FASTAPPROX_SRC "${BLUELAB_DEPS}/fastapprox/src/")
set(_fastapprox_src
  cast.h
  fasterf.h
  fastexp.h
  fastgamma.h
  fasthyperbolic.h
  fastlambertw.h
  fastlog.h
  fastonebigheader.h
  fastpow.h
  fastsigmoid.h
  fasttrig.h
  sse.h
  )
list(TRANSFORM _fastapprox_src PREPEND "${FASTAPPROX_SRC}")
iplug_target_add(_fastapprox INTERFACE
  INCLUDE ${BLUELAB_DEPS}/fastapprox/src
  SOURCE ${_fastapprox_src}
)
