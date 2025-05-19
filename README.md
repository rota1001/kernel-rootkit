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
```
sudo ./user
```

## Features
- Persistence

  Injecting the shellcode and kernel module into `systemd`, which will definitly be executed by root user.
- Function Hooking without ftrace

  Hook with modifying the machine code. It can work without ftrace support. This method is originate from [`rooty`](https://github.com/jermeyyy/rooty). The original version works on 32 bit x86 architecture. This modifies it to work on x86-64 architecture, and writes the machine code by adding the write permission to the page table entry instead of changing `cr0` (Although it still works if we clear the `CET` flag in `cr4`).

## Reference
- [drow](https://github.com/zznop/drow): static code ELF injection
- [rooty](https://github.com/jermeyyy/rooty): Hook with writing shellcode
- [awesome-linux-rootkits](https://github.com/milabs/awesome-linux-rootkits): List of awesome rookits

