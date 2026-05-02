#include <limine.h>
#include <stdint.h>
#include <string.h>
#include <kernel/mmu.h>
#include <kernel/pci.h>
#include <kernel/sbd.h>
#include <kernel/object/iomgr.h>
struct HBA_Port {
    volatile uint32_t command_list_address;
    volatile uint32_t cla_upper;
    volatile uint32_t fis_address;
    volatile uint32_t fis_upper;
    volatile uint32_t interrupt_status;
    volatile uint32_t interrupt_enable;
    volatile uint32_t command_and_status;
    volatile uint32_t reserved_dword;
    volatile uint32_t task_file_data;
    volatile uint32_t signature;
    volatile uint32_t sata_status;
    volatile uint32_t sata_control;
    volatile uint32_t sata_error;
    volatile uint32_t sata_active;
    volatile uint32_t command_issue;
    volatile uint32_t sata_notificiation;
    volatile uint32_t switch_control;
    volatile uint32_t reserved[11];
    volatile uint32_t vendor_specific[4];
}__attribute__((packed));

struct HBA_MemoryRegister {
    volatile uint32_t host_capability;
    volatile uint32_t global_host_control;
    volatile uint32_t interrupt_status;
    volatile uint32_t port_implemented;
    volatile uint32_t version;
    volatile uint32_t ccc_control;
    volatile uint32_t ccc_ports;
    volatile uint32_t em_location;
    volatile uint32_t em_control;
    volatile uint32_t capabilities_extended;
    volatile uint32_t bios_os_handoff_control;
    volatile uint8_t reserved[0xA0 - 0x2C];
    volatile uint8_t vendor_specific[0x100 - 0xA0];
    volatile struct HBA_Port hba_port[32];
}__attribute__((packed));

struct HBA_Command_Header{
    volatile uint8_t header_1;
    volatile uint8_t header_2;
    volatile uint16_t prd_table_length;
    volatile uint32_t prd_byte_count;
    volatile uint32_t ctd_base_address;
    volatile uint32_t ctd_base_upper;
    volatile uint32_t reserved[4];
}__attribute__((packed));

struct HBA_Physical_Region_Descriptor_Entry {
    volatile uint32_t data_base_address;
    volatile uint32_t data_base_upper;
    volatile uint32_t reserved;
    volatile uint32_t byte_count_and_interrupt_on_completion;
}__attribute__((packed));

struct HBA_Command_Table {
    volatile uint8_t command_fis[64];
    volatile uint8_t atapi_command[16];
    volatile uint8_t reserved[48];
    struct HBA_Physical_Region_Descriptor_Entry hda_prdt[1];
}__attribute__((packed));

struct FIS_Register_H2D{
    volatile uint8_t fis_type; //Always 0x27
    volatile uint8_t multiplier_and_cc;
    volatile uint8_t command;
    volatile uint8_t feature;
    volatile uint8_t lba0;
    volatile uint8_t lba1;
    volatile uint8_t lba2;
    volatile uint8_t device;
    volatile uint8_t lba3;
    volatile uint8_t lba4;
    volatile uint8_t lba5;
    volatile uint8_t feature_high;
    volatile uint16_t count;
    volatile uint8_t icc;
    volatile uint8_t control;
    volatile uint32_t reserved[4];
}__attribute__((packed));

struct FIS_Register_D2H {
    uint8_t fis_type;
    uint8_t header_1;
    uint8_t status;
    uint8_t error;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t reserved_1;
    uint16_t count;
    uint8_t reserved_2[6];
};

struct FIS_Register_PIO {
    volatile uint8_t fis_type;
    volatile uint8_t header;
    volatile uint8_t status;
    volatile uint8_t error;
    volatile uint8_t lba0;
    volatile uint8_t lba1;
    volatile uint8_t lba2;
    volatile uint8_t device;
    volatile uint8_t lba3;
    volatile uint8_t lba4;
    volatile uint8_t lba5;
    volatile uint8_t reserved1;
    volatile uint16_t count;
    volatile uint8_t reserved2;
    volatile uint8_t new_status;
    volatile uint16_t transfer_count;
    volatile uint16_t reserved3;
};

struct FIS_Register_Data {
    volatile uint8_t fis_type; //Always 0x46
    volatile uint8_t port_multiplier;
    volatile uint16_t reserved;
    volatile uint32_t data[1];
};

static volatile uint16_t *sata_buffer = 0;
static uintptr_t address = 0;
static volatile struct HBA_MemoryRegister *bar5 = 0;

void itoa(uint64_t num, int base, char *str){
    uint64_t count = num;
    char *digits = "0123456789ABCDEF";
    int len = 0;
    do{
        ++len;
    } while(count /= base);

    do{
        str[--len] = digits[num % base];
    } while(num /= base);
}

