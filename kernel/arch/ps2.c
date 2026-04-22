#include <kernel/acpi.h>
#include <kernel/idt.h>
#include <kernel/port.h>
#include <kernel/sbd.h>
#include <kernel/fs/ramfs.h>
#include <kernel/object/directory.h>
#include <kernel/object/object.h>
#include <stdint.h>

#define PS2_DATA 0x60
#define PS2_COMMAND 0x64

char ps2_keymap[] = {' ', 'E', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0x1C, '?', 'A', 'S', 'D', 'F', 'G',
    'H', 'J', 'K', 'L', ';', '\'', '`', '?', '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0x36,
    '*', 0x38, ' ',
};

static char ps2_buffer[64] = {0};
static struct FileObject *ps2_file = 0;

void ps2_handler(struct x86Registers *regs){
    uint8_t key = inb(PS2_DATA);
    if(key < 0x3A){
        WriteByte(ps2_file, 0, ps2_keymap[key]);
    }
}

void wait_ps2(){
    for(int i = 0; i < 1000; ++i){
        inb(PS2_COMMAND);
    }
}

int HalInitializePs2(){
    if(!HalDoesSupportPs2()) return -1;
    uint8_t status = 0;

    outb(PS2_COMMAND, 0xAD);
    wait_ps2();
    outb(PS2_COMMAND, 0xA7);
    wait_ps2();

    int timeout = 0;
    for(; timeout < 1024; ++timeout){

        inb(PS2_DATA);

        if(!(inb(PS2_COMMAND) & 1)){
            break;
        }
    }

    if(timeout == 1023) return -1;

    outb(PS2_COMMAND, 0x20);
    wait_ps2();
    status = inb(PS2_DATA);
    wait_ps2();

    status |= 0b0101001;

    outb(PS2_COMMAND, 0x60);
    wait_ps2();
    outb(PS2_DATA, status);
    wait_ps2();

    outb(PS2_COMMAND, 0xAE);

    ps2_file = RamfsCreateFile((void *) ResolveObjectName( ObjGetRootObject(),"./Devices"), "Ps2Keyboard", ps2_buffer, 64);

    HalSetIrqEntry(1, ps2_handler);
    HalMapInterrupt(1, 33);
    HalUnmaskInterrupt(1);
    asm("sti");
    return 0;
}
