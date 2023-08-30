add_library(pico-mpr121 INTERFACE)

target_include_directories(pico-mpr121
  INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(pico-mpr121
  INTERFACE
  hardware_i2c
)

target_sources(pico-mpr121
  INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/mpr121.c
  ${CMAKE_CURRENT_LIST_DIR}/include/mpr121.h
)
