# Makefile – makefile of our first driver
TARGET 		:= netdev
DEBUG 		:= -g
WARN   		:= -Wall -Wstrict-prototypes -Wmissing-prototypes
NOWARN 		:= -Wno-unused-function
INCLUDE 	:= -I$(src)/../include/
ccflags-y 	:= -O ${WARN} ${NOWARN} ${DEBUG} ${INCLUDE}

# if KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
ifneq (${KERNELRELEASE},)
    obj-m 	:= ${TARGET}.o
	${TARGET}-objs := fo_access.o fo_send.o fo_recv.o fo_comm.o fo.o netdevmgm.o netlink.o main.o
# Otherwise we were called directly from the command line.
# Invoke the kernel build system.
else

KERNEL_SOURCE := /usr/src/linux
PWD 		:= $(shell pwd)

default:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} modules

clean:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} clean

install:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} modules_install
endif
