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

#define ACCELERATOR_BASE 0x80000000

const char* fileExtension(const char* filepath) {
    const char *ext = strrchr(filepath, '.');
    if (ext == NULL || ext == filepath)
      return "";
    return ext + 1;
}

/*
void write(const void* data, uint64_t addr, uint64_t size) {
    const uint8_t* d = (const uint8_t*)data;
    uint8_t *dest;
    for (uint64_t i = 0; i < size; i++) {
        dest = (uint8_t *)(addr + i);
        *dest = d[i];
    }
}
*/

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
  
    // write to GPU addr range
    std::memcpy(destPtr, content.data(), size);
    //write(content.data(), destination, size);
}

int main()
{
    uint64_t startup_addr = STARTUP_ADDR;

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

    uint64_t *gpu = (uint64_t *)(ACCELERATOR_BASE + 40);
    std::cout << "reading: " << *gpu << std::endl;

    //*gpu = 3;
    //std::cout << "new value: " << *gpu << std::endl;

    return 0;
}