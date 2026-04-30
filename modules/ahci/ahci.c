#include <limine.h>
#include <stdint.h>
#include <string.h>
#include <kernel/mmu.h>
#include <kernel/pci.h>
#include <kernel/sbd.h>

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

void InitializeAhciPort(struct HBA_Port *port, int port_num){
    port->command_and_status &= ~0x1;
    port->command_and_status &= ~0x10;

    uintptr_t ahci_base = KernelAllocateFrame();

    for(int i = 0; i < 0x40000; i += 0x1000){
        KernelMapMmio(ahci_base + i, ahci_base + i);
        if(KernelAllocateFrame() != ahci_base + i + 0x1000) continue;
    }

    while(1){
        if((port->command_and_status & 0x4000) || (port->command_and_status & 0x8000)) continue;
        break;
    }
;
    port->command_list_address = ahci_base + (port_num << 10);
    port->cla_upper = 0;
    port->fis_address = ahci_base + (32 << 10) + (port_num << 8);
    port->fis_upper = 0;
    memset((void *)port->fis_address, 0, 256);

    struct HBA_Command_Header *command_header = (struct HBA_Command_Header *)(port->command_list_address);
    for(int i = 0; i < 32; ++i){
        command_header[i].prd_table_length = 8;
        command_header[i].ctd_base_address = ahci_base + (40 << 10) + (port_num << 13) + (i << 8);
        command_header[i].ctd_base_upper = 0;
        uintptr_t ctd = command_header[i].ctd_base_address + KernelGetHhdmOffset();
        memset((void *)ctd, 0, 256);
    }

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

void AhciSendIdentify(struct HBA_Port *port){
    uint16_t buffer[4096] = {0};
    port->interrupt_status = 0xFFFFFFFF;
    int slot = AhciFindFreeCommandSlot(port);
    if(slot == -1) return;

    struct HBA_Command_Header *command_header = (struct HBA_Command_Header *)(port->command_list_address);
    command_header += slot;
    command_header->header_1 |= (sizeof(struct FIS_Register_H2D) / sizeof(uint32_t)) & 0x1F;
    command_header->header_1 &= ~(1 << 6);
    command_header->prd_table_length = 1;
    struct HBA_Command_Table *command_table = (struct HBA_Command_Table *)(command_header->ctd_base_address);
    memset(command_table, 0, sizeof(struct HBA_Command_Table) + sizeof(struct HBA_Physical_Region_Descriptor_Entry));
    int i = 0;

    command_table->hda_prdt[i].data_base_address = (uint64_t) &buffer & 0xFFFFFFFF;
    command_table->hda_prdt[i].data_base_upper = ((uint64_t)&buffer >> 32) & 0xFFFFFFFF;
    command_table->hda_prdt[i].byte_count_and_interrupt_on_completion |= 511;
    command_table->hda_prdt[i].byte_count_and_interrupt_on_completion |= (1 << 31);

    struct FIS_Register_H2D *command_fis = (struct FIS_Register_H2D *)(&command_table->command_fis);
    command_fis->fis_type = 0x27;
    command_fis->multiplier_and_cc |= 1 << 7;
    command_fis->command = 0xEC;

    command_fis->device = 0;

    if(port->task_file_data & 0x1){
        kprint("An error occured");
        return;
    }

    //while(port->task_file_data & (0x88));
    port->command_issue = 1 << slot;

    struct FIS_Register_Data fis_pio = {0};
    memcpy(&fis_pio, (void *)((uint64_t)port->fis_address + 0x20), 64);
    kprint("FIS type: %x\n", fis_pio.fis_type);

    while(!((port->command_and_status >> 15) & 1));
    struct FIS_Register_Data *fis_data = (struct FIS_Register_Data *)((uint64_t)port->fis_address + 0x60);
    kprint("FIS type: %x\n", fis_data->fis_type);
    while(1) asm("hlt");
}

void InitializeAhci(){
    struct PCIObject *pci_object = HalSearchForPciDevice(0x1, 0x6, 0);
    if(!pci_object){
        kprint("No device");
        return;
    }
    struct PCIDevice *pci_dev = (struct PCIDevice *)(pci_object->pci_information);


    kprint("Initializing AHCI...\n");
    uint32_t pci_status_command = KernelReadPciConfigWord(pci_object->bus, pci_object->device, pci_object->function, 0x4);
    pci_status_command |= 0x1;
    pci_status_command |= 0x2;
    pci_status_command |= 0x4;
    pci_status_command &= ~(1 << 10);
    uint32_t pci_status = KernelReadPciConfigWord(pci_object->bus, pci_object->device, pci_object->function, 0x6);
    pci_status_command |= pci_status << 16;
    KernelWritePciConfigDword(pci_object->bus, pci_object->device, pci_object->function, 0x4, pci_status_command);
    kprint("Memory bar: %x\n", pci_dev->base_address[5]);
    struct HBA_MemoryRegister *bar5 = (void *)(((uint64_t)pci_dev->base_address[5] & 0xFFFFFFF0));
    KernelMapMmio((uint64_t) bar5, (uint64_t) bar5);
    KernelMapMmio((uint64_t) bar5 + 0x1000, (uint64_t) bar5 + 0x1000);
    uint32_t port_implemented = bar5->port_implemented;
    for(int i = 0; i < 32; ++i){
        if(!(port_implemented & 1)) continue;
        if(((bar5->hba_port[i].sata_status >> 8) & 0x0F) != 0x01) continue;
        if((bar5->hba_port[i].sata_status & 0x0F) != 0x03) continue;
        kprint("Found a device: %x\n", bar5->hba_port[i].signature);
        InitializeAhciPort(&bar5->hba_port[i], i);
        AhciSendIdentify(&bar5->hba_port[i]);
        kprint("Initialized the device: %x\n", bar5->hba_port[i].command_list_address);
        port_implemented >>= 1;
    }

    bar5->global_host_control |= 0x1; //Reset the device
    bar5->global_host_control |= 0x2; //Enable interrupts
    bar5->global_host_control |= 1 << 31; //Enable AHCI mode;
}
