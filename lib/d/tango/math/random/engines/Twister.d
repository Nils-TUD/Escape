/*******************************************************************************

        copyright:      Copyright (C) 1997--2004, Makoto Matsumoto,
                        Takuji Nishimura, and Eric Landry; All rights reserved
        

        license:        BSD style: $(LICENSE)

        version:        Jan 2008: Initial release

        author:         KeYeR (D interface) keyer@team0xf.com
                        fawzi (converted to engine)
      
*******************************************************************************/

module tango.math.random.engines.Twister;
private import Integer = tango.text.convert.Integer;

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
    private enum : uint {
        // Period parameters
        N          = 624,
        M          = 397,
        MATRIX_A   = 0x9908b0df,        // constant vector a 
        UPPER_MASK = 0x80000000,        //  most significant w-r bits 
        LOWER_MASK = 0x7fffffff,        // least significant r bits
    }
    const int canCheckpoint=true;
    const int canSeed=true;

    private uint[N] mt;                     // the array for the state vector  
    private uint mti=mt.length+1;           // mti==mt.length+1 means mt[] is not initialized 
    
    /// returns a random uint
    uint next ()
    {
        uint y;
        static uint mag01[2] =[0, MATRIX_A];

        if (mti >= mt.length) { 
            int kk;

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
        y ^= (y << 7)  &  0x9d2c5680UL;
        y ^= (y << 15) &  0xefc60000UL;
        y ^= (y >> 18);

        return y;
    }
    /// returns a random byte
    ubyte nextB(){
        return cast(ubyte)(next() & 0xFF);
    }
    /// returns a random long
    ulong nextL(){
        return ((cast(ulong)next)<<32)+cast(ulong)next;
    }
    
    /// initializes the generator with a uint as seed
    void seed (uint s)
    {
        mt[0]= s & 0xffff_ffffU;  // this is very suspicious, was the previous line incorrectly translated from C???
        for (mti=1; mti<mt.length; mti++){
            mt[mti] = cast(uint)(1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);
            mt[mti] &= 0xffff_ffffUL; // this is very suspicious, was the previous line incorrectly translated from C???
        }
    }
    /// adds entropy to the generator
    void addEntropy(uint delegate() r){
        int i, j, k;
        i=1;
        j=0;

        for (k = mt.length; k; k--)   {
            mt[i] = cast(uint)((mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL))+ r() + j); 
            mt[i] &=  0xffff_ffffUL;  // this is very suspicious, was the previous line incorrectly translated from C???
            i++;
            j++;

            if (i >= mt.length){
                    mt[0] = mt[mt.length-1];
                    i=1;
            }
        }

        for (k=mt.length-1; k; k--)     {
            mt[i] = cast(uint)((mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL))- i); 
            mt[i] &=  0xffffffffUL;  // this is very suspicious, was the previous line incorrectly translated from C???
            i++;

            if (i>=mt.length){
                    mt[0] = mt[mt.length-1];
                    i=1;
            }
        }
        mt[0] |=  0x80000000UL; 
        mti=0;
    }
    /// seeds the generator
    void seed(uint delegate() r){
        seed (19650218UL);
        addEntropy(r);
    }
    /// writes the current status in a string
    char[] toString(){
        char[] res=new char[7+(N+1)*9];
        int i=0;
        res[i..i+7]="Twister";
        i+=7;
        res[i]='_';
        ++i;
        Integer.format(res[i..i+8],mti,"x8");
        i+=8;
        foreach (val;mt){
            res[i]='_';
            ++i;
            Integer.format(res[i..i+8],val,"x8");
            i+=8;
        }
        assert(i==res.length,"unexpected size");
        return res;
    }
    /// reads the current status from a string (that should have been trimmed)
    /// returns the number of chars read
    size_t fromString(char[] s){
        size_t i;
        assert(s[0..7]=="Twister","unexpected kind, expected Twister");
        i+=7;
        assert(s[i]=='_',"no separator _ found");
        ++i;
        uint ate;
        mti=cast(uint)Integer.convert(s[i..i+8],16,&ate);
        assert(ate==8,"unexpected read size");
        i+=8;
        foreach (ref val;mt){
            assert(s[i]=='_',"no separator _ found");
            ++i;
            val=cast(uint)Integer.convert(s[i..i+8],16,&ate);
            assert(ate==8,"unexpected read size");
            i+=8;
        }
        return i;
    }
    
}

