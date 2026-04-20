#include <stdatomic.h>

struct Spinlock{
    char lock;
};

void lock(struct Spinlock *lock);
void unlock(struct Spinlock *lock);
