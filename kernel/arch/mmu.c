#include <stdint.h>
#include <string.h>
#include <limine.h>
#include <kernel/spinlock.h>

#define PAGE_SIZE 0x1000
#define PAGE_ENTRY 0x80000000000001FF


uintptr_t kernel_pml4[512]__attribute__((aligned(PAGE_SIZE))) = {0};

uintptr_t kernel_hpd[512]__attribute__((aligned(PAGE_SIZE))) = {0};
uintptr_t kernel_lpd[512]__attribute__((aligned(PAGE_SIZE))) = {0};
uintptr_t kernel_pt[512][1024]__attribute__((aligned(PAGE_SIZE))) = {0};

uintptr_t hhdm_hpd[512]__attribute__((aligned(PAGE_SIZE))) = {0};
uintptr_t hhdm_lpd[4][512]__attribute__((aligned(PAGE_SIZE))) = {0};
uintptr_t hhdm_pt[4][512][1024]__attribute__((aligned(PAGE_SIZE))) = {0};

uintptr_t mem_list[0x40000] = {0};
uint64_t stack_position = 0;

uintptr_t KernelGetPhysicalAddress(uintptr_t *pml4, uintptr_t virtual_address){
    uintptr_t pml4_index = (virtual_address >> 39) & 0x1FF;
    uintptr_t hpd_index = (virtual_address >> 30) & 0x1FF;
    uintptr_t lpd_index = (virtual_address >> 21) & 0x1FF;
    uintptr_t pt_index = (virtual_address >> 12) & 0x1FF;

    uintptr_t hpd = pml4[pml4_index];
    if(!(hpd & 1)) return -4;

    uintptr_t lpd = ((uintptr_t *)((hpd & ~PAGE_ENTRY) + KernelGetHhdmOffset()))[hpd_index];
    if(!(lpd & 1)) return -3;

    uintptr_t pt = ((uintptr_t *)((lpd & ~PAGE_ENTRY) + KernelGetHhdmOffset()))[lpd_index];
    if(!(pt & 1)) return -2;

    uintptr_t page = ((uintptr_t *)((pt & ~PAGE_ENTRY) + KernelGetHhdmOffset()))[pt_index];
    if(!(page & 1)) return -1;

    return page & ~0x8000000000000FFF;
}

void KernelInitializeMemStack(){
    struct limine_memmap_response *memmap_response = KernelGetMemmapFromBootloader();
    uint64_t pos = 0;
    if(memmap_response == 0) /*PANIC: There is no memory map provided */ asm("hlt");
    for(int i = 0; i < memmap_response->entry_count; ++i){
       if((memmap_response->entries[i]->type) == 0){
            for(uint64_t base = memmap_response->entries[i]->base; base < memmap_response->entries[i]->base + memmap_response->entries[i]->length; base += 0x1000){
                mem_list[pos++] = base;
            }
       }
    }
}

uintptr_t KernelAllocateFrame(){
    return mem_list[stack_position++];
}

int KernelDeallocateFrame(uintptr_t memory_address){
    if (memory_address & 0xFFF) return -1;
    if(stack_position == 0) return -1;
    mem_list[--stack_position] = memory_address;
    return 0;
}

void *KernelExpandHeap(){
    return (void *)(KernelAllocateFrame() + KernelGetHhdmOffset());
}

void KernelMapPage(uintptr_t *root, uint64_t virtual_address, uint64_t physical_address){
    uintptr_t pml4_index = (virtual_address >> 39) & 0x1FF;
    uintptr_t hpd_index = (virtual_address >> 30) & 0x1FF;
    uintptr_t lpd_index = (virtual_address >> 21) & 0x1FF;
    uintptr_t pt_index = (virtual_address >> 12) & 0x1FF;

    if(root == 0) return;

    uint64_t hpd_address = root[pml4_index];

    if(!(hpd_address & 1)){
        hpd_address = KernelAllocateFrame();
        root[pml4_index] = hpd_address | 3;
    }

    hpd_address |= KernelGetHhdmOffset();

    uint64_t *hpd = (void *)((hpd_address & ~PAGE_ENTRY) | KernelGetHhdmOffset());

    uint64_t lpd_address = hpd[hpd_index];

    if(!(lpd_address & 1)){
        lpd_address = KernelAllocateFrame();
        hpd[hpd_index] = lpd_address | 3;
    }

    uint64_t *lpd = (void *) ((lpd_address & ~PAGE_ENTRY) | KernelGetHhdmOffset());

    uint64_t pt_address = lpd[lpd_index];

    if(!(pt_address & 1)){
        pt_address = KernelAllocateFrame();
        lpd[lpd_index] = pt_address | 3;
    }

    uint64_t *pt = (void *)((pt_address & ~PAGE_ENTRY) | KernelGetHhdmOffset());

    if(pt[pt_index] & 1) return;
    pt[pt_index] = physical_address | 3;
}

