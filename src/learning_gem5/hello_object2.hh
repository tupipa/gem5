
#ifndef __LEARNING_GEM5_HELLO_OBJECT_HH__
#define __LEARNING_GEM5__HELLO_OBJECT_HH__

//
//

#include "params/HelloObject2.hh"
#include "sim/sim_object.hh"

class HelloObject2: public SimObject
{

  private:
    // A function to be executed upon the event fires
    void processEvent();
    // An Event instance.
    // `EventFunctionWrapper` allows to execute any function
    EventFunctionWrapper event;

    // The latency for the next event to be fired
    Tick latency;

    // The number of times to fire the event
    int timesLeft;

  public:
    // The constructor of all SimObject assumes it will take a parameter
    // object. This parameter object is automatically created by the build
    // system. The parameter object is based on the python class for the
    // SimObject. The name is autogenerated based on python class name, i.e.
    // HelloObject*Params*
    HelloObject2(HelloObject2Params *p);

    // startup() function is where SimObject are allowed to schedule internal
    // events. It will execute after simulation begins for the first time,
    // i.e. the `simulate()` function is called from a Python config file.
    void startup();


};




#endif // __LEARNING_GEM5_HELLO_OBJECT_HH__
