#include "syscall_helper.h"

static unsigned long x64_sys_call_addr = 0;

static unsigned long sys_call_leaks[NR_syscalls];

static noinline unsigned long find_address_up(int level)
{
    level++;
    if (!level)
        return 0;
    volatile unsigned long addr;
    __asm__ __volatile__("mov %%rbp, %[addr]\n" : [addr] "=r"(addr));
    for (int i = 0; i < level - 1; i++)
        addr = *(unsigned long *) addr;
    addr = *(unsigned long *) (addr + 8);
    return addr;
}


/* Helper to check if 64-bit value is sign-extended from 32-bit */
static bool is_sign_extended_32bit(const uint8_t *imm)
{
    const uint32_t *val = (const uint32_t *) imm;
    return (val[1] == 0) || (val[1] == 0xFFFFFFFF);
}

static size_t get_modrm_length(const uint8_t *modrm_ptr)
{
    uint8_t modrm = *modrm_ptr;
    uint8_t mod = modrm >> 6;
    uint8_t rm = modrm & 0x7;

    size_t len = 1; /* ModR/M byte itself */

    /* SIB byte (if applicable) */
    if (rm == 4 && mod != 3) /* SIB present */
        len += 1;

    /* Displacement */
    if (mod == 1) /* 1-byte disp */
        len += 1;
    else if (mod == 2) /* 4-byte disp */
        len += 4;
    else if (mod == 0 && rm == 5) /* RIP-relative */
        len += 4;

    return len;
}

/**
 * get_instruction_length - Calculate the length of an x86_64 instruction
 * @ip: Pointer to the instruction start
 *
 * This function analyzes the instruction opcode and prefixes to determine
 * the total instruction length. It supports common instructions including
 * MOV, CMP, TEST, XOR, JMP, CALL, RET, and conditional jumps.
 *
 * Return: Number of bytes in the instruction, or 0 if unknown
 */
