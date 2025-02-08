section .data
    source dq 0
    dest dq 0 
    key dq 0 
    len dq 0 
    rownum dq 0
    ccol dq 0
    crow dq 0
    temp dq 0

section .text
    global aDecipherTransposition

aDecipherTransposition:
    push rbp
    push rbx
    mov rbp, rsp

    mov qword[ccol],0
    mov qword[crow],0
    mov qword[temp],0
    mov qword[rownum],0
    mov qword[len],0

    mov qword [source], rdi ; source
    mov qword [dest], rsi ; dest
    mov qword [key], rdx ; key

    mov rsi, qword[key]
    mov rax, 0
    mov al, byte[rsi]
    mov qword[key],rax

    ;get len
    mov rsi, qword[source]
cycle_len:
    mov al, byte[rsi]
    cmp al, 0
    je skip_len
    inc qword[len]
    inc rsi
    jmp cycle_len
skip_len:

mov rax, qword[len]
mov rdx, 0
mov rsi, qword[key]
mov rbx, rsi

div rbx

mov qword[rownum], rax
;

mov rdx, qword[dest]

mov rsi, qword[rownum]
mov rbx, qword[key]
mov qword[key], rsi
mov qword[rownum], rbx

cycle:
    mov rbx, qword[ccol]
    cmp rbx, qword[key]
    je end

    mov rcx, qword[source]
    add rcx, qword[ccol]

    cycle2:
    mov rbx, qword[crow]
    cmp rbx, qword[rownum]
    je cycle2end

    mov bl, byte[rcx]
    mov byte[rdx],bl

    inc rdx
    add rcx, qword[key]

    inc qword[crow]
    jmp cycle2

    cycle2end:
    mov qword[crow], 0
    inc qword[ccol]
    jmp cycle

end:
    mov byte [dest], 0
    mov rsp, rbp
    pop rbx
    pop rbp
    ret