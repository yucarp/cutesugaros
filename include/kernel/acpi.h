#include <stdint.h>

void KernelParseAcpi();
void HalInitializeProcessors();
void HalUnmaskInterrupt(uint8_t interrupt_no);
void HalMapInterrupt(uint8_t interrupt_no, uint8_t vector);
void HalEndOfInterrupt();
void HalEnableTimer();
