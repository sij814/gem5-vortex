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
    : PioDevice(p),
    pioAddr(p.pio_addr),
    intGpu(p.int_gpu),
    numClusters(p.num_clusters),
    numCores(p.num_cores),
    numWarps(p.num_warps),
    numThreads(p.num_threads),
    running(0),
    tickEvent([this]{processTick();}, name())
{
    DPRINTF(Vortex, "Creating Vortex\n");

    sim = new vortex::opae_simx();
}

void
Vortex::init()
{
    DPRINTF(Vortex, "Vortex Initialized\n");

    PioDevice::init();
    sim->init();
    schedule(tickEvent, 0);
}

void Vortex::processTick() 
{
    if (curTick() % 100000000 == 0) {
        DPRINTF(Vortex, "Vortex tick = %d\n", SimPlatform::instance().cycles());
    }

    // simulate Gem5 tick
    schedule(tickEvent, curTick() + 1);
}

void Vortex::serialize(CheckpointOut &cp) const
{
}

void Vortex::unserialize(CheckpointIn &cp)
{
}

Tick Vortex::read(PacketPtr pkt) 
{
    const Addr addr(pkt->getAddr());
    uint64_t value = 0;

    DPRINTF(Vortex, "read() %x\n", addr);
    sim->read_mmio64(0, addr, &value);

    // example read
    pkt->setLE<uint64_t>(value);
    pkt->makeResponse();

    if (addr - pioAddr == 40 && ((value & 3) == 3)) {
        vortex_start();
    }

    DPRINTF(Vortex, "read() done for %x\n", addr);

    return 0;
}

Tick Vortex::write(PacketPtr pkt) 
{
    const Addr addr(pkt->getAddr());
    uint64_t data;
    switch (pkt->getSize()) {
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

    sim->write_mmio64(0, addr, data);
    //sim->write_mmio64(0, addr, *data);

    // example write
    pkt->makeAtomicResponse();

    // obtained from start() of vortex.cpp for opae
    if (addr - pioAddr == 40 && data == 3) {
        vortex_start();
    }

    return 0;
}

AddrRangeList Vortex::getAddrRanges() const
{
    return AddrRangeList({ RangeSize(pioAddr, pioAddr + 0xFFFF) });
}

int Vortex::vortex_start() {
    DPRINTF(Vortex, "vortex start()\n");

    sim->start();

    DPRINTF(Vortex, "running = %d\n", running);

    return 0;
}

int Vortex::vortex_read(vx_device_h hdevice, uint32_t addr, uint32_t* value) 
{
    DPRINTF(Vortex, "vortex read()\n");

    vx_dcr_read(hdevice, addr, value);

    return 0;
}

int Vortex::vortex_write(vx_device_h hdevice, uint32_t addr, uint32_t value) 
{
    DPRINTF(Vortex, "vortex write() %x\n", addr);

    vx_dcr_write(hdevice, addr, value);

    return 0;
}

} // namespace gem5
