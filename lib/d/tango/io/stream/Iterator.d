/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: December 2005

        author:         Kris

*******************************************************************************/

module tango.io.stream.Iterator;

private import tango.io.stream.Buffered;

protected import tango.io.device.Conduit : InputFilter, InputBuffer, InputStream;

/*******************************************************************************

        The base class for a set of stream iterators. These operate
        upon a buffered input stream, and are designed to deal with
        partial content. That is, stream iterators go to work the
        moment any data becomes available in the buffer. Contrast
        this behaviour with the tango.text.Util iterators, which
        operate upon the extent of an array.

        There are two types of iterators supported; exclusive and
        inclusive. The former are the more common kind, where a token
        is delimited by elements that are considered foreign. Examples
        include space, comma, and end-of-line delineation. Inclusive
        tokens are just the opposite: they look for patterns in the
        text that should be part of the token itself - everything else
        is considered foreign. Currently tango.io.stream includes the
        exclusive variety only.

        Each pattern is exposed to the client as a slice of the original
        content, where the slice is transient. If you need to retain the
        exposed content, then you should .dup it appropriately. 

        The content provided to these iterators is intended to be fully
        read-only. All current tokenizers abide by this rule, but it is
        possible a user could mutate the content through a token slice.
        To enforce the desired read-only aspect, the code would have to
        introduce redundant copying or the compiler would have to support
        read-only arrays (now in D2).

        See LineIterator, CharIterator, RegexIterator, QuotedIterator,
        and SimpleIterator

*******************************************************************************/

class Iterator(T) : InputFilter 
{
        private InputBuffer     source;
        protected T[]           slice,
                                delim;

        /***********************************************************************

                The pattern scanner, implemented via subclasses

        ***********************************************************************/

        abstract protected size_t scan (void[] data);

        /***********************************************************************

                Instantiate with a buffer

        ***********************************************************************/

        this (InputStream stream = null)
        {       
                super (stream);
                if (stream)
                    set (stream);
        }

        /***********************************************************************

                Set the provided stream as the scanning source

        ***********************************************************************/

        Iterator set (InputStream stream)
        {
                assert (stream);
                source = BufferedInput.create (stream);
                super.source = source;
                return this;
        }

        /***********************************************************************

                Return the current token as a slice of the content

        ***********************************************************************/

        final T[] get ()
        {
                return slice;
        }

        /**********************************************************************

                Iterate over the set of tokens. This should really
                provide read-only access to the tokens, but D does
                not support that at this time

        **********************************************************************/

        int opApply (int delegate(ref T[]) dg)
        {
                bool more;
                int  result;

                do {
                   more = consume;
                   result = dg (slice);
                   } while (more && !result);
                return result;
        }

        /**********************************************************************

                Iterate over a set of tokens, exposing a token count 
                starting at zero

        **********************************************************************/

        int opApply (int delegate(ref int, ref T[]) dg)
        {
                bool more;
                int  result,
                     tokens;

                do {
                   more = consume;
                   result = dg (tokens, slice);
                   ++tokens;
                   } while (more && !result);
                return result;
        }

        /**********************************************************************

                Iterate over a set of tokens and delimiters, exposing a 
                token count starting at zero

        **********************************************************************/

        int opApply (int delegate(ref int, ref T[], ref T[]) dg)
        {
                bool more;
                int  result,
                     tokens;

                do {
                   delim = null;
                   more = consume;
                   result = dg (tokens, slice, delim);
                   ++tokens;
                   } while (more && !result);
                return result;
        }

        /***********************************************************************

                Locate the next token. Returns the token if found, null
                otherwise. Null indicates an end of stream condition. To
                sweep a conduit for lines using method next():
                ---
                auto lines = new LineIterator!(char) (new File("myfile"));
                while (lines.next)
                       Cout (lines.get).newline;
                ---

                Alternatively, we can extract one line from a conduit:
                ---
                auto line = (new LineIterator!(char) (new File("myfile"))).next;
                ---

                The difference between next() and foreach() is that the
                latter processes all tokens in one go, whereas the former
                processes in a piecemeal fashion. To wit:
                ---
                foreach (line; new LineIterator!(char) (new File("myfile")))
                         Cout(line).newline;
                ---
                
        ***********************************************************************/

        final T[] next ()
        {
                if (consume() || slice.length)
                    return slice;
                return null;
        }

        /***********************************************************************

                Set the content of the current slice to the provided 
                start and end points

        ***********************************************************************/

        protected final size_t set (T* content, size_t start, size_t end)
        {
                slice = content [start .. end];
                return end;
        }

        /***********************************************************************

                Set the content of the current slice to the provided 
                start and end points, and delimiter to the segment
                between end & next (inclusive)

        ***********************************************************************/

        protected final size_t set (T* content, size_t start, size_t end, size_t next)
        {
                slice = content [start .. end];
                delim = content [end .. next+1];
                return end;
        }

        /***********************************************************************

                Called when a scanner fails to find a matching pattern.
                This may cause more content to be loaded, and a rescan
                initiated

        ***********************************************************************/

        protected final size_t notFound ()
        {
                return Eof;
        }

        /***********************************************************************

                Invoked when a scanner matches a pattern. The provided
                value should be the index of the last element of the
                matching pattern, which is converted back to a void[]
                index.

        ***********************************************************************/

        protected final size_t found (size_t i)
        {
                return (i + 1) * T.sizeof;
        }

        /***********************************************************************

                See if set of characters holds a particular instance

        ***********************************************************************/

        protected final bool has (T[] set, T match)
        {
                foreach (T c; set)
                         if (match is c)
                             return true;
                return false;
        }

        /***********************************************************************

                Consume the next token and place it in 'slice'. Returns 
                true when there are potentially more tokens

        ***********************************************************************/

        private bool consume ()
        {
                if (source.next (&scan))
                    return true;

                // consume trailing token
                source.reader ((void[] arr) 
                              { 
                              slice = (cast(T*) arr.ptr) [0 .. arr.length/T.sizeof];
                              return cast(size_t)arr.length; 
                              });
                return false;
        }
}


