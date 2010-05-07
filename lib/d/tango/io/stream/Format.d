/*******************************************************************************

        copyright:      Copyright (c) 2007 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: Oct 2007

        author:         Kris

*******************************************************************************/

module tango.io.stream.Format;

private import tango.io.device.Conduit;

private import tango.text.convert.Layout;

/*******************************************************************************

        A bridge between a Layout instance and a stream. This is used for
        the Stdout & Stderr globals, but can be used for general purpose
        buffer-formatting as desired. The Template type 'T' dictates the
        text arrangement within the target buffer ~ one of char, wchar or
        dchar (utf8, utf16, or utf32). 
        
        FormatOutput exposes this style of usage:
        ---
        auto print = new FormatOutput!(char) (...);

        print ("hello");                        => hello
        print (1);                              => 1
        print (3.14);                           => 3.14
        print ('b');                            => b
        print (1, 2, 3);                        => 1, 2, 3         
        print ("abc", 1, 2, 3);                 => abc, 1, 2, 3        
        print ("abc", 1, 2) ("foo");            => abc, 1, 2foo        
        print ("abc") ("def") (3.14);           => abcdef3.14

        print.format ("abc {}", 1);             => abc 1
        print.format ("abc {}:{}", 1, 2);       => abc 1:2
        print.format ("abc {1}:{0}", 1, 2);     => abc 2:1
        print.format ("abc ", 1);               => abc
        ---

        Note that the last example does not throw an exception. There
        are several use-cases where dropping an argument is legitimate,
        so we're currently not enforcing any particular trap mechanism.

        Flushing the output is achieved through the flush() method, or
        via an empty pair of parens: 
        ---
        print ("hello world") ();
        print ("hello world").flush;

        print.format ("hello {}", "world") ();
        print.format ("hello {}", "world").flush;
        ---
        
        Special character sequences, such as "\n", are written directly to
        the output without any translation (though an output-filter could
        be inserted to perform translation as required). Platform-specific 
        newlines are generated instead via the newline() method, which also 
        flushes the output when configured to do so:
        ---
        print ("hello ") ("world").newline;
        print.format ("hello {}", "world").newline;
        print.formatln ("hello {}", "world");
        ---

        The format() method supports the range of formatting options 
        exposed by tango.text.convert.Layout and extensions thereof; 
        including the full I18N extensions where configured in that 
        manner. To create a French instance of FormatOutput:
        ---
        import tango.text.locale.Locale;

        auto locale = new Locale (Culture.getCulture ("fr-FR"));
        auto print = new FormatOutput!(char) (locale, ...);
        ---

        Note that FormatOutput is *not* intended to be thread-safe
        
*******************************************************************************/

class FormatOutput(T) : OutputFilter
{       
        public  alias OutputFilter.flush flush;

        private T[]             eol;
        private Layout!(T)      convert;
        private bool            flushLines;

        public alias print      opCall;

        version (Win32)
                 private const T[] Eol = "\r\n";
             else
                private const T[] Eol = "\n";

        /**********************************************************************

                Construct a FormatOutput instance, tying the provided stream
                to a layout formatter

        **********************************************************************/

        this (OutputStream output, T[] eol = Eol)
        {
                this (Layout!(T).instance, output, eol);
        }

        /**********************************************************************

                Construct a FormatOutput instance, tying the provided stream
                to a layout formatter

        **********************************************************************/

        this (Layout!(T) convert, OutputStream output, T[] eol = Eol)
        {
                assert (convert);
                assert (output);

                this.convert = convert;
                this.eol = eol;
                super (output);
        }

        /**********************************************************************

                Layout using the provided formatting specification

        **********************************************************************/

        final FormatOutput format (T[] fmt, ...)
        {
                convert (&emit, _arguments, _argptr, fmt);
                return this;
        }

        /**********************************************************************

                Layout using the provided formatting specification

        **********************************************************************/

        final FormatOutput formatln (T[] fmt, ...)
        {
                convert (&emit, _arguments, _argptr, fmt);
                return newline;
        }

        /**********************************************************************

                Unformatted layout, with commas inserted between args. 
                Currently supports a maximum of 24 arguments

        **********************************************************************/

        final FormatOutput print (...)
        {
                static  T[] slice =  "{}, {}, {}, {}, {}, {}, {}, {}, "
                                     "{}, {}, {}, {}, {}, {}, {}, {}, "
                                     "{}, {}, {}, {}, {}, {}, {}, {}, ";

                assert (_arguments.length <= slice.length/4, "FormatOutput :: too many arguments");

                if (_arguments.length is 0)
                    sink.flush;
                else
                   convert (&emit, _arguments, _argptr, slice[0 .. _arguments.length * 4 - 2]);
                         
                return this;
        }

        /***********************************************************************

                Output a newline and optionally flush

        ***********************************************************************/

        final FormatOutput newline ()
        {
                sink.write (eol);
                if (flushLines)
                    sink.flush;
                return this;
        }

        /**********************************************************************

                Control implicit flushing of newline(), where true enables
                flushing. An explicit flush() will always flush the output.

        **********************************************************************/

        final FormatOutput flush (bool yes)
        {
                flushLines = yes;
                return this;
        }

        /**********************************************************************

                Return the associated output stream

        **********************************************************************/

        final OutputStream stream ()
        {
                return sink;
        }

        /**********************************************************************

                Set the associated output stream

        **********************************************************************/

        final FormatOutput stream (OutputStream output)
        {
                sink = output;
                return this;
        }

        /**********************************************************************

                Return the associated Layout

        **********************************************************************/

        final Layout!(T) layout ()
        {
                return convert;
        }

        /**********************************************************************

                Set the associated Layout

        **********************************************************************/

        final FormatOutput layout (Layout!(T) layout)
        {
                convert = layout;
                return this;
        }

        /**********************************************************************

                Sink for passing to the formatter

        **********************************************************************/

        private final uint emit (T[] s)
        {
                auto count = sink.write (s);
                if (count is Eof)
                    conduit.error ("FormatOutput :: unexpected Eof");
                return count;
        }
}


/*******************************************************************************
        
*******************************************************************************/
        
debug (Format)
{
        import tango.io.device.Array;

        void main()
        {
                auto print = new FormatOutput!(char) (new Array(1024, 1024));

                for (int i=0;i < 1000; i++)
                     print(i).newline;
        }
}
