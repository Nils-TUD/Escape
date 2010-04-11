/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Initial release: March 2004
        
        author:         Kris

*******************************************************************************/

module tango.io.GrowBuffer;

private import  tango.io.Buffer;

public  import  tango.io.model.IBuffer;

/*******************************************************************************

        Subclass to provide support for content growth. This is handy when
        you want to keep a buffer around as a scratchpad.

*******************************************************************************/

class GrowBuffer : Buffer
{
        private uint increment;

        /***********************************************************************
        
                Create a GrowBuffer with the specified initial size.

        ***********************************************************************/

        this (uint size = 1024, uint increment = 1024)
        {
                super (size);

                assert (increment >= 32);
                this.increment = increment;
        }

        /***********************************************************************
        
                Create a GrowBuffer with the specified initial size.

        ***********************************************************************/

        this (IConduit conduit, uint size = 1024)
        {
                this (size, size);
                setConduit (conduit);
        }

        /***********************************************************************
        
                Read a chunk of data from the buffer, loading from the
                conduit as necessary. The specified number of bytes is
                loaded into the buffer, and marked as having been read 
                when the 'eat' parameter is set true. When 'eat' is set
                false, the read position is not adjusted.

                Returns the corresponding buffer slice when successful.

        ***********************************************************************/

        override void[] slice (uint size, bool eat = true)
        {   
                if (size > readable)
                   {
                   if (conduit is null)
                       error (underflow);

                   if (size + position_ > capacity_)
                       makeRoom (size);

                   // populate tail of buffer with new content
                   do {
                      if (fill(conduit) == IConduit.Eof)
                          error (eofRead);
                      } while (size > readable);
                   }

                uint i = position_;
                if (eat)
                    position_ += size;
                return data [i .. i + size];               
        }

        /***********************************************************************
        
                Append an array of data to this buffer. This is often used 
                in lieu of a Writer.

        ***********************************************************************/

        override IBuffer append (void *src, uint length)        
        {               
                if (length > writable)
                    makeRoom (length);

                copy (src, length);
                return this;
        }

        /***********************************************************************

                Try to fill the available buffer with content from the 
                specified conduit. In particular, we will never ask to 
                read less than 32 bytes ~ this permits conduit-filters 
                to operate within a known environment. 

                Returns the number of bytes read, or IConduit.Eof
        
        ***********************************************************************/

        override uint fill (IConduit conduit)
        {
                if (writable < 32)
                    makeRoom (increment);

                return write (&conduit.read);
        } 

        /***********************************************************************

                make some room in the buffer
                        
        ***********************************************************************/

        override uint makeRoom (uint size)
        {
                if (size < increment)
                    size = increment;

                capacity_ += size;
                data.length = capacity_;               
                return writable();
        }
}
