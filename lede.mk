#cross compile для mips

#путь где лежит buildroot openwrt
WRTP = /home/prog/openwrt/2021-openwrt/openwrt-2021

#определяемся с типом устройства 
ifeq ($(DEVICE),digi-ex15)
  TOOLCHAIN_DIR := toolchain-mipsel_24kc_gcc-8.4.0_musl
  TARGET_DIR := target-mipsel_24kc_musl
  KERNEL_SUB_DIR := linux-ramips_mt7621
  CROSS_COMPILE_PREFIX := mipsel
endif

ifeq ($(DEVICE),rb941)
  TOOLCHAIN_DIR := toolchain-mips_24kc_gcc-8.4.0_musl
  TARGET_DIR := target-mips_24kc_musl
  KERNEL_SUB_DIR := linux-ath79_mikrotik
  CROSS_COMPILE_PREFIX := mips
endif

ifeq ($(DEVICE),x86_64)
  TOOLCHAIN_DIR := toolchain-x86_64_gcc-8.4.0_musl
  TARGET_DIR := target-x86_64_musl
  KERNEL_SUB_DIR := linux-x86_64
  CROSS_COMPILE_PREFIX := x86_64
endif

#эта переменная нужна компиляроту. указывает на диру с toolchain.
export STAGING_DIR = $(WRTP)/staging_dir/$(TOOLCHAIN_DIR)
#указываем какой префикс юзать перед сажем gcc или ldd(то есть какое имя у файла бинарника компилятора или линкера)
export CROSS_COMPILE = $(CROSS_COMPILE_PREFIX)-openwrt-linux-
#путь к исходнику ядра
KERNEL_DIR = $(WRTP)/build_dir/$(TARGET_DIR)/$(KERNEL_SUB_DIR)/linux-5.4.96
#где лежат бинарники компилятора(gcc, ldd)
PATH := $(PATH):$(STAGING_DIR)/bin
#указываем архитекруту
#export ARCH = $(ARCH)
#дальше все как обычно для модуля

#тут должна быть только одна строка команды. иначе будет ошибка. если нужно
#более одной строки то используй объединение команд посредством ; и ( )
define ManualBuild/Install
	mkdir -p $(WRTP)/files/tests
	cat $(TARGETS) > $(WRTP)/files/tests/$(TARGETS)
endef
