#include <stdint.h>

struct Task {
    uint64_t rip, rsp, cr3;
};

void KernelInitializeProcess(void *rip);
void KernelSwitchProcess();
