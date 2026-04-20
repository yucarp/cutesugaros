#include <limine.h>
#include <stdint.h>
#include <string.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/kmalloc.h>
#include <kernel/mmu.h>
#include <kernel/sbd.h>
#include <kernel/object/directory.h>
#include <kernel/object/object.h>
#include <kernel/object/processor.h>

struct RSDP {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
}__attribute__((packed));

struct SDTHeader {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
}__attribute__((packed));

struct RSDT {
    struct SDTHeader sdt_header;
    uint32_t pointer_to_other_tables;
}__attribute__ ((packed));

struct MADT {
    struct SDTHeader sdt_header;
    uint32_t lapic_address;
    uint32_t flags;
}__attribute__((packed));

static struct MADT *madt = 0;
static uintptr_t lapic_address = 0;
static uintptr_t ioapic_address = 0;
static uintptr_t initial_stacks[16][512] = {0};
static uintptr_t *stack_pointer = (uintptr_t *) &initial_stacks;
static volatile int wait = 0;

uint32_t read_lapic(uint32_t offset){
    return ((uint32_t *)lapic_address)[offset / 4];
}

void write_lapic(uint32_t offset, uint32_t value){
    ((uint32_t *)lapic_address)[offset / 4] = value;
}

uint32_t read_ioapic(uint32_t reg){
    ((uint32_t *)ioapic_address)[0] = reg;
    return ((uint32_t *)ioapic_address)[4];
}

void write_ioapic(uint32_t reg, uint32_t value){
    ((uint32_t *)ioapic_address)[0] = reg;
    ((uint32_t *)ioapic_address)[4] = value;
}

uint64_t rdtsc(){
    uint32_t low, high;
    asm volatile("rdtsc":"=a"(low),"=d"(high));
    return ((uint64_t)high << 32) | low;
}

void ap_start(){
    KernelLoadGdt();
    HalLoadIdtAp();
    kprint("Processor initialized!");
    wait = 1;
    while (1) asm("hlt");
}

void HalInitializeProcessors(){
    uint32_t high, low;

    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0x1B));

    lapic_address = ((((uint64_t)high << 32) | low) & ~0xFFF) + KernelGetHhdmOffset();

    char* processor_str = "./Devices/Processor\0\0";

    uintptr_t cr3 = 0;
    asm("mov %%cr3, %0" : "=r"(cr3) : );
    extern uintptr_t kernel_pml4[512];
    extern void ap_trampoline();

    KernelMapPage(kernel_pml4, 0x8000, 0x8000);

    memcpy((void *)0x8000, &ap_trampoline, 0x1000);
    memcpy((void *)0x8C00, &cr3, 0x8);
    for(int i = 2; i < 10; ++i){
        stack_pointer = initial_stacks[i];
        memcpy((void *)0x8F00, &stack_pointer, 0x8);
        processor_str[19] = '0' + i;
        struct Processor *processor = (void *) ResolveObjectName(ObjGetRootObject(), processor_str);
        if(processor){
            wait = 0;
            write_lapic(0x280, 0);
            write_lapic(0x310, (read_lapic(0x310) & 0x00ffffff) | (processor->id << 24));
            write_lapic(0x300, (read_lapic(0x300) & 0xfff00000) | 0x00C500);
            write_lapic(0x310, (read_lapic(0x310) & 0x00ffffff) | (processor->id << 24));
            write_lapic(0x300, (read_lapic(0x300) & 0xfff00000) | 0x008500);

            write_lapic(0x280, 0);
            write_lapic(0x310, (read_lapic(0x310) & 0x00ffffff) | (processor->id << 24));
            write_lapic(0x300, (read_lapic(0x300) & 0xfff0f800) | 0x000608);
        }
        do {asm("pause");} while(!wait);
    }

    KernelUnmapPage(kernel_pml4, 0x8000);
}

void HalUnmaskInterrupt(uint8_t interrupt_no){
    uint32_t ready_data = read_ioapic(interrupt_no * 2 + 0x10);
    write_ioapic(interrupt_no * 2 + 0x10, ready_data & ~0x10000);
}

void HalMapInterrupt(uint8_t interrupt_no, uint8_t vector){
    uint32_t ready_data = read_ioapic(interrupt_no * 2 + 0x10);
    write_ioapic(interrupt_no * 2 + 0x10, ready_data | vector);
}

void HalEndOfInterrupt(){
    write_lapic(0xB0, 0);
}

void HalEnableTimer(){
    uint32_t high, low;

    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0x1B));

    lapic_address = ((((uint64_t)high << 32) | low) & ~0xFFF) + KernelGetHhdmOffset();

    uint64_t old_time = rdtsc();
    write_lapic(0x3E0, 0x3);
    write_lapic(0x380, 0xFFFFFFFFF);

    while(rdtsc() < old_time + 100000){
    }

    write_lapic(0x320, 0x10000);
    uint32_t ticks = 0xFFFFFFFFF - read_lapic(0x390);

    write_lapic(0xF0, 0x127);
    write_lapic(0x320, 0x20020);
    write_lapic(0x3E0, 0x3);
    write_lapic(0x380, ticks);
    asm("sti");
}

void KernelParseAcpi(){
    struct RSDP *rsdp_ptr = (void *)((uintptr_t)KernelGetRsdpAddress() | KernelGetHhdmOffset());
    struct RSDT *rsdt = (void *)(rsdp_ptr->rsdt_address | KernelGetHhdmOffset());

    struct Directory *device_dir = (void *) ResolveObjectName(ObjGetRootObject(), "./Devices");

    uint32_t *other_headers = (void *)&(rsdt->pointer_to_other_tables);
    for(int i = 0; i <= (rsdt->sdt_header.length - sizeof(struct SDTHeader)) / 4; ++i){
        struct SDTHeader *header = (void *)(other_headers[i] + KernelGetHhdmOffset());
        if(strncmp("APIC", header->signature, 4) == 0) {
            madt = (void *)header;
        }
    }

    if(madt){
        uint8_t *pointer = (uint8_t *)((uint64_t)madt + 0x2c);
        for(int i = 0; i < madt->sdt_header.length - sizeof(struct SDTHeader) - 8;){
            //kprint("Entry type: %d\n", *(pointer + i)); For debugging purposes
            if(*(pointer + i) == 0){
                CreateProcessor(device_dir, *(pointer + i + 3), 0);
            } else if (*(pointer + i) == 1){
                ioapic_address = *((uint32_t *)(pointer + i + 4)) | KernelGetHhdmOffset();
            }
            i += *(pointer + i + 1);
        }
    }
}
