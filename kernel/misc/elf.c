#include <limine.h>
#include <string.h>
#include <kernel/kmalloc.h>
#include <kernel/mmu.h>
#include <kernel/pci.h>
#include <kernel/sbd.h>
#include <kernel/task.h>
#include <kernel/object/filesystem.h>
#include <kernel/object/iomgr.h>

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
}__attribute__((packed));

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
}__attribute__((packed));

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

struct FunctionSymbolList {
    char name[256];
    void *base_address;
    void *function;
    struct FunctionSymbolList *next;
};

static struct FunctionSymbolList *symbol_list = 0;

void add_to_symbols(void *function, char *name){
    kprint(name);
    struct FunctionSymbolList *new_symbol_list = malloc(sizeof(struct FunctionSymbolList));
    new_symbol_list->function = function;
    memcpy(new_symbol_list->name, name, strlen(name) + 1);

    if(!symbol_list){
        symbol_list = new_symbol_list;
        return;
    }
    struct FunctionSymbolList *traversal_list = symbol_list;
    while(traversal_list->next){
        if(!strncmp(symbol_list->name, name, strlen(name) + 1)) {kprint("noooo"); return;}
        traversal_list = traversal_list->next;
    }
    traversal_list->next = new_symbol_list;
}

void *search_for_modules(char *name){
    struct FunctionSymbolList *traversal_list = symbol_list;
    while(traversal_list){
        if(!strncmp(traversal_list->name, name, strlen(name) + 1)) return traversal_list->function;
        traversal_list = traversal_list->next;
    }
    return 0;
}

void KernelInitializeSymbols(){
    add_to_symbols(kprint, "kprint");
    add_to_symbols(KernelGetHhdmOffset, "KernelGetHhdmOffset");
    add_to_symbols(KernelGetFramebuffer, "KernelGetFramebuffer");
    add_to_symbols(IoCreateCharDevice, "IoCreateCharDevice");
    add_to_symbols(IoCreateBlockDevice, "IoCreateBlockDevice");
    add_to_symbols(HalSearchForPciDevice, "HalSearchForPciDevice");
    add_to_symbols(KernelWritePciConfigDword, "KernelWritePciConfigDword");
    add_to_symbols(KernelReadPciConfigWord, "KernelReadPciConfigWord");
}

uintptr_t KernelLoadElfLibrary(uint8_t *ptr){
    uint64_t current_cr3 = 0;
    asm("mov %%cr3, %0": "=r"(current_cr3) : :);
    current_cr3 += KernelGetHhdmOffset();
    struct ELFHeader *header = (void *)ptr;
    if(header->magic_number[0] != 0x7F) return 0;

    uintptr_t base_address = KernelAllocateFrame() + 0xffffc00000000000;

    struct ELFSectionHeader *sheader = (void *) ((uint64_t) header + header->section_header_offset);
    struct ELFProgramHeader *pheader = (void *) (ptr + header->program_header_offset);
    for(int i = 0; i < header->program_table_entry_number; ++i){
        for(uint64_t x = 0; x <= ((pheader->memory_size / 0x1000) + 1) * 0x1000; x += 0x1000){
            KernelMapPage((void *) current_cr3, base_address + x, KernelAllocateFrame(), 1);
            memcpy((void *)(pheader->virtual_address + base_address), ptr + pheader->file_offset, pheader->file_size);
        }
        pheader = (void *)((char *)pheader + header->program_table_entry_size);
    }

    for(int i = 0; i < header->section_table_entry_number; ++i){
        if(sheader[i].type == 2){
            struct ELFSym *symbol_table = (void *)((uint64_t) header + sheader[i].offset);
            char *string_table = (void *)((uint64_t) header + sheader[header->string_table_index - 1].offset);
            for(int j = 0; j < sheader[i].size / sheader[i].entry_size; ++j){
                if(((symbol_table[j].info & 0xF) == 2) && symbol_table[j].value){
                    memcpy((void *)(base_address + symbol_table[j].value), ptr + symbol_table[j].value, symbol_table[j].size);
                    add_to_symbols((void *) (symbol_table[j].value + base_address), string_table + symbol_table[j].name);
                }
            }
        }
    }
    return base_address;
}

void *KernelLoadElfExecutable(struct FileObject *fo){

    if(!fo) return 0;

    char *temp = malloc(fo->size);
    for(int i = 0; i < fo->size; ++i){
        temp[i] = ReadByte(fo, i);
    }

    struct ELFHeader *header = (void *)temp;
    if(header->magic_number[0] != 0x7F) return 0;

    struct ELFProgramHeader *pheader = (void *) (temp + header->program_header_offset);
    for(int i = 0; i < header->program_table_entry_number; ++i){
        for(uint64_t x = 0; x <= pheader->file_size; x += 0x1000){
            KernelMapPage((void *) (GetCurrentProcess()->cr3 + KernelGetHhdmOffset()), pheader->virtual_address + x, KernelAllocateFrame(), 1);
        }
        memcpy((void *)pheader->virtual_address, temp + pheader->file_offset, pheader->file_size);
        pheader = (void *)((char *)pheader + header->program_table_entry_size);
    }

    return (void *) header->program_entry_offset;
}

void KernelResolveElfRelocations(uint8_t *ptr, uintptr_t address){
    struct ELFHeader *header = (void *)ptr;
    if(header->magic_number[0] != 0x7F) return;

    struct ELFSectionHeader *sheader = (void *) ((uint64_t) header + header->section_header_offset);

    for(int i = 0; i < header->section_table_entry_number; ++i){
        if(sheader[i].type == 4){
            struct ELFRela *relocation_table =  (void *)((uint64_t) header + sheader[i].offset);
            struct ELFSym *symbol_table = (void *)((uint64_t) header + sheader[sheader[i].link].offset);
            struct ELFSectionHeader *target_tab = (void *) ((uint64_t) header + sheader[sheader[i].info].offset);
            char *string_table = (void *)((uint64_t) header + sheader[sheader[i].link + 1].offset);
            for(int j = 0; j < sheader[i].size / sheader[i].entry_size; ++j){
                char *name = string_table + symbol_table[relocation_table[j].info >> 32].name;
                switch (relocation_table[j].info & 0xFF){
                    case 6:
                        uintptr_t data_target = relocation_table[j].offset + address;
                        void *rel = search_for_modules(name);
                        if(rel) memcpy((void *) data_target, rel, symbol_table[relocation_table[j].info >> 32].size);
                        (*(uint64_t *)data_target) = (uint64_t) search_for_modules(name);
                        break;
                    case 7:
                        uintptr_t func_target = relocation_table[j].offset + target_tab->address + address;
                        (*(uint64_t *)func_target) = (uint64_t) search_for_modules(name);
                        break;
                    case 8:
                        uintptr_t rel_target = relocation_table[j].offset + address;
                        (*(uint64_t *)rel_target) =  address + relocation_table[j].addend;
                        break;
                }
            }
        }
    }
}
