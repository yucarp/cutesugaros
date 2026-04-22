#include <string.h>
#include <kernel/kmalloc.h>
#include <kernel/object/directory.h>
#include <kernel/object/filesystem.h>

char RamfsReadByte(struct FileObject *file, long offset){
    if(offset > file->size) return 0;
    return file->data[offset];
}

void RamfsWriteByte(struct FileObject *file, long offset, char byte){
    if(offset > file->size) return;
    file->data[offset] = byte;
}

struct FileSystemFunctions ramfs_functions = {
    (char (*)(struct FileObject *, long)) RamfsReadByte,
    (void (*)(struct FileObject *, long, char)) RamfsWriteByte
};

struct FileObject *RamfsCreateFile(struct Directory *root, char *name, char *buffer, long size){
    struct FileObject *new_fo = malloc(sizeof(struct FileObject));
    new_fo->size = size;
    memcpy(new_fo->header.name, name, 32);
    new_fo->data = buffer;
    new_fo->connected_fs = &ramfs_functions;
    DirectoryAddChild(root, (void *) new_fo);
    return new_fo;
}