void InitializeAhciPort(struct HBA_Port *port, int port_num){
    port->command_and_status &= ~0x1;
    port->command_and_status &= ~0x10;

    uintptr_t ahci_base = KernelAllocateFrame();
    KernelMapMmio(ahci_base + MMIO_OFFSET, ahci_base);

    while(1){
        if((port->command_and_status & 0x4000) || (port->command_and_status & 0x8000)) continue;
        break;
    }
;
    port->command_list_address = KernelAllocateFrame();
    port->cla_upper = 0;
    KernelMapMmio(port->command_list_address + MMIO_OFFSET, port->command_list_address);
    port->fis_address = KernelAllocateFrame();
    KernelMapMmio(port->fis_address + MMIO_OFFSET, port->fis_address);
    port->fis_upper = 0;
    memset((void *)(port->fis_address + MMIO_OFFSET), 0, 256);


    struct HBA_Command_Header *command_header = (struct HBA_Command_Header *)(port->command_list_address + MMIO_OFFSET);
    for(int i = 0; i < 32; ++i){
        command_header[i].prd_table_length = 8;
        command_header[i].ctd_base_address = KernelAllocateFrame();
        KernelMapMmio(command_header[i].ctd_base_address + MMIO_OFFSET, command_header[i].ctd_base_address);
        command_header[i].ctd_base_upper = 0;
        uintptr_t ctd = command_header[i].ctd_base_address;
        memset((void *)(ctd + MMIO_OFFSET), 0, 256);
    }

    sata_buffer = (void *)(KernelAllocateFrame() + MMIO_OFFSET);
    KernelMapMmio((uint64_t)sata_buffer, ((uint64_t) sata_buffer) - MMIO_OFFSET);
    while(port->command_and_status & 0x8000);

    port->command_and_status |= 0x1;
    port->command_and_status |= 0x10;
}

int AhciFindFreeCommandSlot(struct HBA_Port *port){
    uint32_t slots = (port->sata_active | port->command_issue);
    for(int i = 0; i < 32; ++i){
        if((slots & 1) == 0) return i;
        slots >>= 1;
    }
    return -1;
}

uint16_t *AhciSendIdentify(struct HBA_Port *port){
    memset(sata_buffer, 0, 1024);
    port->interrupt_status = 0xFFFFFFFF;
    int slot = AhciFindFreeCommandSlot(port);
    if(slot == -1) return 0;

    struct HBA_Command_Header *command_header = (struct HBA_Command_Header *)(port->command_list_address + MMIO_OFFSET);
    command_header += slot;
    command_header->header_1 |= (sizeof(struct FIS_Register_H2D) / sizeof(uint32_t)) & 0x1F;
    command_header->header_1 &= ~(1 << 6);
    command_header->prd_table_length = 1;
    struct HBA_Command_Table *command_table = (struct HBA_Command_Table *)(command_header->ctd_base_address + MMIO_OFFSET);
    memset(command_table, 0, sizeof(struct HBA_Command_Table) + sizeof(struct HBA_Physical_Region_Descriptor_Entry));

    command_table->hda_prdt[0].data_base_address = (uint64_t) (sata_buffer) - MMIO_OFFSET & 0xFFFFFFFF;
    command_table->hda_prdt[0].byte_count_and_interrupt_on_completion |= 511;
    command_table->hda_prdt[0].byte_count_and_interrupt_on_completion |= (1 << 31);

    struct FIS_Register_H2D *command_fis = (struct FIS_Register_H2D *)((uint64_t)&command_table->command_fis);
    command_fis->fis_type = 0x27;
    command_fis->multiplier_and_cc |= 1 << 7;
    command_fis->command = 0xEC;
    command_fis->device = 0;

    if(port->task_file_data & 0x1){
        kprint("An error occured");
        return 0;
    }

    while(port->task_file_data & (0x88));
    port->command_issue = 1 << slot;

    struct FIS_Register_PIO fis_pio = {0};
    memcpy(&fis_pio, (void *)((uint64_t)port->fis_address + MMIO_OFFSET + 0x20), 0x14);

    while(port->command_issue & (1 << slot));

    return sata_buffer;

}

