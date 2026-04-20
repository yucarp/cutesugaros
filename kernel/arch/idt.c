#include <stdint.h>
#include <kernel/acpi.h>
#include <kernel/idt.h>
#include <kernel/sbd.h>
#include <kernel/task.h>

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

void (*irq_entries[16]) (struct x86Registers*) = {0};

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
    else if(regs->interrupt_no >= 32) { HalEndOfInterrupt(); acpi_timer_handler(); kprint("Hello");}
    else {kprint("An error has occured, fault no: %x", regs->interrupt_no); asm("hlt");}
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
