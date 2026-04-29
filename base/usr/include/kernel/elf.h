#include <stdint.h>
#include <kernel/object/filesystem.h>

void KernelInitializeSymbols();
uintptr_t KernelLoadElfLibrary(uint8_t *ptr);
void *KernelLoadElfExecutable(struct FileObject *fo);
void KernelElfExec(struct FileObject *fo);
void KernelResolveElfRelocations(uint8_t *ptr, uintptr_t);
