add_sky_hal_example(sky-rollback-boot.elf flash iwdg)
target_sources(sky-rollback-boot.elf PRIVATE "../../boot-app/boot/enter_app.S")
target_link_options(sky-rollback-boot.elf BEFORE PRIVATE "-Wl,--defsym=__slot_length=0x4000")
