#include <stdint.h>
#include <kernel/object/filesystem.h>

extern uint64_t invokesystemcall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

int exec(const char *file){
    struct FileObject *fo = (void *) invokesystemcall(2, 0, (uint64_t) file, 0, 0);
    if(!fo) return -1;
    int (*loc) (void) = 0;
    if(loc = (int(*)(void))invokesystemcall(8, (uint64_t) fo, 0, 0, 0)) return loc();
    else return -1;
}

int fork(){
    return invokesystemcall(1, 0, 0, 0, 0);
}
