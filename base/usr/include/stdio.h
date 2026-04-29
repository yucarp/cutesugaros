#include <stdarg.h>
#include <kernel/object/object.h>

typedef struct {
    struct ObjectHeader *object;
    long position;
    void *buffer;
    char eof;
    char error;
} FILE;

extern FILE *stdout;

FILE *fopen(const char *filename, const char *mode);
int fprintf(FILE * stream, const char * format, va_list arguments);
int printf(const char *format, ...);
int fgetc(FILE *stream);
int fputc(FILE *stream, char c);
