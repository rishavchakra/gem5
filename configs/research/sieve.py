import m5
from m5.objects import *


class L1Cache(Cache):
    def connectCPU(self, cpu):
        # This is basically an abstract class,
        # This should never be constructed/called
        raise NotImplementedError


class L1ICache(L1Cache):
    # TODO: Get cache information from some CPU
    # Which one?
    size = "16kB"

    def connectCPU(self, cpu):
        self.cpu_side = cpu.icache_port


class L1DCache(L1Cache):
    size = "64kB"

    def connectCPU(self, cpu):
        self.cpu_side = cpu.icache_port


class L2Cache(Cache):
    def connectCPU(self, bus):
        self.cpu_side = bus.mem_side_ports

    def connectMemSideBus(self, bus):
        self.mem_side = bus.cpu_side_ports


class L3Cache(Cache):
    def connectCPU(self, bus):
        self.cpu_side = bus.mem_side_ports

    def connectMemSideBus(self, bus):
        self.mem_side = bus.cpu_side_ports


system = System()

# Clock
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

# Memory
system.mem_mode = "timing"
# 512MB of memory should be plenty
system.mem_ranges = [AddrRange("512MB")]

# ARM CPU, not x86 or RISC-V
system.cpu = ArmTimingSimpleCPU()

system.cpu.createInterruptController()
# Connections don't need to be specified for ARM

system.system_port = system.membus.cpu_side_ports

# DRAM
system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports
system.membus = SystemXBar()

# system.cpu.icache_port = system.membus.cpu_side_ports
# system.cpu.dcache_port = system.membus.cpu_side_ports
