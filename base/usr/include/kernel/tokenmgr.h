#include <stdint.h>

void *KernelGetTokenForProcess(uint64_t id, uint8_t permissions);
int KernelCheckToken(uint8_t permission);
