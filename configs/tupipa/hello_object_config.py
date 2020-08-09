
#
# Configuration script for HelloObject2.
#
# Tutorial code at
#  https://www.gem5.org/documentation/learning_gem5/part2/helloobject/
#

import m5

from m5.objects import *


print("Creating root object")
# A root object is required by all gem5 instances
root = Root(full_system = False)


print("Initiate the HelloObject2")
# This will call the python constructor
# This instantiation needs to be the child of the root
root.hello2 = HelloObject2()
root.hello2.time_to_wait = '1ns'
root.hello2.number_of_fires = 23

print("Instantiate the m5 module")
m5.instantiate()

# Run
print("Beginning simulation")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'
        .format(m5.curTick(), exit_event.getCause()))




