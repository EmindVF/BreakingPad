section .data
    filename dq 0
    dest dq 0
    error dq 0 
    fd dq 0

section .text
    global readFile
    extern malloc

readFile:
    ; Function prologue
    push rbp
    mov rbp, rsp

    ;mov rdi, qword [rbp + 16] ; filename
    ;mov rsi, qword [rbp + 24] ; dest
    ;mov rdx, qword [rbp + 32] ; error

    mov qword [filename], rdi ; filename
    mov qword [dest], rsi ; dest
    mov qword [error], rdx ; error

    ; Open the file
    mov rax, 2            
    mov rdi, qword [filename]        
    mov rsi, 0         
    mov rdx, 0      
    syscall

    cmp rax, 0            
    jl handle_error      

    mov qword [fd], rax

    mov rdi, qword [fd] 
    mov rax, 8 
    mov rsi, 0 
    mov rdx, 2 
    syscall
    mov r8, rax 

    mov rax, 3 ; 
    mov rdi, qword [fd]
    syscall

    ; Open the file
    mov rax, 2         
    mov rdi, qword [filename]      
    mov rsi, 0         
    mov rdx, 0      
    syscall

    mov qword [fd], rax

    ;mov rdx, qword [dest]
    ;mov qword [rdx], rax

    ;jmp debugskip

    mov rdi, r8
    inc rdi
    call malloc
    mov byte [rax], 1

    mov rdi, qword[fd] 
    mov rsi, rax 
    mov rdx, r8 
    mov rax, 0 
    syscall

    mov rdx, qword [dest]
    ;mov rbx, [rax]
    mov qword [rdx], rsi

    add rsi, r8
    mov byte[rsi],0

    mov rax, 3 ; sys_close system call
    mov rdi, qword [fd] ; file descriptor
    syscall

debugskip:
    pop rbp
    ret

handle_error:
    ; Set error to 1 (failure)
    mov rdx, qword [error]
    mov byte [rdx], 1

    pop rbp
    ret