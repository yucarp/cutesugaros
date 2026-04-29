#include <stdint.h>
#include <string.h>
#include <kernel/kmalloc.h>
#include <kernel/tokenmgr.h>
#include <kernel/task.h>
#include <kernel/object/object.h>
#include <kernel/object/directory.h>

static struct Directory *RootObject = 0;

void InitializeObjectManager(){
    RootObject = malloc(sizeof(struct Directory));
    RootObject->header.type = 1;
    memcpy(RootObject->header.name, ".", 2);
    CreateDirectory(RootObject, "Devices");
}

struct Directory *ObjGetRootObject(){
    if(KernelCheckToken(1)) return RootObject;
    return 0;
}
