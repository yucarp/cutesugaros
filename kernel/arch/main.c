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
#include <kernel/object/object.h>
#include <kernel/object/processor.h>

void task1(){
    while(1){
        asm("hlt");
    }
}

void task2(){
    while(1){
        asm("hlt");
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
    //HalInitializeProcessors();
    KernelInitializeProcess(0);
    KernelInitializeProcess(task1);
    KernelInitializeProcess(task2);
    HalEnableTimer();
    HalUnmaskInterrupt(0);
    HalMapInterrupt(0, 32);
    kprint("Timer has been enabled.\n");
    while(1) asm("hlt");
}
