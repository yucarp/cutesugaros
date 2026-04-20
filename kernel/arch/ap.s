.code16
.org 0x8000

.global ap_trampoline
ap_trampoline:
    cli

    mov $0xA0, %eax
    mov %eax, %cr4

    mov $0x8C00, %edx
    mov (%edx), %edx
    mov %edx, %cr3

    mov $0xC0000080, %ecx
    rdmsr
    or $0x100, %eax
    wrmsr

    mov $0x80000011, %ebx
    mov %ebx, %cr0

    lgdtl (.gdt_pointer - ap_trampoline + 0x8000)
    ljmp $0x08, $(.long_mode - ap_trampoline + 0x8000)

.gdt:
    .long 0, 0
    .long 0x0000FFFF, 0x00AF9A00
    .long 0x0000FFFF, 0x00CF9200
.gdt_pointer:
    .word .gdt_pointer - .gdt - 1
    .long .gdt - ap_trampoline + 0x8000
    .long 0
.code64
.align 16
.long_mode:
    mov $0x8F00, %rdx
    mov (%rdx), %rsp
    .extern ap_start
    lea ap_start, %rax
    callq %rax

