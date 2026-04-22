#include <limine.h>
#include <string.h>
#include <kernel/acpi.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/mmu.h>
#include <kernel/sbd.h>
#include <kernel/task.h>
#include <kernel/fs/ext2.h>
#include <kernel/object/directory.h>
#include <kernel/object/filesystem.h>
#include <kernel/object/iomgr.h>
#include <kernel/object/object.h>
#include <kernel/object/processor.h>

extern void switch_to_user(void *req);
extern int HalInitializePs2();

void task1(){
    char *x = KernelGetModule(0);
    extern void *KernelLoadElfExecutable(uint8_t *ptr);
    void *pe = KernelLoadElfExecutable(x);
    switch_to_user(pe);
    while(1){
        asm("hlt");
    }
}

void task2(){
    kprint("Hello from ring 0");
    while(1){
    }
}

void KernelStart(){
    if(1){
        KernelStartSerialDebugging();
        kprint("Serial debugging has started...\n");
    }
    KernelLoadGdt();
    KernelInitializePaging();
    kprint("Memory structures has been initialized.\n");
    InitializeObjectManager();
    KernelParseAcpi();
    kprint("Platform specific structures has been initialized.\n");
    kprint("Phase 0 done!\n");
    HalInitializeInterrupts();
    kprint("Interrupts has been enabled.\n");
    HalInitializeProcessors();
    //HalEnableTimer();
    HalInitializePs2();
    struct BlockDevice *ramdisk = IoCreateRamdisk(KernelGetModule(1), 512);
    InitializeExt2FilesystemFromMemory(ramdisk);
    kprint("Timer has been enabled.\n");
    while(1) asm("hlt");
}
