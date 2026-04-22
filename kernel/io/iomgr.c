#include <kernel/kmalloc.h>
#include <kernel/object/directory.h>
#include <kernel/object/iomgr.h>
#include <kernel/object/object.h>

struct BlockDevice *IoCreateBlockDevice(struct BlockDeviceFunctions *functions){
    struct BlockDevice *new_device = malloc(sizeof(struct BlockDevice));
    new_device->functions = functions;
    DirectoryAddChild((void *)ResolveObjectName(ObjGetRootObject(), "./Devices"), (void *)new_device);
    return new_device;
}

char *IoReadFromBlockDevice(struct BlockDevice *device, long block_num){
    return ((struct BlockDeviceFunctions *)device->functions)->read(device, block_num);
}
