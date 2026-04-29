#include <kernel/kmalloc.h>
#include <kernel/object/directory.h>
#include <kernel/object/iomgr.h>
#include <kernel/object/object.h>

char *IoReadBlockFromRamdisk(struct BlockDevice *ramfs, long block_num);

struct BlockDeviceFunctions ramdisk_functions = {
    IoReadBlockFromRamdisk
};

struct BlockDevice *IoCreateRamdisk(char* buffer, uint16_t block_size){
    struct BlockDevice *new_ramdisk = IoCreateBlockDevice(&ramdisk_functions, "Ramdisk");
    new_ramdisk->specific_info = (uint64_t) buffer;
    new_ramdisk->block_size = block_size;
    return new_ramdisk;
}

char *IoReadBlockFromRamdisk(struct BlockDevice *ramdisk, long block_num){
    return ((char *)ramdisk->specific_info) + block_num * ramdisk->block_size;
}
