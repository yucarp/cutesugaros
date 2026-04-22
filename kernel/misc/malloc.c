//Heavily inspired from ToaruOS

#include <stddef.h>
#include <stdint.h>
#include <kernel/mmu.h>
#include <kernel/sbd.h>

#define MIN_SIZE 3
#define MAX_SIZE 11

struct MallocBinHeader {
    struct MallocBinHeader *next;
    void *head;
    int size;
};

struct MallocBigBinHeader {
    struct MallocBigBinHeader *next;
    void *head;
    int size;
    char used;
};

struct MallocBinHeader *bin_headers[10] = {0};
struct MallocBigBinHeader *first_big_bin_header = 0;
void push_to_stack(struct MallocBinHeader *header, void *ptr){
    uintptr_t **item = (uintptr_t **)ptr;
    *item = (uintptr_t *)header->head;
    header->head = item;
}

uintptr_t *pop_from_stack(struct MallocBinHeader *header){
    void *item = header->head;
    uintptr_t **head = header->head;
    uintptr_t *next = *head;
    header->head = next;
    return item;
}

int malloc_needs_expansion(struct MallocBinHeader *header){
        if(!header) return 1;
        if(header->head == 0) return 1;
        return 0;
}

void malloc_list_add(struct MallocBinHeader *header, int list_index){
    header->next = bin_headers[list_index];
    bin_headers[list_index] = header;
}

void malloc_list_decouple(int list_index, struct MallocBinHeader *node){
    struct MallocBinHeader *next = node->next;
    bin_headers[list_index] = next;
    node->next = NULL;
}

void *malloc(size_t size){
    size /= 8;
    int list = 0;
    for(; size; size /= 2) ++list;

    if(list >= 9) {
        if(!first_big_bin_header){
            first_big_bin_header = (struct MallocBigBinHeader *)KernelExpandHeap();
            first_big_bin_header->head = (void *)((uintptr_t)first_big_bin_header + sizeof(struct MallocBigBinHeader));
            first_big_bin_header->size = list;
            first_big_bin_header->used = 1;
            first_big_bin_header->next = 0;
            for(int i = 1; i <= (2 << (list - 9)); ++i){
                KernelExpandHeap();
            }
            return first_big_bin_header->head;
        } else {
            struct MallocBigBinHeader *traversal = (struct MallocBigBinHeader *)first_big_bin_header;
            while(traversal->next){
                if((traversal->used == 0) && traversal->size >= list){
                    return traversal->head;
                }
                traversal = traversal->next;
            }
            traversal->next = (struct MallocBigBinHeader *)KernelExpandHeap();
            traversal->next->head = (void *)((uintptr_t)traversal->next + sizeof(struct MallocBigBinHeader));
            traversal->next->size = size;
            traversal->next->used = 1;
            traversal->next->next = 0;
            for(int i = 1; i <= (2 << (list - 9)); ++i){
                KernelExpandHeap();
            }
            return traversal->next->head;
        }
    }

    if(malloc_needs_expansion(bin_headers[list])){
        struct MallocBinHeader *bin_header = (struct MallocBinHeader *)KernelExpandHeap();
        bin_header->head = (void *) ((uintptr_t)bin_header + sizeof(struct MallocBinHeader));

        malloc_list_add(bin_header, list);

        uintptr_t adj = MIN_SIZE + list;
        uintptr_t available = ((0x1000 - sizeof(struct MallocBinHeader)) >> adj) - 1;

        uintptr_t **base = bin_header->head;

        for(int i = 0; i < available; ++i){
            base[i << list] = (uintptr_t *)&base[(i + 1) << list];
        }
        base[available << list] = NULL;
        bin_header->size = list;

    }

    uintptr_t *mem_address = pop_from_stack(bin_headers[list]);

    if(bin_headers[list]->head == 0){
        malloc_list_decouple(list, bin_headers[list]);
    }
    return mem_address;

}

void free(void *memory_address){
    if((uintptr_t) memory_address & 0xFFF) memory_address = (void *) ((uintptr_t) memory_address & ~0xFFF);

    struct MallocBinHeader *header = (struct MallocBinHeader *)memory_address;

    if(header->size >= 9){
        struct MallocBigBinHeader *big_header = (struct MallocBigBinHeader *)memory_address;
        big_header->used = 0;
        return;
    }

    push_to_stack(header, memory_address);

    if(header->head != 0) {
        malloc_list_add(header, header->size);
    }
}
