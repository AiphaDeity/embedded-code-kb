cmake_minimum_required(VERSION 3.20)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
project(sky_uart LANGUAGES C ASM)

set(CUBE_F4_ROOT "" CACHE PATH "Path to STM32Cube_FW_F4_V1.28.3")
set(hal "${CUBE_F4_ROOT}/Drivers/STM32F4xx_HAL_Driver")
set(cmsis "${CUBE_F4_ROOT}/Drivers/CMSIS")
set(device "${cmsis}/Device/ST/STM32F4xx")
set(kernel "${CUBE_F4_ROOT}/Middlewares/Third_Party/FreeRTOS/Source")
set(queue_demo "${CMAKE_CURRENT_SOURCE_DIR}/../../freertos/sky-uart-queue")
if(NOT EXISTS "${hal}/Inc/stm32f4xx_hal_conf_template.h")
  message(FATAL_ERROR "Set CUBE_F4_ROOT to the extracted STM32CubeF4 firmware package.")
endif()
configure_file("${hal}/Inc/stm32f4xx_hal_conf_template.h" stm32f4xx_hal_conf.h COPYONLY)

set(cpu -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
add_compile_options(${cpu} -Og -g3 -ffunction-sections -fdata-sections)
add_compile_definitions(STM32F407xx USE_HAL_DRIVER)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}"
  "${hal}/Inc" "${cmsis}/Include" "${device}/Include")
set(linker_script "${CMAKE_CURRENT_SOURCE_DIR}/../common/stm32f407ve.ld")
add_link_options(${cpu} --specs=nano.specs --specs=nosys.specs
  "-T${linker_script}" -Wl,--gc-sections)

add_library(sky_board OBJECT board.c
  "${device}/Source/Templates/gcc/startup_stm32f407xx.s"
  "${device}/Source/Templates/system_stm32f4xx.c")
foreach(driver hal hal_cortex hal_gpio hal_uart hal_dma hal_rcc)
  target_sources(sky_board PRIVATE "${hal}/Src/stm32f4xx_${driver}.c")
endforeach()

add_executable(sky-bare.elf main.c)
add_executable(sky-queue.elf "${queue_demo}/main.c"
  "${kernel}/tasks.c" "${kernel}/queue.c" "${kernel}/list.c"
  "${kernel}/portable/GCC/ARM_CM4F/port.c" "${kernel}/portable/MemMang/heap_4.c")
target_include_directories(sky-queue.elf PRIVATE "${queue_demo}"
  "${kernel}/include" "${kernel}/portable/GCC/ARM_CM4F")
foreach(target sky-bare.elf sky-queue.elf)
  target_link_libraries(${target} PRIVATE sky_board)
  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${linker_script}")
endforeach()
