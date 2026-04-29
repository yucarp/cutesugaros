#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern uint64_t invokesystemcall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

FILE _stdout = {0, 0, 0, 0, 0};

FILE *stdout = &_stdout;

void itoa(uint64_t num, int base, char *str){
    uint64_t count = num;
    char *digits = "0123456789ABCDEF";
    int len = 0;
    do{
        ++len;
    } while(count /= base);

    do{
        str[--len] = digits[num % base];
    } while(num /= base);
}

void fputstr(FILE *stream, char *str){
    int syscall_number = 0;

    switch (stream->object->type){
        case 2:
            syscall_number = 5;
            break;
        case 3:
            syscall_number = 3;
            break;
    }

    while(*str){
        invokesystemcall(syscall_number, (uint64_t) stream->object, 0, *str++, 0);
    }
}

FILE * fopen(const char *filename, const char *mode){
    FILE *_file = malloc(sizeof(FILE));
    if(!mode || !filename) return 0;

    _file->object = (void *) invokesystemcall(2, 0, (uint64_t) filename, 0, 0);

    if(!_file->object) return 0;

    return _file;
}

int fgetc(FILE *stream){
    int syscall_number = 0;
    if(!stream) return -1;

    switch (stream->object->type){
        case 2:
            return 0;
        case 5:
            syscall_number = 3;
            break;
    }
    return invokesystemcall(syscall_number, (uint64_t) stream->object, 0, 0, 0);
}

int fputc(FILE *stream, const char c){
    int syscall_number = 0;
    switch (stream->object->type){
        case 2:
            syscall_number = 5;
            break;
        case 5:
            syscall_number = 4;
            break;
    }
    invokesystemcall(syscall_number, (uint64_t) stream->object, stream->position, c, 0);
    return 0;
}

int fprintf(FILE * stream, const char * format, va_list arguments){
    int syscall_number = 0;
    if(!stream || !format) return -1;

   switch (stream->object->type){
        case 2:
            syscall_number = 5;
            break;
        default:
            syscall_number = 4;
            break;
    }
    char num_str[65] = {0};
    char *c = format;
    while(*c){
        if(*c == '%'){
            switch(*++c){
                case '%':
                    invokesystemcall(syscall_number, (uint64_t) stream->object, 0, *c++, 0);
                    continue;
                case 'c':
                    num_str[0] = va_arg(arguments, int);
                    fputstr(stream, num_str);
                    ++c;
                    continue;
                case 'd':
                    int dec = va_arg(arguments, int);
                    itoa(dec, 10, num_str);
                    fputstr(stream, num_str);
                    ++c;
                    continue;
                case 'x':
                    int hex = va_arg(arguments, int);
                    itoa(hex, 16, num_str);
                    fputstr(stream, num_str);
                    ++c;
                    continue;
            }
        }
        invokesystemcall(syscall_number, (uint64_t) stream->object, 0, *c++, 0);

    }
    return 1;
}

int printf(const char *format, ...){
    va_list arguments;
    va_start(arguments, format);
    if (stdout->object) {fprintf(stdout, format, arguments); return 0;}
    else return 1;
}
