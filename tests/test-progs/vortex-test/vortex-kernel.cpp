#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include "../../../ext/vortex/runtime/include/vortex.h"
#include "../../../ext/vortex/build/hw/VX_config.h"

#include <vector>

#define DATA_STORE_ADDR  (uint32_t*)0x10000000
#define ACCELERATOR_BASE (uint32_t*)0x20000000
#define DMA_SRC_ADDR     (uint32_t*)0x20000004
#define DMA_DEST_ADDR    (uint32_t*)0x20000008
#define DMA_SIZE         (uint32_t*)0x2000000c
#define DMA_START        (uint32_t*)0x20000010
#define DMA_STATUS       (uint32_t*)0x20000014
#define KERNEL_START     (uint32_t*)0x20000028


const char* fileExtension(const char* filepath) {
    const char *ext = strrchr(filepath, '.');
    if (ext == NULL || ext == filepath)
      return "";
    return ext + 1;
}

void loadBinImage(const char* filename, uint64_t destination) {
    std::ifstream ifs(filename);
    if (!ifs) {
      std::cout << "error: " << filename << " not found" << std::endl;
      std::abort();
    } 

    ifs.seekg(0, ifs.end);
    size_t size = ifs.tellg();
    std::vector<uint8_t> content(size);
    ifs.seekg(0, ifs.beg);
    ifs.read((char*)content.data(), size);

    void* destPtr = reinterpret_cast<void*>(destination);
  
    std::cout << "kernel side addr: " << (uint32_t)content.data() << std::endl;

    *DMA_SRC_ADDR = (uint32_t)DATA_STORE_ADDR;
    *DMA_DEST_ADDR = (uint32_t)destPtr;
    *DMA_SIZE = size;

    memcpy(reinterpret_cast<void*>(DATA_STORE_ADDR), content.data(), size);

    *DMA_START = 1;

    *DMA_STATUS = 0;
    for (;;) {
        int64_t status = *DMA_STATUS;
        if (status) {
            break;
        }
    }

    std::cout << "DMA Done" << std::endl;

    *KERNEL_START = 3;

    // copy to GPU addr range
    //std::memcpy(destPtr, content.data(), size);
    //std::memcpy((uint32_t*)*DMA_DEST_ADDR, (uint32_t*)*DMA_SRC_ADDR, *DMA_SIZE);
}

int main()
{
    uint64_t startup_addr = (uint64_t)(ACCELERATOR_BASE) + (uint64_t)(STARTUP_ADDR);

    const char* program = "/home/sij814/gem5-vortex/ext/vortex/build/tests/kernel/conform/conform.bin";
    std::string program_ext(fileExtension(program));

    if (program_ext == "bin" || program_ext == "vxbin") {
        loadBinImage(program, startup_addr);
        std::cout << "bin file detected" << std::endl;
    } else if (program_ext == "hex") {
        //ram_->loadHexImage(program);
        std::cout << "hex file detected" << std::endl;
    } else {
        std::cout << "*** error: only *.bin or *.hex images supported." << std::endl;
        return -1;
    }
    //uint8_t *gpu = (uint8_t *)(ACCELERATOR_BASE + 40);
}