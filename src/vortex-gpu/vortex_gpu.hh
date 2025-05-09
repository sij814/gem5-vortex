#ifndef __VORTEX_GPU_VORTEX_GPU_HH__
#define __VORTEX_GPU_VORTEX_GPU_HH__

#include <vector>

#include "dev/io_device.hh"
#include "params/Vortex.hh"
#include "sim/eventq.hh"
#include "sim/system.hh"
#include "enums/MemoryMode.hh"
#include "mem/packet_access.hh"
#include "vortex/runtime/include/vortex.h"
#include "vortex/build/hw/VX_config.h"
#include "opae_simx.h"
#include "dev/dma_device.hh"
#include "mem/simple_mem.hh"

namespace gem5
{
struct VortexParams;

class Vortex : public DmaDevice
{
    public:
        Vortex(const VortexParams &p);

        void init() override;

    // Checkpointing
    // TODO: Implement this later
    public:
        void serialize(CheckpointOut &cp) const override;
        void unserialize(CheckpointIn &cp) override;
    
    public:
        Tick read(PacketPtr pkt) override;
        Tick write(PacketPtr pkt) override;
        AddrRangeList getAddrRanges() const override;

    private:
        EventFunctionWrapper tickEvent;
        EventFunctionWrapper dmaReadEvent;
        EventFunctionWrapper dmaWriteEvent;
        EventFunctionWrapper startEvent;

        void processTick();

    // Put wrapper functions here
    protected:
        int vortex_start();
        void dmaReadEventDone();
        void dmaWriteEventDone();
    /*
    private:
        void setCallback(const_vortex_t &callback);
    };
    */

    protected:
        const Addr pioAddr;
        const uint32_t intGpu;
        const uint32_t numClusters;
        const uint32_t numCores;
        const uint32_t numWarps;
        const uint32_t numThreads;

        vortex::opae_simx* sim;

        std::vector<uint8_t> dmaBuffer;
    
    private:
        uint32_t running;
        uint32_t registers[128];
        gem5::memory::SimpleMemory *ram;
        uint32_t status;
};

} // namespace gem5

#endif // __VORTEX-GPU_VORTEX_GPU_HH__