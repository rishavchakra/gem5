from m5.objects import *

"""
Base cache specifications
"""


class L1Cache(Cache):
    # 2-way set associativity
    # assoc = 4
    # Latency for retrieving ptr tag?
    tag_latency = 2
    # Latency for retrieving ptr address data?
    data_latency = 2
    # Latency for retrieving the ??
    response_latency = 2
    # Miss Status Holding Register
    mshrs = 4
    tgts_per_mshr = 20


class L1ICache(L1Cache):
    # Cache size doesn't really matter as much as associativity,
    # as long as cache sizes remain constant between trials
    size = "1KiB"


class L1DCache(L1Cache):
    # Cache size doesn't really matter as much as associativity,
    # as long as cache sizes remain constant between trials
    size = "1KiB"


class L2Cache(Cache):
    # Cache size doesn't really matter as much as associativity,
    # as long as cache sizes remain constant between trials
    size = "4KiB"
    # 8-way set associativity
    # assoc = 8
    tag_latency = 20
    data_latency = 20
    response_latency = 20
    mshrs = 20
    tgts_per_mshr = 12


class L3Cache(Cache):
    # Cache size doesn't really matter as much as associativity,
    # as long as cache sizes remain constant between trials
    size = "16KiB"
    # assoc = 16
    tag_latency = 200
    data_latency = 200
    response_latency = 200
    mshrs = 32
    tgts_per_mshr = 8


"""
Tree-SIEVE caches
"""


class L1I_TreeSIEVE(L1ICache):
    replacement_policy = TreeSIEVERP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L1D_TreeSIEVE(L1DCache):
    replacement_policy = TreeSIEVERP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L2_TreeSIEVE(L2Cache):
    replacement_policy = TreeSIEVERP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L3_TreeSIEVE(L3Cache):
    replacement_policy = TreeSIEVERP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


"""
SIEVE caches
"""


class L1I_SIEVE(L1ICache):
    replacement_policy = SIEVERP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L1D_SIEVE(L1DCache):
    replacement_policy = SIEVERP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L2_SIEVE(L2Cache):
    replacement_policy = SIEVERP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L3_SIEVE(L3Cache):
    replacement_policy = SIEVERP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


"""
Random Replacement caches
"""


class L1I_RR(L1ICache):
    replacement_policy = RandomRP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L1D_RR(L1DCache):
    replacement_policy = RandomRP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L2_RR(L2Cache):
    replacement_policy = RandomRP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L3_RR(L3Cache):
    replacement_policy = RandomRP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


"""
FIFO caches
"""


class L1I_FIFO(L1ICache):
    replacement_policy = FIFORP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L1D_FIFO(L1DCache):
    replacement_policy = FIFORP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L2_FIFO(L2Cache):
    replacement_policy = FIFORP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L3_FIFO(L3Cache):
    replacement_policy = FIFORP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


"""
LRU caches
"""


class L1I_LRU(L1ICache):
    replacement_policy = LRURP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L1D_LRU(L1DCache):
    replacement_policy = LRURP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L2_LRU(L2Cache):
    replacement_policy = LRURP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L3_LRU(L3Cache):
    replacement_policy = LRURP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


"""
Second Chance caches
"""


class L1I_SecondChance(L1ICache):
    replacement_policy = SecondChanceRP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L1D_SecondChance(L1DCache):
    replacement_policy = SecondChanceRP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L2_SecondChance(L2Cache):
    replacement_policy = SecondChanceRP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L3_SecondChance(L3Cache):
    replacement_policy = SecondChanceRP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


"""
Tree PLRU caches
"""


class L1I_TreePLRU(L1ICache):
    replacement_policy = TreePLRURP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L1D_TreePLRU(L1DCache):
    replacement_policy = TreePLRURP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L2_TreePLRU(L2Cache):
    replacement_policy = TreePLRURP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L3_TreePLRU(L3Cache):
    replacement_policy = TreePLRURP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


"""
Weighted LRU caches
"""


class L1I_WeightedLRU(L1ICache):
    replacement_policy = WeightedLRURP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L1D_WeightedLRU(L1DCache):
    replacement_policy = WeightedLRURP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L2_WeightedLRU(L2Cache):
    replacement_policy = WeightedLRURP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc


class L3_WeightedLRU(L3Cache):
    replacement_policy = WeightedLRURP()

    def __init__(self, assoc):
        super().__init__()
        self.assoc = assoc
