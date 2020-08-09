
#
# A simple memory object with Packets
# Sample code from:
#   https://www.gem5.org/documentation/learning_gem5/part2/memoryobject/
#

from m5.params import *
from m5.proxy import *
from MemObject import MemObject


#
# Inherit from MemObject, to interact with the memory system
#   MemObject class has two pure virtual functions:
#   - getMasterPort
#   - getSlavePort
#
class SimpleMemobj2(MemObject):

    # `type` is the C++ class that you are wrapping with this Python SimObject
    # Convention to keep this C++ class name the same as python class name
    # This class need to be manually implemented in C++
    type = 'SimpleMemobj2'

    # `cxx_header` is the file that contains the decl. of the class used as
    # the `type` parameter.
    # Again, convention suggests to use same file name as the class name but
    # with all lower cases.
    # This header file needs to be manually implemented in C++
    cxx_header = "learning_gem5/tupipa/simple_memobj2.hh"


    ##############################################
    # Three parameters (send from Python to C++)
    # Two port connecting CPU i/d cache
    # One port connecting to Memory bus
    ##############################################

    # SlavePort class
    # Note the name of the port will be used for the impl. in C++
    inst_port = SlavePort("CPU side port, receives requests")
    data_port = SlavePort("CPU side port, receives requests")

    # MasterPort class
    # Note the name of the port will be used for the impl. in C++
    mem_side = MasterPort("Memory side port, sends requests")






