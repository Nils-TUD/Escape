/*******************************************************************************

        copyright:      Copyright (c) 2006 Tango. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Jan 2006: initial release

        author:         Kris, Nthalk

*******************************************************************************/

module tango.io.stream.Quotes;

private import tango.io.stream.Iterator;

/*******************************************************************************

        Iterate over a set of delimited, optionally-quoted, text fields.

        Each field is exposed to the client as a slice of the original
        content, where the slice is transient. If you need to retain the
        exposed content, then you should .dup it appropriately. 

        The content exposed via an iterator is supposed to be entirely
        read-only. All current iterators abide by this rule, but it is
        possible a user could mutate the content through a get() slice.
        To enforce the desired read-only aspect, the code would have to 
        introduce redundant copying or the compiler would have to support 
        read-only arrays.

        Usage:
        ---
        auto f = new File ("my.csv");
        auto l = new LineIterator (f);
        auto b = new Array (0);
        auto q = new Quotes!(char)(",", b);
        
        foreach (line; l)
                {
                b.assign (line);
                foreach (field, index; q)
                         Stdout (index, field);
                Stdout.newline;
                }
        ---

        See Iterator, Lines, Patterns, Delimiters
        
*******************************************************************************/

class Quotes(T) : Iterator!(T)
{
        private T[] delim; 
      
        /***********************************************************************

                This splits on delimiters only. If there is a quote, it
                suspends delimiter splitting until the quote is finished.

        ***********************************************************************/

        this (T[] delim, InputStream stream = null)
        {
                super (stream);
                this.delim = delim;
        }
        
        /***********************************************************************

                This splits on delimiters only. If there is a quote, it
                suspends delimiter splitting until the quote is finished.

        ***********************************************************************/

        protected size_t scan (void[] data)
        {
                T    quote = 0;
                int  escape = 0;
                auto content = (cast(T*) data.ptr) [0 .. data.length / T.sizeof];

                foreach (i, c; content)
                         // within a quote block?
                         if (quote)
                            {   
                            if (c is '\\')
                                ++escape;
                            else
                               {
                               // matched the initial quote char?
                               if (c is quote && escape % 2 is 0)
                                   quote = 0;
                               escape = 0;
                               }
                            }
                         else
                            // begin a quote block?
                            if (c is '"' || c is '\'')
                                quote = c;
                            else 
                               if (has (delim, c))
                                   return found (set (content.ptr, 0, i));
                return notFound;
        }
}


/*******************************************************************************

*******************************************************************************/

debug (UnitTest)
{
        private import tango.io.Stdout;
        private import tango.text.Util;
        private import tango.io.device.Array;

        unittest 
        {
                char[][] expected = 
                         [
                         `0`
                         ,``
                         ,``
                         ,`"3"`
                         ,`""`
                         ,`5`
                         ,`",6"`
                         ,`"7,"`
                         ,`8`
                         ,`"9,\\\","`
                         ,`10`
                         ,`',11",'`
                         ,`"12"`
                         ];

                auto b = new Array (expected.join (","));
                foreach (i, f; new Quotes!(char)(",", b))
                         if (i >= expected.length)
                            Stdout.formatln ("uhoh: unexpected match: {}, {}", i, f);
                         else 
                            if (f != expected[i])
                                Stdout.formatln ("uhoh: bad match: {}, {}, {}", i, f, expected[i]);
        }
}

