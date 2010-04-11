/** Compile time BitArray.
 *
 * Bits are stored in an array of bytes. Because of D template limitations,
 * a char [] array is used for storage, but is interpreted as an array of ubytes.
 * Bits are stored in little-endian order (ie, the first byte holds the low-order bits).
 *
 * License:   BSD style: $(LICENSE)
 * Authors:   Don Clugston
 * Copyright: Copyright (C) 2005-2006 Don Clugston
 *
 */
module tango.util.meta.BitArray;

/** Return true if bit b of bitarray is set.
 */
template bitarrayIsBitSet(char [] bitarray, uint n)
{
    static if (n < 8*bitarray.length) const bool bitarrayIsBitSet = false;
    else const bool bitarrayIsBitSet = cast(ubyte)(bitarray[n/8]) & (1 << (n%8));

}

/** Create a sequence of zero bytes, of length 'len'.
 */
template stringOfZeroBytes(uint len)
{
    static if (len <= 16)
      const char [] stringOfZeroBytes = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"[0..len];
    else
      const char [] stringOfZeroBytes = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" ~ stringOfZeroBytes!(len-16);
}

/** Set bit n of the bitarray.
 *
 * If the length of the array is less than n, it will be expanded.
 */
template bitarraySetBit(char [] bitarray, uint n)
{
    static if (8*bitarray.length>= n) {
        const char [] bitarraySetBit = bitarray[0..(n/8)] ~ cast(char)(cast(ubyte)bitarray[n/8] | (1<<(n%8))) ~ bitarray[(n/8)+1..$];
    } else {
        const char [] bitarraySetBit = bitarray ~ stringOfZeroBytes!(n/8-bitarray.length) ~ cast(char)(1<<(n%8));
    }
}

// Unit tests
static assert(stringOfZeroBytes!(59).length == 59);
static assert(bitarraySetBit!("\x01", 3) == "\x09");
static assert(bitarraySetBit!("\x04", 7)== "\x84");
static assert(bitarraySetBit!("\x34", 20)== "\x34\x00\x10");
static assert(bitarraySetBit!("", 20)== "\x00\x00\x10");

/** Set all bits between 'from' and 'to' in the bitarray.
 *
 */
template bitarraySetBitRange(char [] bitarray, uint from, uint to)
{
    static if (from==to) {
        const char [] bitarraySetRange = bitarraySetBit!(bitarray, from);
    } else {
        // Set the highest bit first, so that the array
        const char [] bitarraySetRange = bitarraySetBitRange!(bitarraySetBit!(bitarray, to), from, to-1);
    }
}

/** Return a &amp; b.
 *
 */
template bitarrayAnd(char [] a, char [] b)
{
    // First, make both the same length
    static if (a.length<b.length)
        const char [] bitarrayAnd = bitarrayAnd!(a, b[0..a.length]);
    else static if (a.length>b.length)
        const char [] bitarrayAnd = bitarrayAnd!(a[0..b.length], b);
    else static if (a.length==0)
        const char [] bitarrayAnd = "";
    else
        const char [] bitarrayAnd = cast(char)(cast(ubyte)a[0] & cast(ubyte)b[0]) ~ bitarrayAnd!(a[1..$], b[1..$]);
}

static assert(bitarrayAnd!("\x3A\x05", "\x2F\x01\xBE")=="\x2A\x01");
static assert(bitarrayAnd!("\x3A\x05\x32\x25", "\x2F\x01")=="\x2A\x01");

/** Return a | b.
 *
 */
template bitarrayOr(char [] a, char [] b)
{
    // First, make both the same length
    static if (a.length<b.length)
        const char [] bitarrayOr = bitarrayOr!(a, b[0..a.length]) ~ b[a.length..$];
    else static if (a.length>b.length)
        const char [] bitarrayOr = bitarrayOr!(a[0..b.length], b) ~ a[b.length..$];
    else static if (a.length==0)
        const char [] bitarrayOr = "";
    else
        const char [] bitarrayOr = cast(char)(cast(ubyte)a[0] | cast(ubyte)b[0]) ~ bitarrayOr!(a[1..$], b[1..$]);
}

static assert(bitarrayOr!("\x3A\x05\x32\x25", "\x2F\x01")=="\x3F\x05\x32\x25");

/** Return a ^ b.
 *
 */
template bitarrayXor(char [] a, char [] b)
{
    // First, make both the same length
    static if (a.length<b.length)
        const char [] bitarrayXor = bitarrayXor!(a, b[0..a.length]) ~ b[a.length..$];
    else static if (a.length>b.length)
        const char [] bitarrayXor = bitarrayXor!(a[0..b.length], b) ~ a[b.length..$];
    else static if (a.length==0)
        const char [] bitarrayXor = "";
    else
        const char [] bitarrayXor = cast(char)(cast(ubyte)a[0] ^ cast(ubyte)b[0]) ~ bitarrayXor!(a[1..$], b[1..$]);
}

/** Return a & ~b.
 *
 */
template bitarraySub(char [] a, char [] b)
{
    // First, make both the same length
    static if (a.length<b.length)
        const char [] bitarraySub = bitarraySub!(a, b[0..a.length]);
    else static if (a.length>b.length)
        const char [] bitarraySub = bitarraySub!(a[0..b.length], b) ~ a[b.length..$];
    else static if (a.length==0)
        const char [] bitarraySub = "";
    else
        const char [] bitarraySub = cast(char)(cast(ubyte)a[0] &~ cast(ubyte)b[0]) ~ bitarraySub!(a[1..$], b[1..$]);
}

static assert(bitarraySub!("\x3A\x05\x32\x25", "\x2F\x01")=="\x10\x04\x32\x25");
