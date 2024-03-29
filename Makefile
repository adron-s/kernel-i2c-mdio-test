#make по ':all' собирает сразу модули для x86 и mips
ifndef ARCH
  export ARCH = x86
  export KERNEL_DIR = /lib/modules/$(shell uname -r)/build
endif
ifeq ($(ARCH),mips)
 include $(PWD)/lede.mk
endif
ifeq ($(ARCH),x86_64)
 include $(PWD)/lede.mk
endif
ifeq ($(ARCH),arm)
 include $(PWD)/arm.mk
endif
ifeq ($(ARCH),arm64)
 include $(PWD)/arm64.mk
endif

# standard flags for module builds
EXTRA_CFLAGS += -DLINUX -D__KERNEL__ -DMODULE -O2 -pipe -Wall

TARGET=mdio-sfp-i2c.o
obj-m:=$(TARGET)

TARGETS := $(obj-m:.o=.ko)
ccflags-y += -Wall

all:
#	make x86
#	make arm
	make mips
#	make ramips
#	make e2k

arch:
	$(MAKE) -C $(KERNEL_DIR) M=$$PWD

x86:
	$(MAKE) arch ARCH=x86_64 DEVICE=x86_64

mips:
	$(MAKE) arch ARCH=mips MANUAL_BUILD=lede-mips DEVICE=rb941

ramips:
	$(MAKE) arch ARCH=mips MANUAL_BUILD=openwrt-mips DEVICE=digi-ex15

arm:
	$(MAKE) arch ARCH=arm

kenzo:
	$(MAKE) arch ARCH=arm64 DEVICE=kenzo

e2k:
	$(MAKE) arch ARCH=e2k

clean:		
		rm -f .*.cmd *.mod.c *.ko *.o *~ core $(TARGETS)
		rm -fr .tmp_versions
		rm -f *.symvers built-in.a modules.order
