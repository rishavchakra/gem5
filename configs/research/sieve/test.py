from caches import *

import m5
from m5.objects import *

system = System()

"""
CPU setup
"""

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = "timing"
system.mem_ranges = [AddrRange("512MB")]
system.cpu = ArmTimingSimpleCPU()

"""
Cache and memory setup
"""

# Busses
system.membus = SystemXBar()
system.l2bus = L2XBar()

# Memory levels (caches, DRAM)
system.cpu.icache = L1I_SIEVE()
system.cpu.dcache = L1D_SIEVE()
system.l2cache = L2_SIEVE()

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

# Connecting caches via busses
system.cpu.icache.cpu_side = system.cpu.icache_port
system.cpu.dcache.cpu_side = system.cpu.dcache_port

system.cpu.icache.mem_side = system.l2bus.cpu_side_ports
system.cpu.dcache.mem_side = system.l2bus.cpu_side_ports
# system.cpu.icache.mem_side = system.membus.cpu_side_ports
# system.cpu.dcache.mem_side = system.membus.cpu_side_ports

system.l2cache.cpu_side = system.l2bus.mem_side_ports
system.l2cache.mem_side = system.membus.cpu_side_ports

system.system_port = system.membus.cpu_side_ports

system.cpu.createInterruptController()
# Only needed for x86
# system.cpu.interrupts[0].pio = system.membus.mem_side_ports
# system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
# system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports
"""
Runtime setup
"""
thispath = os.path.dirname(os.path.realpath(__file__))
binary = os.path.join(
    thispath,
    "../../../",
    "tests/test-progs/hello/bin/arm/linux/hello",
)
system.workload = SEWorkload.init_compatible(binary)

process = Process()
process.cmd = [binary]
system.cpu.workload = process
system.cpu.createThreads()

root = Root(full_system=False, system=system)

m5.instantiate()

print("SIEVE: starting test simulation!")
exit_event = m5.simulate()
print(f"SIEVE: exiting\ntime: {m5.curTick()}\ncause: {exit_event.getCause()}")
