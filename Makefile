TARGET = rootkit
rootkit-objs = main.o
obj-m := $(TARGET).o

PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build

all: module user

user: module
	ld -s -r -b binary rootkit.ko -o rootkit.ko.o
	gcc user.c rootkit.ko.o -o user

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
