#include "symbols.h"

static unsigned long x64_sys_call_addr = 0;

inline unsigned long find_address_up(int level)
{
    if (!level)
        return 0;
    unsigned long addr;
    __asm__ __volatile__("mov %%rbp, %[addr]\n" : [addr] "=r"(addr));
    for (int i = 0; i < level - 1; i++)
        addr = *(unsigned long *) addr;
    addr = *(unsigned long *) (addr + 8);
    return addr;
}

inline void init_x64_sys_call(void)
{
    unsigned long addr = find_address_up(7);
    while (*(unsigned int *) addr != 0xe5894855)
        addr--;
    addr &= ~0xf;
    x64_sys_call_addr = addr;
}

unsigned long get_x64_sys_call_addr(void)
{
    return x64_sys_call_addr;
}
