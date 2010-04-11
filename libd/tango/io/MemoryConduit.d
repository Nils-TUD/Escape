/*******************************************************************************

        copyright:      Copyright (c) 2006 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: Dec 2006

        author:         Kris

*******************************************************************************/

module tango.io.MemoryConduit;

public import tango.io.Conduit;

/*******************************************************************************

        Implements reading and writing of memory as a Conduit. Conduits
        are the primary means of accessing external data

*******************************************************************************/

class MemoryConduit : Conduit
{
        private void[] content;
        
        /***********************************************************************

                Construct a conduit with the given style and seek abilities.
                Conduits are either seekable or non-seekable.

        ***********************************************************************/

        this ()
        {
                super (Access.ReadWrite, false);
        }

        /***********************************************************************

                Return a preferred size for buffering conduit I/O

        ***********************************************************************/

        uint bufferSize ()
        {
                return 1024 * 4;
        }

        /***********************************************************************

                Return the name of this device

        ***********************************************************************/

        protected char[] getName()
        {
                return "<memory>";
        }
        
        /***********************************************************************

                Return the underlying OS handle of this Conduit

        ***********************************************************************/

        final Handle fileHandle ()
        {
                return 0;
        }

        /***********************************************************************

                Return the content held

        ***********************************************************************/

        final void[] slice ()
        {
                return content;
        }

        /***********************************************************************

                Read a chunk of bytes from the file into the provided
                array (typically that belonging to an IBuffer)

        ***********************************************************************/

        protected override uint reader (void[] dst)
        {
                if (content.length)
                   {
                   auto len = content.length;
                   if (len > dst.length)
                       len = dst.length;
                   
                   dst[0 .. len] = content [0 .. len];
                   content = content [len .. $];
                   return len;
                   }
                else
                   return Eof;
        }

        /***********************************************************************

                Write a chunk of bytes to the file from the provided
                array (typically that belonging to an IBuffer)

        ***********************************************************************/

        protected override uint writer (void[] src)
        {
                content ~= src;
                return src.length;
        }
}

