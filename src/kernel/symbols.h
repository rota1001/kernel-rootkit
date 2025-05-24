#include <linux/module.h>

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
