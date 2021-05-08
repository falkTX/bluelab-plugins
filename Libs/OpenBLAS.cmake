set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_OpenBLAS INTERFACE)
iplug_target_add(_OpenBLAS INTERFACE
  INCLUDE ${BLUELAB_DEPS}/OpenBLAS
  LINK ${BLUELAB_DEPS}/OpenBLAS/libopenblas_sandybridge-r0.3.9_linux.a
)
