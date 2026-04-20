#include <stdint.h>
#include <string.h>
#include <kernel/kmalloc.h>
#include <kernel/sbd.h>
#include <kernel/object/directory.h>
#include <kernel/object/processor.h>

static int processor_count = 0;

struct Processor *CreateProcessor(struct Directory *root, uint32_t id, uint32_t flags){
    struct Processor *new_processor = malloc(sizeof(struct Processor));
    new_processor->header.type = 2;
    memcpy(new_processor->header.name, "Processor\0\0\0\0", 32);
    new_processor->header.name[9] = '0' + ++processor_count;
    new_processor->id = id;
    new_processor->flags = flags;
    DirectoryAddChild(root, (void *)new_processor);
    return new_processor;
}
