##ccflags-y += -D CONFIG_TRIM_UNUSED_KSYMS
ccflags-y += -D CONFIG_DVB_NET

KERNEL_VERSION=$(notdir $(KERNEL_DIR))
ver_main := $(word 1,$(subst ., ,$(subst linux-,,$(KERNEL_VERSION))))
#URL=https://www.kernel.org/pub/linux/kernel/v$(ver_main).x/$(KERNEL_VERSION).tar.xz
URL=https://www.kernel.org/pub/linux/kernel/v$(ver_main).x
#HASH=$(word 1,$(shell wget $(URL)/sha256sums.asc -O -|grep $(KERNEL_VERSION).tar.xz))
DVB_CORE_DIR=drivers/media/dvb-core
DVB_USB_DIR=drivers/media/usb/dvb-usb
DVB_USB_V2_DIR=drivers/media/usb/dvb-usb-v2
DVB_FRONTENDS_DIR=drivers/media/dvb-frontends

dvb-core-objs += linuxdvb/dvbdev.o linuxdvb/dmxdev.o linuxdvb/dvb_demux.o 
dvb-core-objs += linuxdvb/dvb_ca_en50221.o linuxdvb/dvb_frontend.o linuxdvb/dvb_net.o 
dvb-core-objs += linuxdvb/dvb_ringbuffer.o linuxdvb/dvb_math.o

#linux kernel version <= 4.10.0
#dvb-core-objs += linuxdvb/dvb_filter.o

obj-m += dvb-core.o

dvb-usb-objs += linuxdvb/dvb-usb-firmware.o linuxdvb/dvb-usb-init.o linuxdvb/dvb-usb-urb.o 
dvb-usb-objs += linuxdvb/dvb-usb-i2c.o linuxdvb/dvb-usb-dvb.o linuxdvb/dvb-usb-remote.o linuxdvb/usb-urb.o
obj-m += dvb-usb.o

dvb_usb_v2-objs := linuxdvb/dvb_usb_core.o linuxdvb/dvb_usb_urb.o linuxdvb/usb_urb.o
obj-m += dvb_usb_v2.o

obj-m += dtmbusb-fe.o
obj-m += dtmbusb-dev.o


PWD=$(shell pwd)
#KERNEL_DIR=/home/tony/PandoraBox-SDK-ralink-mt7621_gcc-5.5.0_uClibc-1.0.x.Linux-x86_64-2018-12-14-git-ba60306f2/build_dir/target-mipsel_1004kc+dsp_uClibc-1.0.x/linux-ralink_mt7621/linux-3.14.79
#TOOLCHAIN="/home/tony/PandoraBox-SDK-ralink-mt7621_gcc-5.5.0_uClibc-1.0.x.Linux-x86_64-2018-12-14-git-ba60306f2/staging_dir/toolchain-mipsel_1004kc+dsp_gcc-5.5.0_uClibc-1.0.x/bin/mipsel-openwrt-linux-"

#KERNEL_TO_DIR=/home/tony/openwrt-sdk-18.06.0-ar71xx-generic_gcc-7.3.0_musl.Linux-x86_64/build_dir/target-mips_24kc_musl/linux-ar71xx_generic/linux-4.14.104

ccflags-y += -I$(KERNEL_DIR)/drivers/media/usb/dvb-usb/
ccflags-y += -I$(KERNEL_DIR)/drivers/media/dvb-core/
ccflags-y += -I$(KERNEL_DIR)/drivers/media/dvb-frontends/
ccflags-y += -I$(KERNEL_DIR)/drivers/media/usb/dvb-usb-v2/
#ccflags-y += -Ilinuxdvb/

all:
	make -C $(KERNEL_DIR) \
	ARCH=$(ARCH) \
	CROSS_COMPILE=$(TOOLCHAIN) \
	M=$(PWD) \
	modules

prepare:
	wget -c $(URL)/$(KERNEL_VERSION).tar.xz -O $(PWD)/dl/$(KERNEL_VERSION).tar.xz
	tar xvf $(PWD)/dl/$(KERNEL_VERSION).tar.xz -C $(KERNEL_DIR)/ --wildcards --strip-components 1 \
		$(KERNEL_VERSION)/$(DVB_CORE_DIR)/* \
		$(KERNEL_VERSION)/$(DVB_USB_DIR)/dvb*.h \
		$(KERNEL_VERSION)/$(DVB_USB_V2_DIR)/dvb*.h \
		$(KERNEL_VERSION)/$(DVB_FRONTENDS_DIR)/dvb*
	tar xvf $(PWD)/dl/$(KERNEL_VERSION).tar.xz -C $(PWD)/linuxdvb/ --wildcards --strip-components 4 \
		$(KERNEL_VERSION)/$(DVB_CORE_DIR)/*.c \
		$(KERNEL_VERSION)/$(DVB_FRONTENDS_DIR)/dvb*.*
	tar xvf $(PWD)/dl/$(KERNEL_VERSION).tar.xz -C $(PWD)/linuxdvb/ --wildcards --strip-components 5 \
		$(KERNEL_VERSION)/$(DVB_USB_DIR)/usb-urb.c \
		$(KERNEL_VERSION)/$(DVB_USB_DIR)/dvb-usb*.c \
		$(KERNEL_VERSION)/$(DVB_USB_DIR)/dvb-usb*.h \
		$(KERNEL_VERSION)/$(DVB_USB_V2_DIR)/usb_urb.c \
		$(KERNEL_VERSION)/$(DVB_USB_V2_DIR)/dvb_usb*
test:
	echo $(PWD)

clean:
	rm -f *.ko
	rm -f *.o
	rm -f *.mod
	rm -f *.mod.c
	rm -f *.mod.o
	rm -f *.order
	rm -f *.sysvers
	rm -f .*.cmd
	rm -f .*.swp
	rm -f Module.symvers
	rm -rf .tmp_versions/
	rm linuxdvb/*.o linuxdvb/.*.cmd
							  
#make command:
#make
#make clean

