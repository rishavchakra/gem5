from typing import Optional
from ....utils.overrides import *

from ..abstract_cache_hierarchy import AbstractCacheHierarchy
from .abstract_classic_cache_hierarchy import AbstractClassicCacheHierarchy

from m5.objects import (
    BadAddr,
    BaseXBar,
    Cache,
    Port,
    SystemXBar,
    TreePLRURP,
    SIEVERP,
    RandomRP,
    FIFORP,
    LRURP,
    SecondChanceRP,
    TwoTreeRP,
    ThreeTreeRP,
)


def get_repl(repl_str: str):
    ret = TreePLRURP()
    match repl_str:
        case "sieve":
            ret = SIEVERP()
        case "rr":
            ret = RandomRP()
        case "fifo":
            ret = FIFORP()
        case "lru":
            ret = LRURP()
        case "second-chance":
            ret = SecondChanceRP()
        case "2tree":
            ret = TwoTreeRP()
        case "3tree":
            ret = ThreeTreeRP()
    return ret


class ResearchCacheHierarchy(AbstractClassicCacheHierarchy):
    def __init__(
        self,
        l1d_size: str,
        l1i_size: str,
        assoc: int = 8,
        repl=TreePLRURP(),
        membus: Optional[BaseXBar] = None,
        save_trace: bool = False,
    ) -> None:
        AbstractClassicCacheHierarchy.__init__(self=self)
        self.membus = membus
        if not membus:
            # Default membus
            self.membus = SystemXBar(width=64)
            self.membus.badaddr_responder = BadAddr()
            self.membus.default = membus.badaddr_responder.pio
        self._l1d_size = l1d_size
        self._l1i_size = l1i_size
        self._assoc = assoc
        self._repl = repl
        self.save_trace = save_trace

    @overrides(AbstractClassicCacheHierarchy)
    def get_mem_side_port(self) -> Port:
        return self.membus.mem_side_ports

    @overrides(AbstractClassicCacheHierarchy)
    def get_cpu_side_port(self) -> Port:
        return self.membus.cpu_side_ports

    @overrides(AbstractCacheHierarchy)
    def incorporate_cache(self, board: AbstractBoard) -> None:
        # Set up the system port for functional access from the simulator.
        board.connect_system_port(self.membus.cpu_side_ports)

        for _, port in board.get_mem_ports():
            self.membus.mem_side_ports = port

        self.l1icaches = [
            L1ICache(
                size=self._l1i_size,
                assoc=self._assoc,
                replacement_policy=get_repl(self._repl),
            )
            for i in range(board.get_processor().get_num_cores())
        ]

        self.l1dcaches = [
            # L1DCache(size=self._l1d_size)
            L1DCache(
                size=self._l1d_size,
                assoc=self._assoc,
                replacement_policy=get_repl(self._repl),
            )
            for i in range(board.get_processor().get_num_cores())
        ]
        # ITLB Page walk caches
        self.iptw_caches = [
            MMUCache(size="8KiB") for _ in range(board.get_processor().get_num_cores())
        ]
        # DTLB Page walk caches
        self.dptw_caches = [
            MMUCache(size="8KiB") for _ in range(board.get_processor().get_num_cores())
        ]

        if board.has_coherent_io():
            self._setup_io_cache(board)

        for i, cpu in enumerate(board.get_processor().get_cores()):
            cpu.connect_icache(self.l1icaches[i].cpu_side)
            cpu.connect_dcache(self.l1dcaches[i].cpu_side)

            self.l1icaches[i].mem_side = self.membus.cpu_side_ports
            self.l1dcaches[i].mem_side = self.membus.cpu_side_ports

            self.iptw_caches[i].mem_side = self.membus.cpu_side_ports
            self.dptw_caches[i].mem_side = self.membus.cpu_side_ports

            cpu.connect_walker_ports(
                self.iptw_caches[i].cpu_side, self.dptw_caches[i].cpu_side
            )

            if board.get_processor().get_isa() == ISA.X86:
                int_req_port = self.membus.mem_side_ports
                int_resp_port = self.membus.cpu_side_ports
                cpu.connect_interrupt(int_req_port, int_resp_port)
            else:
                cpu.connect_interrupt()

    def _setup_io_cache(self, board: AbstractBoard) -> None:
        """Create a cache for coherent I/O connections"""
        self.iocache = Cache(
            assoc=8,
            tag_latency=50,
            data_latency=50,
            response_latency=50,
            mshrs=20,
            size="1KiB",
            tgts_per_mshr=12,
            addr_ranges=board.mem_ranges,
        )
        self.iocache.mem_side = self.membus.cpu_side_ports
        self.iocache.cpu_side = board.get_mem_side_coherent_io_port()
