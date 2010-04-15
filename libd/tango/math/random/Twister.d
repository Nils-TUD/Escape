/*******************************************************************************

        copyright:      Copyright (C) 1997--2004, Makoto Matsumoto,
                        Takuji Nishimura, and Eric Landry; All rights reserved
        

        license:        BSD style: $(LICENSE)

        version:        Jan 2008: Initial release

        author:         KeYeR (D interface) keyer@team0xf.com
      
*******************************************************************************/

module tango.math.random.Twister;


version (Win32)
	private extern(Windows) int QueryPerformanceCounter (ulong *);
else version (Escape)
{
	extern (C) ulong cpu_rdtsc();
}
else version (Posix)
{
	private import tango.stdc.posix.sys.time;
}
        
/*******************************************************************************

        Wrapper for the Mersenne twister.
        
        The Mersenne twister is a pseudorandom number generator linked to
        CR developed in 1997 by Makoto Matsumoto and Takuji Nishimura that
        is based on a matrix linear recurrence over a finite binary field
        F2. It provides for fast generation of very high quality pseudorandom
        numbers, having been designed specifically to rectify many of the 
        flaws found in older algorithms.
        
        Mersenne Twister has the following desirable properties:
        ---
        1. It was designed to have a period of 2^19937 - 1 (the creators
           of the algorithm proved this property).
           
        2. It has a very high order of dimensional equidistribution.
           This implies that there is negligible serial correlation between
           successive values in the output sequence.
           
        3. It passes numerous tests for statistical randomness, including
           the stringent Diehard tests.
           
        4. It is fast.
        ---

*******************************************************************************/

struct Twister
{
        public alias natural  toInt;
        public alias fraction toReal;
        
        private enum : uint                     // Period parameters
                {
                N          = 624,
                M          = 397,
                MATRIX_A   = 0x9908b0df,        // constant vector a 
                UPPER_MASK = 0x80000000,        //  most significant w-r bits 
                LOWER_MASK = 0x7fffffff,        // least significant r bits
                }

        private uint[N] mt;                     // the array for the state vector  
        private uint mti=mt.length+1;           // mti==mt.length+1 means mt[] is not initialized 
        private uint vLastRand;                 // The most recent random uint returned. 

        /**********************************************************************

                A global, shared instance, seeded via startup time

        **********************************************************************/

        public static Twister instance; 

        static this ()
        {
                instance.seed;
        }

        /**********************************************************************

                Creates and seeds a new generator with the current time

        **********************************************************************/

        static Twister opCall ()
        {
                Twister rand;
                rand.seed;
                return rand;
        }

        /**********************************************************************

                Returns X such that 0 <= X < max
                
        **********************************************************************/

        uint natural (uint max)
        {
                return natural % max;
        }
        
        /**********************************************************************

                Returns X such that min <= X < max
                
        **********************************************************************/

        uint natural (uint min, uint max)
        {
                return (natural % (max-min)) + min;
        }

        /**********************************************************************

                Returns X such that 0 <= X <= uint.max 

        **********************************************************************/

        uint natural (bool pAddEntropy = false)
        {
                uint y;
                static uint mag01[2] =[0, MATRIX_A];

                if (mti >= mt.length) { 
                        int kk;

                        if (pAddEntropy || mti > mt.length)   
                        {
                                seed (5489U, pAddEntropy); 
                        }

                        for (kk=0;kk<mt.length-M;kk++)
                        {
                                y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
                                mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 1U];
                        }
                        for (;kk<mt.length-1;kk++) {
                                y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
                                mt[kk] = mt[kk+(M-mt.length)] ^ (y >> 1) ^ mag01[y & 1U];
                        }
                        y = (mt[mt.length-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
                        mt[mt.length-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 1U];

                        mti = 0;
                }

                y = mt[mti++];

                y ^= (y >> 11);
                y ^= (y << 7)  &  0x9d2c5680U;
                y ^= (y << 15) &  0xefc60000U;
                y ^= (y >> 18);

                vLastRand = y;
                return y;
        }

        /**********************************************************************

                generates a random number on [0,1] interval
                
        **********************************************************************/

        double inclusive ()
        {
                return natural*(1.0/cast(double)uint.max);
        }

        /**********************************************************************

                generates a random number on (0,1) interval 

        **********************************************************************/

        double exclusive ()
        {
                return ((cast(double)natural) + 0.5)*(1.0/(cast(double)uint.max+1.0));
        }

        /**********************************************************************

                generates a random number on [0,1) interval 

        **********************************************************************/

        double fraction ()
        {
                return natural*(1.0/(cast(double)uint.max+1.0));
        }

        /**********************************************************************

                generates a random number on [0,1) with 53-bit resolution

        **********************************************************************/

        double fractionEx ()
        {
                uint a = natural >> 5, b = natural >> 6;
                return(a * 67108864.0 + b) * (1.0 / 9007199254740992.0);
        }
        
        /**********************************************************************

                Seed the generator with current time

        **********************************************************************/

        void seed ()
        {
                ulong s;

                version (Escape)
                		s = cpu_rdtsc();
        		else version (Posix)
                        {
                        timeval tv;

                        gettimeofday (&tv, null);
                        s = tv.tv_usec;
                        }
				else version (Win32)
                         QueryPerformanceCounter (&s);

                return seed (cast(uint) s);
        }

        /**********************************************************************

                initializes the generator with a seed 

        **********************************************************************/

        void seed (uint s, bool pAddEntropy = false)
        {
                mt[0]= (s + (pAddEntropy ? vLastRand + cast(uint) this : 0)) & 0xffffffffU;
                for (mti=1; mti<mt.length; mti++){
                        mt[mti] = (1812433253U * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);
                        mt[mti] &= 0xffffffffU;
                }
        }

        /**********************************************************************

                initialize by an array with array-length init_key is
                the array for initializing keys
                
        **********************************************************************/

        void init (uint[] init_key, bool pAddEntropy = false)
        {
                int i, j, k;
                i=1;
                j=0;
                
                seed (19650218U, pAddEntropy);
                for (k = (mt.length > init_key.length ? mt.length : init_key.length); k; k--)   {
                        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525U))+ init_key[j] + j; 
                        mt[i] &=  0xffffffffU; 
                        i++;
                        j++;

                        if (i >= mt.length){
                                mt[0] = mt[mt.length-1];
                                i=1;
                        }

                        if (j >= init_key.length){
                                j=0;
                        }
                }

                for (k=mt.length-1; k; k--)     {
                        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941U))- i; 
                        mt[i] &=  0xffffffffU; 
                        i++;

                        if (i>=mt.length){
                                mt[0] = mt[mt.length-1];
                                i=1;
                        }
                }
                mt[0] |=  0x80000000U; 
                mti=0;
        }
}


/******************************************************************************


******************************************************************************/

debug (Twister)
{
        import tango.io.Stdout;
        import tango.time.StopWatch;

        void main()
        {
                auto dbl = Twister();
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
