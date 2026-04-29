#include <stdarg.h>
#include <stdint.h>
#include <kernel/object/iomgr.h>

static struct CharDevice *char_dev = 0;

void ChangeStandardOutput(struct CharDevice *cd){
    char_dev = cd;
}

void itoaa(uint64_t num, int base, char *str){
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

void putstrr(char *str){
    while(*str){
        IoWriteToCharDevice(char_dev, 0, *str++);
    }
}

void dprintf(char *str, ...){
    va_list arguments;
    char num_str[65] = {0};
    va_start(arguments, str);
    char *c = str;
    while(*c){
        if(*c == '%'){
            switch(*++c){
                case '%':
                    IoWriteToCharDevice(char_dev, 0, *str++);
                    continue;
                case 'c':
                    num_str[0] = va_arg(arguments, int);
                    putstrr(num_str);
                    ++c;
                    continue;
                case 'd':
                    int dec = va_arg(arguments, int);
                    itoaa(dec, 10, num_str);
                    putstrr(num_str);
                    ++c;
                    continue;
                case 'x':
                    int hex = va_arg(arguments, int);
                    itoaa(hex, 16, num_str);
                    putstrr(num_str);
                    ++c;
                    continue;
            }
        }
        IoWriteToCharDevice(char_dev, 0, *c++);

    }
}
