#include <stddef.h>

void * memcpy(void * restrict dst, const void * restrict src, size_t n);
void * memset(void * dst, int c, size_t n);
int strncmp(const char *src, const char *dst, size_t n);
int strlen(char *str);
char *strcat(char *dst, const char *src);
