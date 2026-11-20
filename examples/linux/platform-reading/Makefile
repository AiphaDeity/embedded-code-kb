obj-m := kb_reading.o
KDIR ?= /lib/modules/$(shell uname -r)/build

.PHONY: all module overlay clean
all: module overlay
module:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules
overlay:
	dtc -@ -I dts -O dtb -o reading-overlay.dtbo reading-overlay.dts
clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
	rm -f reading-overlay.dtbo
