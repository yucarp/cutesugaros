#include <limine.h>
#include <stdint.h>
#include <kernel/kmalloc.h>
#include <kernel/mmu.h>
#include <kernel/spinlock.h>
#include <kernel/task.h>
#include <kernel/tokenmgr.h>

extern void switch_task(struct Task *next, struct Task *previous);
extern void switch_cr3(struct Task *next, struct Task *previous);

struct TaskList {
    struct Task *task;
    struct TaskList *next;
};

struct TaskList *first_task = 0;
struct TaskList *prev_task = 0;
struct TaskList *current_task = 0;

static int process_id = 0;

struct Spinlock spinlock = {0};
void KernelInitializeProcess(void *rip){
    ++process_id;
    if(!first_task){
        first_task = malloc(sizeof(struct TaskList));
        first_task->task = malloc(sizeof(struct Task));
        first_task->task->rsp = (uint64_t) malloc(0x2000) + 0x2000;
        first_task->task->rip = (uint64_t) rip;
        first_task->task->id = process_id;
        first_task->task->token = KernelGetTokenForProcess(process_id, 0);
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
    traversal_list->next->task->id = process_id;
    traversal_list->next->task->token = KernelGetTokenForProcess(process_id, 0);
    traversal_list->next->task->rip = (uint64_t) rip;
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

struct Task *find_process(uint64_t id){
    struct TaskList *traversal_list = first_task;
    while(traversal_list){
        if(traversal_list->task->id == id) return traversal_list->task;
        traversal_list = traversal_list->next;
    }
    return 0;
}

void KernelCloneProcess(uint64_t id){
    struct Task *task = find_process(id);
    if(!task) return;
    KernelInitializeProcess((void *)task->rip);
}

void KernelSwitchProcess(){
    if(!current_task){return; }
    if(!current_task->next) {return;}
    lock(&spinlock);
    prev_task = current_task;
    current_task = current_task->next;
    asm("mov %0, %%cr3": : "r"(current_task->task->cr3) :);
    switch_task(current_task->task, prev_task->task);
}

struct Task *GetCurrentProcess(){
    if(!current_task) return 0;
    return current_task->task;
}

uint64_t GetCurrentProcessId(){
    if(!current_task) return 0;
    return current_task->task->id;
}
