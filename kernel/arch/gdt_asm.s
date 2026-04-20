.global reload_segments
reload_segments:
    push $0x08
    leaq reload_segments_final, %rax
    push %rax
    retfq

reload_segments_final:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %gs
    mov %ax, %ss
    ret
