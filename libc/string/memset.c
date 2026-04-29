#include <stddef.h>

void * memset(void * dst, int c, size_t n) {
    asm volatile("cld; rep stosb" : "=c"((int){0}) : "rdi"(dst), "a"(c), "c"(n): "flags", "memory", "rdi");
    return dst;
}
