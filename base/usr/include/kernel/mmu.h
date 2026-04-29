#include <stdint.h>

void KernelInitializePaging();
void *KernelExpandHeap();
uintptr_t KernelAllocateFrame();
void KernelMapPage(uintptr_t *root, uint64_t virtual_address, uint64_t physical_address, char user);
void KernelMapMmio(uint64_t virtual_address, uint64_t physical_address);
int KernelUnmapPage(uintptr_t *root, uint64_t virtual_address);
uintptr_t *KernelNewPageStructure(uintptr_t *old);
