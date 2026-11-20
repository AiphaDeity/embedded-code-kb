add_sky_cmsis_example(sky-boot.elf)
target_sources(sky-boot.elf PRIVATE enter_app.S)
target_link_options(sky-boot.elf BEFORE PRIVATE "-Wl,--defsym=__slot_length=0x4000")
target_link_options(sky-boot.elf PRIVATE "-Wl,-Map=sky-boot.map")
