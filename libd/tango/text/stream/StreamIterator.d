/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: December 2005

        author:         Kris

*******************************************************************************/

module tango.text.stream.StreamIterator;

public  import tango.io.Buffer;

private import Text = tango.text.Util;

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
        is considered foreign. Currently the only inclusive token type
        is exposed by RegexToken; everything else is of the exclusive
        variety.

        The content provided to these iterators is intended to be fully
        read-only. All current tokenizers abide by this rule, but it is
        possible a user could mutate the content through a token slice.
        To enforce the desired read-only aspect, the code would have to
        introduce redundant copying or the compiler would have to support
        read-only arrays.

        See LineIterator, CharIterator, RegexIterator, QuotedIterator,
        and SimpleIterator

*******************************************************************************/

class StreamIterator(T)
{
        protected T[]           slice;
        protected IBuffer       buffer;

        /***********************************************************************

                The pattern scanner, implemented via subclasses

        ***********************************************************************/

        abstract protected uint scan (void[] data);

        /***********************************************************************

                Uninitialized instance ~ use set() to configure input

        ***********************************************************************/

        this () {}

        /***********************************************************************

                Instantiate with a buffer

        ***********************************************************************/

        this (IBuffer buffer)
        {
                set (buffer);
        }

        /***********************************************************************

                Instantiate with a conduit

        ***********************************************************************/

        this (IConduit conduit)
        {
                set (conduit);
        }

        /***********************************************************************

                Set the provided conduit as the scanning source

        ***********************************************************************/

        final StreamIterator set (IConduit conduit)
        {
                if (buffer is null)
                    buffer = new Buffer (conduit);
                else
                   buffer.clear.setConduit (conduit);
                return this;
        }

        /***********************************************************************

                Set the current buffer to scan

        ***********************************************************************/

        final StreamIterator set (IBuffer buffer)
        {
                this.buffer = buffer;
                return this;
        }

        /***********************************************************************

                Return the current token as a slice of the content

        ***********************************************************************/

        final T[] get ()
        {
                return slice;
        }

        /***********************************************************************

                Return the associated buffer. This can be provided to
                a Reader or another Iterator ~ each will stay in synch
                with one another

        ***********************************************************************/

        final IBuffer getBuffer ()
        {
                return buffer;
        }

        /**********************************************************************

                Iterate over the set of tokens. This should really
                provide read-only access to the tokens, but D does
                not support that at this time

        **********************************************************************/

        int opApply (int delegate(inout T[]) dg)
        {
                T[]     token;
                int     result;
                
                while ((token = next).ptr)
                      {
                      result = dg (token);
                      if (result)
                          break;
                      }
                return result;
        }

        /***********************************************************************

                Locate the next token. Returns the token if found, null
                otherwise. Null indicates an end of stream condition. To
                sweep a conduit for lines using method next():
                
                ---
                auto lines = new LineIterator (new FileConduit("myfile"));
                while (lines.next)
                       Cout (lines.get).newline;
                ---

                Alternatively, we can extract one line from a conduit:
                
                ---
                auto line = (new LineIterator(new FileConduit("myfile"))).next;
                ---

                The difference between next() and foreach() is that the
                latter processes all tokens in one go, whereas the former
                processes in a piecemeal fashion. To wit:

                ---
                foreach (line; new LineIterator (new FileConduit("myfile"))
                         Cout(line).newline;
                ---

        ***********************************************************************/

        final T[] next ()
        {
                if (buffer.next (&scan) || slice.length > 0)
                    return get();
                return null;
        }

        /***********************************************************************

                Set the content of the current slice

        ***********************************************************************/

        protected final uint set (T* content, uint start, uint end)
        {
                slice = content [start .. end];
                return end;
        }

        /***********************************************************************

                Convert void[] from buffer into an appropriate array

        ***********************************************************************/

        protected final T[] convert (void[] data)
        {
                return cast(T[]) data [0 .. data.length & ~(T.sizeof-1)];
        }

        /***********************************************************************

                Called when a scanner fails to find a matching pattern.
                This may cause more content to be loaded, and a rescan
                initiated

        ***********************************************************************/

        protected final uint notFound (T[] content)
        {
                slice = content;
                return IConduit.Eof;
        }

        /***********************************************************************

                Invoked when a scanner matches a pattern. The provided
                value should be the index of the last element of the
                matching pattern, which is converted back to a void[]
                index.

        ***********************************************************************/

        protected final uint found (uint i)
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
}


