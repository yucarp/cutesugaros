#include <stdint.h>
#include <kernel/acpi.h>
#include <kernel/elf.h>
#include <kernel/idt.h>
#include <kernel/mmu.h>
#include <kernel/sbd.h>
#include <kernel/spinlock.h>
#include <kernel/object/directory.h>
#include <kernel/object/filesystem.h>
#include <kernel/object/iomgr.h>
#include <kernel/task.h>

#define SYSCALL_FUNCTION uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t)
struct IDTEntry {
    uint16_t offset_low;
    uint16_t segment_selector;
    uint8_t ist;
    uint8_t type;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct IDTPointer {
    uint16_t size;
    void *base;
}__attribute__((packed));

struct IDTEntry idt[256] = {0};
struct IDTPointer idtp = {0, 0};
struct Spinlock sp = {0};

void (*irq_entries[16]) (struct x86Registers*) = {0};
uint64_t (*syscall_entries[9]) (uint64_t, uint64_t, uint64_t, uint64_t) = {
    (SYSCALL_FUNCTION) GetCurrentProcessId,
    (SYSCALL_FUNCTION) KernelCloneProcess,
    (SYSCALL_FUNCTION) ResolveObjectName,
    (SYSCALL_FUNCTION) ReadByte,
    (SYSCALL_FUNCTION) WriteByte,
    (SYSCALL_FUNCTION) IoWriteToCharDevice,
    (SYSCALL_FUNCTION) KernelExpandHeap,
    (SYSCALL_FUNCTION) ChangeWorkingDirectory,
    (SYSCALL_FUNCTION) KernelLoadElfExecutable
};

void set_idt_entry(int index, uintptr_t handler, uint16_t segment_selector, uint8_t type, uint8_t ist){
    idt[index].offset_low = (handler & 0xFFFF);
    idt[index].offset_middle = ((handler >> 16) & 0xFFFF);
    idt[index].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[index].segment_selector = segment_selector;
    idt[index].type = type;
    idt[index].ist = ist;
}

void acpi_timer_handler(){
    KernelSwitchProcess();
}

void isr_handler(struct x86Registers *regs){
    if (regs->interrupt_no == 2){
        asm("cli");
        asm("hlt");
    }
    else if(regs->interrupt_no >= 32) {
        HalEndOfInterrupt();
        if(irq_entries[regs->interrupt_no - 32]) irq_entries[regs->interrupt_no - 32](regs);
    }

    else {kprint("An error has occured, fault no: %x", regs->interrupt_no); asm("hlt");}
}

void syscall_handler_inner(struct x86Registers *regs){
    if(regs->rax > 8) return;
    if(regs->rax == 1){regs->rax = syscall_entries[regs->rax](regs->rip, regs->ursp, regs->rdx, regs->rcx);}
    else regs->rax = syscall_entries[regs->rax](regs->rdi, regs->rsi, regs->rdx, regs->rcx);
}

void HalSetIrqEntry(int index, void *function){
    if(index > 15) return;
    irq_entries[index] = function;
}

void HalInitializeInterrupts(){
    idtp.base = &idt;
    idtp.size = sizeof(idt);

    void *isr_functions[32] = {_isr0, _isr1, _isr2, _isr3, _isr4, _isr5, _isr6, _isr7, _isr8, _isr9, _isr10,
        _isr11, _isr12, _isr13, _isr14, _isr15, _isr16, _isr17, _isr18, _isr19, _isr20, _isr21, _isr22, _isr23,
        _isr24, _isr25, _isr26, _isr27, _isr28, _isr29, _isr30, _isr31
    };

    void *irq_functions[16] = {_irq0, _irq1, _irq2, _irq3, _irq4, _irq5, _irq6, _irq7, _irq8, _irq9, _irq10, _irq11, _irq12, _irq13, _irq14, _irq15};

    for(int i = 0; i < 32; ++i){
        set_idt_entry(i, (uintptr_t) isr_functions[i], 0x08, 0x8E, 0);
    };

    for(int i = 0; i < 16; ++i){
        set_idt_entry(32 + i, (uintptr_t) irq_functions[i], 0x08, 0x8E, 0);
    };

    extern void syscall_handler(struct x86Registers *);
    set_idt_entry(128, (uintptr_t)syscall_handler, 0x08, 0x8E | 0x60, 0);
    asm volatile (
        "lidt %0"
        : : "m"(idtp)
    );
}


void HalLoadIdtAp(){
    asm volatile (
        "lidt %0"
        : : "m"(idtp)
    );
}
