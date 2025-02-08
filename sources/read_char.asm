section .data
    buf db 0

section .text
    global readChar

readChar:
    push rbp
    push rbx
    mov rbp, rsp

    push rsi
    push rdi

    mov rax, 0          
    mov rdi, 0          
    lea rsi, [buf]      
    mov rdx, 1          
    syscall

    mov rbx, rax

    pop rdi

    mov al, [rsi]

    mov byte [rdi], al

    pop rsi
    mov dword [rsi], ebx

    ;mov rax, 1           
    ;mov rdi, 1           
    ;lea rsi, [buf]  
    ;mov rdx, 1           
    ;syscall

    mov rsp, rbp
    pop rbx
    pop rbp
    ret
