
#
# Tutorial from:
#  https://www.gem5.org/documentation/learning_gem5/part1/cache_config/
#
# The second example using Gem5 to simulate l1 l2 caches
#

##########################
# To run this script
#
# build/X86/gem5.opt configs/tutorial/two_level.py
#
###########################


import m5

from m5.objects import *
from caches import *
from optparse import OptionParser

#################################
# Options from command line
#
################################

parser = OptionParser()
parser.add_option('--l1i_size', help="L1 instruction cache size")
parser.add_option('--l1d_size', help="L1 data cache size")
parser.add_option('--l2_size', help="Unified L2 cache size")

(options, args) = parser.parse_args()


####################################
#  System Hardware Configuration
###################################

#
# Create an object of the entire the system
#
system = System()

# Clock
## Setup a clock and the voltage for the clock
clk_domain = SrcClockDomain()
clk_domain.clock = "1GHz"
clk_domain.voltage_domain = VoltageDomain()

system.clk_domain = clk_domain

# Memory
## Timing mode is mostly used, except in special cases
## like fast forwarding and restoring from a checkpoint
system.mem_mode = 'timing'
## Memory size of 512 MB
system.mem_ranges = [AddrRange('512MB')]

# Processor
## A timing based CPU
system.cpu = TimingSimpleCPU()

# L1 Cache
system.cpu.icache = L1ICache(options)
system.cpu.dcache = L1DCache(options)

# L1 -- CPU
system.cpu.icache.connectCPU(system.cpu)
system.cpu.dcache.connectCPU(system.cpu)

# L1 -- L2 bus
system.l2bus = L2XBar()
system.cpu.icache.connectBus(system.l2bus)
system.cpu.dcache.connectBus(system.l2bus)

# L2 Cache
system.l2cache = L2Cache(options)

# L2 -- L2 bus
system.l2cache.connectCPUSideBus(system.l2bus)

# Memory bus
system.membus = SystemXBar()

# l2 -- memory
system.l2cache.connectMemSideBus(system.membus)

# Bus <---> CPU cache
## Master/Slave port abstraction
#system.cpu.icache_port = system.membus.slave
#system.cpu.dcache_port = system.membus.slave

# IO controller
system.cpu.createInterruptController()

## x86 specific: PIO/INT ports <---> membus
system.cpu.interrupts[0].pio = system.membus.master
system.cpu.interrupts[0].int_master = system.membus.slave
system.cpu.interrupts[0].int_slave = system.membus.master

system.system_port = system.membus.slave


# Memory controller
## DDR3 controller
system.mem_ctrl = DDR3_1600_8x8()
system.mem_ctrl.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.master


#################################
# System Software: SE: Processes
#################################
#
# SE: Syscall Emulation mode:
#   user space code only
#
# FS: Full system mode:
#   needed if you explore OS interactions, suchas page table walks.
#

# Process
process = Process()
process.cmd = ['tests/test-progs/hello/bin/x86/linux/hello']
system.cpu.workload = process
system.cpu.createThreads()

# Instantiate the simulation
## Root object
root = Root(full_system = False, system = system)
## Instantiate the simulation, which will go through all of the SimObjects
## we have created in python and then create the C++ equivalents
m5.instantiate()


# Kick off
print("Simulation begin...")
exit_event = m5.simulate()

# Inspect the result
print('Exiting @ tick{} because {}'
        .format(m5.curTick(), exit_event.getCause()))


