libsled
=======

This library can be used to control the linear sled located in the [Sensorimotor Lab](http://www.sensorimotorlab.com/) of [Radboud University](http://www.ru.nl/english/). It is driven by a Kollmorgen S700 which is connected to our control computer via a PEAK CAN interface.

Initializing the library
------------------------

To initialize the sled library, use the sled\_create function. The library currently depends on libevent2 to provide platform independent event handling, therefore an initialized event\_base should be passed to the sled library.

    sled_t *sled = sled_create(event_base);

After using the library, use the sled\_destroy function to free memory. Note that we do not currently disable the sled motor.

    sled_destroy(sled);

Real-time control
-----------------

_Warning: The real-time control API is not stable yet, and therefore subject to change._

It is possible to retreive the last position from the sled using the sled\_rt\_get\_position function.

    double position;
    sled_rt_get_position(sled, &position);

It is possible for the function to return the same sample more than once, for example when called at too high a frequency. 

To provide the sled with a new (position) set-point, the sled\_rt\_set\_position should be invoked.

    double position = 0;
    sled_rt_set_position(sled, position);

Modules
-------

* The "sled" files implement the external API
* The "interface" files implement the low-level CAN/CANOpen interface.

What is currently missing:
* State machines (device, sdo, nmt, ds402)

Interface functions will only be invoked through the state machine 
(with the exception of intf.create and intf.destroy). Interface
callbacks will only notify the state machines.


Copyright and license
---------------------

Copyright (c) 2013 Ivar Clemens

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

