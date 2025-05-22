#include "vortex-gpu/vortex_gpu.hh"
#include "debug/Vortex.hh"
#include "sim/system.hh"
#include "mem/request.hh"
#include "arch.h"
#include "processor.h"
#include "mem.h"
#include "constants.h"
#include <util.h>
#include "core.h"
#include "VX_types.h"
#include "VX_config.h"
#include "simx_device.h"

///////////////////////
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <vector>

#define CMD_MEM_READ     AFU_IMAGE_CMD_MEM_READ
#define CMD_MEM_WRITE    AFU_IMAGE_CMD_MEM_WRITE
#define CMD_RUN          AFU_IMAGE_CMD_RUN
#define CMD_DCR_WRITE    AFU_IMAGE_CMD_DCR_WRITE

#define MMIO_CMD_TYPE    (AFU_IMAGE_MMIO_CMD_TYPE * 4)
#define MMIO_CMD_ARG0    (AFU_IMAGE_MMIO_CMD_ARG0 * 4)
#define MMIO_CMD_ARG1    (AFU_IMAGE_MMIO_CMD_ARG1 * 4)
#define MMIO_CMD_ARG2    (AFU_IMAGE_MMIO_CMD_ARG2 * 4)
#define MMIO_STATUS      (AFU_IMAGE_MMIO_STATUS * 4)
#define MMIO_DEV_CAPS    (AFU_IMAGE_MMIO_DEV_CAPS * 4)
#define MMIO_ISA_CAPS    (AFU_IMAGE_MMIO_ISA_CAPS * 4)
#define MMIO_SCOPE_READ  (AFU_IMAGE_MMIO_SCOPE_READ * 4)
#define MMIO_SCOPE_WRITE (AFU_IMAGE_MMIO_SCOPE_WRITE * 4)

