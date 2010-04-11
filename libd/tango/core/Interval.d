/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: March 2004      

        author:         Kris

*******************************************************************************/

module tango.core.Interval;

/*******************************************************************************

        Time interval multipliers ~ all Tango intervals are based upon 
        microseconds. Note that recent D compilers support arithmetic
        applied upon Interval members to be passed as a fully-typed
        Interval argument; e.g.

        ---
        void sleep (Interval period);

        sleep (Inteval.second * 5);
        ---

*******************************************************************************/

enum Interval : ulong 
                {
                micro    = 1, 
                milli    = 1000, 
                second   = 1_000_000,
                minute   = 60_000_000,

                infinity = ulong.max,

                Microsec = micro, 
                Millisec = milli, 
                Second   = second, 
                Minute   = minute,
                };

