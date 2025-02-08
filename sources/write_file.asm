section .data
    filename dq 0
    source dq 0
    error dq 0 

section .text
    global writeFile

writeFile:
    ; Function prologue
    push rbp
    mov rbp, rsp

    ;mov rdi, qword [rbp + 16] ; filename
    ;mov rsi, qword [rbp + 24] ; source
    ;mov rdx, qword [rbp + 32] ; error

    mov qword [filename], rdi ; filename
    mov qword [source], rsi ; source
    mov qword [error], rdx ; error

    ;mov byte [rdx], 1

    ; Open the file
    mov rax, 2            ; syscall number for `open`
    mov rdi, qword [filename]          ; filename
    mov rsi, 641 | 64           ; flags:O_WRONLY | O_CREAT | O_TRUNC
    mov rdx, 0o777        
    syscall

    cmp rax, 0            ; check if file descriptor is valid
    jnl skip_error       ; jump to error handling if negative value returned

    ; Open the file
    mov rax, 2            ; syscall number for `open`
    mov rdi, qword [filename]          ; filename
    mov rsi, 641           ; flags:O_WRONLY | O_CREAT | O_TRUNC
    mov rdx, 0o777        
    syscall

    cmp rax, 0            ; check if file descriptor is valid
    jl handle_error       ; jump to error handling if negative value returned

skip_error:
    ; Write to the file
    mov rdi, rax    ;file descriptor
    mov rsi, qword [source]   

writeCycle:
    mov rax, 0
    mov al, byte[rsi]
    cmp rax, 0
    je writeEnd
    
    mov rax, 1            ; syscall number for `write`
    mov rdx, 1        ; length
    syscall

    inc rsi
    jmp writeCycle

writeEnd:
    mov rax, 3            ; syscall number for `close`
    ;mov rdi, rdi          ; file descriptor
    syscall

    ; Function epilogue
    pop rbp
    ret

handle_error:
    ; Set error to 1 (failure)
    mov rdx, qword [error]
    mov byte [rdx], 1

    ; Function epilogue
    pop rbp
    ret