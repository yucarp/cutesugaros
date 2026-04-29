#include <limine.h>
#include <stdint.h>
#include <string.h>
#include <kernel/mmu.h>
#include <kernel/pci.h>
#include <kernel/sbd.h>

struct HBA_Port {
    uint32_t command_list_address;
    uint32_t cla_upper;
    uint32_t fis_address;
    uint32_t fis_upper;
    uint32_t interrupt_status;
    uint32_t interrupt_enable;
    uint32_t command_and_status;
    uint32_t reserved_dword;
    uint32_t task_file_data;
    uint32_t signature;
    uint32_t sata_status;
    uint32_t sata_control;
    uint32_t sata_error;
    uint32_t sata_active;
    uint32_t command_issue;
    uint32_t sata_notificiation;
    uint32_t switch_control;
    uint32_t reserved[11];
    uint32_t vendor_specific[4];
}__attribute__((packed));

struct HBA_MemoryRegister {
    uint32_t host_capability;
    uint32_t global_host_control;
    uint32_t interrupt_status;
    uint32_t port_implemented;
    uint32_t version;
    uint32_t ccc_control;
    uint32_t ccc_ports;
    uint32_t em_location;
    uint32_t em_control;
    uint32_t capabilities_extended;
    uint32_t bios_os_handoff_control;
    uint8_t reserved[0xA0 - 0x2C];
    uint8_t vendor_specific[0x100 - 0xA0];
    struct HBA_Port hba_port[32];
}__attribute__((packed));

struct HBA_Command_Header{
    uint8_t header_1;
    uint8_t header_2;
    uint16_t prd_table_length;
    uint32_t prd_byte_count;
    uint32_t ctd_base_address;
    uint32_t ctd_base_upper;
    uint32_t reserved[4];
}__attribute__((packed));

struct HBA_Physical_Region_Descriptor_Entry {
    uint32_t data_base_address;
    uint32_t data_base_upper;
    uint32_t reserved;
    uint32_t byte_count_and_interrupt_on_completion;
}__attribute__((packed));

struct HBA_Command_Table {
    uint8_t command_fis[64];
    uint8_t atapi_command[16];
    uint8_t reserved[48];
    struct HBA_Physical_Region_Descriptor_Entry hda_prdt[1];
}__attribute__((packed));

struct FIS_Register_H2D{
    uint8_t fis_type; //Always 0x27
    uint8_t multiplier_and_cc;
    uint8_t command;
    uint8_t feature;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t feature_high;
    uint16_t count;
    uint8_t icc;
    uint8_t control;
    uint32_t reserved[4];
}__attribute__((packed));

void InitializeAhciPort(struct HBA_Port *port, int port_num){
    port->command_and_status &= ~0x1;
    port->command_and_status &= ~0x10;

    while(1){
        if((port->command_and_status & 0x4000) || (port->command_and_status & 0x8000)) continue;
        break;
    }

    uintptr_t command_port_address = 0x400000 + (port_num << 10) + KernelGetHhdmOffset();
    port->command_list_address = 0x400000 + (port_num << 10);
    port->cla_upper = 0;

    uintptr_t fis_address = 0x400000 + (32 << 10) + (port_num << 8) + KernelGetHhdmOffset();
    port->fis_address = 0x400000 + (32 << 10) + (port_num << 8);
    port->fis_upper = 0;
    memset((void *)fis_address, 0, 256);

    struct HBA_Command_Header *command_header = (struct HBA_Command_Header *)(command_port_address);
    for(int i = 0; i < 32; ++i){
        command_header[i].prd_table_length = 8;
        command_header[i].ctd_base_address = 0x400000 + (40 << 10) + (port_num << 13) + (i << 8);
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

    struct HBA_Command_Header *command_header = (struct HBA_Command_Header *)(port->command_list_address + KernelGetHhdmOffset());
    command_header += slot;
    command_header->header_1 |= (sizeof(struct FIS_Register_H2D) / sizeof(uint32_t)) & 0x1F;
    command_header->header_1 &= ~(1 << 6);
    command_header->prd_table_length = 1;
    struct HBA_Command_Table *command_table = (struct HBA_Command_Table *)(command_header->ctd_base_address + KernelGetHhdmOffset());
    memset(command_table, 0, sizeof(struct HBA_Command_Table) + sizeof(struct HBA_Physical_Region_Descriptor_Entry));
    int i = 0;
    for(; i < (command_header->header_1 & 0x1F); ++i){
        command_table->hda_prdt[i].data_base_address = (uint64_t) &buffer & 0xFFFFFFFF;
        command_table->hda_prdt[i].data_base_upper = ((uint64_t)&buffer >> 32) & 0xFFFFFFFF;
        command_table->hda_prdt[i].byte_count_and_interrupt_on_completion |= 8192;
        command_table->hda_prdt[i].byte_count_and_interrupt_on_completion |= (1 << 31);
    }

    struct FIS_Register_H2D *command_fis = (struct FIS_Register_H2D *)(&command_table->command_fis);
    command_fis->fis_type = 0x27;
    command_fis->multiplier_and_cc |= 1 << 7;
    command_fis->command = 0xEC;

    command_fis->lba0 = 0;
    command_fis->lba1 = 0;
    command_fis->lba2 = 0;
    command_fis->lba3 = 0;
    command_fis->lba4 = 0;
    command_fis->lba5 = 0;
    command_fis->count = 0;
    command_fis->device = 0;

    if(port->task_file_data & 0x1){
        kprint("An error occured");
        return;
    }

    //while(port->task_file_data & (0x88));
    port->command_issue = 1 << slot;

    /*while(1){
        if((port->command_issue & (1 << slot)) == 0) break;
        if(port->interrupt_status & 0x1){
            kprint("Error");
            return;
        }
    }*/

    for(int i = 0; i < 4096; ++i){
        if(buffer[i]) kprint("Buffer: %x\n", buffer[i]);
    }
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
        //AhciSendIdentify(&bar5->hba_port[i]);
        kprint("Initialized the device: %x\n", bar5->hba_port[i].command_list_address);
        port_implemented >>= 1;
    }

    bar5->global_host_control |= 0x1; //Reset the device
    bar5->global_host_control |= 0x2; //Enable interrupts
    bar5->global_host_control |= 1 << 31; //Enable AHCI mode;
}
