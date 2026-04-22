AS := x86_64-elf-as
CC := x86_64-elf-gcc
CFLAGS := -ffreestanding -Wall -Wextra -Iinclude -g -mcmodel=kernel -m64 -fdata-sections -ffunction-sections -mno-sse -mno-red-zone -fno-stack-protector -fno-lto

LIBCOBJS=\
libc/string/memcpy.o\
libc/string/strncmp.o

OBJS=\
kernel/arch/acpi.o\
kernel/arch/ap.o\
kernel/arch/gdt.o\
kernel/arch/gdt_asm.o\
kernel/arch/idt.o\
kernel/arch/irq.o\
kernel/arch/main.o\
kernel/arch/mmu.o\
kernel/arch/ps2.o\
kernel/arch/task.o\
kernel/fs/ext2.o\
kernel/fs/ramfs.o\
kernel/io/iomgr.o\
kernel/io/ramdisk.o\
kernel/misc/elf.o\
kernel/misc/malloc.o\
kernel/misc/limine.o\
kernel/misc/sbd.o\
kernel/misc/spinlock.o\
kernel/misc/tokenmgr.o\
kernel/object/directory.o\
kernel/object/filesystem.o\
kernel/object/processor.o\
kernel/object/object.o

.SUFFIXES: .o .c .s

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS)

.s.o:
	$(AS) -MD -c $< -o $@

all: kernel.bin

kernel.bin: $(OBJS) $(LIBCOBJS)
	$(CC) -T linker.ld -o $@ -ffreestanding -nostdlib -lgcc -static -mcmodel=kernel $(OBJS) $(LIBCOBJS)
