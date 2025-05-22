#include "devices.h"

enum operation_num { GET_ROOT };

/**
 * cmd_ioctl - VFS interface to execute command
 */
long cmd_ioctl(struct file *flip, unsigned int cmd, unsigned long arg);

void utils_init(void);
