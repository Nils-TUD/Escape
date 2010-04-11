/*******************************************************************************

        copyright:      Copyright (c) 2007 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: Nov 2007

        author:         Kris

        UTF conversion streams, supporting cross-translation of char, wchar 
        and dchar variants. For supporting endian variations, configure the
        appropriate EndianStream upstream of this one (closer to the source)

*******************************************************************************/

module tango.io.stream.Utf;

private import tango.io.device.Conduit;

private import tango.io.stream.Buffered;

private import Utf = tango.text.convert.Utf;

/*******************************************************************************

        Streaming UTF converter. Type T is the target or destination type, 
        while S is the source type. Both types are either char/wchar/dchar.

*******************************************************************************/

class UtfInput(T, S) : InputFilter, InputFilter.Mutator
{       
        static if (!is (S == char) && !is (S == wchar) && !is (S == dchar)) 
                    pragma (msg, "Source type must be char, wchar, or dchar");

        static if (!is (T == char) && !is (T == wchar) && !is (T == dchar)) 
                    pragma (msg, "Target type must be char, wchar, or dchar");

        private InputBuffer buffer;

        /***********************************************************************

                Create a buffered utf input converter

        ***********************************************************************/

        this (InputStream stream)
        {
                super (buffer = BufferedInput.create (stream));
        }
        
        /***********************************************************************

                Consume input of type T, and return the number of array 
                elements comsumed. 

                Returns Eof upon end-of-flow

        ***********************************************************************/

        final size_t consume (T[] dst)
        {
                auto x = read (dst);
                if (x != Eof)
                    x /= T.sizeof;
                return x;
        }

        /***********************************************************************

        ***********************************************************************/

        final override size_t read (void[] dst)
        {
                static if (is (S == T))
                           return super.read (dst);
                else
                   {
                   uint   consumed,
                          produced;

                   size_t reader (void[] src)
                   {
                        if (src.length < S.sizeof)
                            return Eof;

                        auto output = BufferedInput.convert!(T)(dst);
                        auto input  = BufferedInput.convert!(S)(src);

                        static if (is (T == char))
                                   produced = Utf.toString(input, output, &consumed).length;

                        static if (is (T == wchar))
                                   produced = Utf.toString16(input, output, &consumed).length;

                        static if (is (T == dchar))
                                   produced = Utf.toString32(input, output, &consumed).length;

                        // consume buffer content
                        return consumed * S.sizeof;
                   }

                   // must have some space available for converting
                   if (dst.length < T.sizeof)
                       conduit.error ("UtfStream.read :: target array is too small");

                   // convert next chunk of input
                   if (buffer.next(&reader) is false)
                       return Eof;

                   return produced * T.sizeof;
                   }
        }
}


/*******************************************************************************
        
        Streaming UTF converter. Type T is the target or destination type, 
        while S is the source type. Both types are either char/wchar/dchar.

        Note that the arguments are reversed from those of UtfInput

*******************************************************************************/

class UtfOutput (S, T) : OutputFilter, OutputFilter.Mutator
{       
        static if (!is (S == char) && !is (S == wchar) && !is (S == dchar)) 
                    pragma (msg, "Source type must be char, wchar, or dchar");

        static if (!is (T == char) && !is (T == wchar) && !is (T == dchar)) 
                    pragma (msg, "Target type must be char, wchar, or dchar");


        private OutputBuffer buffer;

        /***********************************************************************

                Create a buffered utf output converter

        ***********************************************************************/

        this (OutputStream stream)
        {
                super (buffer = BufferedOutput.create (stream));
        }

        /***********************************************************************

                Consume input of type T, and return the number of array 
                elements consumed. 

                Returns Eof upon end-of-flow

        ***********************************************************************/

        final size_t consume (S[] dst)
        {
                auto x = write (dst);
                if (x != Eof)
                    x /= S.sizeof;
                return x;
        }

        /***********************************************************************
        
                Write to the output stream from a source array. The provided 
                src content is converted as necessary. Note that an attached
                output buffer must be at least four bytes wide to accommodate
                a conversion.

                Returns the number of bytes consumed from src, which may be
                less than the quantity provided

        ***********************************************************************/

        final override size_t write (void[] src)
        {
                static if (is (S == T))
                           return super.write (src);
                else
                   {
                   uint   consumed,
                          produced;

                   size_t writer (void[] dst)
                   {
                        // buffer must be at least 4 bytes wide 
                        // to contain a generic conversion
                        if (dst.length < 4)
                            return Eof;

                        auto input = BufferedOutput.convert!(S)(src);
                        auto output = BufferedOutput.convert!(T)(dst);

                        static if (is (T == char))
                                   produced = Utf.toString(input, output, &consumed).length;

                        static if (is (T == wchar))
                                   produced = Utf.toString16(input, output, &consumed).length;

                        static if (is (T == dchar))
                                   produced = Utf.toString32(input, output, &consumed).length;

                        return produced * T.sizeof;
                   }
                    
                   // write directly into buffered content and
                   // flush when the output is full
                   if (buffer.writer(&writer) is Eof)
                      {
                      buffer.flush;
                      if (buffer.writer(&writer) is Eof)
                          return Eof;
                      }
                   return consumed * S.sizeof;
                   }
        }
}


/*******************************************************************************
        
*******************************************************************************/
        
debug (Utf)
{
        import tango.io.Stdout;
        import tango.io.device.Array;

        void main()
        {
                auto inp = new UtfInput!(dchar, char)(new Array("hello world"));
                auto oot = new UtfOutput!(dchar, char)(new Array(20));
                oot.copy(inp);
                assert (oot.buffer.slice == "hello world");
        }
}
