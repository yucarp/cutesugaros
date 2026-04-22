#include <stdint.h>

extern void reload_segments();
extern void flush_tss();

struct GDTPtr {
    uint16_t size;
    uintptr_t pointer;
} __attribute__((packed));

struct GDTDescriptor {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access_byte;
    uint8_t flags_and_limit;
    uint8_t base_high;
};

struct TSS{
    uint32_t reserved;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved_2;
    uint64_t ists[7];
    uint64_t reserved_3;
    uint16_t reserved_4;
    uint16_t io_permission;
    uint16_t reserved_5;
}__attribute__((packed));

struct GDTDescriptor GDT[7] = {
    {0, 0, 0, 0, 0, 0},
    {0xFFFF, 0, 0, 0x9A, 0xAF, 0},
    {0xFFFF, 0, 0, 0x92, 0xCF, 0},
    {0xFFFF, 0, 0, 0xFA, 0xAF, 0},
    {0xFFFF, 0, 0, 0xF2, 0xCF, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0}
};

struct TSS tss = {0};
struct GDTPtr gdt_pointer = {sizeof(GDT) - 1, (uintptr_t)(&GDT)};
uint64_t gs_base[1024] = {0};

void KernelLoadGdt(){
    asm("lgdt %0": : "m"(gdt_pointer): "memory");
    reload_segments();

    uint64_t stack_pointer = 0;
    asm ("mov %%rsp, %0": "=r"(stack_pointer) ::);
    tss.rsp0 = stack_pointer;
    GDT[5].limit_low = sizeof(tss) - 1;
    GDT[5].base_low = (uint64_t)&tss & 0xFFFF;
    GDT[5].base_middle = ((uint64_t)&tss >> 16) & 0xFF;
    GDT[5].access_byte = 0x89;
    GDT[5].flags_and_limit = 0x0F;
    GDT[5].base_high = ((uint64_t)&tss >> 24) & 0xFF;
    GDT[6].limit_low = ((uint64_t)&tss >> 32) & 0xFFFF;
    GDT[6].base_low = ((uint64_t)&tss >> 48) & 0xFFFF;

    uint32_t high = (uint64_t) (&gs_base) >> 32, low = (uint64_t)(&gs_base) & 0xFFFFFFFF;

    asm volatile("wrmsr" : :  "a"(low), "d"(high), "c"(0xC0000102));

    flush_tss();
}
