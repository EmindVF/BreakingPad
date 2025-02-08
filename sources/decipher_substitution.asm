section .data
    source dq 0
    dest dq 0 
    key dq 0 
    keyr resb 26

section .text
    global aDecipherSubstitution

aDecipherSubstitution:
    push rbp
    push rbx
    mov rbp, rsp

    mov qword [source], rdi ; source
    mov qword [dest], rsi ; dest
    mov qword [key], rdx ; key

; reverse key map

mov rcx, 0
mov rsi, qword[key]
rev_cycle:
    cmp rcx, 26
    je skip_rev

    mov rax, 0
    mov al, byte [rsi]

    mov rdx, keyr
    add rdx, rax

    mov byte[rdx], cl

    inc rsi
    inc rcx
    jmp rev_cycle

jmp end

skip_rev:    

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
    
    mov rsi, keyr
    add rsi, rax

    mov al, byte [rsi]
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
    
    mov rsi, keyr
    add rsi, rax

    mov al, byte [rsi]
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