#include <kernel/object/directory.h>
#include <kernel/object/filesystem.h>

struct FileObject *RamfsCreateFile(struct Directory *root, char *name, char *buffer, long size);
