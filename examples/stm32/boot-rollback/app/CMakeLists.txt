# 从 C 版布局读取地址，同时供链接器、中断地址和打包器使用。
file(STRINGS "../layout.h" layout)
foreach(symbol A_VECTOR A_CAPACITY B_VECTOR B_CAPACITY)
  foreach(line IN LISTS layout)
    if(line MATCHES "^#define ${symbol} (0x[0-9A-Fa-f]+)U$")
      set(${symbol} "${CMAKE_MATCH_1}")
    endif()
  endforeach()
  if(NOT DEFINED ${symbol})
    message(FATAL_ERROR "Missing ${symbol} in layout.h")
  endif()
endforeach()

function(add_slot name slot origin capacity)
  set(target "sky-rollback-${name}.elf")
  add_sky_hal_example(${target} flash iwdg)
  math(EXPR offset "${origin} - 0x08000000" OUTPUT_FORMAT HEXADECIMAL)
  target_link_options(${target} BEFORE PRIVATE "-Wl,--defsym=__slot_origin=${origin},--defsym=__slot_length=${capacity}")
  target_compile_definitions(${target} PRIVATE SKY_APP_SLOT=${slot} USER_VECT_TAB_ADDRESS "VECT_TAB_OFFSET=${offset}U")
  set(image "${CMAKE_BINARY_DIR}/slot-${name}.bin")
  set(header "${CMAKE_BINARY_DIR}/slot-${name}-header.bin")
  set(packer "${CMAKE_CURRENT_SOURCE_DIR}/../../boot-app/pack.py")
  add_custom_command(OUTPUT "${image}" "${header}"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:${target}>" "${image}"
    COMMAND "${Python3_EXECUTABLE}" "${packer}" "${image}" "${header}" --origin "${origin}" --capacity "${capacity}"
    DEPENDS ${target} "${packer}" VERBATIM)
  add_custom_target(pack-${name} ALL DEPENDS "${image}" "${header}")
endfunction()

add_slot(a SLOT_A "${A_VECTOR}" "${A_CAPACITY}")
add_slot(b SLOT_B "${B_VECTOR}" "${B_CAPACITY}")