static size_t get_instruction_length(const uint8_t *ip)
{
    uint8_t opcode = *ip;
    size_t length = 0;
    bool has_rex = false;

    /* Check for ENDBR64 */
    if (*(uint32_t *) ip == 0xFA1E0FF3)
        return 4;

    /* Check for REX prefix (0x40-0x4F) */
    if ((opcode & 0xF0) == 0x40) {
        has_rex = true;
        length++;
        opcode = ip[1]; /* Next byte is the actual opcode */
    }

    if (opcode == 0x66) {
        length++;
        opcode = ip[length];
    }

    switch (opcode) {
    /* Single-byte instructions */
    case 0x50 ... 0x57: /* PUSH r64 */
    case 0x58 ... 0x5F: /* POP r64 */
    case 0xC3:          /* RET */
    case 0xCC:          /* INT3 */
    case 0x90:          /* NOP */
        return length + 1;

    /* MOV with immediate to register */
    case 0xB0 ... 0xB7: /* MOV r8, imm8 */
        return length + 2;

    case 0xB8 ... 0xBF:                  /* MOV r16/32/64, imm16/32/64 */
        if (has_rex && (ip[0] & 0x48)) { /* REX.W present */
            /* Check if it's using sign-extended 32-bit immediate */
            if (is_sign_extended_32bit(&ip[length + 2])) {
                return length + 6; /* REX + opcode + imm32 */
            }
            return length + 10;              /* REX + opcode + imm64 */
        } else if (ip[length + 1] == 0x66) { /* 16-bit prefix */
            return length + 4;               /* 66 + opcode + imm16 */
        } else {
            return length + 5; /* opcode + imm32 */
        }

    /* Two-byte instructions (opcode + ModR/M) */
    case 0x84: /* TEST r/m8, r8 */
    case 0x85: /* TEST r/m16/32/64, r16/32/64 */
    case 0x88: /* MOV r/m8, r8 */
    case 0x89: /* MOV r/m64, r64 */
    case 0x8A: /* MOV r8, r/m8 */
    case 0x8B: /* MOV r64, r/m64 */
    case 0x30: /* XOR r/m8, r8 */
    case 0x31: /* XOR r/m16/32/64, r16/32/64 */
    case 0x32: /* XOR r8, r/m8 */
    case 0x33: /* XOR r16/32/64, r/m16/32/64 */
    case 0x39: /* CMP r/m64, r64 */
    case 0x3B: /* CMP r64, r/m64 */
        return length + 2;

    /* Immediate operand instructions */
    case 0x3C:             /* CMP AL, imm8 */
    case 0x34:             /* XOR AL, imm8 */
    case 0xA8:             /* TEST AL, imm8 */
        return length + 2; /* opcode + imm8 */

    case 0x3D:                         /* CMP RAX, imm32 */
    case 0x35:                         /* XOR RAX, imm32 */
    case 0xA9:                         /* TEST RAX, imm32 */
        if (has_rex && (ip[0] & 0x48)) /* REX.W */
            return length + 5;         /* REX + opcode + imm32 */
        else
            return length + (ip[length + 1] == 0x66 ? 3 : 5);

    /* Jump instructions */
    case 0xEB: /* JMP rel8 */
        return length + 2;

    case 0xE8: /* CALL rel32 */
    case 0xE9: /* JMP rel32 */
        return length + 5;

    /* Conditional jumps (1-byte displacement) */
    case 0x74: /* JE/JZ rel8 */
    case 0x75: /* JNE/JNZ rel8 */
    case 0x77: /* JA rel8 */
    case 0x7C: /* JL rel8 */
        return length + 2;

    case 0xEA: /* JMP far ptr */
        return length + 7;

    /* Complex instructions with ModR/M */
    case 0x80:   /* Group 1 (ALU ops with imm8) */
    case 0x81:   /* Group 1 (ALU ops with imm16/32) */
    case 0x83:   /* Group 1 (ALU ops with sign-extended imm8) */
    case 0xC6:   /* MOV r/m8, imm8 */
    case 0xC7:   /* MOV r/m16/32/64, imm16/32 */
    case 0xF6:   /* Group 3 (TEST/NOT/NEG/MUL/DIV with r/m8) */
    case 0xF7: { /* Group 3 (TEST/NOT/NEG/MUL/DIV with r/m16/32/64) */
        u8 modrm = ip[length + 1];
        u8 mod = modrm >> 6;
        u8 reg = (modrm >> 3) & 0x7;
        size_t base_len = length + 2; /* opcode + ModRM */

        /* Handle MOV immediate instructions first */
        if (opcode == 0xC6 || opcode == 0xC7) {
            bool is_8bit = (opcode == 0xC6);
            bool is_64bit = (has_rex && (ip[0] & 0x48));
            bool is_16bit = (ip[length + 1] == 0x66);

            if (mod == 0b11) { /* Register operand */
                return base_len + (is_8bit    ? 1
                                   : is_16bit ? 2
                                   : is_64bit ? 4
                                              : 4);
            } else { /* Memory operand */
                size_t mem_len = base_len;

                /* Add SIB byte if needed */
                if ((modrm & 0x7) == 0x4) /* SIB present */
                    mem_len += 1;

                /* Add displacement */
                if (mod == 0x01)
                    mem_len += 1;
                else if (mod == 0x02)
                    mem_len += 4;

                /* Add immediate */
                mem_len += (is_8bit ? 1 : is_16bit ? 2 : 4);

                return mem_len;
            }
        } else if (opcode == 0x80 || opcode == 0x81 || opcode == 0x83) {
            if (reg == 6 &&
                (opcode == 0x80 || opcode == 0x81 || opcode == 0x83)) {
                /* XOR with immediate */
                if (opcode == 0x81) {
                    if (has_rex && (ip[0] & 0x48))
                        return length + 7;
                    else if (ip[length + 1] == 0x66)
                        return length + 4;
                    else
                        return length + 6;
                }
                return length + (opcode == 0x80 ? 3 : 3);
            } else if (reg == 7 &&
                       (opcode == 0x80 || opcode == 0x81 || opcode == 0x83)) {
                /* CMP with immediate */
                return length + (opcode == 0x81 ? 6 : 3);
            }
        } else if (opcode == 0xF6 || opcode == 0xF7) {
            if (reg == 0) { /* TEST with immediate */
                if (opcode == 0xF7) {
                    if (has_rex && (ip[0] & 0x48))
                        return length + 7;
                    else if (ip[length + 1] == 0x66)
                        return length + 4;
                    else
                        return length + 6;
                }
                return length + 3;
            }
        }
        break;
    }

    /* Two-byte opcode instructions (0F prefix) */
    case 0x0F: {
        uint8_t second_byte = ip[length + 1];
        length += 2; /* 0F + second byte */

        switch (second_byte) {
        /* Conditional jumps (4-byte displacement) */
        case 0x80: /* JO rel32 */
        case 0x81: /* JNO rel32 */
        case 0x82: /* JB/JNAE/JC rel32 */
        case 0x83: /* JNB/JAE/JNC rel32 */
        case 0x84: /* JZ/JE rel32 */
        case 0x85: /* JNZ/JNE rel32 */
        case 0x86: /* JBE/JNA rel32 */
        case 0x87: /* JA/JNBE rel32 */
        case 0x88: /* JS rel32 */
        case 0x89: /* JNS rel32 */
        case 0x8A: /* JP/JPE rel32 */
        case 0x8B: /* JNP/JPO rel32 */
        case 0x8C: /* JL/JNGE rel32 */
        case 0x8D: /* JNL/JGE rel32 */
        case 0x8E: /* JLE/JNG rel32 */
        case 0x8F: /* JNLE/JG rel32 */
            return length + 4;

        /* MOVZX/MOVSX */
        case 0xB6: /* MOVZX r32, r/m8 */
        case 0xB7: /* MOVZX r32, r/m16 */
        case 0xBE: /* MOVSX r32, r/m8 */
        case 0xBF: /* MOVSX r32, r/m16 */
            return length + get_modrm_length(ip + length);

        /* CPUID, RDTSC, etc */
        case 0xA2: /* CPUID */
        case 0x31: /* RDTSC */
        case 0x01: /* SGDT/INVLPG (system) */
            return length;

        /* SSE/AVX instructions */
        case 0x10 ... 0x17: /* MOVUPS/MOVUPD/MOVSS/MOVSD */
        case 0x28 ... 0x2F: /* MOVAPS/MOVAPD */
        case 0x58 ... 0x5F: /* ADDPS/ADDPD/ADDSS/ADDSD */
        case 0xC2:          /* CMPPS/CMPPD */
            return length + get_modrm_length(ip + length);

        /* PREFETCH/TEST */
        case 0x18: /* PREFETCH */
        case 0x1F: /* NOP (multi-byte) */
            if (second_byte == 0x1F) {
                /* Handle multi-byte NOP (e.g., 66 0F 1F 00) */
                uint8_t modrm = ip[length];
                if ((modrm & 0xC0) == 0xC0) /* Register form */
                    return length + 1;
                else
                    return length + get_modrm_length(ip + length);
            }
            return length + get_modrm_length(ip + length);

        /* SYSCALL/SYSRET */
        case 0x05: /* SYSCALL */
        case 0x07: /* SYSRET */
            return length;

        default:
            /* Unknown 0F-prefixed instruction */
            return 0;
        }
    }

    /* Indirect jumps */
    case 0xFF: {
        uint8_t modrm = ip[length + 1];
        uint8_t mod = modrm >> 6;
        uint8_t reg = (modrm >> 3) & 0x7;

        if (reg == 4 || reg == 5) { /* JMP indirect */
            size_t base_len = length + 2;
            if (mod == 0b01)
                return base_len + 1;
            else if (mod == 0b10)
                return base_len + 4;
            else if (mod == 0b00 && (modrm & 0x7) == 0b100)
                return base_len + 1;
            else
                return base_len;
        }
        break;
    }

    default:
        break;
    }

    return 0; /* Unknown instruction */
}

