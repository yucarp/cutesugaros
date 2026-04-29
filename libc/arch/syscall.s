.global invokesystemcall
invokesystemcall:
    movq %rdi, %rax
    movq %rsi, %rdi
    movq %rdx, %rsi
    movq %rcx, %rdx
    movq %rdx, %r9

    int $128
    ret
