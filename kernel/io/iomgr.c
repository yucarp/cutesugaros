#include <string.h>
#include <kernel/kmalloc.h>
#include <kernel/sbd.h>
#include <kernel/object/directory.h>
#include <kernel/object/iomgr.h>
#include <kernel/object/object.h>

struct BlockDevice *IoCreateBlockDevice(struct BlockDeviceFunctions *functions, char *name){
    struct BlockDevice *new_device = malloc(sizeof(struct BlockDevice));
    new_device->functions = functions;
    memcpy(new_device->header.name, name, strlen(name));
    new_device->header.type = 3;
    DirectoryAddChild((void *)ResolveObjectName(ObjGetRootObject(), "./Devices"), (void *)new_device);
    return new_device;
}

struct CharDevice *IoCreateCharDevice(struct CharDeviceFunctions *functions, char *name){
    struct CharDevice *new_device = malloc(sizeof(struct CharDevice));
    new_device->functions = functions;
    memcpy(new_device->header.name, name, strlen(name));
    new_device->header.type = 2;
    DirectoryAddChild((void *)ResolveObjectName(ObjGetRootObject(), "./Devices"), (void *)new_device);
    return new_device;
}

char *IoReadFromBlockDevice(struct BlockDevice *device, long block_num){
    return ((struct BlockDeviceFunctions *)device->functions)->read(device, block_num);
}

void IoWriteToCharDevice(struct CharDevice *device, long offset, char c){
    ((struct CharDeviceFunctions *)device->functions)->write(device, offset, c);
}