uint16_t * AhciReadSectorInner(struct HBA_Port *port, long start){
    memset(sata_buffer, 0, 1024);
    port->interrupt_status = 0xFFFFFFFF;
    int slot = AhciFindFreeCommandSlot(port);
    if (slot == -1) return sata_buffer;

    struct HBA_Command_Header *command_header = (struct HBA_Command_Header *)(port->command_list_address + MMIO_OFFSET);
    command_header += slot;
    command_header->header_1 |= (sizeof(struct FIS_Register_H2D) / sizeof(uint32_t)) & 0x1F;
    command_header->header_1 &= ~(1 << 6);
    command_header->prd_table_length = 1;

    struct HBA_Command_Table *command_table = (struct HBA_Command_Table *)(command_header->ctd_base_address + MMIO_OFFSET);
    memset(command_table, 0, sizeof(struct HBA_Command_Table) + sizeof(struct HBA_Physical_Region_Descriptor_Entry));

    command_table->hda_prdt[0].data_base_address = (uint64_t)sata_buffer & 0xFFFFFFFF;
    command_table->hda_prdt[0].byte_count_and_interrupt_on_completion |= 511;
    command_table->hda_prdt[0].byte_count_and_interrupt_on_completion &= ~(1 << 31);

    struct FIS_Register_H2D *command_fis = (struct FIS_Register_H2D *)(&command_table->command_fis);
    command_fis->fis_type = 0x27;
    command_fis->multiplier_and_cc |= 1 << 7;
    command_fis->command = 0x25;

    start += 1;
    command_fis->lba0 = (uint8_t)(start & 0xFF);
    command_fis->lba1 = (uint8_t)((start >> 8) & 0xFF);
    command_fis->lba2 = (uint8_t)((start >> 16) & 0xFF);
    command_fis->lba3 = (uint8_t)((start >> 24) & 0xFF);
    command_fis->lba4 = (uint8_t)((start >> 32) & 0xFF);
    command_fis->lba5 = (uint8_t)((start >> 40) & 0xFF);
    command_fis->count = 1;

    while(port->task_file_data & (0x88));
    port->command_issue = 1 << slot;

    while(port->command_issue & (1 << slot)){
        if(port->task_file_data & 0x1){
            kprint("An error occured\n");
            return sata_buffer;
        }
    }

    if(port->task_file_data & 0x1){
        kprint("An error occured\n");
        return sata_buffer;
    }

    return sata_buffer;
}

char * AhciReadSector(struct BlockDevice *sata_device, long offset){
    return (char *) AhciReadSectorInner(&(bar5->hba_port[sata_device->specific_info]), offset);
}

static struct BlockDeviceFunctions ahci_functions = {
    0
};

void AhciAddDevice(int port){
    char sata_identifier[14] = "SataDevice";
    char numstr[3] = {0};
    itoa(port, 10, numstr);
    strcat(sata_identifier, numstr);
    ahci_functions.read = &AhciReadSector;
    struct BlockDevice *bd = IoCreateBlockDevice((void *) (((uint64_t) &ahci_functions)), sata_identifier);
    bd->specific_info = port;
    bd->block_size = 512;
    uint16_t *identify_data = AhciSendIdentify(&bar5->hba_port[port]);
}

void InitializeAhci(uintptr_t address){
    address = address;
    struct PCIObject *pci_object = HalSearchForPciDevice(0x1, 0x6, 0);
    if(!pci_object){
        kprint("No device");
        return;
    }
    struct PCIDevice *pci_dev = (struct PCIDevice *)(pci_object->pci_information);

    uint16_t buffer[256] = {0};
    uint32_t pci_status_command = KernelReadPciConfigWord(pci_object->bus, pci_object->device, pci_object->function, 0x4);
    pci_status_command |= 0x7;
    pci_status_command &= ~(1 << 10);
    uint32_t pci_status = KernelReadPciConfigWord(pci_object->bus, pci_object->device, pci_object->function, 0x6);
    pci_status_command |= pci_status << 16;
    KernelWritePciConfigDword(pci_object->bus, pci_object->device, pci_object->function, 0x4, pci_status_command);
    bar5 = (void *)(((uint64_t)pci_dev->base_address[5] & 0xFFFFFFF0));
    KernelMapMmio((uint64_t) bar5, (uint64_t) bar5);
    KernelMapMmio((uint64_t) bar5 + 0x1000, (uint64_t) bar5 + 0x1000);
    uint32_t port_implemented = bar5->port_implemented;
    for(int i = 0; i < 32; ++i){
        if(!(port_implemented & 1)) { port_implemented >>= 1; continue;}
        if(((bar5->hba_port[i].sata_status >> 8) & 0x0F) != 0x01) { port_implemented >>= 1; continue;}
        if((bar5->hba_port[i].sata_status & 0x0F) != 0x03) { port_implemented >>= 1; continue; }
        kprint("Found a device: %x\n", bar5->hba_port[i].signature);
        InitializeAhciPort(&bar5->hba_port[i], i);
        AhciAddDevice(i);
        port_implemented >>= 1;
    }

    *((volatile uint32_t *)(&bar5->global_host_control)) |= 0x2; //Enable interrupts
    *((volatile uint32_t *)(&bar5->global_host_control)) |= 1 << 31; //Enable AHCI mode;
}
