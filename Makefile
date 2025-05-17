TARGET = rootkit
rootkit-objs = main.o
obj-m := $(TARGET).o

PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build

all: module

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
