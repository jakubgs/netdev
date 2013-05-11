# Makefile – makefile of our first driver

# if KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq (${KERNELRELEASE},)
    obj-m := netdev.o
# Otherwise we were called directly from the command line.
# Invoke the kernel build system.
else

KERNEL_SOURCE := /mnt/melchior/projects/kernel/linux-3.8.8/
PWD := $(shell pwd)

default:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} modules

clean:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} clean

install:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} modules_install
endif
