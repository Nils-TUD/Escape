/*******************************************************************************

        copyright:      Copyright (c) 2007 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: Nov 2007

        author:         Kris

        Streams to expose simple native types as discrete elements. I/O
        is buffered and should yield fair performance.

*******************************************************************************/

module tango.io.stream.Typed;

private import tango.io.device.Conduit;

private import tango.io.stream.Buffered;

/*******************************************************************************

        Type T is the target or destination type

*******************************************************************************/

class TypedInput(T) : InputFilter
{       
        /***********************************************************************

        ***********************************************************************/

        this (InputStream stream)
        {
                super (BufferedInput.create (stream));
        }
        
        /***********************************************************************

                Override this to give back a useful chaining reference

        ***********************************************************************/

        final override TypedInput flush ()
        {
                super.flush;
                return this;
        }

        /***********************************************************************

                Read a value from the stream. Returns false when all 
                content has been consumed

        ***********************************************************************/

        final bool read (ref T x)
        {
                return source.read((&x)[0..1]) is T.sizeof;
        }

        /***********************************************************************

                Iterate over all content

        ***********************************************************************/

        final int opApply (int delegate(ref T x) dg)
        {
                T x;
                int ret;

                while ((source.read((&x)[0..1]) is T.sizeof))
                        if ((ret = dg (x)) != 0)
                             break;
                return ret;
        }
}



/*******************************************************************************
        
        Type T is the target or destination type.

*******************************************************************************/

class TypedOutput(T) : OutputFilter
{       
        /***********************************************************************

        ***********************************************************************/

        this (OutputStream stream)
        {
                super (BufferedOutput.create (stream));
        }

        /***********************************************************************
        
                Append a value to the output stream

        ***********************************************************************/

        final void write (ref T x)
        {
                sink.write ((&x)[0..1]);
        }
}


/*******************************************************************************
        
*******************************************************************************/
        
debug (UnitTest)
{
        import tango.io.Stdout;
        import tango.io.stream.Utf;
        import tango.io.device.Array;

        unittest
        {
                Array output;

                auto inp = new TypedInput!(char)(new Array("hello world"));
                auto oot = new TypedOutput!(char)(output = new Array(20));

                foreach (x; inp)
                         oot.write (x);
                assert (output.slice == "hello world");

                auto xx = new TypedInput!(char)(new UtfInput!(char, dchar)(new Array("hello world"d)));
                char[] yy;
                foreach (x; xx)
                         yy ~= x;
                assert (yy == "hello world");
        }
}
