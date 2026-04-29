AS := x86_64-cutesugar-as
CC := x86_64-cutesugar-gcc
CFLAGS := -ffreestanding -Wall -Wextra -Ibase/usr/include -g -mcmodel=kernel -m64 -fdata-sections -ffunction-sections -mno-sse -mno-red-zone -fno-stack-protector -fno-lto -fno-pic
LIBCFLAGS := -ffreestanding -Wall -Wextra -Ibase/usr/include -g -m64 -fdata-sections -ffunction-sections -mno-sse -mno-red-zone -fno-stack-protector -fno-lto -fPIC

LIBCOBJS=\
libc/main.ro\
libc/arch/crt0.o\
libc/arch/crti.o\
libc/arch/crtn.o\
libc/arch/syscall.o\
libc/stdio/stdio.ro\
libc/stdlib/malloc.ro\
libc/string/memcpy.ro\
libc/string/memset.ro\
libc/string/strcat.ro\
libc/string/strlen.ro\
libc/string/strncmp.ro\
libc/unistd/chdir.ro\
libc/unistd/exec.ro\
libc/unistd/getpid.ro

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
kernel/misc/dprintf.o\
kernel/misc/elf.o\
kernel/misc/malloc.o\
kernel/misc/limine.o\
kernel/misc/pci.o\
kernel/misc/sbd.o\
kernel/misc/spinlock.o\
kernel/misc/tokenmgr.o\
kernel/object/directory.o\
kernel/object/filesystem.o\
kernel/object/processor.o\
kernel/object/object.o

.SUFFIXES: .o .c .s .ro

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS)

.c.ro:
	$(CC) -MD -c $< -o $@ -std=gnu11 -fPIC -ffreestanding -Wall -Wextra -Iinclude -g

.s.o:
	$(AS) -MD -c $< -o $@

all: libc.a kernel.bin bootvid.so kerlib.so ahci.so

libc.a: $(LIBCOBJS)
	x86_64-cutesugar-ar rcs libc.a $(LIBCOBJS)

kernel.bin: $(OBJS)
	$(CC) -T linker.ld -o $@ -ffreestanding -nostdlib -static -mcmodel=kernel $(OBJS) libc.a

kerlib.so: kernel_library.ro
	$(CC) -o $@ -Wl,-shared -fPIC -ffreestanding -nostdlib kernel_library.ro

bootvid.so: modules/bootvid/bootvid.ro
	$(CC) -o $@ -Wl,-shared -fPIC -ffreestanding -nostdlib modules/bootvid/bootvid.ro kerlib.so

ahci.so: modules/ahci/ahci.ro
	$(CC) -o $@ -Wl,-shared -fPIC -ffreestanding -nostdlib modules/ahci/ahci.ro kerlib.so libc.a
