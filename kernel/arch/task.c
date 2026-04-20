#include <limine.h>
#include <stdint.h>
#include <kernel/kmalloc.h>
#include <kernel/mmu.h>
#include <kernel/sbd.h>
#include <kernel/spinlock.h>
#include <kernel/task.h>

extern void switch_task(struct Task *next, struct Task *previous);
extern void switch_cr3(struct Task *next, struct Task *previous);

struct TaskList {
    struct Task *task;
    struct TaskList *next;
};

struct TaskList *first_task = 0;
struct TaskList *prev_task = 0;
struct TaskList *current_task = 0;

struct Spinlock spinlock = {0};
void KernelInitializeProcess(void *rip){
    if(!first_task){
        first_task = malloc(sizeof(struct TaskList));
        first_task->task = malloc(sizeof(struct Task));
        first_task->task->rsp = (uint64_t) malloc(0x2000) + 0x2000;
        uint64_t *rsp = (void *) first_task->task->rsp;
        *--rsp = (uint64_t) rip;
        *--rsp = 0;
        *--rsp = 0;
        *--rsp = 0;
        *--rsp = 0;
        *--rsp = 0;
        *--rsp = 0;
        first_task->task->rsp = (uint64_t) rsp;
        first_task->task->cr3 = (uint64_t) KernelNewPageStructure() - KernelGetHhdmOffset();
        current_task = first_task;
        return;
    }

    struct TaskList *traversal_list = first_task;

    while(traversal_list->next){
        if(traversal_list->next == first_task) break;
        traversal_list = traversal_list->next;
    }

    traversal_list->next = malloc(sizeof(struct TaskList));
    traversal_list->next->task = malloc(sizeof(struct Task));
    traversal_list->next->task->rsp = (uint64_t) malloc(0x2000) + 0x2000;
    traversal_list->next->task->cr3 = (uint64_t) KernelNewPageStructure() - KernelGetHhdmOffset();
    traversal_list->next->next = first_task;
    uint64_t *rsp = (void *) traversal_list->next->task->rsp;
    *--rsp = (uint64_t) rip;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    traversal_list->next->task->rsp = (uint64_t) rsp;
}

void KernelSwitchProcess(){
    lock(&spinlock);
    if(!current_task){return; }
    if(!current_task->next) {return;}
    prev_task = current_task;
    current_task = current_task->next;
    switch_task(current_task->task, prev_task->task);
}

struct Task *GetCurrentProcess(){
    return current_task->task;
}
