#include <stdarg.h>
#include <stdint.h>
#include <kernel/port.h>

#define COM1_PORT 0x3F8

int KernelStartSerialDebugging(){
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT, 0x03);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x1E);
    outb(COM1_PORT, 0xAA);

    if(inb(COM1_PORT) != 0xAA) return -1;

    outb(COM1_PORT + 4, 0x03);
    return 0;
}

void serial_send_data(char c){
    while(!(inb(COM1_PORT + 5) & 0x20));
    outb(COM1_PORT, c);
}

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

void putstr(char *str){
    while(*str){
        serial_send_data(*str++);
    }
}

void kprint(char *str, ...){
    va_list arguments;
    char num_str[65] = {0};
    va_start(arguments, str);
    char *c = str;
    while(*c){
        if(*c == '%'){
            switch(*++c){
                case '%':
                    serial_send_data(*c++);
                    continue;
                case 'd':
                    int dec = va_arg(arguments, int);
                    itoa(dec, 10, num_str);
                    putstr(num_str);
                    ++c;
                    continue;
                case 'x':
                    int hex = va_arg(arguments, int);
                    itoa(hex, 16, num_str);
                    putstr(num_str);
                    ++c;
                    continue;
            }
        }
        serial_send_data(*c++);

    }
}
