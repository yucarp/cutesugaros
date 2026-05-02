#include <limine.h>
#include <kernel/sbd.h>
#include <kernel/object/iomgr.h>
#include <characters.h>

static struct limine_framebuffer *main_framebuffer = 0;
static uint64_t terminal_x = 0;
static uint64_t terminal_y = 0;

static void VgaFramebufferWriteChar(struct CharDevice *device, long offset, char c);

static struct CharDeviceFunctions vga_functions = {
    VgaFramebufferWriteChar
};

void VgaFramebufferInitialize(){
    main_framebuffer = KernelGetFramebuffer();
    struct CharDevice *dev = IoCreateCharDevice(&vga_functions, "BootVideo");
    dev->specific_info = (uint64_t) (main_framebuffer);
}

//Offset is ignored
static void VgaFramebufferWriteChar(struct CharDevice *device, long offset, char c){
    if(!device) return;
    if(!device->specific_info) return;
    main_framebuffer = (struct limine_framebuffer *)(device->specific_info);

    if(c == '\n'){
        terminal_x = 0;
        terminal_y += 8;
        return;
    }

    for(int height = 0; height < 8; ++height){
        for(int width = 0; width < 8; ++width){
            if (FramebufferCharacters[c][height] & (1 << (width))) ((uint32_t *)main_framebuffer->address)[terminal_x + width + main_framebuffer->width * (terminal_y + height)] = 0xFFFFFFFF;
        }
    }

    terminal_x += 8;
}
