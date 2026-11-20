add_sky_cmsis_example(sky-app.elf)
target_link_options(sky-app.elf BEFORE PRIVATE
  "-Wl,--defsym=__slot_origin=0x08008000,--defsym=__slot_length=0x78000")
target_link_options(sky-app.elf PRIVATE "-Wl,-Map=sky-app.map")
target_compile_definitions(sky-app.elf PRIVATE USER_VECT_TAB_ADDRESS VECT_TAB_OFFSET=0x8000U)
