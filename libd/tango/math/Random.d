/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: April 2004

        author:         various

*******************************************************************************/

module tango.math.Random;


version (Win32)
{
	private extern(Windows) int QueryPerformanceCounter (ulong *);
}

version (Posix)
{
	private import tango.stdc.posix.sys.time;
}

version(Escape)
{
	extern (C) ulong cpu_rdtsc();
}

/******************************************************************************

        KISS (via George Marsaglia & Paul Hsieh)

        the idea is to use simple, fast, individually promising
        generators to get a composite that will be fast, easy to code
        have a very long period and pass all the tests put to it.
        The three components of KISS are

                x(n)=a*x(n-1)+1 mod 2^32
                y(n)=y(n-1)(I+L^13)(I+R^17)(I+L^5),
                z(n)=2*z(n-1)+z(n-2) +carry mod 2^32

        The y's are a shift register sequence on 32bit binary vectors
        period 2^32-1; The z's are a simple multiply-with-carry sequence
        with period 2^63+2^32-1.

        The period of KISS is thus 2^32*(2^32-1)*(2^63+2^32-1) > 2^127

******************************************************************************/

class Random
{
        private uint kiss_x = 1;
        private uint kiss_y = 2;
        private uint kiss_z = 4;
        private uint kiss_w = 8;
        private uint kiss_carry = 0;
        private uint kiss_k;
        private uint kiss_m;

        /**********************************************************************

        **********************************************************************/

        this ()
        {
                ulong s;

                version (Posix)
                        {
                        timeval tv;

                        gettimeofday (&tv, null);
                        s = tv.tv_usec;
                        }
                version (Win32)
                         QueryPerformanceCounter (&s);
                version (Escape)
                		s = cast(uint)cpu_rdtsc();

                seed (cast(uint) s);
        }

        /**********************************************************************

        **********************************************************************/

        final void seed (uint seed)
        {
                kiss_x = seed | 1;
                kiss_y = seed | 2;
                kiss_z = seed | 4;
                kiss_w = seed | 8;
                kiss_carry = 0;
        }

        /**********************************************************************

        **********************************************************************/

        final uint next (uint limit)
        {
                return next() % limit;
        }

        /**********************************************************************

        **********************************************************************/

        final uint next ()
        {
                kiss_x = kiss_x * 69069 + 1;
                kiss_y ^= kiss_y << 13;
                kiss_y ^= kiss_y >> 17;
                kiss_y ^= kiss_y << 5;
                kiss_k = (kiss_z >> 2) + (kiss_w >> 3) + (kiss_carry >> 2);
                kiss_m = kiss_w + kiss_w + kiss_z + kiss_carry;
                kiss_z = kiss_w;
                kiss_w = kiss_m;
                kiss_carry = kiss_k >> 30;
                return kiss_x + kiss_y + kiss_w;
        }
}

