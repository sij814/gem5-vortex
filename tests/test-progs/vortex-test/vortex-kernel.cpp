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

// CPU Address Range
#define DATA_STORE_ADDR   (uint32_t*)0x10000000

// GPU Address Range
#define ACCELERATOR_BASE  (uint32_t*)0x20000000
#define DMA_SRC_ADDR      (uint32_t*)0x20000004
#define DMA_DEST_ADDR     (uint32_t*)0x20000008
#define DMA_SIZE          (uint32_t*)0x2000000c
#define DMA_START         (uint32_t*)0x20000010
#define DMA_STATUS        (uint32_t*)0x20000014
#define KERNEL_RUNNING    (uint32_t*)0x20000018
#define KERNEL_START_ADDR (uint32_t*)0x2000001c
#define KERNEL_ARG_ADDR   (uint32_t*)0x20000020
#define KERNEL_START      (uint32_t*)0x20000028

// Kernel argument struct for regression test
typedef struct {
    uint32_t num_points;
    uint64_t src0_addr;
    uint64_t src1_addr;
    uint64_t dst_addr;  
  } kernel_arg_t;

kernel_arg_t kernel_arg = {};

const char* fileExtension(const char* filepath) {
    const char *ext = strrchr(filepath, '.');
    if (ext == NULL || ext == filepath)
      return "";
    return ext + 1;
}

int performDMARead(void* dest_addr, void* data_addr, uint32_t size) {
    *DMA_SRC_ADDR = (uint32_t)DATA_STORE_ADDR;
    *DMA_DEST_ADDR = (uint32_t)dest_addr;
    *DMA_SIZE = size;

    memcpy(reinterpret_cast<void*>(DATA_STORE_ADDR), data_addr, size);

    *DMA_START = 1;

    *DMA_STATUS = 0;
    for (;;) {
        int64_t status = *DMA_STATUS;
        if (status) {
            break;
        }
    }
    std::cout << "DMA Read Done for address: " << std::hex << dest_addr << std::endl;
    return 1;
}

int performDMAWrite(void* src_addr, void* data_addr, uint32_t size) {
    *DMA_SRC_ADDR = (uint32_t)src_addr;
    *DMA_DEST_ADDR = (uint32_t)DATA_STORE_ADDR;
    *DMA_SIZE = size;

    *DMA_START = 2;

    *DMA_STATUS = 0;
    for (;;) {
        int64_t status = *DMA_STATUS;
        if (status) {
            break;
        }
    }

    memcpy(data_addr, reinterpret_cast<void*>(DATA_STORE_ADDR), size);
    
    std::cout << "DMA Write Done for address: " << std::hex << src_addr << std::endl;
    return 1;
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

    void* destAddr = reinterpret_cast<void*>(destination);
  
    performDMARead(destAddr, content.data(), size);
}

int main()
{
    // kernel and kernel argument addresses
    uint32_t kernel_addr = (uint32_t)(STARTUP_ADDR);
    uint32_t kernel_arg_addr = kernel_addr + 0x100000;

    // read program kernel in
    const char* program = "/home/sij814/gem5-vortex/tests/test-progs/vortex-test/kernel.vxbin";
    //const char* program = "/home/sij814/gem5-vortex/ext/vortex/build/tests/kernel/hello/hello.bin";
    std::string program_ext(fileExtension(program));

    if (program_ext == "bin" || program_ext == "vxbin") {
        loadBinImage(program, kernel_addr);
        std::cout << "bin file detected" << std::endl;
    }

    // setup kernel arguments
    uint32_t num_points = 8;
    uint32_t buf_size = num_points * sizeof(uint32_t);

    // allocate temporary device memory mapping
    kernel_arg.num_points = num_points;
    kernel_arg.src0_addr  = USER_BASE_ADDR + 0x0;
    kernel_arg.src1_addr  = USER_BASE_ADDR + 0x1000;
    kernel_arg.dst_addr   = USER_BASE_ADDR + 0x2000;

    // allocate host buffers
    std::vector<uint32_t> h_src0(num_points);
    std::vector<uint32_t> h_src1(num_points);
    std::vector<uint32_t> h_dst(num_points);
  
    for (uint32_t i = 0; i < num_points; ++i) {
      h_src0[i] = i;
      h_src1[i] = i;
    }

    std::cout << "dev_src0=0x" << std::hex << kernel_arg.src0_addr << std::endl;
    std::cout << "dev_src1=0x" << std::hex << kernel_arg.src1_addr << std::endl;
    std::cout << "dev_dst=0x" << std::hex << kernel_arg.dst_addr << std::endl;

    // upload the kernel argument into GPU
    performDMARead(reinterpret_cast<void*>(kernel_arg.src0_addr), h_src0.data(), buf_size);
    performDMARead(reinterpret_cast<void*>(kernel_arg.src1_addr), h_src1.data(), buf_size);
    performDMARead(reinterpret_cast<void*>(kernel_arg_addr), &kernel_arg, sizeof(kernel_arg_t));

    std::cout << "starting kernel" << std::endl;
    *KERNEL_START_ADDR = kernel_addr + 0x10;
    *KERNEL_ARG_ADDR = kernel_arg_addr;
    *KERNEL_START = 3;

    // wait for kernel
    for (;;) {
      int64_t status = *KERNEL_RUNNING;
      if (!status) {
        break;
      }
    }

    std::cout << "kernel finished" << std::endl;

    // download the data from GPU
    performDMAWrite(reinterpret_cast<void*>(kernel_arg.dst_addr), h_dst.data(), buf_size);

    for (int i = 0; i < h_dst.size(); i++) {
      std::cout << "Value at " << i << ": " << h_dst[i] << std::endl;
    }
}