set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_r8brain INTERFACE)
set(R8BRAIN_SRC "${BLUELAB_DEPS}/r8brain/")
set(_r8brain_src
  CDSPBlockConvolver.h
  CDSPFIRFilter.h
  CDSPFracInterpolator.h
  CDSPHBDownsampler.h
  CDSPHBUpsampler.h
  CDSPProcessor.h
  CDSPRealFFT.h
  CDSPResampler.h
  CDSPSincFilterGen.h
  fft4g.h
  pffft.cpp
  pffft.h
  r8bbase.cpp
  r8bbase.h
  r8bconf.h
  r8butil.h
  )
list(TRANSFORM _r8brain_src PREPEND "${R8BRAIN_SRC}")
iplug_target_add(_r8brain INTERFACE
  INCLUDE ${BLUELAB_DEPS}/r8brain
  SOURCE ${_r8brain_src}
)
