set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_ini_parser INTERFACE)
set(INI_PARSER_SRC "${BLUELAB_DEPS}/ini_parser/")
set(_ini_parser_src
  ini.h
  )
list(TRANSFORM _ini_parser_src PREPEND "${INI_PARSER_SRC}")
iplug_target_add(_ini_parser INTERFACE
  INCLUDE ${BLUELAB_DEPS}/ini_parser/
  SOURCE ${_ini_parser_src}
)