int KernelUnmapPage(uintptr_t *root, uint64_t virtual_address){
    uintptr_t pml4_index = (virtual_address >> 39) & 0x1FF;
    uintptr_t hpd_index = (virtual_address >> 30) & 0x1FF;
    uintptr_t lpd_index = (virtual_address >> 21) & 0x1FF;
    uintptr_t pt_index = (virtual_address >> 12) & 0x1FF;

    if(root == 0) return -5;

    uint64_t hpd_address = root[pml4_index];

    if(!(hpd_address & 1))  return -4;

    hpd_address |= KernelGetHhdmOffset();

    uint64_t *hpd = (void *)((hpd_address & ~PAGE_ENTRY) | KernelGetHhdmOffset());

    uint64_t lpd_address = hpd[hpd_index];

    if(!(lpd_address & 1)) return -3;

    uint64_t *lpd = (void *) ((lpd_address & ~PAGE_ENTRY) | KernelGetHhdmOffset());

    uint64_t pt_address = lpd[lpd_index];

    if(!(pt_address & 1)) return -2;

    uint64_t *pt = (void *)((pt_address & ~PAGE_ENTRY) | KernelGetHhdmOffset());

    if(!(pt[pt_index] & 1)) return -1;
    pt[pt_index] = 0;

    return 0;
}

uintptr_t *KernelNewPageStructure(){
    uintptr_t *new_root = (void *)(KernelAllocateFrame() + KernelGetHhdmOffset());
    memcpy(&new_root[256], &kernel_pml4[256], 2048);
    return new_root;
}

void KernelInitializePaging(){
    uint64_t hhdm = KernelGetHhdmOffset();
    uintptr_t *old_root = 0;
    asm("mov %%cr3, %0" : "=r"(old_root) : :);
    old_root += hhdm / 8;
    uint64_t kernel_physical_address = KernelGetPhysicalAddress((uintptr_t *)old_root, 0xffffffff80000000);

    kernel_pml4[511] = (uintptr_t) KernelGetPhysicalAddress(old_root, (uintptr_t) kernel_hpd) | 3;
    kernel_hpd[510] = KernelGetPhysicalAddress(old_root, (uintptr_t) kernel_lpd) | 3;

    for(uint64_t kpd = 0; kpd < 512; ++kpd){
        kernel_lpd[kpd] = KernelGetPhysicalAddress(old_root, (uintptr_t) kernel_pt[kpd]) | 3;
        for(uint64_t kpt = 0; kpt < 1024; ++kpt){
            kernel_pt[kpd][kpt] = (kernel_physical_address + (kpd << 21) + (kpt << 12)) | 3;
        }
    }

    kernel_pml4[256] = (uintptr_t) KernelGetPhysicalAddress(old_root, (uintptr_t) hhdm_hpd) | 3;

    for(uint64_t hpdp = 0; hpdp < 4; ++hpdp){
        hhdm_hpd[hpdp] = KernelGetPhysicalAddress(old_root, (uintptr_t) hhdm_lpd[hpdp]) | 3;
        for(uint64_t hpd = 0; hpd < 512; ++hpd){
            hhdm_lpd[hpdp][hpd] = KernelGetPhysicalAddress(old_root, (uintptr_t) hhdm_pt[hpdp][hpd]) | 3;
            for(uint64_t hpt = 0; hpt < 1024; ++hpt){
                hhdm_pt[hpdp][hpd][hpt] = ((hpdp << 30) + (hpd << 21) + (hpt << 12)) | 3;
            }
        }
    }

    uintptr_t kernel_pml4_address = KernelGetPhysicalAddress(old_root, (uintptr_t) kernel_pml4);

    asm("mov %0, %%cr3": : "r"(kernel_pml4_address) :);
    KernelInitializeMemStack();
}
