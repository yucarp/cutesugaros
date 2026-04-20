#include <stdint.h>

extern void reload_segments();

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

struct GDTDescriptor GDT[5] = {
    {0, 0, 0, 0, 0, 0},
    {0xFFFF, 0, 0, 0x9A, 0xAF, 0},
    {0xFFFF, 0, 0, 0x92, 0xCF, 0},
    {0xFFFF, 0, 0, 0xFA, 0xAF, 0},
    {0xFFFF, 0, 0, 0xF2, 0xCF, 0}
};

struct GDTPtr gdt_pointer = {sizeof(GDT) - 1, (uintptr_t)(&GDT)};

void KernelLoadGdt(){
    asm("lgdt %0": : "m"(gdt_pointer): "memory");
    reload_segments();
}
