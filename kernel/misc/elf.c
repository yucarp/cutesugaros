#include <limine.h>
#include <string.h>
#include <kernel/mmu.h>
#include <kernel/sbd.h>
#include <kernel/task.h>

struct ELFHeader {
    uint8_t magic_number[4];
    uint8_t bits;
    uint8_t endian;
    uint8_t header_version;
    uint8_t abi;
    uint8_t padding[8];
    uint16_t type;
    uint16_t instruction_set;
    uint32_t elf_version;
    uint64_t program_entry_offset;
    uint64_t program_header_offset;
    uint64_t section_header_offset;
    uint32_t flags;
    uint16_t elf_header_size;
    uint16_t program_table_entry_size;
    uint16_t program_table_entry_number;
    uint16_t section_table_entry_size;
    uint16_t section_table_entry_number;
    uint16_t string_table_index;
};

struct ELFProgramHeader{
    uint32_t segment_type;
    uint32_t flags;
    uint64_t file_offset;
    uint64_t virtual_address;
    uint64_t physical_address;
    uint64_t file_size;
    uint64_t memory_size;
    uint64_t alignment;
};

struct ELFSectionHeader{
    uint32_t name;
    uint32_t type;
    uint64_t flags;
    uint64_t address;
    uint64_t offset;
    uint64_t size;
    uint32_t link;
    uint32_t info;
    uint64_t alignment;
    uint64_t entry_size;
};

struct ELFRela{
    uint64_t offset;
    uint64_t info;
    int64_t addend;
};

struct ELFSym {
    uint32_t name;
    uint8_t info;
    uint8_t other;
    uint16_t section_header_index;
    uint64_t value;
    uint64_t size;
};

void KernelLoadElfLibrary(uint8_t *ptr){

}

void *KernelLoadElfExecutable(uint8_t *ptr){
    struct ELFHeader *header = (void *)ptr;
    if(header->magic_number[0] != 0x7F) return 0;

    struct ELFProgramHeader *pheader = (void *) (ptr + header->program_header_offset);
    for(int i = 0; i < header->program_table_entry_number; ++i){
        KernelMapPage((void *) (GetCurrentProcess()->cr3 + KernelGetHhdmOffset()), pheader->virtual_address, KernelAllocateFrame(), 1);
        memcpy((void *)pheader->virtual_address, ptr + pheader->file_offset, pheader->file_size);
        pheader = (void *)((char *)pheader + header->program_table_entry_size);
    }

    return (void *) header->program_entry_offset;
}

void KernelResolveElfRelocations(uint8_t *ptr){
    struct ELFHeader *header = (void *)ptr;
    if(header->magic_number[0] != 0x7F) return;

    struct ELFSectionHeader *sheader = (void *) ((uint64_t) header + header->section_header_offset);

    for(int i = 0; i < header->section_table_entry_number; ++i){
        if(sheader[i].type == 4){
            struct ELFRela *relocation_table =  (void *)((uint64_t) header + sheader[i].offset);
            struct ELFSym *symbol_table = (void *)((uint64_t) header + sheader[sheader[i].link].offset);
            char *string_table = (void *)((uint64_t) header + sheader[sheader[i].link + 1].offset);
            for(int j = 0; j < sheader[i].size / sheader[i].entry_size; ++j){
                kprint(string_table + symbol_table[relocation_table[j].info >> 32].name);
            }
        }
    }
}
