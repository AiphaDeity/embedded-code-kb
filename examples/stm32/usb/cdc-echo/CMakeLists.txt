cmake_minimum_required(VERSION 3.20)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
project(sky_usb_cdc LANGUAGES C ASM)
include("../../common/sky-hal.cmake")
add_sky_hal_example(sky-usb-cdc.elf pcd pcd_ex pwr pwr_ex)

# 1209:0001 为 pid.codes 的非唯一测试号。实验室使用，分发或生产设备须使用已分配的编号。
set(SKY_USB_VID "0x1209" CACHE STRING "USB vendor ID")
set(SKY_USB_PID "0x0001" CACHE STRING "USB product ID")
target_compile_definitions(sky-usb-cdc.elf PRIVATE HSE_VALUE=8000000U
  "SKY_USB_VID=${SKY_USB_VID}" "SKY_USB_PID=${SKY_USB_PID}")
set(usb "${CUBE_F4_ROOT}/Middlewares/ST/STM32_USB_Device_Library")
set(hal "${CUBE_F4_ROOT}/Drivers/STM32F4xx_HAL_Driver")
target_include_directories(sky-usb-cdc.elf PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}" "${usb}/Core/Inc" "${usb}/Class/CDC/Inc")
target_sources(sky-usb-cdc.elf PRIVATE usb_port.c descriptors.c
  "${hal}/Src/stm32f4xx_ll_usb.c" "${usb}/Core/Src/usbd_core.c"
  "${usb}/Core/Src/usbd_ctlreq.c" "${usb}/Core/Src/usbd_ioreq.c"
  "${usb}/Class/CDC/Src/usbd_cdc.c")
