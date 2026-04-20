#include <kernel/object/directory.h>
#include <kernel/object/filesystem.h>
#include <kernel/kmalloc.h>

struct FileSystem *CreateFilesystem(struct Directory *root, struct FileSystemFunctions fs){
    struct FileSystem *new_fs = malloc(sizeof(struct FileSystem));
    new_fs->functions.read_char_function = fs.read_char_function;
    DirectoryAddChild(root, (void *) new_fs);
    return new_fs;
}

char ReadByte(struct FileObject *fo, long offset){
    return ((struct FileSystemFunctions *) fo->connected_fs)->read_char_function(fo, offset);
}
