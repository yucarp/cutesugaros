#include <stdint.h>

struct Task {
    uint64_t rip, rsp, cr3, id;
    void *token, *cwd;
};

void KernelInitializeProcess(void *rip, uint64_t permissions);
void KernelSwitchProcess();
int KernelCloneProcess(uint64_t rip, uint64_t rsp);
struct Task *GetCurrentProcess();
uint64_t GetCurrentProcessId();
