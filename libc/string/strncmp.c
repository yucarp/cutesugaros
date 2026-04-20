#include <stddef.h>

int strncmp(const char *src, const char *dst, size_t n){
    if(n == 0) return 0;

    while(n-- && *dst == *src){
        if(!n || !*src) break;
        ++dst;
        ++src;
    }

    return (*(unsigned char *)src) - (*(unsigned char *)dst);
}
