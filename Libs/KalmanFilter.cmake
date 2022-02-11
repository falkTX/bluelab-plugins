set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_kalman_filter INTERFACE)
set(KALMAN_FILTER_SRC "${BLUELAB_DEPS}/KalmanFilter/src/")
set(_kalman_filter_src
  SimpleKalmanFilter.cpp
  SimpleKalmanFilter.h
  )
list(TRANSFORM _kalman_filter_src PREPEND "${KALMAN_FILTER_SRC}")
iplug_target_add(_kalman_filter INTERFACE
  INCLUDE ${BLUELAB_DEPS}/KalmanFilter/src
  SOURCE ${_kalman_filter_src}
)
