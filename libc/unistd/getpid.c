#include <stdint.h>

extern uint64_t invokesystemcall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

int getpid(){
    return invokesystemcall(0, 0, 0, 0, 0);
}
