include("${CMAKE_CURRENT_LIST_DIR}/sky-cmsis.cmake")

function(add_sky_hal_example target)
  set(hal "${CUBE_F4_ROOT}/Drivers/STM32F4xx_HAL_Driver")
  if(NOT EXISTS "${hal}/Inc/stm32f4xx_hal_conf_template.h")
    message(FATAL_ERROR "Set CUBE_F4_ROOT to the extracted STM32CubeF4 firmware package.")
  endif()
  configure_file("${hal}/Inc/stm32f4xx_hal_conf_template.h" stm32f4xx_hal_conf.h COPYONLY)

  add_sky_cmsis_example(${target})
  target_sources(${target} PRIVATE
    "${hal}/Src/stm32f4xx_hal.c"
    "${hal}/Src/stm32f4xx_hal_cortex.c"
    "${hal}/Src/stm32f4xx_hal_gpio.c"
    "${hal}/Src/stm32f4xx_hal_rcc.c")
  foreach(driver IN LISTS ARGN)
    target_sources(${target} PRIVATE "${hal}/Src/stm32f4xx_hal_${driver}.c")
  endforeach()
  target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${hal}/Inc")
  target_compile_definitions(${target} PRIVATE USE_HAL_DRIVER)
endfunction()
