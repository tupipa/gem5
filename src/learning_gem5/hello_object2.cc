
#include "learning_gem5/hello_object2.hh"

#include <iostream>

#include "debug/Hello2.hh"

HelloObject2::HelloObject2(HelloObject2Params *params):
    SimObject(params)
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