noinline void init_x64_sys_call(void)
{
    volatile unsigned long addr;
    int cnt;
    int calls_num;
    int ans = 0;
    for (int i = 1; i <= 9; i++) {
        addr = find_address_up(i);
        if (*(char *) (addr - 5) != 0xe8)
            continue;
        addr = addr + *(int *) (addr - 4);
        cnt = 0;
        calls_num = 0;
        while (1) {
            if ((*(unsigned int *) addr & 0xffffff) ==
                0xe58948)  // mov rbp, rsp
                cnt++;
            if (cnt == 2)
                break;
            if (*(char *) addr == 0xe8) {
                calls_num++;
                addr += 5;
                continue;
            }
            size_t len = get_instruction_length((const uint8_t *) addr);
            if (!len)
                break;
            addr += len;
        }
        if (calls_num > 100) {
            ans = i;
            break;
        }
    }
    if (ans) {
        addr = find_address_up(ans);
        x64_sys_call_addr = addr + *(int *) (addr - 4);
    }
}


unsigned long get_x64_sys_call_addr(void)
{
    return x64_sys_call_addr;
}

/**
 * syscall_stealer - Steal the syscall
 *
 * This is the function to hook syscall function. It will put the syscall in the
 * `ax` field of `regs`.
 */
noinline static long syscall_stealer(struct pt_regs *regs)
{
    volatile unsigned long addr = find_address_up(1);
    addr = *(int *) (addr - 4) + addr;
    regs->ax = addr;
    return 0;
}

int init_syscall_table(void *data)
{
    unsigned long addr = x64_sys_call_addr;
    int cnt = 0;
    while (1) {
        if ((*(unsigned int *) addr & 0xffffff) == 0xe58948)  // mov rbp, rsp
            cnt++;

        if (cnt == 2)
            break;
        if (*(char *) addr == 0xe8) {
            unsigned long func = addr + 5 + *(int *) (addr + 1);
            hook_start(func, (unsigned long) syscall_stealer, "yee");
            addr += 5;
            continue;
        }
        size_t len = get_instruction_length((const uint8_t *) addr);
        if (len == 0)
            break;
        addr += len;
    }
    struct pt_regs regs;
    for (int i = 0; i < NR_syscalls; i++) {
        if (i == __NR_exit_group || i == __NR_exit)
            continue;
        ((long (*)(struct pt_regs *, unsigned int)) x64_sys_call_addr)(&regs,
                                                                       i);
        sys_call_leaks[i] = regs.ax;
    }
    hook_release();
    return 0;
}

unsigned long get_syscall(int num)
{
    if (num >= NR_syscalls)
        return 0;
    return sys_call_leaks[num];
}