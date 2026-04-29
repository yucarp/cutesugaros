#include <stdint.h>
#include <string.h>
#include <kernel/kmalloc.h>
#include <kernel/pci.h>
#include <kernel/port.h>
#include <kernel/sbd.h>
#include <kernel/object/object.h>
#include <kernel/object/directory.h>

uint16_t KernelReadPciConfigWord(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset){
    uint32_t address = 0;
    uint16_t temp = 0;
    address = (bus << 16) | (slot << 11) | (function << 8) | (offset & 0xFC) | 0x80000000;

    outl(0xCF8, address);
    temp = (uint16_t)(inl(0xCFC) >> ((offset & 2) * 8) & 0xFFFF);
    return temp;
}

void KernelWritePciConfigDword(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset, uint32_t value){
    uint32_t address = 0;
    address = (bus << 16) | (slot << 11) | (function << 8) | (offset & 0xFC) | 0x80000000;

    outl(0xCF8, address);
    outl(0xCFC, value);
}

void HalInitializePci(){
    struct Directory *dev_dir = (void *) ResolveObjectName(0, "./Devices");
    if(!dev_dir) return;
    CreateDirectory(dev_dir, "PciDevices");
}

int HalCheckPciDevice(uint8_t bus, uint8_t slot, uint8_t func){
    if(KernelReadPciConfigWord(bus, slot, func, 0) == 0xFFFF) {
        return 0;
    }

    return 1;
}

struct PCIObject *HalAddPciDevice(uint8_t bus, uint8_t slot, uint8_t function, char *name){
    struct Directory *pci_dir = (void *) ResolveObjectName(0, "./Devices/PciDevices");
    if(!pci_dir) return 0;

    struct PCIObject *pci_object = malloc(sizeof(struct PCIObject));

    if(KernelReadPciConfigWord(bus, slot, function, 0) == 0xFFFF) {
        free(pci_object);
        return 0;
    }

    pci_object->bus = bus;
    pci_object->device = slot;
    pci_object->function = function;

    if((KernelReadPciConfigWord(bus, slot, 0, 0xE) & 0xFF) == 0x1){
        struct PCItoPCI *pci_to_pci = malloc(sizeof(struct PCItoPCI));
        pci_to_pci->header.device_id = KernelReadPciConfigWord(bus, slot, 0, 2);
        pci_to_pci->header.vendor_id = KernelReadPciConfigWord(bus, slot, 0, 0);
        pci_to_pci->header.subclass = KernelReadPciConfigWord(bus, slot, 0, 0xA) & 0xFF;
        pci_to_pci->header.class_code = (KernelReadPciConfigWord(bus, slot, 0, 0xA) >> 8) & 0xFF;
        pci_to_pci->header.header_type = KernelReadPciConfigWord(bus, slot, 0, 0xE) & 0xFF;
        pci_to_pci->primary_bus_number = KernelReadPciConfigWord(bus, slot, 0, 0x1C) & 0xFF;
        pci_to_pci->secondary_bus_number = (KernelReadPciConfigWord(bus, slot, 0, 0x1C) >> 8) & 0xFF;
        pci_to_pci->capability_pointer = KernelReadPciConfigWord(bus, slot, 0, 0x34) & 0xFF;
        memcpy(pci_object->header.name, name, strlen(name) + 1);
        pci_object->type = 1;
        pci_object->pci_information = pci_to_pci;
        DirectoryAddChild(pci_dir, (void *)pci_object);
        return pci_object;
    }

    struct PCIDevice *pci_device = malloc(sizeof(struct PCIDevice));
    pci_device->header.device_id = KernelReadPciConfigWord(bus, slot, function, 2);
    pci_device->header.vendor_id = KernelReadPciConfigWord(bus, slot, function, 0);
    pci_device->header.subclass = KernelReadPciConfigWord(bus, slot, function, 0xA) & 0xFF;
    pci_device->header.class_code = (KernelReadPciConfigWord(bus, slot, function, 0xA) >> 8) & 0xFF;
    pci_device->header.header_type = KernelReadPciConfigWord(bus, slot, function, 0xE) & 0xFF;
    pci_device->capability_pointer = KernelReadPciConfigWord(bus, slot, function, 0x34) & 0xFF;
    for(int i = 0; i < 6; ++i){
        pci_device->base_address[i] = KernelReadPciConfigWord(bus, slot, function, 0x10 + 4 * i) | ((KernelReadPciConfigWord(bus, slot, function, 0x12 + 4 * i) << 16));
    }
    memcpy(pci_object->header.name, name, strlen(name) + 1);
    pci_object->type = 0;
    pci_object->pci_information = pci_device;
    DirectoryAddChild(pci_dir, (void *)pci_object);
    return pci_object;
}

void pci_itoa(uint64_t num, int base, char *str){
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

static long device_count = 0;

void HalCheckPciBus(int bus_num){
    for(int dev = 0; dev < 32; ++dev){
        if(HalCheckPciDevice(bus_num, dev, 0)) {
            char name[16] = "PciDevice";
            char numstr[4] = {0};
            pci_itoa(device_count, 10, numstr);
            strcat(name, numstr);
            device_count++;
            struct PCIObject *x = HalAddPciDevice(bus_num, dev, 0, name);
            if(x->type == 1){
                char sec_bus_num = ((struct PCItoPCI *)x->pci_information)->secondary_bus_number;
                HalCheckPciBus(sec_bus_num);
            }
            if (((struct PCItoPCI *)x->pci_information)->header.header_type & 0x80){
                for(int func = 1; func < 8; ++func){
                    if(HalCheckPciDevice(bus_num, dev, func)){
                        memset(numstr, 0, 4);
                        memset(name, 0, 16);
                        memcpy(name, "PciDevice", 9);
                        pci_itoa(device_count, 10, numstr);
                        strcat(name, numstr);
                        device_count++;
                        x = HalAddPciDevice(bus_num, dev, func, name);
                        if(x->type == 1){
                            char sec_bus_num = ((struct PCItoPCI *)x->pci_information)->secondary_bus_number;
                            HalCheckPciBus(sec_bus_num);
                        }
                    }
                }
            }
        }
    }
}

struct PCIObject *HalSearchForPciDevice(uint8_t class, uint8_t subclass, int number){
    for(int i = 0; i < 8192; ++i){
        char name[48] = "./Devices/PciDevices/PciDevice";
        char numstr[4] = {0};
        pci_itoa(i, 10, numstr);
        strcat(name, numstr);
        struct PCIObject *pci_object = (void *) ResolveObjectName(0, name);
        if(!pci_object) return 0;
        if(((struct PCIDevice *)(pci_object->pci_information))->header.class_code != class ||
            ((struct PCIDevice *)(pci_object->pci_information))->header.subclass != subclass
        ){continue;}
        if(number) {--number; continue;}
        return pci_object;
    }
}
