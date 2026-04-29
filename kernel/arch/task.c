#include <limine.h>
#include <stdint.h>
#include <string.h>
#include <kernel/idt.h>
#include <kernel/kmalloc.h>
#include <kernel/mmu.h>
#include <kernel/sbd.h>
#include <kernel/spinlock.h>
#include <kernel/task.h>
#include <kernel/tokenmgr.h>
#include <kernel/object/object.h>

extern void switch_task(struct Task *next, struct Task *previous);
extern void resume_user();

struct TaskList {
    struct Task *task;
    struct TaskList *next;
};

struct TaskList *first_task = 0;
struct TaskList *prev_task = 0;
struct TaskList *current_task = 0;

static int process_id = 0;

struct Spinlock spinlock = {0};
void KernelInitializeProcess(void *rip, uint64_t permissions){
    ++process_id;
    if(!first_task){
        first_task = malloc(sizeof(struct TaskList));
        first_task->task = malloc(sizeof(struct Task));
        first_task->task->rsp = (uint64_t) malloc(0x2000) + 0x2000;
        first_task->task->rip = (uint64_t) rip;
        first_task->task->id = process_id;
        first_task->task->token = KernelGetTokenForProcess(process_id, permissions);
        first_task->task->cwd = ObjGetRootObject();
        uint64_t *rsp = (void *) first_task->task->rsp;
        *--rsp = (uint64_t) rip;
        *--rsp = 0;
        *--rsp = 0;
        *--rsp = 0;
        *--rsp = 0;
        *--rsp = 0;
        *--rsp = 0;
        first_task->task->rsp = (uint64_t) rsp;
        asm("mov %%cr3, %0": "=r"(first_task->task->cr3) : :);
        current_task = first_task;
        return;
    }

    struct TaskList *traversal_list = first_task;

    while(traversal_list->next){
        if(traversal_list->next == first_task) break;
        traversal_list = traversal_list->next;
    }

    struct TaskList *new_task = malloc(sizeof(struct TaskList));
    new_task->task = malloc(sizeof(struct Task));
    new_task->task->rsp = (uint64_t) malloc(0x2000) + 0x2000;
    new_task->task->id = process_id;
    new_task->task->token = KernelGetTokenForProcess(process_id, permissions);
    new_task->task->rip = (uint64_t) rip;
    new_task->task->cr3 = (uint64_t) KernelNewPageStructure(0) - KernelGetHhdmOffset();
    new_task->task->cwd = ObjGetRootObject();
    new_task->next = first_task;
    uint64_t *rsp = (void *) new_task->task->rsp;
    *--rsp = (uint64_t) rip;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    new_task->task->rsp = (uint64_t) rsp;
    traversal_list->next = new_task;
}

struct Task *find_process(uint64_t id){
    struct TaskList *traversal_list = first_task;
    while(traversal_list){
        if(traversal_list->task->id == id) return traversal_list->task;
        traversal_list = traversal_list->next;
    }
    return 0;
}
int KernelCloneProcess(uint64_t rip, uint64_t ursp){
    process_id++;
    extern void resume_user();

    struct TaskList *traversal_list = first_task;

    while(traversal_list->next){
        if(traversal_list->next == first_task) break;
        traversal_list = traversal_list->next;
    }

    struct TaskList *new_task = malloc(sizeof(struct TaskList));
    uint64_t cr3 = 0;
    asm("mov %%cr3, %0": "=r"(cr3) : :);
    cr3 += KernelGetHhdmOffset();
    new_task->task = malloc(sizeof(struct Task));
    new_task->task->rsp = (uint64_t) malloc(0x128) + 0x128;
    new_task->task->id = process_id;
    new_task->task->token = KernelGetTokenForProcess(process_id, 1);
    new_task->task->rip = (uint64_t) rip;
    new_task->task->cr3 = (uint64_t) KernelNewPageStructure((uintptr_t *) cr3) - KernelGetHhdmOffset();
    new_task->task->cwd = ObjGetRootObject();
    new_task->next = first_task;

    uint64_t *rsp = (void *) new_task->task->rsp;
    uint64_t *new_stack = malloc(0x2000) + 0x2000;
    uint64_t *urrsp = ((uint64_t *) ursp);
    uintptr_t *location = rsp - 2;

    memcpy(&new_stack[0], &urrsp[0], 2048);

    *--rsp = 35;
    *--rsp = 0;
    *--rsp = 0x202;
    *--rsp = 27;
    *--rsp = rip;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = (uint64_t) &resume_user;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *--rsp = 0;
    *location = (uint64_t) new_stack;
    new_task->task->rsp = (uint64_t) rsp;
    traversal_list->next = new_task;

    return new_task->task->id;
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
