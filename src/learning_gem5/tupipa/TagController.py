
from m5.params import *
from m5.SimObject import SimObject


###########################################################################
#
# Lele Ma, a tag controller sits between L2 and Memory:
#
# 1. forwards all data request from l2 to memory, without processing
#
# 2. each data request additionally generates a tag memory request to
#   tag cache, and a tag cache miss will also send request to mem.
#
# One cpu side port, and two memside port.
#
###########################################################################
#
# Define a TagController
#
#  L1 -- L2 -- TagController -- Memory
#
#  TagController:
#
#  One input from L2; internal two path(tag + data); merged path to memory
#
#  L2 -> request -> |  tag controller                 | --> Memory bus
#                <- |-> data request forword -------->| <--
#                   |-> tag request ->|  tag cache    |
#                                     |-> tag req   ->|
#
#


class TagController(SimObject):
    type = 'TagController'
    cxx_header = "learning_gem5/tupipa/tag_controller.hh"

    # The grouping_factor determines how many tags in the leaf node
    # This also determines how many root nodes:
    #   No. of root nodes =
    #       <memory range> / (grouping_factor * tag_width)
    tag_width = Param.Int(1, "Number of bytes the tag is per 32-bit")
    grouping_factor = Param.Int(1, "Number of words should be grouped")

    tag_cache = Param.Cache("tag cache module in tag controller")

    data_port = SlavePort("CPU side port, receives requests")
    mem_side_data = MasterPort("Memory side port, sends data requests")
    mem_side_tag = MasterPort("Memory side port, sends tag requests")

    # see src/cpu/BaseCPU.py

    _mem_side_ports = ['mem_side_data','mem_side_tag']

    def connectMemSidePorts(self, bus):
        for p in self._mem_side_ports:
            exec('self.%s = bus.slave' %p)