namespace gem5
{

Vortex::Vortex(const VortexParams &p)
    : DmaDevice(p),
    pioAddr(p.pio_addr),
    intGpu(p.int_gpu),
    numClusters(p.num_clusters),
    numCores(p.num_cores),
    numWarps(p.num_warps),
    numThreads(p.num_threads),
    ram(p.vortex_ram),
    initStatus(0),
    running(0),
    status(0),
    tickEvent([this]{ processTick(); }, name()),
    dmaReadEvent([this]{ dmaReadEventDone(); }, name()),
    dmaWriteEvent([this]{ dmaWriteEventDone(); }, name()),
    startEvent([this]{ vortex_start(); }, name())
{
    DPRINTF(Vortex, "Creating Vortex\n");

    sim = new vortex::simx_device();
}

void
Vortex::init()
{
    DPRINTF(Vortex, "Vortex Initialized\n");

    DmaDevice::init();
    sim->init();
    //ram->init();
    schedule(tickEvent, 0);
}

void Vortex::processTick() 
{        
    if (curTick() % 1000000000 == 0) {
        DPRINTF(Vortex, "Vortex tick = %d running = %d\n", SimPlatform::instance().cycles(), sim->get_running());
    }
    // simulate Gem5 tick
    schedule(tickEvent, curTick() + 10);

    if (sim->get_running()) {
        sim->proc_tick();
        if (curTick() % 10000 == 0) {
            DPRINTF(Vortex, "running Vortex tick = %d\n", SimPlatform::instance().cycles());
        }
    } else {
        if (running) {
            MMIORegisters[MMIO_STATUS] = 0;
            running = 0;
        }
    }
}

void Vortex::serialize(CheckpointOut &cp) const
{
}

void Vortex::unserialize(CheckpointIn &cp)
{
}

Tick Vortex::read(PacketPtr pkt) 
{
    const Addr addr(pkt->getAddr() - pioAddr);

    DPRINTF(Vortex, "read() %x\n", addr);

    uint64_t data = MMIORegisters[addr];
    uint64_t size = pkt->getSize();

    switch (size) {
        case sizeof(uint64_t):
            pkt->setLE<uint64_t>(data);
            break;
        case sizeof(uint32_t):
            pkt->setLE<uint32_t>(data);
            break;
        case sizeof(uint16_t):
            pkt->setLE<uint16_t>(data);
            break;
        case sizeof(uint8_t):
            pkt->setLE<uint8_t>(data);
            break;
    }

    pkt->makeResponse();

    DPRINTF(Vortex, "read() done for %x data %x\n", addr, data);

    return 0;
}

Tick Vortex::write(PacketPtr pkt) 
{
    const Addr addr(pkt->getAddr() - pioAddr);
    uint64_t data;
    uint64_t size = pkt->getSize();
    switch (size) {
        case sizeof(uint64_t):
            data = pkt->getLE<uint64_t>();
            break;
        case sizeof(uint32_t):
            data = pkt->getLE<uint32_t>();
            break;
        case sizeof(uint16_t):
            data = pkt->getLE<uint16_t>();
            break;
        case sizeof(uint8_t):
            data = pkt->getLE<uint8_t>();
            break;
    }
    DPRINTF(Vortex, "write() %x, %x\n", addr, data);

    MMIORegisters[addr] = data;
    
    if (addr == MMIO_CMD_TYPE) {
        switch (data) {
            case CMD_MEM_READ: 
            case CMD_MEM_WRITE: {
                status = 0;
                const Addr stagingAddr(MMIORegisters[MMIO_CMD_ARG0]);
                const Addr deviceAddr(MMIORegisters[MMIO_CMD_ARG1]);
                uint64_t size = MMIORegisters[MMIO_CMD_ARG2];

                dmaBuffer.resize(size);

                if (data == CMD_MEM_WRITE) {
                    DPRINTF(Vortex, "dmaRead stagingAddr=%x, deviceAddr=%x, size=%ld\n", stagingAddr, deviceAddr, size);
                    dmaRead(stagingAddr, size, &dmaReadEvent, dmaBuffer.data(), 0);
                } else {
                    sim->read_mem((void*)dmaBuffer.data(), deviceAddr, dmaBuffer.size());
                    DPRINTF(Vortex, "dmaWrite stagingAddr=%x, deviceAddr=%x, size=%ld\n", stagingAddr, deviceAddr, size);
                    dmaWrite(stagingAddr, size, &dmaWriteEvent, dmaBuffer.data(), 0);
                }
                MMIORegisters[MMIO_STATUS] = 1;
                break;
            } case CMD_RUN: {
                MMIORegisters[MMIO_STATUS] = 1;
                vortex_start();
                break;
            } case CMD_DCR_WRITE: {
                uint32_t addr = (uint32_t)MMIORegisters[MMIO_CMD_ARG0];
                uint32_t value = (uint32_t)MMIORegisters[MMIO_CMD_ARG1];
                
                DPRINTF(Vortex, "DCR Write addr=%d value=%d\n", addr, value);

                sim->dcr_write(addr, value);
                break;
            }
        }
    }

    pkt->makeAtomicResponse();

    return 0;
}

AddrRangeList Vortex::getAddrRanges() const
{
    return AddrRangeList({ RangeSize(pioAddr, pioAddr + 0xFFFFFF) });
}

int Vortex::vortex_start() {
    DPRINTF(Vortex, "vortex start()\n");

    sim->start();

    running = 1;

    return 0;
}

void
Vortex::dmaReadEventDone()
{
    const Addr deviceAddr(MMIORegisters[MMIO_CMD_ARG1]);
    DPRINTF(Vortex, "DMA Read Done\n");

    sim->write_mem((void*)dmaBuffer.data(), deviceAddr, dmaBuffer.size());

    MMIORegisters[MMIO_STATUS] = 0;
}

void
Vortex::dmaWriteEventDone()
{
    DPRINTF(Vortex, "DMA Write Done\n");
    MMIORegisters[MMIO_STATUS] = 0;
}

} // namespace gem5
