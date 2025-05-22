# kernel-rootkit
This kernel rootkit aims at linux v6.11+ kernel versions with x86-64 architecture. All the features will be tested on ubuntu 24.04 and some other experiment environments.

## Claim
This rootkit is for **Educational Purpose**, do not use it to do anything illegally.

This rootkit will potentially cause **IRREVERSIBLE DAMAGE** to your system, please run it in experiment environment.

## Compilation
### Install Dependency
```
sudo apt install linux-headers-`uname -r`
```

### Compile
```
git clone https://github.com/rota1001/kernel-rootkit.git
cd kernel-rootkit
make
```
## Usage
Please read the **Claim** before you do it.

If you know what you are doing, use `sudo` to execute this program. Your `systemd` will be patched, and the rootkit will be automatically inserted everytime you boot your kernel.
The parameter is the path to the target root, and all the path setting in the program will be relative to this path. For example, if the rootfs is stored on a partition, you can mount it on a directory and pass the absolute path of that partition as the parameter to run this program.
```
sudo ./user [absolute path to target root]
```

You can use the shell script `create-usb.sh` to create a minimal USB.
```
./create-usb.sh
```

You can use the shell script `attack.sh` to implant the rootkit on the target partition. This can be run in the Live USB created above, and will be placed at the root directory defaultly.
```
./attack.sh
```

## Features
- Persistence

  Injecting the shellcode and kernel module into `systemd`, which will definitly be executed by root user.
- Function Hooking without ftrace

  Hook with modifying the machine code. It can work without ftrace support. This method is originate from [`rooty`](https://github.com/jermeyyy/rooty). The original version works on 32 bit x86 architecture. This modifies it to work on x86-64 architecture, and writes the machine code by adding the write permission to the page table entry instead of changing `cr0` (Although it still works if we clear the `CET` flag in `cr4`).

- Create a minimal Live USB with one click

  This provides a shell script to create a minimal Live USB to implant the rootkit.

- User Interface for implanting rootkit

  This provides users with a shell script to choose what disk partition they want to implant the rootkit to.
## Reference
- [drow](https://github.com/zznop/drow): static code ELF injection
- [rooty](https://github.com/jermeyyy/rooty): Hook with writing shellcode
- [awesome-linux-rootkits](https://github.com/milabs/awesome-linux-rootkits): List of awesome rookits

