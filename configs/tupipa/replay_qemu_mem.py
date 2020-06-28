# Copyright (c) 2015-2016 ARM Limited
# All rights reserved.
#
# Lele Ma, 2020-06-19
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from __future__ import print_function
from __future__ import absolute_import

import gzip
import six
import optparse
import os

import m5
from m5.objects import *
from m5.util import addToPath
from m5.stats import periodicStatDump

addToPath('../')
from common import ObjectList
from common import MemConfig

addToPath('../../util')
import protolib

if six.PY3:
    long = int

# Lele: this is written based on configs/dram/lat_mem_rd.py
#
# this script is helpful to observe the memory latency for various
# levels in a cache hierarchy, and various cache and memory
# configurations, in essence replicating the lmbench lat_mem_rd thrash
# behaviour

# import the packet proto definitions, and if they are not found,
# attempt to generate them automatically
try:
    import packet_pb2
except:
    print("Did not find packet proto definitions, attempting to generate")
    from subprocess import call
    error = call(['protoc', '--python_out=configs/tupipa',
                  '--proto_path=src/proto', 'src/proto/packet.proto'])
    if not error:
        print("Generated packet proto definitions")

        try:
            import google.protobuf
        except:
            print("Please install the Python protobuf module")
            exit(-1)

        import packet_pb2
    else:
        print("Failed to import packet proto definitions")
        exit(-1)

parser = optparse.OptionParser()

parser.add_option("--mem-type", type="choice", default="DDR3_1600_8x8",
                  choices=ObjectList.mem_list.get_names(),
                  help = "type of memory to use")
parser.add_option("--mem-size", action="store", type="string",
                  default="16MB",
                  help="Specify the memory size")
parser.add_option("--reuse-trace", action="store_true",
                  help="Prevent generation of traces and reuse existing")

parser.add_option("--enable-shadow-tags", action="store_true",
                  help="Enable tag cache for shadow memory at L3 layer.")

parser.add_option("--read-reqs-per-addr", action="store", type="int",
                  default="8",
                  help="Specify the number of read requests per address")

parser.add_option("--write-reqs-per-addr", action="store", type="int",
                  default="8",
                  help="Specify the number of write requests per address")

parser.add_option("--tagcache-inclusive", action="store_true",
                  help="Prevent generation of traces and reuse existing")



(options, args) = parser.parse_args()

if args:
    print("Error: script doesn't take any positional arguments")
    sys.exit(1)

# start by creating the system itself, using a multi-layer 2.0 GHz
# crossbar, delivering 64 bytes / 3 cycles (one header cycle) which
# amounts to 42.7 GByte/s per layer and thus per port
system = System(membus = SystemXBar(width = 32))
system.clk_domain = SrcClockDomain(clock = '2.0GHz',
                                   voltage_domain =
                                   VoltageDomain(voltage = '1V'))

mem_range = AddrRange(options.mem_size)
print("Mem Range: ", int(mem_range.end))
system.mem_ranges = [mem_range]

# do not worry about reserving space for the backing store
system.mmap_using_noreserve = True

# currently not exposed as command-line options, set here for now
options.mem_channels = 1
options.mem_ranks = 1
options.external_memory_system = 0
options.tlm_memory = 0
options.elastic_trace_en = 0

MemConfig.config_mem(options, system)

# there is no point slowing things down by saving any data
for ctrl in system.mem_ctrls:
    ctrl.null = True

    # the following assumes that we are using the native DRAM
    # controller, check to be sure
    if isinstance(ctrl, m5.objects.DRAMCtrl):
        # make the DRAM refresh interval sufficiently infinite to avoid
        # latency spikes
        ctrl.tREFI = '100s'

# use the same concept as the utilisation sweep, and print the config
# so that we can later read it in
cfg_file_name = os.path.join(m5.options.outdir, "lat_mem_rd.cfg")
cfg_file = open(cfg_file_name, 'w')

# set an appropriate burst length in bytes
burst_size = 64
system.cache_line_size = burst_size

# lazy version to check if an integer is a power of two
def is_pow2(num):
    return num != 0 and ((num & (num - 1)) == 0)

# assume we start every range at 0
max_range = int(mem_range.end)

# start at a size of 4 kByte, and go up till we hit the max, increase
# the step every time we hit a power of two
# min_range = 4096
# Lele: keep only one range
ranges = [max_range]
#step = 1024

