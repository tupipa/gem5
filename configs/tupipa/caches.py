
# Tutorial code from:
#  https://www.gem5.org/documentation/learning_gem5/part1/cache_config/
#
# A system with L1 data and code cache, L2 cache, and main memory
#
# Single core with classic cache
#
# The Gem5 Cache SimObject is located at
#
#    src/mem/cache/Cache.py
#
# BaseCache Parameters
#  - assoc
#  - write_buffers
#  - Param.* default values and the description of parameters;
#    if no default value, must be set before simulation
#

import m5
from m5.objects import Cache


# L1 Cache
class L1Cache(Cache):

    # See src/mem/cache/Cache.py: BaseCache class
    # for the meaning for these parameters

    # Associativity
    assoc = 2
    # Data access latency
    tag_latency = 2

    data_latency = 2

    # Latency for the return path on a miss
    response_latency = 2

    # Number of MSHRs (max outstanding requests)
    # MSHR: Miss status holding register
    mshrs = 4

    # Max number of accesses per MSHR
    tgts_per_mshr = 20

    def __init__(self, options=None):
        super(L1Cache, self).__init__()
        pass

    def connectCPU(self, cpu):
        # need to define this in a base class
        raise NotImplementedError
    def connectBus(self, bus):
        self.mem_side = bus.slave

class L1ICache(L1Cache):

    size = '16kB'

    def __init__(self, options=None):
        super(L1ICache, self).__init__(options)
        if not options or not options.l1i_size:
            return
        self.size = options.l1i_size

    def connectCPU(self, cpu):
        self.cpu_side = cpu.icache_port

class L1DCache(L1Cache):
    size = '64kB'

    def __init__(self, options=None):
        super(L1DCache, self).__init__(options)
        if not options or not options.l1d_size:
            return
        self.size = options.l1d_size

    def connectCPU(self, cpu):
        self.cpu_side = cpu.dcache_port

class L2Cache(Cache):
    size = '256kB'
    assoc = 8
    tag_latency = 20
    data_latency = 20
    response_latency = 20
    mshrs = 20
    tgts_per_mshr = 12

    def __init__(self, options=None):
        super(L2Cache, self).__init__()
        if not options or not options.l2_size:
            return
        self.size = options.l2_size


    # CPU side bus, the l1-l2 connection
    def connectCPUSideBus(self, bus):
        self.cpu_side = bus.master

    # Mem side bus, the l2-mem connection
    def connectMemSideBus(self, bus):
        self.mem_side = bus.slave




