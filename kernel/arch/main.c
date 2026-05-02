#include <limine.h>
#include <string.h>
#include <kernel/acpi.h>
#include <kernel/dprintf.h>
#include <kernel/elf.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/mmu.h>
#include <kernel/pci.h>
#include <kernel/sbd.h>
#include <kernel/task.h>
#include <kernel/fs/ext2.h>
#include <kernel/fs/ramfs.h>
#include <kernel/object/directory.h>
#include <kernel/object/filesystem.h>
#include <kernel/object/iomgr.h>
#include <kernel/object/object.h>
#include <kernel/object/processor.h>

extern void switch_to_user(void *req);
extern int HalInitializePs2();
extern void *search_for_modules(char *name);
void task1(){
    struct FileObject *fo = (void *) ResolveObjectName(0, "./Shell");
    void (*ptr) (void) = KernelLoadElfExecutable(fo);
    kprint("%x", ptr);
    if(ptr) switch_to_user(ptr);
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
    KernelInitializeSymbols();
    kprint("Platform specific structures has been initialized.\n");
    kprint("Phase 0 done!\n");
    HalInitializeInterrupts();
    kprint("Interrupts has been enabled.\n");
    HalInitializeProcessors();
    char *vga_driver = KernelGetModule(2);
    char *ahci_driver = KernelGetModule(3);
    uintptr_t addr = KernelLoadElfLibrary(vga_driver);
    KernelResolveElfRelocations(vga_driver, addr);
    uintptr_t addr2 = KernelLoadElfLibrary(ahci_driver);
    KernelResolveElfRelocations(ahci_driver, addr2);
    KernelInitializeProcess(0, 1);
    HalEnableTimer();
    kprint("Timer has been enabled.\n");
    void (*VgaFramebufferInitialize) (void) = search_for_modules("VgaFramebufferInitialize");
    if(VgaFramebufferInitialize) VgaFramebufferInitialize();
    else {
        kprint("No boot video, halting.");
        while(1) asm("hlt");
    }
    struct CharDevice *cd = (void *)ResolveObjectName(0, "./Devices/BootVideo");
    ChangeStandardOutput(cd);
    dprintf("VGA initialized.\n");
    HalInitializePci();
    HalCheckPciBus(0);
    dprintf("PCI initialized.\n");
    void (*InitializeAhci) (void) = search_for_modules("InitializeAhci");
    if(InitializeAhci) InitializeAhci();
    else dprintf("No AHCI module found");
    HalInitializePs2();
    struct BlockDevice *sata = (void *)ResolveObjectName(0, "./Devices/SataDevice0");
    InitializeExt2FilesystemFromBlockDevice(sata);

    while(1) asm("hlt");
}
