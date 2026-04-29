.section .text
.global _start
_start:
    movq $0, %rbp
    pushq %rbp
    pushq %rbp
    movq %rsp, %rbp

    call main
