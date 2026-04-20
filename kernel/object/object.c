#include <stdint.h>
#include <string.h>
#include <kernel/kmalloc.h>
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
    return RootObject;
}
