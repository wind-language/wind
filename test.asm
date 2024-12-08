section .text
    global _start

add:
    push rbp
    mov rbp, rsp
    mov [rbp], edi
    mov [rbp-4], esi
    mov eax, [rbp]
    add eax, [rbp-4]
    pop rbp
    ret
main:
    push rbp
    mov rbp, rsp
    sub rsp,16
    mov edi, 1
    mov esi, 2
    call add
    mov [rbp], eax
    mov eax, [rbp]
    leave
    ret

_start:
    call main
    syscall
    mov rax, 60
    mov rdi, 0
    syscall
