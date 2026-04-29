#include <stdarg.h>
#include <kernel/object/iomgr.h>

void ChangeStandardOutput(struct CharDevice *cd);
void dprintf(char *str, ...);
