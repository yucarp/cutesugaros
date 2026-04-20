#pragma once

#include <stdint.h>
#include <kernel/object/object.h>

struct Directory {
    struct ObjectHeader header;
    struct ObjectHeaderList *header_list;
};

void DirectoryAddChild(struct Directory *directory, struct ObjectHeader *object);
void DirectoryRemoveChild(struct Directory *directory, struct ObjectHeader *object);
struct Directory *CreateDirectory(struct Directory *root, char *name);
struct ObjectHeader *ResolveObjectName(struct Directory *root, char *name);

