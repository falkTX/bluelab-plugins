set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_RTConvolve INTERFACE)
set(RTCONVOLVE_SRC "${BLUELAB_DEPS}/RTConvolve/Source/")
set(_RTConvolve_src
  ConvolutionManager.h
  TimeDistributedFFTConvolver.h
  TimeDistributedFFTConvolver.hpp
  UniformPartitionConvolver.h
  UniformPartitionConvolver.hpp
  #util/fft.hpp
  #util/SincFilter.hpp
  #util/util.cpp
  #util/util.h
  )
list(TRANSFORM _RTConvolve_src PREPEND "${RTCONVOLVE_SRC}")
iplug_target_add(_RTConvolve INTERFACE
  INCLUDE ${BLUELAB_DEPS}/RTConvolve/Source
  ${BLUELAB_DEPS}/RTConvolve/Source/util
  SOURCE ${_RTConvolve_src}
)
