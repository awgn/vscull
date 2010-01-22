# Vscull Makefile 
#
# (c) 2009 Nicola Bonelli <n.bonelli@netresults.it>
#

MODULE_NAME = vscull
$(MODULE_NAME)-objs = vscull_module.o

# if KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.

ifneq ($(KERNELRELEASE),)
	obj-m	:= $(MODULE_NAME).o

EXTRA_CFLAGS += -I$(PWD)/../include

# Otherwise we were called directly from the command
# line; invoke the kernel build system.

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -f *~
	rm -f Module.symvers Module.markers modules.order
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install
	depmod -ae

endif
