#include <stdint.h>

extern uint64_t invokesystemcall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

int chdir(char *c){
    return invokesystemcall(7, 0, (uint64_t) c, 0, 0);
}
