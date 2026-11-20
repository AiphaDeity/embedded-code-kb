include("${CMAKE_CURRENT_LIST_DIR}/../../stm32/common/sky-hal.cmake")

function(add_sky_freertos_example target)
  add_sky_hal_example(${target})
  set(kernel "${CUBE_F4_ROOT}/Middlewares/Third_Party/FreeRTOS/Source")
  set(common "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
  target_sources(${target} PRIVATE "${common}/demo.c"
    "${kernel}/tasks.c" "${kernel}/queue.c" "${kernel}/list.c"
    "${kernel}/portable/GCC/ARM_CM4F/port.c")
  if("static" IN_LIST ARGN)
    target_compile_definitions(${target} PRIVATE SKY_USE_STATIC_ALLOCATION)
  else()
    target_sources(${target} PRIVATE "${kernel}/portable/MemMang/heap_4.c")
  endif()
  target_include_directories(${target} PRIVATE "${common}" "${kernel}/include"
    "${kernel}/portable/GCC/ARM_CM4F")
  if("events" IN_LIST ARGN)
    target_sources(${target} PRIVATE "${kernel}/event_groups.c")
  endif()
  if("timers" IN_LIST ARGN)
    target_compile_definitions(${target} PRIVATE SKY_USE_TIMERS)
    target_sources(${target} PRIVATE "${kernel}/timers.c")
  endif()
endfunction()
