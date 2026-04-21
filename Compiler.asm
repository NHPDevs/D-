BITS 64

section .text
global _start

_start:
    mov rsi, [rsp + 16]
    xor rdi, rdi
    xor rdx, rdx
    mov rax, 2
    syscall
    test rax, rax
    js .exit
    
    mov r15, rax
    mov rdi, r15
    sub rsp, 65536
    mov rsi, rsp
    mov rdx, 65536
    xor rax, rax
    syscall
    mov r14, rax
    
    mov rdi, r15
    mov rax, 3
    syscall
    
    xor r12, r12
    xor r13, r13
    mov r11, rsp
    add r11, 65536
    
.lex:
    cmp r12, r14
    jge .parse
    movzx eax, byte [rsp + r12]
    cmp al, 32
    jle .skip
    cmp al, 47
    jne .nocom
    cmp byte [rsp + r12 + 1], 47
    jne .nocom
.cmt:
    inc r12
    cmp r12, r14
    jge .parse
    cmp byte [rsp + r12], 10
    jne .cmt
    jmp .lex
.nocom:
    cmp al, 97
    jl .noid
    cmp al, 122
    jg .noid
.id:
    mov r10, r12
.idl:
    inc r12
    cmp r12, r14
    jge .idd
    movzx eax, byte [rsp + r12]
    cmp al, 97
    jl .chkd
    cmp al, 122
    jle .idl
.chkd:
    cmp al, 48
    jl .idd
    cmp al, 57
    jle .idl
.idd:
    mov rax, r12
    sub rax, r10
    cmp rax, 3
    je .int
    cmp rax, 2
    je .if
    cmp rax, 5
    je .whl
    cmp rax, 6
    je .ret
    jmp .lex
.int:
    cmp byte [rsp + r10], 105
    jne .lex
    cmp byte [rsp + r10 + 1], 110
    jne .lex
    cmp byte [rsp + r10 + 2], 116
    jne .lex
    or r13, 1
    jmp .lex
.if:
    cmp byte [rsp + r10], 105
    jne .lex
    cmp byte [rsp + r10 + 1], 102
    jne .lex
    or r13, 2
    jmp .lex
.whl:
    cmp byte [rsp + r10], 119
    jne .lex
    or r13, 4
    jmp .lex
.ret:
    cmp byte [rsp + r10], 114
    jne .lex
    or r13, 8
    jmp .lex
.noid:
    cmp al, 48
    jl .nonu
    cmp al, 57
    jg .nonu
.num:
    xor r10, r10
.numl:
    movzx eax, byte [rsp + r12]
    cmp al, 48
    jl .numd
    cmp al, 57
    jg .numd
    sub al, 48
    imul r10, 10
    add r10, rax
    inc r12
    cmp r12, r14
    jl .numl
.numd:
    mov byte [r11], 0x48
    inc r11
    mov byte [r11], 0xC7
    inc r11
    mov byte [r11], 0xC0
    inc r11
    mov [r11], r10d
    add r11, 4
    jmp .lex
.nonu:
    cmp al, 43
    je .add
    cmp al, 45
    je .sub
    cmp al, 42
    je .mul
    cmp al, 47
    je .div
    cmp al, 40
    je .lp
    cmp al, 41
    je .rp
    cmp al, 123
    je .lb
    cmp al, 125
    je .rb
    cmp al, 59
    je .sem
.skip:
    inc r12
    jmp .lex
.add:
    mov byte [r11], 0x5B
    inc r11
    mov byte [r11], 0x48
    inc r11
    mov byte [r11], 0x01
    inc r11
    mov byte [r11], 0xD8
    inc r11
    inc r12
    jmp .lex
.sub:
    mov byte [r11], 0x5B
    inc r11
    mov byte [r11], 0x48
    inc r11
    mov byte [r11], 0x29
    inc r11
    mov byte [r11], 0xD8
    inc r11
    inc r12
    jmp .lex
.mul:
    mov byte [r11], 0x5B
    inc r11
    mov byte [r11], 0x48
    inc r11
    mov byte [r11], 0x0F
    inc r11
    mov byte [r11], 0xAF
    inc r11
    mov byte [r11], 0xC3
    inc r11
    inc r12
    jmp .lex
.div:
    mov byte [r11], 0x5B
    inc r11
    mov byte [r11], 0x48
    inc r11
    mov byte [r11], 0x99
    inc r11
    mov byte [r11], 0x48
    inc r11
    mov byte [r11], 0xF7
    inc r11
    mov byte [r11], 0xFB
    inc r11
    inc r12
    jmp .lex
.lp:
    mov byte [r11], 0x53
    inc r11
    inc r12
    jmp .lex
.rp:
    inc r12
    jmp .lex
.lb:
    mov byte [r11], 0x55
    inc r11
    mov byte [r11], 0x48
    inc r11
    mov byte [r11], 0x89
    inc r11
    mov byte [r11], 0xE5
    inc r11
    inc r12
    jmp .lex
.rb:
    inc r12
    jmp .lex
.sem:
    inc r12
    jmp .lex
    
.parse:
    test r13, 8
    jz .noret
    mov byte [r11], 0x48
    inc r11
    mov byte [r11], 0x89
    inc r11
    mov byte [r11], 0xEC
    inc r11
    mov byte [r11], 0x5D
    inc r11
    mov byte [r11], 0xC3
    inc r11
.noret:
    
    mov rsi, [rsp + 8]
    xor rdi, rdi
    mov rax, 2
    mov rdx, 0x1B6
    syscall
    test rax, rax
    js .exit
    
    mov r15, rax
    mov rdi, r15
    lea rsi, [rsp + 65536]
    mov rdx, r11
    sub rdx, rsi
    mov rax, 1
    syscall
    
    mov rdi, r15
    mov rax, 3
    syscall
    
.exit:
    xor rdi, rdi
    mov rax, 60
    syscall
