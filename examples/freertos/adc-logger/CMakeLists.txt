cmake_minimum_required(VERSION 3.20)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
project(sky_freertos_adc_logger LANGUAGES C ASM)
include("../common/sky-freertos.cmake")
add_sky_freertos_example(sky-freertos-adc-logger.elf)
target_sources(sky-freertos-adc-logger.elf PRIVATE
  "${CUBE_F4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_adc.c"
  "${CUBE_F4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c")
