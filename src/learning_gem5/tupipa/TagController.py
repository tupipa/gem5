
from m5.params import *
from m5.SimObject import SimObject

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

