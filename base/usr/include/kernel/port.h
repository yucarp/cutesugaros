#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value){
    asm volatile("outb %b0, %w1" :: "a"(value), "Nd"(port) : "memory");
}

static inline void outw(uint16_t port, uint16_t value){
    asm volatile("outw %w0, %w1" :: "a"(value), "Nd"(port) : "memory");
}

static inline void outl(uint16_t port, uint32_t value){
    asm volatile("outl %0, %w1" :: "a"(value), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port){
    uint8_t value = 0;
    asm volatile("inb %w1, %b0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}

static inline uint16_t inw(uint16_t port){
    uint16_t value = 0;
    asm volatile("inw %w1, %w0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}

static inline uint32_t inl(uint16_t port){
    uint32_t value = 0;
    asm volatile("inl %w1, %0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}

void iowait();
