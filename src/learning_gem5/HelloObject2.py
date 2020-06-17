
#
# tutorial code from:
#  https://www.gem5.org/documentation/learning_gem5/part2/helloobject/
#

from m5.params import *
from m5.SimObject import SimObject

class HelloObject2(SimObject):

    # `type` is the C++ class that you are wrapping with this Python SimObject
    # Convention to keep this C++ class name the same as python class name
    # This class need to be manually implemented in C++
    type = 'HelloObject2'

    # `cxx_header` is the file that contains the decl. of the class used as
    # the `type` parameter.
    # Again, convention suggests to use same file name as the class name but
    # with all lower cases.
    # This header file needs to be manually implemented in C++
    cxx_header = "learning_gem5/hello_object2.hh"




