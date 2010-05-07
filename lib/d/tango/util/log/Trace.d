/*******************************************************************************

        copyright:      Copyright (c) 2007 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Oct 2007: Initial release

        author:         Kris

        Synchronized, formatted console output. Usage is:
        ---
        Trace.formatln ("hello {}", "world");
        ---

        Note that this has become merely a wrapper around Log.formatln(), so
        please use that API instead

*******************************************************************************/

module tango.util.log.Trace;

public import tango.util.log.Config;

/*******************************************************************************

        redirect to the Log module

*******************************************************************************/

public alias Log Trace;

/*******************************************************************************

*******************************************************************************/

debug (Trace)
{
        void main()
        {
                Trace.formatln ("hello {}", "world");
                Trace ("hello {}", "world");
        }
}













/+
        /**********************************************************************

                Print a range of raw memory as a hex dump.
                Characters in range 0x20..0x7E are printed, all others are
                shown as dots.

                ----
000000:  47 49 46 38  39 61 10 00  10 00 80 00  00 48 5D 8C  GIF89a.......H].
000010:  FF FF FF 21  F9 04 01 00  00 01 00 2C  00 00 00 00  ...!.......,....
000020:  10 00 10 00  00 02 11 8C  8F A9 CB ED  0F A3 84 C0  ................
000030:  D4 70 A7 DE  BC FB 8F 14  00 3B                     .p.......;
                ----

        **********************************************************************/

        final void memory (void[] mem)
        {
            auto data = cast(ubyte[]) mem;
            synchronized (mutex)
            {
                for( int row = 0; row < data.length; row += 16 )
                {
                    // print relative offset
                    convert.convert (&sink, "{:X6}:  ", row );

                    // print data bytes
                    for( int idx = 0; idx < 16 ; idx++ )
                    {
                        // print byte or stuffing spaces
                        if ( idx + row < data.length )
                            convert (&sink, "{:X2} ", data[ row + idx ] );
                        else
                            output.write ("   ");

                        // after each 4 bytes group an extra space
                        if (( idx & 0x03 ) == 3 )
                            output.write (" ");
                    }

                    // ascii view
                    // all char 0x20..0x7e are OK for printing,
                    // other values are printed as a dot
                    ubyte[16] ascii = void;
                    int asciiIdx;
                    for ( asciiIdx = 0;
                        (asciiIdx<16) && (asciiIdx+row < data.length);
                        asciiIdx++ )
                    {
                        ubyte c = data[ row + asciiIdx ];
                        if ( c < 0x20 || c > 0x7E )
                            c = '.';
                        ascii[asciiIdx] = c;
                    }
                    output.write (ascii[ 0 .. asciiIdx ]);
                    output.write (Eol);
                }
                if (flushLines)
                    output.flush;
            }
        }
+/

