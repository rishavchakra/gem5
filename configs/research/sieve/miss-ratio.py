import argparse

from caches import *

import system

from m5.objects import *

access_patterns = [
    "seq",
    "seq-control",
    "zipf",
    "zipf-control",
]

replacement_policies = [
    "sieve",
    "tree-sieve",
    "lru",
    "fifo",
    "rr",
    "second-chance",
    "tree-plru",
    "weighted-lru",
]

parser = argparse.ArgumentParser(
    prog="MissRatioTesting",
    description="This tests the miss ratios of various replacement policies.\n\
        This is only to be used with gem5",
)
_ = parser.add_argument("-m", "--mempattern")
_ = parser.add_argument("-r", "--replpolicy")
_ = parser.add_argument("-a", "--associativity")
args = parser.parse_args()

if args.mempattern not in access_patterns:
    print("ERROR: memory access pattern argument invalid")
    exit(1)

if args.replpolicy not in replacement_policies:
    print("ERROR: replacement policy argument invalid")
    exit(1)

# Have to do this because running multiple simulations
# sequentially in gem5 is weird
access_pattern = args.mempattern
replacement_policy = args.replpolicy
assoc = args.associativity


s = system.BaseSystem()

# Create caches
match replacement_policy:
    case "tree-sieve":
        s.system.cpu.icache = L1I_TreeSIEVE(assoc)
        s.system.cpu.dcache = L1D_TreeSIEVE(assoc)
        # s.system.l2cache = L2_TreeSIEVE(assoc)
    case "sieve":
        s.system.cpu.icache = L1I_SIEVE(assoc)
        s.system.cpu.dcache = L1D_SIEVE(assoc)
        # s.system.l2cache = L2_SIEVE(assoc)
    case "lru":
        s.system.cpu.icache = L1I_LRU(assoc)
        s.system.cpu.dcache = L1D_LRU(assoc)
        # s.system.l2cache = L2_LRU(assoc)
    case "fifo":
        s.system.cpu.icache = L1I_FIFO(assoc)
        s.system.cpu.dcache = L1D_FIFO(assoc)
        # s.system.l2cache = L2_FIFO(assoc)
    case "rr":
        s.system.cpu.icache = L1I_RR(assoc)
        s.system.cpu.dcache = L1D_RR(assoc)
        # s.system.l2cache = L2_RR(assoc)
    case "second-chance":
        s.system.cpu.icache = L1I_SecondChance(assoc)
        s.system.cpu.dcache = L1D_SecondChance(assoc)
        # s.system.l2cache = L2_SecondChance(assoc)
    case "tree-plru":
        s.system.cpu.icache = L1I_TreePLRU(assoc)
        s.system.cpu.dcache = L1D_TreePLRU(assoc)
        # s.system.l2cache = L2_TreePLRU(assoc)
    case "weighted-lru":
        s.system.cpu.icache = L1I_WeightedLRU(assoc)
        s.system.cpu.dcache = L1D_WeightedLRU(assoc)
        # s.system.l2cache = L2_WeightedLRU(assoc)

# Connect caches
s.system.cpu.icache.cpu_side = s.system.cpu.icache_port
s.system.cpu.dcache.cpu_side = s.system.cpu.dcache_port
s.system.cpu.icache.mem_side = s.system.membus.cpu_side_ports
s.system.cpu.dcache.mem_side = s.system.membus.cpu_side_ports
# s.system.cpu.icache.mem_side = s.system.l2bus.cpu_side_ports
# s.system.cpu.dcache.mem_side = s.system.l2bus.cpu_side_ports
# s.system.l2cache.cpu_side = s.system.l2bus.mem_side_ports
# s.system.l2cache.mem_side = s.system.membus.cpu_side_ports

match access_pattern:
    case "seq":
        s.connect_executable("sequential-access")
    case "seq-control":
        s.connect_executable("sequential-access-control")
    case "zipf":
        s.connect_executable("zipf-access")
    case "zipf-control":
        s.connect_executable("zipf-access-control")

s.run()
