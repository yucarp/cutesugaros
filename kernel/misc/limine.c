#include <stdint.h>
#include <limine.h>

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

void *KernelGetRsdpAddress(){
    if(rsdp_request.response == 0) return 0;
    return rsdp_request.response->address;
}

uint64_t KernelGetHhdmOffset(){
    if(hhdm_request.response == 0) return 0;
    return hhdm_request.response->offset;
}

struct limine_memmap_response *KernelGetMemmapFromBootloader(){
    return memmap_request.response;
}

void *KernelGetModule(int number){
    if(!module_request.response) return 0;
    if(module_request.response->module_count < number) return 0;
    return module_request.response->modules[number]->address;
}

struct limine_framebuffer *KernelGetFramebuffer(){
    if(!framebuffer_request.response) return 0;
    return framebuffer_request.response->framebuffers[0];
}
