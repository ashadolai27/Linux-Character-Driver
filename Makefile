KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

obj-m := char_driver.o
char_driver-objs := src/char_driver.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
