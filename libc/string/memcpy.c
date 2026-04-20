#include <stddef.h>

void * memcpy(void * restrict dst, const void * restrict src, size_t n) {
    asm volatile("cld; rep movsb" : "=c"((int){0}) : "D"(dst), "S"(src), "c"(n) : "flags", "memory");
    return dst;
}
