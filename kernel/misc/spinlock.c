#include <stdatomic.h>
#include <stdint.h>
#include <kernel/spinlock.h>
#include <kernel/sbd.h>

void lock(struct Spinlock *lock){
    asm volatile("cli");
    while(1){
        while(atomic_load(&lock->lock)){
            asm volatile("pause");
        }
        if(!__atomic_exchange_n(&lock->lock, 1, __ATOMIC_ACQUIRE)) break;
    }
}

void unlock(struct Spinlock *lock){
    atomic_store(&lock->lock, 0);
    asm volatile("sti");
}
