#pragma once

#include <kernel/object/object.h>

struct BlockDevice{
    struct ObjectHeader header;
    uint16_t block_size;
    uint64_t specific_info;
    void *functions;
};

struct CharDevice{
    struct ObjectHeader header;
    uint16_t block_size;
    uint64_t specific_info;
    void *functions;
};

struct BlockDeviceFunctions {
    char * (*read)(struct BlockDevice *, long);
};

struct CharDeviceFunctions {
    void (*write)(struct CharDevice *, long, char);
};

struct BlockDevice *IoCreateBlockDevice(struct BlockDeviceFunctions* functions, char *name);
struct CharDevice *IoCreateCharDevice(struct CharDeviceFunctions* functions, char *name);
struct BlockDevice *IoCreateRamdisk(char* buffer, uint16_t block_size);
char *IoReadFromBlockDevice(struct BlockDevice *device, long block_num);
void IoWriteToCharDevice(struct CharDevice *device, long offset, char);
