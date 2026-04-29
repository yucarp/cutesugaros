#include <stdint.h>

struct Processor {
    struct ObjectHeader header;
    uint32_t id;
    uint32_t flags;
};

struct Processor *CreateProcessor(struct Directory *root, uint32_t id, uint32_t flags);
