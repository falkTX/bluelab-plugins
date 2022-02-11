set(BLUELAB_DEPS ${IPLUG2_DIR}/BL-Dependencies)

#
add_library(_crossover_filter INTERFACE)
set(CROSSOVER_FILTER_SRC "${BLUELAB_DEPS}/CrossoverFilter/")
set(_crossover_filter_src
  CFxRbjFilter.h
  LinkwitzRileyTwo.h
  )
list(TRANSFORM _crossover_filter_src PREPEND "${CROSSOVER_FILTER_SRC}")
iplug_target_add(_crossover_filter INTERFACE
  INCLUDE ${BLUELAB_DEPS}/CrossoverFilter
  SOURCE ${_crossover_filter_src}
)
