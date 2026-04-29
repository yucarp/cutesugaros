#pragma once

#include <stdint.h>
#include <kernel/object/iomgr.h>
#include <kernel/object/object.h>

struct FileObject {
    struct ObjectHeader header;
    char *data;
    long size;
    void *connected_fs;
};

struct FileSystemFunctions {
    char (*read_char_function) (struct FileObject *, long);
    void (*write_char_function) (struct FileObject *, long, char);
};

struct FileSystem {
    struct ObjectHeader header;
    void *fs_data;
    struct BlockDevice *block_device;
    struct FileSystemFunctions functions;
};

struct FileSystem *CreateFilesystem(struct Directory *root, struct FileSystemFunctions fs);

char ReadByte(struct FileObject *fo, long offset);
void WriteByte(struct FileObject *fo, long offset, char byte);