#while ranges[-1] < max_range:
#    new_range = ranges[-1] + step
#    if is_pow2(new_range):
#        step *= 2
#    ranges.append(new_range)

# how many times to repeat the measurement for each data point
iterations = 2

# 150 ns in ticks, this is choosen to be high enough that transactions
# do not pile up in the system, adjust if needed
itt = 150 * 1000

# TODO: read trace from qemu-generated tracing data and write to
# traffic_gen compatitble format
#
# for every data point, we create a trace containing a random address
# sequence, so that we can play back the same sequence for warming and
# the actual measurement
def create_trace(filename, max_addr, burst_size, itt):
    try:
        print("Trying to open file ", filename)
        proto_out = gzip.open(filename, 'wb')
    except IOError:
        print("Failed to open ", filename, " for writing")
        exit(-1)

    # write the magic number in 4-byte Little Endian, similar to what
    # is done in src/proto/protoio.cc
    proto_out.write("gem5")

    # add the packet header
    header = packet_pb2.PacketHeader()
    header.obj_id = "lat_mem_rd for range 0:" + str(max_addr)
    # assume the default tick rate (1 ps)
    header.tick_freq = 1000000000000
    protolib.encodeMessage(proto_out, header)

    # create a list of every single address to touch
    addrs_all = list(range(max_addr/2, max_addr, burst_size))

    addrs = addrs_all[0:1]
    total_addrs = len(addrs)

    import random
    random.shuffle(addrs)

    tick = 0

    # create a packet we can re-use for all the addresses
    packet = packet_pb2.Packet()
    # ReadReq is 1 in src/mem/packet.hh Command enum
    packet.cmd = 1
    packet.size = int(burst_size)

    read_reqs_per_addr = options.read_reqs_per_addr
    write_reqs_per_addr = options.write_reqs_per_addr

    write_left = write_reqs_per_addr

    total_reqs = 0

    for addr in addrs:

      # generate a read req, and a following write if has write in options
      for rqst in range(read_reqs_per_addr):
         # ReadReq is 1 in src/mem/packet.hh Command enum
         packet.cmd = 1
         packet.tick = long(tick)
         packet.addr = long(addr)
         protolib.encodeMessage(proto_out, packet)
         tick = tick + itt
         total_reqs = total_reqs + 1

         # generate a write
         if (write_left > 0):
            # WriteReq is 4 in src/mem/packet.hh Command enum
            packet.cmd = 4
            packet.tick = long(tick)
            packet.addr = long(addr)
            protolib.encodeMessage(proto_out, packet)
            tick = tick + itt
            write_left = write_left - 1
            total_reqs = total_reqs + 1
      for rqst in range(write_left):
         # WriteReq is 4 in src/mem/packet.hh Command enum
         packet.cmd = 4
         packet.tick = long(tick)
         packet.addr = long(addr)
         protolib.encodeMessage(proto_out, packet)
         tick = tick + itt
         total_reqs = total_reqs + 1

    print("Total number of addr in traces: ", str(total_addrs))
    print("Total number of requests in traces: ", str(total_reqs))
    proto_out.close()

# this will take a while, so keep the user informed
print("Generating traces, please wait...")

nxt_range = 0
nxt_state = 0
period = long(itt * (max_range / burst_size))

# TODO now we create the states for the input file only
for r in ranges:
    filename = os.path.join(m5.options.outdir,
                            'lat_mem_rd%d.trc.gz' % nxt_range)

    if not options.reuse_trace:
        # create the actual random trace for this range
        create_trace(filename, r, burst_size, itt)

    # the warming state
    cfg_file.write("STATE %d %d TRACE %s 0\n" %
                   (nxt_state, period, filename))
    nxt_state = nxt_state + 1

    # the measuring states
    for i in range(iterations):
        cfg_file.write("STATE %d %d TRACE %s 0\n" %
                       (nxt_state, period, filename))
        nxt_state = nxt_state + 1

    nxt_range = nxt_range + 1

cfg_file.write("INIT 0\n")

# go through the states one by one
for state in range(1, nxt_state):
    cfg_file.write("TRANSITION %d %d 1\n" % (state - 1, state))

cfg_file.write("TRANSITION %d %d 1\n" % (nxt_state - 1, nxt_state - 1))

cfg_file.close()

# create a traffic generator, and point it to the file we just created
system.tgen = TrafficGen(config_file = cfg_file_name,
                         progress_check = '10s')

