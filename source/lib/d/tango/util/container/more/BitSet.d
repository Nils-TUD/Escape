/*******************************************************************************

        copyright:      Copyright (c) 2009 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Sept 2009: Initial release

        since:          0.99.9

        author:         Kris

*******************************************************************************/

module tango.util.container.more.BitSet;

private import std.intrinsic;

/******************************************************************************

        A fixed or dynamic set of bits. Note that this does no memory 
        allocation of its own when Size != 0, and does heap allocation 
        when Size is zero. Thus you can have a fixed-size low-overhead 
        'instance, or a heap oriented instance. The latter has support
        for resizing, whereas the former does not.

        Note that leveraging intrinsics is slower when using dmd ...

******************************************************************************/

struct BitSet (int Count=0) 
{               
        public alias and        opAnd;
        public alias or         opOrAssign;
        public alias xor        opXorAssign;
        private const           width = size_t.sizeof * 8;

        static if (Count == 0)
                   private size_t[] bits;
               else
                  private size_t [(Count+width-1)/width] bits;

        /**********************************************************************

                Set the indexed bit, resizing as necessary for heap-based
                instances (IndexOutOfBounds for statically-sized instances)

        **********************************************************************/

        void add (size_t i)
        {
                static if (Count == 0)
                           size (i);
                or (i);
        }

        /**********************************************************************

                Test whether the indexed bit is enabled 

        **********************************************************************/

        bool has (size_t i)
        {
                auto idx = i / width;
                return idx < bits.length && (bits[idx] & (1 << (i % width))) != 0;
                //return idx < bits.length && bt(&bits[idx], i % width) != 0;
        }

        /**********************************************************************

                Like get() but a little faster for when you know the range
                is valid

        **********************************************************************/

        bool and (size_t i)
        {
                return (bits[i / width] & (1 << (i % width))) != 0;
                //return bt(&bits[i / width], i % width) != 0;
        }

        /**********************************************************************

                Turn on an indexed bit

        **********************************************************************/

        void or (size_t i)
        {
                bits[i / width] |= (1 << (i % width));
                //bts (&bits[i / width], i % width);
        }
        
        /**********************************************************************

                Invert an indexed bit

        **********************************************************************/

        void xor (size_t i)
        {
                bits[i / width] ^= (1 << (i % width));
                //btc (&bits[i / width], i % width);
        }
        
        /**********************************************************************

                Clear an indexed bit

        **********************************************************************/

        void clr (size_t i)
        {
                bits[i / width] &= ~(1 << (i % width));
                //btr (&bits[i / width], i % width);
        }

        /**********************************************************************

                Clear all bits

        **********************************************************************/

        BitSet* clr ()
        {
                bits[] = 0;
                return this;
        }
        
        /**********************************************************************

                Clone this BitSet and return it

        **********************************************************************/

        BitSet dup ()
        {
                BitSet x;
                static if (Count == 0)
                           x.bits.length = this.bits.length;
                x.bits[] = bits[];
                return x;
        }

        /**********************************************************************

                Return the number of bits we have room for

        **********************************************************************/

        size_t size ()
        {
                return width * bits.length;
        }

        /**********************************************************************

                Expand to include the indexed bit (dynamic only)

        **********************************************************************/

        static if (Count == 0) BitSet* size (size_t i)
        {
                i = i / width;
                if (i >= bits.length)
                    bits.length = i + 1;
                return this;
        }
}
