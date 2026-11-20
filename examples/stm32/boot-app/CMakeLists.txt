cmake_minimum_required(VERSION 3.20)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
project(sky_boot_app LANGUAGES C ASM)

include("../common/sky-cmsis.cmake")
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_subdirectory(boot)
add_subdirectory(app)

set(image "${CMAKE_CURRENT_BINARY_DIR}/app.bin")
set(header "${CMAKE_CURRENT_BINARY_DIR}/app-header.bin")
add_custom_command(OUTPUT "${image}" "${header}"
  COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:sky-app.elf>" "${image}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/pack.py" "${image}" "${header}"
  DEPENDS sky-app.elf "${CMAKE_CURRENT_SOURCE_DIR}/pack.py"
  VERBATIM)
add_custom_target(pack-app ALL DEPENDS "${image}" "${header}")