# add a communication monitor
system.monitor = CommMonitor()
system.monitor.footprint = MemFootprintProbe()

# connect the traffic generator to the system
system.tgen.port = system.monitor.slave

# create the actual cache hierarchy, for now just go with something
# basic to explore some of the options
from common.Caches import *

##########################################################################
## Define a TagController
#
#  L1 -- L2 -- TagController -- Memory
#
#  TagController:
#
#  One input from L2; internal two path(tag + data); merged path to memory
#
# L2 -> request -> |  tag controller                 | --> Memory
#               <- |-> data request forword -------->| <--
#                  |-> tag request ->|  tag cache    |
#                                    |-> tag req   ->|
#
##########################################################################
class TagCache(Cache):
    assoc = 16
    tag_latency = 20
    data_latency = 20
    sequential_access = True
    response_latency = 40
    mshrs = 32
    tgts_per_mshr = 12
    write_buffers = 16

# a starting point for an L3 cache
class L3Cache(Cache):
    assoc = 16
    tag_latency = 20
    data_latency = 20
    sequential_access = True
    response_latency = 40
    mshrs = 32
    tgts_per_mshr = 12
    write_buffers = 16

# note that everything is in the same clock domain, 2.0 GHz as
# specified above
system.l1cache = L1_DCache(size = '64kB')

# Monitor (master) --> l1cache (slave)
system.monitor.master = system.l1cache.cpu_side

system.l2cache = L2Cache(size = '512kB', writeback_clean = True)

# Connect L1 -- xbar -- L2
# L1 cache.mem_side (master) --> (slave) l2cache.xbar (master) -- L2.cpu_side
system.l2cache.xbar = L2XBar()
system.l1cache.mem_side = system.l2cache.xbar.slave
system.l2cache.cpu_side = system.l2cache.xbar.master

# Create Last Level Cache
# Lele: for last Level Cache: either use a regular L3 or a tagController
if (options.enable_shadow_tags):
    print ("Using Tag Controller as Last Level Cache...")
    ###############################################
    # attach TagController object to the system
    ###############################################
    tag_controller = TagController()
    system.tag_controller = tag_controller

    # Create Tag Cache that lives in the TagController
    # Follow a regular L3 Cache Size
    if (options.tagcache_inclusive):
        # inclusive tag cache
        tag_cache = TagCache(size = '4MB', writeback_clean = True)
    else:
        # exclusive tag cache
        tag_cache = TagCache(size = '4MB', clusivity = 'mostly_excl')

    tag_controller.tag_cache = tag_cache

    ########################
    #  L2 -- TagController
    ########################
    # L2 -- tagcon.xbar -- tagcon.data_port TagController
    # bus between L2 and tag_controller
    # tag_controller.xbar = L2XBar()
    # system.l2cache.mem_side = tag_controller.xbar.slave
    # tag_controller.data_port = tag_controller.xbar.master

    # Simpler way: no xbar
    # L2 -- tagcon.data_port TagController
    system.l2cache.mem_side = tag_controller.data_port

    #####################################
    # TagController ---...--- Memory Bus
    #####################################

    # TagController --(data)-- Memory bus
    # connection between tag_controller and memory bus
    # see learning_gem5/part1/caches.py:connectBus()
    tag_controller.mem_side_data = system.membus.slave

    # TagController --(tag)--tag cache -- Memory Bus
    ## Inside TagController:
    # tagcon.mem_side_tag -- tag_cache.cpu_side
    #                         tag_cache.mem_side -- mem_side_tag
    #
    # tag_cache.xbar.mem_ranges=[max_range/2]
    # tag_cache.addr_ranges = [max_range/2]
    tag_controller.mem_side_tag = tag_cache.cpu_side
    tag_cache.mem_side = system.membus.slave

else:
    print ("Connect L2 Cache directly to Main Memory")
    system.l2cache.mem_side = system.membus.slave

# connect the system port even if it is not used in this example
system.system_port = system.membus.slave

# every period, dump and reset all stats
periodicStatDump(period)

# run Forrest, run!
root = Root(full_system = False, system = system)
root.system.mem_mode = 'timing'

m5.instantiate()
m5.simulate(nxt_state * period)

# print all we need to make sense of the stats output
print("replay-qemu-trace with %d iterations, ranges:" % iterations)
for r in ranges:
    print(r)
