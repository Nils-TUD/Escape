/*******************************************************************************

        copyright:      Copyright (c) 2007 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: Nov 2007

        author:         Kris

        Streams for swapping endian-order. The stream is treated as a set
        of same-sized elements. Note that partial elements are not mutated

*******************************************************************************/

module tango.io.stream.Endian;

private import tango.core.ByteSwap;

private import tango.io.device.Conduit;

private import tango.io.stream.Buffered;

/*******************************************************************************

        Type T is the element type

*******************************************************************************/

class EndianInput(T) : InputFilter, InputFilter.Mutator
{       
        static if ((T.sizeof != 2) && (T.sizeof != 4) && (T.sizeof != 8)) 
                    pragma (msg, "EndianInput :: type should be of length 2, 4, or 8 bytes");

        /***********************************************************************

        ***********************************************************************/

        this (InputStream stream)
        {
                super (BufferedInput.create (stream));
        }
        
        /***********************************************************************

                Read from conduit into a target array. The provided dst 
                will be populated with content from the conduit. 

                Returns the number of bytes read, which may be less than
                requested in dst (or IOStream.Eof for end-of-flow). Note
                that a trailing partial element will be placed into dst,
                but the returned length will effectively ignore it

        ***********************************************************************/

        final override size_t read (void[] dst)
        {
                auto len = source.read (dst[0 .. dst.length & ~(T.sizeof-1)]);
                if (len != Eof)
                   {
                   // the final read may be misaligned ...
                   len &= ~(T.sizeof - 1);

                   static if (T.sizeof == 2)
                              ByteSwap.swap16 (dst.ptr, len);

                   static if (T.sizeof == 4)
                              ByteSwap.swap32 (dst.ptr, len);

                   static if (T.sizeof == 8)
                              ByteSwap.swap64 (dst.ptr, len);
                   }
                return len;
        }
}



/*******************************************************************************
        
        Type T is the element type

*******************************************************************************/

class EndianOutput (T) : OutputFilter, OutputFilter.Mutator
{       
        static if ((T.sizeof != 2) && (T.sizeof != 4) && (T.sizeof != 8)) 
                    pragma (msg, "EndianOutput :: type should be of length 2, 4, or 8 bytes");

        private OutputBuffer output;

        /***********************************************************************

        ***********************************************************************/

        this (OutputStream stream)
        {
                super (output = BufferedOutput.create (stream));
        }

        /***********************************************************************
        
                Write to output stream from a source array. The provided 
                src content will be consumed and left intact.

                Returns the number of bytes written from src, which may
                be less than the quantity provided. Note that any partial 
                elements will not be consumed

        ***********************************************************************/

        final override size_t write (void[] src)
        {
                size_t writer (void[] dst)
                {
                        auto len = dst.length;
                        if (len > src.length)
                            len = src.length;

                        len &= ~(T.sizeof - 1);
                        dst [0..len] = src [0..len];

                        static if (T.sizeof == 2)
                                   ByteSwap.swap16 (dst.ptr, len);

                        static if (T.sizeof == 4)
                                   ByteSwap.swap32 (dst.ptr, len);

                        static if (T.sizeof == 8)
                                   ByteSwap.swap64 (dst.ptr, len);

                        return len;
                }

                return output.writer (&writer);
        }
}


/*******************************************************************************
        
*******************************************************************************/
        
debug (UnitTest)
{
        import tango.io.device.Array;
        
        unittest
        {
                auto inp = new EndianInput!(dchar)(new Array("hello world"d));
                auto oot = new EndianOutput!(dchar)(new Array(64));
                oot.copy (inp);
                assert (oot.output.slice == "hello world"d);
        }
}
