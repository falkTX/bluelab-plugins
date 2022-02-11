set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_glm INTERFACE)
#set(GLM_SRC "${BLUELAB_DEPS}/glm")
#set(_glm_src)
#list(TRANSFORM _glm_src PREPEND "${GLM_SRC}")
iplug_target_add(_glm INTERFACE
  INCLUDE ${BLUELAB_DEPS}/glm
  #SOURCE ${_glm_src}
)
