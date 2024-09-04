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

s = system.BaseSystem()

# Create caches
match replacement_policy:
    case "sieve":
        s.system.cpu.icache = L1I_SIEVE()
        s.system.cpu.dcache = L1D_SIEVE()
    case "lru":
        s.system.cpu.icache = L1I_LRU()
        s.system.cpu.dcache = L1D_LRU()
    case "fifo":
        s.system.cpu.icache = L1I_FIFO()
        s.system.cpu.dcache = L1D_FIFO()
    case "rr":
        s.system.cpu.icache = L1I_RR()
        s.system.cpu.dcache = L1D_RR()
    case "second-chance":
        s.system.cpu.icache = L1I_SecondChance()
        s.system.cpu.dcache = L1D_SecondChance()
    case "tree-plru":
        s.system.cpu.icache = L1I_TreePLRU()
        s.system.cpu.dcache = L1D_TreePLRU()
    case "weighted-lru":
        s.system.cpu.icache = L1I_WeightedLRU()
        s.system.cpu.dcache = L1D_WeightedLRU()

# Connect caches
s.system.cpu.icache.cpu_side = s.system.cpu.icache_port
s.system.cpu.dcache.cpu_side = s.system.cpu.dcache_port
s.system.cpu.icache.mem_side = s.system.membus.cpu_side_ports
s.system.cpu.dcache.mem_side = s.system.membus.cpu_side_ports

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
