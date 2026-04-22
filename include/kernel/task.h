#include <stdint.h>

struct Task {
    uint64_t rip, rsp, cr3, id;
    void *token;
};

void KernelInitializeProcess(void *rip);
void KernelSwitchProcess();
void KernelCloneProcess();
struct Task *GetCurrentProcess();
uint64_t GetCurrentProcessId();
