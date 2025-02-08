section .data
    source dq 0
    dest dq 0 
    key dq 0 

section .text
    global aCipherCaesar

aCipherCaesar:
    push rbp
    push rbx
    mov rbp, rsp

    mov qword [source], rdi ; source
    mov qword [dest], rsi ; dest
    mov qword [key], rdx ; key

    mov rsi, qword[key]
    mov rax, 0
    mov al, byte[rsi]
    mov qword[key], rax

cycle:
    mov rsi, qword[source]
    mov al, byte [rsi]
    cmp al, 0
    je end

    cmp al, 65
    jb skip_transform
    cmp al, 91
    jb transUpper

    cmp al, 97
    jb skip_transform
    cmp al, 123
    jb transLower

skip_transform:
    mov rsi, qword[dest]
    mov byte[rsi], al
    inc qword[source]
    inc qword[dest]
    jmp cycle

transUpper:
    mov bl, al
    mov rax, 0
    mov al, bl
    sub al, 65
    add rax, qword[key]
    cmp rax, 26
    jl skip_fix1
    sub al, 26
skip_fix1:
    add al, 65
    mov rsi, qword[dest]
    mov byte[rsi], al
    inc qword[source]
    inc qword[dest]
    jmp cycle

transLower:
    mov bl, al
    mov rax, 0
    mov al, bl
    sub al, 97
    add rax, qword[key]
    cmp rax, 26
    jl skip_fix2
    sub al, 26
skip_fix2:
    add al, 97
    mov rsi, qword[dest]
    mov byte[rsi], al
    inc qword[source]
    inc qword[dest]
    jmp cycle

end:
    mov byte [dest], 0
    mov rsp, rbp
    pop rbx
    pop rbp
    ret
