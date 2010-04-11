/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
        
        version:        Initial release: November 2005
        
        author:         Kris

*******************************************************************************/

module tango.io.filter.EndianFilter;

private import  tango.io.Buffer,
                tango.io.Conduit;

private import  tango.core.ByteSwap,
                tango.core.Exception;


class EndianFilter : ConduitFilter
{
        alias void function (void*, uint) Swap;

        private uint mask;
        private Swap swap;
        
        private this (uint width, Swap swapper)
        {
                mask = ~(width - 1);
                swap = swapper;
        }

        uint reader (void[] dst)
        {
                //notify ("byte-swapping input ...\n"c);

                dst = dst [0..(length & mask)];

                int ret = next.reader (dst);
                if (ret == Conduit.Eof)
                    return ret;

                int ret1;
                if (ret & ~mask)
                    do {
                       ret1 = next.reader (dst[ret..ret+1]);
                       if (ret1 == Conduit.Eof)
                           throw new IOException ("EndianFilter.reader :: truncated input");
                       } while (ret1 == 0);

                swap (dst.ptr, ret + ret1);
                return ret;
        }

        uint writer (void[] src)
        {
                //notify ("byte-swapping output ...\n"c);

                int     written;
                int     ret, 
                        len = src.length & mask;

                src = src [0..len];
                swap (src.ptr, len);

                do {
                   ret = next.writer (src[written..len]);
                   if (ret == Conduit.Eof)
                       return ret;
                   } while ((written += ret) < len);

                return len;                        
        }
}

class EndianFilter16 : EndianFilter
{
        this () {super (2, &ByteSwap.swap16);}
}

class EndianFilter32 : EndianFilter
{
        this () {super (4, &ByteSwap.swap32);}
}

