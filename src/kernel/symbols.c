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
    x64_sys_call_addr = addr;
}


/* Helper to check if 64-bit value is sign-extended from 32-bit */
static bool is_sign_extended_32bit(const uint8_t *imm)
{
    const uint32_t *val = (const uint32_t *) imm;
    return (val[1] == 0) || (val[1] == 0xFFFFFFFF);
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
size_t get_instruction_length(const uint8_t *ip)
{
    uint8_t opcode = *ip;
    size_t length = 0;
    bool has_rex = false;

    /* Check for REX prefix (0x40-0x4F) */
    if ((opcode & 0xF0) == 0x40) {
        has_rex = true;
        length++;
        opcode = ip[1]; /* Next byte is the actual opcode */
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
                return base_len +
                       (is_8bit ? 1 : is_16bit ? 2 : is_64bit ? 4 : 4);
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
        uint8_t opcode2 = ip[length + 1];
        length += 2; /* 0F + second opcode */

        switch (opcode2) {
        case 0x1F: {
            uint8_t modrm = ip[length + 2];
            uint8_t mod = modrm >> 6;

            /* Base length: 0F 1F + ModRM */
            size_t nop_length = length + 3;

            /* Handle displacement bytes */
            if (mod == 0x01) {
                nop_length += 1; /* disp8 */
            } else if (mod == 0x02) {
                nop_length += 4; /* disp32 */
            }
            /* mod == 0x00 needs no displacement */

            return nop_length;
        }
        case 0x80 ... 0x8F: /* Jcc rel32 */
            return length + 4;
        case 0x90 ... 0x9F: /* SETcc */
        case 0xB6:          /* MOVZX */
        case 0xBE:          /* MOVSX */
        case 0xC1:          /* XADD */
            return length + 1;
        default:
            return length;
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

unsigned long get_x64_sys_call_addr(void)
{
    return x64_sys_call_addr;
}
