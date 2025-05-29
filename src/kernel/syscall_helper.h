#include <linux/module.h>
#include <linux/stop_machine.h>
#include <linux/syscalls.h>
#include "hook.h"

/**
 * init_x64_sys_call - resolve the address of `x64_sys_call`
 *
 * If get_x64_sys_call_addr is needed to be called, this function has to be
 * called first. Moreover, this function has to be called in the init function
 * of kernel module without any indirectly called.
 */
inline void init_x64_sys_call(void);

/**
 * get_x64_syscall_addr - get the address of `x64_sys_call`
 */
unsigned long get_x64_sys_call_addr(void);

/**
 * init_syscall_table - Init the syscall table
 *
 * If this function is needed to be called, it has to be called before any hook
 * happened since it calls the `hook_release` function. If you know what you are
 * doing, you are able not to follow this rule.
 *
 * The prototype of this function is a callback function of the `stop_machine`
 * function. This has to be called with all other tasks stoped since it will
 * crash in SMP system as syscalls frequently being called.
 *
 * @data: redundant parameter to conform to the prototype
 */
int init_syscall_table(void *data);

/**
 * get_syscall - Get the syscall address
 *
 * You can get the specific syscall address with this function.
 * Notice that the `init_syscall_table` has to be called before this function to
 * be called. The `exit` syscall and `exit_group` syscall cannot be obtained by
 * this function now.
 *
 * @num: The syscall number
 *
 * Return: The address of the syscall function, as well as the __x64_sys_*
 * function. Returns 0 if the syscall is not found or table is uninitialized.
 */
unsigned long get_syscall(int num);
