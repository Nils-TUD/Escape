/*******************************************************************************

        copyright:      Copyright (c) 2008. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: May 2008

        author:         Various

        Since:          0.99.7

        With gratitude to Dr Jurgen A Doornik. See his paper entitled
        "Conversion of high-period random numbers to floating point"
        
*******************************************************************************/

module tango.math.random.Kiss;


version (Win32)
	private extern(Windows) int QueryPerformanceCounter (ulong *);

version (Posix)
{
	private import tango.stdc.posix.sys.time;
}

version(Escape)
{
	extern (C) ulong cpu_rdtsc();
}


/******************************************************************************

        KISS (from George Marsaglia)

        The idea is to use simple, fast, individually promising
        generators to get a composite that will be fast, easy to code
        have a very long period and pass all the tests put to it.
        The three components of KISS are
        ---
                x(n)=a*x(n-1)+1 mod 2^32
                y(n)=y(n-1)(I+L^13)(I+R^17)(I+L^5),
                z(n)=2*z(n-1)+z(n-2) +carry mod 2^32
        ---

        The y's are a shift register sequence on 32bit binary vectors
        period 2^32-1; The z's are a simple multiply-with-carry sequence
        with period 2^63+2^32-1. The period of KISS is thus
        ---
                2^32*(2^32-1)*(2^63+2^32-1) > 2^127
        ---

        Note that this should be passed by reference, unless you really
        intend to provide a local copy to a callee
        
******************************************************************************/

struct Kiss
{
        public alias natural  toInt;
        public alias fraction toReal;
        
        private uint kiss_k;
        private uint kiss_m;
        private uint kiss_x = 1;
        private uint kiss_y = 2;
        private uint kiss_z = 4;
        private uint kiss_w = 8;
        private uint kiss_carry = 0;
        
        private const double M_RAN_INVM32 = 2.32830643653869628906e-010,
                             M_RAN_INVM52 = 2.22044604925031308085e-016;
      
        /**********************************************************************

                A global, shared instance, seeded via startup time

        **********************************************************************/

        public static Kiss instance; 

        static this ()
        {
                instance.seed;
        }

        /**********************************************************************

                Creates and seeds a new generator with the current time

        **********************************************************************/

        static Kiss opCall ()
        {
                Kiss rand;
                rand.seed;
                return rand;
        }

        /**********************************************************************

                Seed the generator with current time

        **********************************************************************/

        void seed ()
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

                return seed (cast(uint) s);
        }

        /**********************************************************************

                Seed the generator with a provided value

        **********************************************************************/

        void seed (uint seed)
        {
                kiss_x = seed | 1;
                kiss_y = seed | 2;
                kiss_z = seed | 4;
                kiss_w = seed | 8;
                kiss_carry = 0;
        }

        /**********************************************************************

                Returns X such that 0 <= X <= uint.max

        **********************************************************************/

        uint natural ()
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

        /**********************************************************************

                Returns X such that 0 <= X < max

                Note that max is exclusive, making it compatible with
                array indexing

        **********************************************************************/

        uint natural (uint max)
        {
                return natural % max;
        }

        /**********************************************************************

                Returns X such that min <= X < max

                Note that max is exclusive, making it compatible with
                array indexing

        **********************************************************************/

        uint natural (uint min, uint max)
        {
                return (natural % (max-min)) + min;
        }
        
        /**********************************************************************
        
                Returns a value in the range [0, 1) using 32 bits
                of precision (with thanks to Dr Jurgen A Doornik)

        **********************************************************************/

        double fraction ()
        {
                return ((cast(int) natural) * M_RAN_INVM32 + (0.5 + M_RAN_INVM32 / 2));
        }

        /**********************************************************************

                Returns a value in the range [0, 1) using 52 bits
                of precision (with thanks to Dr Jurgen A Doornik)

        **********************************************************************/

        double fractionEx ()
        {
                return ((cast(int) natural) * M_RAN_INVM32 + (0.5 + M_RAN_INVM52 / 2) + 
                       ((cast(int) natural) & 0x000FFFFF) * M_RAN_INVM52);
        }
}



/******************************************************************************


******************************************************************************/

debug (Kiss)
{
        import tango.io.Stdout;
        import tango.time.StopWatch;

        void main()
        {
                auto dbl = Kiss();
                auto count = 100_000_000;
                StopWatch w;

                w.start;
                double v1;
                for (int i=count; --i;)
                     v1 = dbl.fraction;
                Stdout.formatln ("{} fraction, {}/s, {:f10}", count, count/w.stop, v1);

                w.start;
                for (int i=count; --i;)
                     v1 = dbl.fractionEx;
                Stdout.formatln ("{} fractionEx, {}/s, {:f10}", count, count/w.stop, v1);

                for (int i=count; --i;)
                    {
                    auto v = dbl.fraction;
                    if (v <= 0.0 || v >= 1.0)
                       {
                       Stdout.formatln ("fraction {:f10}", v);
                       break;
                       }
                    v = dbl.fractionEx;
                    if (v <= 0.0 || v >= 1.0)
                       {
                       Stdout.formatln ("fractionEx {:f10}", v);
                       break;
                       }
                    }
        }
}
