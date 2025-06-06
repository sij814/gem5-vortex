import m5
from m5.objects import *

m5.util.addToPath("../../")

from caches import *

thispath = os.path.dirname(os.path.realpath(__file__))
default_binary = os.path.join(
    thispath,
    "../../",
    'tests/test-progs/vortex-test/vecaddx/main_test',
)

# Binary to execute
SimpleOpts.add_option("--binary", nargs="?", default=default_binary)

args = SimpleOpts.parse_args()

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('512MB'), 
                    AddrRange(Addr("512MB"), size="512MB"),
                    AddrRange(Addr(0xa0000000), size="512MB")]
#system.mem_ranges = [AddrRange('512MB')]

system.cpu = ArmTimingSimpleCPU()

system.membus = SystemXBar()

# Create an L1 instruction and data cache
system.cpu.icache = L1ICache(args)
system.cpu.dcache = L1DCache(args)

# Connect the instruction and data caches to the CPU
system.cpu.icache.connectCPU(system.cpu)
system.cpu.dcache.connectCPU(system.cpu)

# Create a memory bus, a coherent crossbar, in this case
system.l2bus = L2XBar()

# Hook the CPU ports up to the l2bus
system.cpu.icache.connectBus(system.l2bus)
system.cpu.dcache.connectBus(system.l2bus)

# Create an L2 cache and connect it to the l2bus
system.l2cache = L2Cache(args)
system.l2cache.connectCPUSideBus(system.l2bus)

# Create a memory bus
system.membus = SystemXBar()

# Connect the L2 cache to the membus
system.l2cache.connectMemSideBus(system.membus)

system.cpu.createInterruptController()
#system.cpu.interrupts[0].pio = system.membus.mem_side_ports
#system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
#system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports

system.system_port = system.membus.cpu_side_ports

system.mem_ctrl0 = MemCtrl()
system.mem_ctrl0.dram = DDR3_1600_8x8()
system.mem_ctrl0.dram.range = system.mem_ranges[0]
system.mem_ctrl0.port = system.membus.mem_side_ports

system.gpu = Vortex(
    pio_addr=0x20000000,
    int_gpu=1,
    num_clusters=1,
    num_cores=1,
    num_warps=4,
    num_threads=1,
    vortex_ram=SimpleMemory(
        range=system.mem_ranges[2],
        port=system.membus.mem_side_ports),
)

system.gpu.pio = system.membus.mem_side_ports
system.gpu.dma = system.membus.cpu_side_ports


#binary = 'tests/test-progs/vortex-test/conv3x/main_test'
#binary = 'tests/test-progs/vortex-test/diverge/main_test'
#binary = 'tests/test-progs/vortex-test/sgemmx/main_test'
#binary = 'tests/test-progs/vortex-test/sort/main_test'

# for gem5 V21 and beyond
system.workload = SEWorkload.init_compatible(args.binary)

#env = ["VORTEX_DRIVER=opaesimx"]

process = Process()
process.cmd = [args.binary]
#process.env = env
system.cpu.workload = process
system.cpu.createThreads()

root = Root(full_system=False, system=system)

m5.instantiate()
system.cpu.workload[0].map(
    0x10000000, 0x10000000, 0xFFFFFF
)

system.cpu.workload[0].map(
    0x20000000, 0x20000000, 0xFFFFFF
)

system.cpu.workload[0].map(
    0xa0000000, 0xa0000000, 0xFFFFFF
)

print("Beginning simulation!")
exit_event = m5.simulate()

print('Exiting @ tick {} because {}'
      .format(m5.curTick(), exit_event.getCause()))


