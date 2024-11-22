import shutil

import m5
from m5.objects import *


class BaseSystem:
    def __init__(self):
        system = System()

        system.clk_domain = SrcClockDomain()
        system.clk_domain.clock = "1GHz"
        system.clk_domain.voltage_domain = VoltageDomain()

        system.mem_mode = "timing"
        system.mem_ranges = [AddrRange("512MB")]
        system.cpu = X86TimingSimpleCPU()

        system.membus = SystemXBar()
        # system.l2bus = L2XBar()

        # DRAM controller
        system.mem_ctrl = MemCtrl()
        system.mem_ctrl.dram = DDR3_1600_8x8()
        system.mem_ctrl.dram.range = system.mem_ranges[0]
        system.mem_ctrl.port = system.membus.mem_side_ports

        # Creating and connecting caches is up to implementations

        system.cpu.createInterruptController()
        # Only needed for X86
        system.cpu.interrupts[0].pio = system.membus.mem_side_ports
        system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
        system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports

        self.root = Root(full_system=False, system=system)

        self.system = system

        thispath = os.path.dirname(os.path.realpath(__file__))
        self.path = thispath

    def connect_executable(self, exe):
        binary = os.path.join(
            self.path, "../../../", "tests/research/sieve/", exe
        )
        print(binary)
        self.system.workload = SEWorkload.init_compatible(binary)

        process = Process()
        process.cmd = [binary]
        self.system.cpu.workload = process
        self.system.cpu.createThreads()

    def run(self):
        m5.instantiate()

        print("SIEVE: starting test simulation!")
        exit_event = m5.simulate()
        print(
            f"SIEVE: exiting\n\
\ttime: {m5.curTick()}\n\
\tcause: {exit_event.getCause()}"
        )
