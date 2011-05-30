module rt.compiler.util.hash;

/// hashes bStart[0..length] bytes
extern(C) hash_t rt_hash_str(void *bStart,size_t length, hash_t seed=0){
    static if (is(hash_t==uint)||is(hash_t==int)){
        return cast(hash_t)murmurHashAligned2(bStart,length,cast(uint)seed);
    } else static if (is(hash_t==ulong)||is(hash_t==long)){
        return cast(hash_t)lookup8_hash(cast(ubyte*)bStart,length,cast(ulong)seed);
    } else {
        static assert(0,"unsupported type for hash_t: "~hash_t.stringof);
    }
}

/// hashes bStart[0..length] size_t
extern(C) hash_t rt_hash_block(size_t *bStart,size_t length, hash_t seed=0){
    static if (is(hash_t==uint)||is(hash_t==int)){
        return cast(hash_t)murmurHashAligned2(bStart,length,cast(uint)seed);
    } else static if (is(hash_t==ulong)||is(hash_t==long)){
        return cast(hash_t)lookup8_hash2(cast(ulong*)bStart,length,cast(ulong)seed);
    } else {
        static assert(0,"unsupported type for hash_t: "~hash_t.stringof);
    }
}

/// hashes UTF-8 chars using the UTF codepoints (slower than rt_hash_str that uses the bit representation)
extern(C) uint rt_hash_utf8(char[] str, uint seed=0){
    const uint m = 0x5bd1e995;
    const int r = 24;

    uint h = seed ; // ^ length
    foreach (c;str)
    {
        uint k = cast(uint)c;
        // MIX(h,k,m);
        { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }
    }
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

/// hashes UTF-16 chars using the UTF codepoints (slower than rt_hash_str that uses the bit representation)
extern(C) uint rt_hash_utf16(wchar[] str, uint seed=0){
    const uint m = 0x5bd1e995;
    const int r = 24;

    uint h = seed ; // ^ length
    foreach (c;str)
    {
        uint k = cast(uint)c;
        // MIX(h,k,m);
        { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }
    }
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

/// hashes UTF-32 chars using the UTF codepoints (should be equivalent to rt_hash_str that uses the bit representation)
extern(C) uint rt_hash_utf32(dchar[] str, uint seed=0){
    const uint m = 0x5bd1e995;
    const int r = 24;

    uint h = seed ; // ^ length
    foreach (c;str)
    {
        uint k = cast(uint)c;
        // MIX(h,k,m);
        { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }
    }
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

/// combines two hash values
extern(C) hash_t rt_hash_combine( hash_t val1, hash_t val2 ){
    static if (is(hash_t==uint)||is(hash_t==int)){
        return cast(hash_t)rt_hash_combine32(cast(uint)val1,cast(uint)val2);
    } else static if (is(hash_t==ulong)||is(hash_t==long)){
        return cast(hash_t)rt_hash_combine64(cast(ulong)val1,cast(ulong)val2);
    } else {
        static assert(0,"unsupported type for hash_t: "~hash_t.stringof);
    }
}

/// hashes bStart[0..length] in an architecture neutral way
extern(C) uint rt_hash_str_neutral32(void *bStart,uint length, uint seed=0){
    return murmurHashNeutral2(bStart,length,seed);
}

/// hashes bStart[0..length] in an architecture neutral way
extern(C) ulong rt_hash_str_neutral64(void *bStart,ulong length, ulong seed=0){
    return cast(hash_t)lookup8_hash(cast(ubyte*)bStart,length,seed);
}

/// combines two hash values (32 bit, architecture neutral)
extern(C) uint rt_hash_combine32( uint val, uint seed )
{
    const uint m = 0x5bd1e995;
    const int r = 24;

    uint h = seed ^ 4u;
    ubyte * data = cast(ubyte *)&val;
    uint k;

    k  = (cast(uint)data[0]);
    k |= (cast(uint)data[1]) << 8;
    k |= (cast(uint)data[2]) << 16;
    k |= (cast(uint)data[3]) << 24;
    k *= m; 
    k ^= k >> r; 
    k *= m;
    h *= m;
    h ^= k;
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
} 

/// combines two hash values (64 bit, architecture neutral)
extern(C) ulong rt_hash_combine64( ulong value, ulong level)
{
  ulong a,b,c,len;
  ubyte* k=cast(ubyte*)(&value);

  /* Set up the internal state */
  a = b = level;                         /* the previous hash value */
  c = 0x9e3779b97f4a7c13UL; /* the golden ratio; an arbitrary value */
  c += 8;
  a+=(cast(ulong)k[ 7])<<56;
  a+=(cast(ulong)k[ 6])<<48;
  a+=(cast(ulong)k[ 5])<<40;
  a+=(cast(ulong)k[ 4])<<32;
  a+=(cast(ulong)k[ 3])<<24;
  a+=(cast(ulong)k[ 2])<<16;
  a+=(cast(ulong)k[ 1])<<8;
  a+=(cast(ulong)k[ 0]);
  mixin(mix64abc);
  
  return c;
}

private:

//-----------------------------------------------------------------------------
// murmurHashAligned2, by Austin Appleby

// Same algorithm as murmurHash2, but only does aligned reads - should be safer
// on certain platforms. 

// Performance will be lower than murmurHash2
private uint murmurHashAligned2 ( void * key, uint len, uint seed )
{
    const uint m = 0x5bd1e995;
    const int r = 24;

    ubyte * data = cast(ubyte *)key;

    uint h = seed ^ len;

    int alignV = cast(int)data & 3;

    if(alignV && (len >= 4))
    {
        // Pre-load the temp registers

        uint t = 0, d = 0;

        switch(alignV)
        {
            case 1: t |= (cast(uint)data[2]) << 16;
            case 2: t |= (cast(uint)data[1]) << 8;
            case 3: t |= cast(uint)data[0];
            default:
        }

        t <<= (8 * alignV);

        data += 4-alignV;
        len -= 4-alignV;

        int sl = 8 * (4-alignV);
        int sr = 8 * alignV;

        // Mix

        while(len >= 4)
        {
            d = *cast(uint *)data;
            t = (t >> sr) | (d << sl);

            uint k = t;

            // MIX(h,k,m);
            { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }
            

            t = d;

            data += 4;
            len -= 4;
        }

        // Handle leftover data in temp registers

        d = 0;

        if(len >= alignV)
        {
            switch(alignV)
            {
            case 3: d |= (cast(uint)data[2]) << 16;
            case 2: d |= (cast(uint)data[1]) << 8;
            case 1: d |= (cast(uint)data[0]);
            default:
            }

            uint k = (t >> sr) | (d << sl);
            // MIX(h,k,m);
            { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }

            data += alignV;
            len -= alignV;

            //----------
            // Handle tail bytes

            switch(len)
            {
            case 3: h ^= (cast(uint)data[2]) << 16;
            case 2: h ^= (cast(uint)data[1]) << 8;
            case 1: h ^= (cast(uint)data[0]);
                    h *= m;
            default:
            };
        }
        else
        {
            switch(len)
            {
            case 3: d |= (cast(uint)data[2]) << 16;
            case 2: d |= (cast(uint)data[1]) << 8;
            case 1: d |= (cast(uint)data[0]);
            case 0: h ^= (t >> sr) | (d << sl);
                    h *= m;
            default:
            }
        }

        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;

        return h;
    }
    else
    {
        while(len >= 4)
        {
            uint k = *cast(uint *)data;

            // MIX(h,k,m);
            { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }

            data += 4;
            len -= 4;
        }

        //----------
        // Handle tail bytes

        switch(len)
        {
        case 3: h ^= (cast(uint)data[2]) << 16;
        case 2: h ^= (cast(uint)data[1]) << 8;
        case 1: h ^= (cast(uint)data[0]);
                h *= m;
        default:
        };

        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;

        return h;
    }
}
//-----------------------------------------------------------------------------
// murmurHashNeutral2, by Austin Appleby

// Same as murmurHash2, but endian- and alignment-neutral.
// Half the speed though, alas.

private uint murmurHashNeutral2 ( void * key, uint len, uint seed )
{
    const uint m = 0x5bd1e995;
    const int r = 24;

    uint h = seed ^ len;

    ubyte * data = cast(ubyte *)key;

    while(len >= 4)
    {
        uint k;

        k  = data[0];
        k |= (cast(uint)data[1]) << 8;
        k |= (cast(uint)data[2]) << 16;
        k |= (cast(uint)data[3]) << 24;

        k *= m; 
        k ^= k >> r; 
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch(len)
    {
    case 3: h ^= (cast(uint)data[2]) << 16;
    case 2: h ^= (cast(uint)data[1]) << 8;
    case 1: h ^= (cast(uint)data[0]);
            h *= m;
    default:
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
} 

//#define hashsize(n) (cast(ulong)1<<(n))
//#define hashmask(n) (hashsize(n)-1)

/*
--------------------------------------------------------------------
mix -- mix 3 64-bit values reversibly.
mix() takes 48 machine instructions, but only 24 cycles on a superscalar
  machine (like Intel's new MMX architecture).  It requires 4 64-bit
  registers for 4::2 parallelism.
All 1-bit deltas, all 2-bit deltas, all deltas composed of top bits of
  (a,b,c), and all deltas of bottom bits were tested.  All deltas were
  tested both on random keys and on keys that were nearly all zero.
  These deltas all cause every bit of c to change between 1/3 and 2/3
  of the time (well, only 113/400 to 287/400 of the time for some
  2-bit delta).  These deltas all cause at least 80 bits to change
  among (a,b,c) when the mix is run either forward or backward (yes it
  is reversible).
This implies that a hash using mix64 has no funnels.  There may be
  characteristics with 3-bit deltas or bigger, I didn't test for
  those.
--------------------------------------------------------------------
*/
const char[] mix64abc=`{
  a -= b; a -= c; a ^= (c>>43);
  b -= c; b -= a; b ^= (a<<9); 
  c -= a; c -= b; c ^= (b>>8); 
  a -= b; a -= c; a ^= (c>>38);
  b -= c; b -= a; b ^= (a<<23);
  c -= a; c -= b; c ^= (b>>5); 
  a -= b; a -= c; a ^= (c>>35);
  b -= c; b -= a; b ^= (a<<49);
  c -= a; c -= b; c ^= (b>>11);
  a -= b; a -= c; a ^= (c>>12);
  b -= c; b -= a; b ^= (a<<18);
  c -= a; c -= b; c ^= (b>>22);
}`;

/*
--------------------------------------------------------------------
hash() -- hash a variable-length key into a 64-bit value
  k     : the key (the unaligned variable-length array of bytes)
  len   : the length of the key, counting by bytes
  level : can be any 8-byte value
Returns a 64-bit value.  Every bit of the key affects every bit of
the return value.  No funnels.  Every 1-bit and 2-bit delta achieves
avalanche.  About 41+5len instructions.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 64 bits,
use a bitmask.  For example, if you need only 10 bits, do
  h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.

If you are hashing n strings (ub1 **)k, do it like this:
  for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);

By Bob Jenkins, Jan 4 1997.  bob_jenkins@burtleburtle.net.  You may
use this code any way you wish, private, educational, or commercial,
but I would appreciate if you give me credit.

See http://burtleburtle.net/bob/hash/evahash.html
Use for hash table lookup, or anything where one collision in 2^^64
is acceptable.  Do NOT use for cryptographic purposes.
--------------------------------------------------------------------
*/

ulong lookup8_hash( ubyte* k, ulong length, ulong level)
{
  ulong a,b,c,len;

  /* Set up the internal state */
  len = length;
  a = b = level;                         /* the previous hash value */
  c = 0x9e3779b97f4a7c13UL; /* the golden ratio; an arbitrary value */

  /*---------------------------------------- handle most of the key */
  while (len >= 24)
  {
    a += (k[0]        +(cast(ulong)k[ 1]<< 8)+(cast(ulong)k[ 2]<<16)+(cast(ulong)k[ 3]<<24)
     +(cast(ulong)k[4 ]<<32)+(cast(ulong)k[ 5]<<40)+(cast(ulong)k[ 6]<<48)+(cast(ulong)k[ 7]<<56));
    b += (k[8]        +(cast(ulong)k[ 9]<< 8)+(cast(ulong)k[10]<<16)+(cast(ulong)k[11]<<24)
     +(cast(ulong)k[12]<<32)+(cast(ulong)k[13]<<40)+(cast(ulong)k[14]<<48)+(cast(ulong)k[15]<<56));
    c += (k[16]       +(cast(ulong)k[17]<< 8)+(cast(ulong)k[18]<<16)+(cast(ulong)k[19]<<24)
     +(cast(ulong)k[20]<<32)+(cast(ulong)k[21]<<40)+(cast(ulong)k[22]<<48)+(cast(ulong)k[23]<<56));
    mixin(mix64abc);
    k += 24; len -= 24;
  }

  /*------------------------------------- handle the last 23 bytes */
  c += length;
  switch(len)              /* all the case statements fall through */
  {
  case 23: c+=(cast(ulong)k[22]<<56);
  case 22: c+=(cast(ulong)k[21]<<48);
  case 21: c+=(cast(ulong)k[20]<<40);
  case 20: c+=(cast(ulong)k[19]<<32);
  case 19: c+=(cast(ulong)k[18]<<24);
  case 18: c+=(cast(ulong)k[17]<<16);
  case 17: c+=(cast(ulong)k[16]<<8);
    /* the first byte of c is reserved for the length */
  case 16: b+=(cast(ulong)k[15]<<56);
  case 15: b+=(cast(ulong)k[14]<<48);
  case 14: b+=(cast(ulong)k[13]<<40);
  case 13: b+=(cast(ulong)k[12]<<32);
  case 12: b+=(cast(ulong)k[11]<<24);
  case 11: b+=(cast(ulong)k[10]<<16);
  case 10: b+=(cast(ulong)k[ 9]<<8);
  case  9: b+=(cast(ulong)k[ 8]);
  case  8: a+=(cast(ulong)k[ 7]<<56);
  case  7: a+=(cast(ulong)k[ 6]<<48);
  case  6: a+=(cast(ulong)k[ 5]<<40);
  case  5: a+=(cast(ulong)k[ 4]<<32);
  case  4: a+=(cast(ulong)k[ 3]<<24);
  case  3: a+=(cast(ulong)k[ 2]<<16);
  case  2: a+=(cast(ulong)k[ 1]<<8);
  case  1: a+=(cast(ulong)k[ 0]);
    /* case 0: nothing left to add */
  default:
  }
  mixin(mix64abc);
  /*-------------------------------------------- report the result */
  return c;
}

/*
--------------------------------------------------------------------
 This works on all machines, is identical to hash() on little-endian 
 machines, and it is much faster than hash(), but it requires
 -- that the key be an array of ub8's, and
 -- that all your machines have the same endianness, and
 -- that the length be the number of ub8's in the key
--------------------------------------------------------------------
*/
ulong lookup8_hash2( ulong *k, ulong length, ulong level)
{
  ulong a,b,c,len;

  /* Set up the internal state */
  len = length;
  a = b = level;                         /* the previous hash value */
  c = 0x9e3779b97f4a7c13UL; /* the golden ratio; an arbitrary value */

  /*---------------------------------------- handle most of the key */
  while (len >= 3)
  {
    a += k[0];
    b += k[1];
    c += k[2];
    mixin(mix64abc);
    k += 3; len -= 3;
  }

  /*-------------------------------------- handle the last 2 ub8's */
  c += (length<<3);
  switch(len)              /* all the case statements fall through */
  {
    /* c is reserved for the length */
  case  2: b+=k[1];
  case  1: a+=k[0];
    /* case 0: nothing left to add */
  default:
  }
  mixin(mix64abc);
  /*-------------------------------------------- report the result */
  return c;
}

debug(UnitTest){
    void alignIndependence(T)(ubyte[] val,T function (void*,T len,T oldHash)hashF){
        ubyte[] vval=new ubyte[val.length+16];
        T oldV=0xdeadbeef;
        T hash=hashF(val.ptr,val.length,oldV);
        for(int i=0;i<16;++i){
            vval[i..i+val.length]=val;
            T hashAtt=hashF(vval.ptr+i,val.length,oldV);
            assert(hashAtt==hash);
        }
    }
    
    unittest{
        ubyte[] val=cast(ubyte[])"blaterare";
        alignIndependence!(uint)(val,&murmurHashNeutral2);
        alignIndependence!(uint)(val,&murmurHashAligned2);
        alignIndependence!(ulong)(val,
            function ulong(void* str,ulong len,ulong oldV){
                return lookup8_hash(cast(ubyte*)str,len,oldV);
            }
        );
    }
}
