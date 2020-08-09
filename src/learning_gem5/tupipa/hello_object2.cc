
#include "learning_gem5/tupipa/hello_object2.hh"

#include <iostream>

#include "debug/Hello2.hh"

HelloObject2::HelloObject2(HelloObject2Params *params):
    SimObject(params),
    // create event instance of class EventFunctionWrapper
    // two arguments: a function to execute and a name
    // The function is simply defined to trigger the callback func
    // `processEvent()`; here defined as a simple lambda function, capturing
    // this so that we can call member functions of this class.
    // The name is usually the name of SimObject that owns this event
    event([this]{processEvent();},name()),
    // default latency value, the tick when the event is going to be triggered
    latency(params->time_to_wait),
    // default number of times for the event being triggered
    timesLeft(params->number_of_fires)
{
    // Normally, you should use debug flags in gem5, instead of `std:count`
    std::cout << "Hello World! From a SimObject" << std::endl;
    DPRINTF(Hello2, "Creating the hello object\n");
}


/**
 * `create` function is implicitly created from the SimObject python decl.
 *
 * We must implement this function in the parameter type.
 *
 * This returns a new instantiation of the SimObject.
 *
 */

HelloObject2*
HelloObject2Params::create(){

  return new HelloObject2(this);

}


/**
 *
 * processEvent()
 *
 * The callback function for the event
 *
 */
void HelloObject2::processEvent(){

    timesLeft--;

    DPRINTF(Hello2, "Processing the Event, times left: %d\n", timesLeft);

    if (timesLeft <= 0){
       DPRINTF(Hello2, "Done firing all events\n");
    }else{
       schedule(event, curTick() + latency);
    }

}


/**
 *
 * startup()
 *
 * An event can be generated and scheduled here
 */

void HelloObject2::startup(){
    // Event generated here? How long does the event take?
    // see latency to HelloObject2
    // Event scheduled to run at tick 100
    //schedule(event, 100);
    schedule(event, latency);

}




