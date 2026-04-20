#pragma once

#include <stdint.h>
#include <kernel/object/object.h>

struct FileObject {
    struct ObjectHeader header;
    char *data;
    long size;
    void *connected_fs;
};

struct FileSystemFunctions {
    char (*read_char_function) (struct FileObject *, long);
};

struct FileSystem {
    struct ObjectHeader header;
    void *fs_data;
    struct FileSystemFunctions functions;
};

struct FileSystem *CreateFilesystem(struct Directory *root, struct FileSystemFunctions fs);

char ReadByte(struct FileObject *fo, long offset);
