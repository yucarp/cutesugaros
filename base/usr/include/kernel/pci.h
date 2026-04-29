#include <stdint.h>
#include <kernel/object/object.h>

struct PCIObject{
    struct ObjectHeader header;
    int type;
    int bus;
    int device;
    int function;
    void *pci_information;
};

struct PCIHeader {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t programming_if;
    uint8_t subclass;
    uint8_t class_code;
    uint8_t bist;
    uint8_t header_type;
    uint8_t latency_timer;
    uint8_t cache_line_size;
};

struct PCIDevice {
    struct PCIHeader header;
    uint32_t base_address[6];
    uint32_t cardbus_cis_pointer;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_address;
    uint16_t reserved_1;
    uint16_t capability_pointer;
    uint16_t reserved_2;
    uint8_t reserved_3;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_grant;
    uint8_t max_latency;
};

struct PCItoPCI{
    struct PCIHeader header;
    uint32_t base_address[2];
    uint8_t primary_bus_number;
    uint8_t secondary_bus_number;
    uint8_t subordinate_bus_number;
    uint8_t secondary_latency_timer;
    uint8_t io_base;
    uint8_t io_limit;
    uint16_t secondary_status;
    uint16_t prefetchable_memory_base;
    uint16_t prefetchable_memory_limit;
    uint16_t prefetchable_memory_base_upper;
    uint16_t prefetchable_memory_limit_upper;
    uint16_t io_base_upper;
    uint16_t io_limit_upper;
    uint16_t capability_pointer;
    uint16_t reserved_1;
    uint8_t reserved_2;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint16_t bridge_control;
};

void HalInitializePci();
void HalCheckPciBus(int bus_num);
uint16_t KernelReadPciConfigWord(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset);
void KernelWritePciConfigDword(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset, uint32_t value);
struct PCIObject *HalSearchForPciDevice(uint8_t class, uint8_t subclass, int number);
