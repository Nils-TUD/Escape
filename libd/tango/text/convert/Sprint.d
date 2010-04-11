/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
        
        version:        Initial release: Nov 2005

        author:         Kris

*******************************************************************************/

module tango.text.convert.Sprint;

private import tango.text.convert.Format;

/******************************************************************************

        Constructs sprintf-style output. This is a replacement for the 
        vsprintf() family of functions, and writes its output into a 
        lookaside buffer:

        ---
        // create a Sprint instance
        auto sprint = new Sprint!(char);

        // write formatted text to the console
        Cout (sprint ("{0} green bottles, sitting on a wall\n", 10));
        ---

        Sprint can be handy when you wish to format text for a Logger,
        or similar. It avoids heap activity during the conversion by
        providing a fixed size buffer for output. Please note that the
        class itself is stateful, and therefore a single instance is not
        shareable across multiple threads. 
        
        Note also that Sprint is templated, and can be instantiated for
        wide chars through a Sprint!(dchar) or Sprint!(wchar). The wide
        versions differ in that both the output and the format-string
        are of the target type. Variadic string arguments are transcoded 
        appropriately.

        See tango.text.convert.Format for information on format specifiers.

******************************************************************************/

class Sprint(T)
{
        protected T[]           buffer;
        static Format!(T)       global;
        Format!(T)              formatter;

        alias format            opCall;

        /**********************************************************************

                Create a global formatter, used as a default for new
                Sprint instances
                
        **********************************************************************/

        static this ()
        {
                global = new Format!(T);
        }
        
        /**********************************************************************

                Create new Sprint instances with a buffer of the specified
                size
                
        **********************************************************************/

        this (int size = 256)
        {
                this (size, global);
        }
        
        /**********************************************************************

                Create new Sprint instances with a buffer of the specified
                size, and the provided formatter. The second argument can be
                used to apply cultural specifics (I18N) to Sprint
                
        **********************************************************************/

        this (int size, Format!(T) formatter)
        {
                buffer = new T[size];
                this.formatter = formatter;
        }

        /**********************************************************************

                Format a set of arguments
                
        **********************************************************************/

        T[] format (T[] fmt, ...)
        {
                return formatter.sprint (buffer, fmt, _arguments, _argptr);
        }

        /**********************************************************************

                Format a set of arguments
                
        **********************************************************************/

        T[] format (T[] fmt, TypeInfo[] arguments, void* argptr)
        {
                return formatter.sprint (buffer, fmt, arguments, argptr);
        }
}

