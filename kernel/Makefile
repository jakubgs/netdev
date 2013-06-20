# Makefile – makefile of our first driver
TARGET = netdev

# if KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq (${KERNELRELEASE},)
    obj-m := ${TARGET}.o
	netdev-objs := netlink.o netdev_mod.o
# Otherwise we were called directly from the command line.
# Invoke the kernel build system.
else

KERNEL_SOURCE := /usr/src/linux
PWD 		:= $(shell pwd)
WARN   		:= -W Wall Wstrict-prototypes -Wmissing-prototypes
ccflags-y 	:= ${WARN}

default:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} modules

clean:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} clean

install:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} modules_install
endif
