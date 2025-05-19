#pragma once
#include <linux/list.h>

#define HOOK_SIZE 12

struct hook {
    unsigned long org_func;
    unsigned long evil_func;
    unsigned char org_code[HOOK_SIZE];
    unsigned char evil_code[HOOK_SIZE];
    struct list_head list;
};

/**
 * hook_start - Start hooking
 *
 * @org_func: The function address to be hooked
 * @evil_func: The evil function
 */
void hook_start(unsigned long org_func, unsigned long evil_func);

/**
 * hook_pause - Pause the hook temporily
 *
 * @addr: The address of the hooked funciton
 */
void hook_pause(void *addr);

/**
 * hook_resume - Resume the hook paused by `hook_start`
 *
 * @addr: The address of the paused function
 */
void hook_resume(void *addr);

/**
 * hook_release - Release all the hook
 */
void hook_release(void);

/**
 * CALL_ORIGINAL_FUNC(func, ...) - Call the original function while hooking
 *
 * `func` is the original function followed by the parameters in the ... field
 * In the call, the function will not be recursively hooked.
 * This method is derived from the repository `rooty`[1].
 *
 * [1]: https://github.com/jermeyyy/rooty
 */
#define CALL_ORIGINAL_FUNC(func, ...) \
    do {                              \
        hook_pause(func);             \
        func(__VA_ARGS__);            \
        hook_resume(func);            \
    } while (0)
