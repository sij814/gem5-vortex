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
#include "opae_simx.h"
//#include "opae_sim.h"

///////////////////////
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <vector>
#include "vortex/runtime/common/common.h"

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
    running(0),
    status(0),
    tickEvent([this]{ processTick(); }, name()),
    dmaReadEvent([this]{ dmaReadEventDone(); }, name()),
    dmaWriteEvent([this]{ dmaWriteEventDone(); }, name()),
    startEvent([this]{ vortex_start(); }, name())
{
    DPRINTF(Vortex, "Creating Vortex\n");

    sim = new vortex::opae_simx();
}

void
Vortex::init()
{
    DPRINTF(Vortex, "Vortex Initialized\n");

    DmaDevice::init();
    sim->init();
    ram->init();
    schedule(tickEvent, 0);
}

void Vortex::processTick() 
{
    running = sim->get_running();
    
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

    uint64_t data;
    uint64_t size = pkt->getSize();

    if (addr == 0x14) {
        pkt->setLE<uint32_t>(status);
        pkt->makeResponse();
        return 0;
    }
    
    if (addr == 0x18) {
        pkt->setLE<uint32_t>(running);
        pkt->makeResponse();
        return 0;
    }

    if (addr >= STARTUP_ADDR) {
        sim->read_mmio64(0, addr, &data, size);
    } else {
        data = registers[addr];
    }

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

    DPRINTF(Vortex, "read() done for %x\n", addr);

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

    if (addr >= STARTUP_ADDR) {
        sim->write_mmio64(0, addr, data, size);
    } else {
        registers[addr] = data;
        
        switch (addr) {
            case 0x10: {
                status = 0;
                const Addr srcAddr(registers[0x4]);
                const Addr destAddr(registers[0x8]);
                uint32_t size = registers[0xc];

                dmaBuffer.resize(size);

                if (data == 1) {
                    DPRINTF(Vortex, "dmaRead srcAddr=%x, destAddr=%x, size=%ld\n", srcAddr, destAddr, size);
                    dmaRead(srcAddr, size, &dmaReadEvent, dmaBuffer.data(), 0);
                } else {
                    sim->read_mem((void*)dmaBuffer.data(), srcAddr, dmaBuffer.size());
                    DPRINTF(Vortex, "dmaWrite srcAddr=%x, destAddr=%x, size=%ld\n", srcAddr, destAddr, size);
                    dmaWrite(destAddr, size, &dmaWriteEvent, dmaBuffer.data(), 0);
                }
                break;
            } case 0x28: {
                switch (data) {
                    case 3:
                        vortex_start();
                        break;
                }
                break;
            }
        }
    }

    // example write
    pkt->makeAtomicResponse();

    return 0;
}

AddrRangeList Vortex::getAddrRanges() const
{
    return AddrRangeList({ RangeSize(pioAddr, pioAddr + 0xFFFFFF) });
}

int Vortex::vortex_start() {
    DPRINTF(Vortex, "vortex start()\n");

    sim->start(registers[0x1c], registers[0x20]);

    DPRINTF(Vortex, "running = %d\n", running);

    return 0;
}

void
Vortex::dmaReadEventDone()
{
    const Addr destAddr(registers[0x8]);
    DPRINTF(Vortex, "DMA Read Done\n");

    sim->write_mem((void*)dmaBuffer.data(), destAddr, dmaBuffer.size());
    status = 1;
}

void
Vortex::dmaWriteEventDone()
{
    const Addr destAddr(registers[0x8]);
    DPRINTF(Vortex, "DMA Write Done\n");
    status = 1;
}

} // namespace gem5
