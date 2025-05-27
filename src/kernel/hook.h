#pragma once
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>

#define HOOK_SIZE 12
#define MAX_NAME 16

struct hook {
    unsigned long org_func;
    unsigned long evil_func;
    unsigned char org_code[HOOK_SIZE];
    unsigned char evil_code[HOOK_SIZE];
    unsigned char name[MAX_NAME];
    struct list_head list;
};

/**
 * hook_start - Start hooking
 *
 * @org_func: The function address to be hooked
 * @evil_func: The evil function
 * @name: The name as well as identifier to the func. Programmers have the
 * responsibility to choose identity names for different hooks.
 */
void hook_start(unsigned long org_func,
                unsigned long evil_func,
                const char *name);

/**
 * hook_pause - Pause the hook temporily
 *
 * @addr: The address of the hooked funciton
 */
void hook_pause(unsigned long addr);

/**
 * hook_resume - Resume the hook paused by `hook_start`
 *
 * @addr: The address of the paused function
 */
void hook_resume(unsigned long addr);

/**
 * hook_release - Release all the hook
 */
void hook_release(void);

/**
 * find_hook_by_name - Find the hook structure by name
 */
struct hook *find_hook_by_name(const char *name);

/**
 * CALL_ORIGINAL_FUNC(func, ...) - Call the original function while hooking
 *
 * `func` is the original function followed by the parameters in the ... field
 * In the call, the function will not be recursively hooked.
 * This method is derived from the repository `rooty`[1].
 *
 * [1]: https://github.com/jermeyyy/rooty
 */
#define CALL_ORIGINAL_FUNC(func, ...)      \
    do {                                   \
        hook_pause((unsigned long) func);  \
        (func)(__VA_ARGS__);               \
        hook_resume((unsigned long) func); \
    } while (0)


#define CALL_ORIGINAL_FUNC_RET(func, type, ...)           \
    ({                                                    \
        hook_pause((unsigned long) func);                 \
        type call_org_func_ret_ret = (func)(__VA_ARGS__); \
        hook_resume((unsigned long) func);                \
        call_org_func_ret_ret;                            \
    })

/**
 * CALL_ORIGINAL_FUNC_BY_NAME(name, ...) - Call the origional function by name
 */
#define CALL_ORIGINAL_FUNC_BY_NAME(name, func_type, ...)                     \
    do {                                                                     \
        struct hook *call_by_name_hook_struct = find_hook_by_name(name);     \
        CALL_ORIGINAL_FUNC(((func_type) call_by_name_hook_struct->org_func), \
                           ##__VA_ARGS__);                                   \
    } while (0);

#define CALL_ORIGINAL_FUNC_BY_NAME_RET(name, func_type, type, ...)           \
    ({                                                                       \
        struct hook *call_by_name_ret_hook_struct = find_hook_by_name(name); \
        type call_by_name_ret = CALL_ORIGINAL_FUNC_RET(                      \
            ((func_type) call_by_name_ret_hook_struct->org_func), type,      \
            ##__VA_ARGS__);                                                  \
        call_by_name_ret;                                                    \
    })
