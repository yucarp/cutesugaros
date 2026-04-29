#include <stdint.h>
#pragma once

struct ObjectHeader{
    char name[64];
    char *extended_name;
    uint64_t type;
    uint64_t reference_count;
};

struct ObjectHeaderList {
    struct ObjectHeader *item;
    struct ObjectHeaderList *next;
};

struct ObjectHandle {
    void *referred_object;
};

void InitializeObjectManager();
struct Directory *ObjGetRootObject();
