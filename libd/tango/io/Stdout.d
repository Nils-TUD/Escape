/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: Nov 2005

        author:         Kris

*******************************************************************************/

module tango.io.Stdout;

private import  tango.io.Console;

private import  tango.io.model.IBuffer,
                tango.io.model.IConduit;

private import  tango.text.convert.Format;

/*******************************************************************************

        Platform issues ...
        
*******************************************************************************/

version (DigitalMars)
         alias void* Args;
   else 
      alias char* Args;

/*******************************************************************************

        A bridge between a Format instance and a Buffer. This is used for
        the Stdout & Stderr globals, but can be used for general purpose
        buffer-formatting as desired. The Template type 'T' dictates the
        text arrangement within the target buffer ~ one of char, wchar or
        dchar (utf8, utf16, or utf32)

*******************************************************************************/

class DisplayFormat(T)
{
        public alias print      opCall;

        private T[]             eol;
        private IBuffer         output;
        private Format!(T)      convert;

        /**********************************************************************

                Construct a DisplayFormat instance, tying the provided
                buffer to a formatter

        **********************************************************************/

        this (Format!(T) convert, IBuffer output, T[] eol = "\n")
        {
                this.convert = convert;
                this.output = output;
                this.eol = eol;
        }

        /**********************************************************************

                Format output using the provided formatting specification

        **********************************************************************/

        final DisplayFormat format (T[] fmt, ...)
        {
                convert (&sink, _arguments, _argptr, fmt);
                return this;
        }

        /**********************************************************************

                Format output using the provided formatting specification
                and append a newline

        **********************************************************************/

        final DisplayFormat formatln (T[] fmt, ...)
        {
                convert (&sink, _arguments, _argptr, fmt);
                return newline();
        }

        /**********************************************************************

                Format output using a default layout (variadic version)

        **********************************************************************/

        final DisplayFormat print (...)
        {
                if (_arguments.length > 0)
                    print(&sink, _arguments, _argptr);
                else
                    // zero args is just a flush
                    output.flush();

                return this;
        }

        /**********************************************************************

                Format output using a default layout and append a newline

        **********************************************************************/

        final DisplayFormat println (...)
        {
                if (_arguments.length > 0)
                    print(&sink, _arguments, _argptr);

                return newline();
        }

        /**********************************************************************

                Format output using a default layout

        **********************************************************************/

        protected final DisplayFormat print (Formatter.Sink sink, TypeInfo[] arguments, Args argptr)
        in
        {
                assert(arguments.length > 0 && arguments.length < 10);
        }
        body
        {
                static  T[][] fmt =
                        [
                        "{0}",
                        "{0}, {1}",
                        "{0}, {1}, {2}",
                        "{0}, {1}, {2}, {3}",
                        "{0}, {1}, {2}, {3}, {4}",
                        "{0}, {1}, {2}, {3}, {4}, {5}",
                        "{0}, {1}, {2}, {3}, {4}, {5}, {6}",
                        "{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}",
                        "{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}",
                        "{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}",
                        ];

                convert(sink, arguments, argptr, fmt[arguments.length - 1]);
                return this;
        }

        /***********************************************************************

                output a newline

        ***********************************************************************/

        final DisplayFormat newline ()
        {
                output(eol).flush;
                return this;
        }

        /**********************************************************************

               Flush the output buffer

        **********************************************************************/

        final DisplayFormat flush ()
        {
                output.flush;
                return this;
        }

        /**********************************************************************

                Return the associated buffer

        **********************************************************************/

        final IBuffer buffer ()
        {
                return output;
        }

        /**********************************************************************

                Return the associated conduit

        **********************************************************************/

        final IConduit conduit ()
        {
                return output.conduit;
        }

        /**********************************************************************

                Sink for passing to the formatter

        **********************************************************************/

        private final uint sink (T[] s)
        {
                output (s);
                return s.length;
        }
}

/*******************************************************************************

        Standard, global formatters for console output. If you don't need
        formatted output or unicode translation, consider using the module
        tango.io.Console directly

        Note that both the buffer and conduit in use are exposed by these
        global instances ~ this can be leveraged, for instance, to copy a
        file to the standard output:

        ---
        Stdout.conduit.copy (new FileConduit ("myfile"));
        ---

*******************************************************************************/

public static DisplayFormat!(char) Stdout,
                                   Stderr;

static this()
{
        Stdout = new DisplayFormat!(char) (Formatter, Cout.buffer);
        Stderr = new DisplayFormat!(char) (Formatter, Cerr.buffer);
}

