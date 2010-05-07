/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Mar 2004: Initial release
                        Dec 2006: Outback release

        authors:        Kris

*******************************************************************************/

module tango.io.stream.Buffered;

public import tango.io.model.IConduit;

private import tango.io.device.Conduit;

/******************************************************************************

******************************************************************************/

public alias BufferedInput  Bin;        /// shorthand alias
public alias BufferedOutput Bout;       /// ditto

/******************************************************************************

******************************************************************************/

extern (C)
{       
        int printf (char*, ...);
        private void * memcpy (void *dst, void *src, size_t);
}

/******************************************************************************

******************************************************************************/

private static char[] underflow = "input buffer is empty";
private static char[] eofRead   = "end-of-flow whilst reading";
private static char[] eofWrite  = "end-of-flow whilst writing";
private static char[] overflow  = "output buffer is full";


/*******************************************************************************

        Buffers the flow of data from a upstream input. A downstream 
        neighbour can locate and use this buffer instead of creating 
        another instance of their own. 

        (note that upstream is closer to the source, and downstream is
        further away)

*******************************************************************************/

class BufferedInput : InputFilter, InputBuffer
{
        alias flush             clear;          /// clear/flush are the same
        alias InputFilter.input input;          /// access the source 

        private void[]        data;             // the raw data buffer
        private size_t        index;            // current read position
        private size_t        extent;           // limit of valid content
        private size_t        dimension;        // maximum extent of content

        /***********************************************************************

                Ensure the buffer remains valid between method calls

        ***********************************************************************/

        invariant()
        {
                assert (index <= extent);
                assert (extent <= dimension);
        }

        /***********************************************************************

                Construct a buffer

                Params:
                stream = an input stream
                capacity = desired buffer capacity

                Remarks:
                Construct a Buffer upon the provided input stream.

        ***********************************************************************/

        this (InputStream stream)
        {
                assert (stream);
                this (stream, stream.conduit.bufferSize);
        }

        /***********************************************************************

                Construct a buffer

                Params:
                stream = an input stream
                capacity = desired buffer capacity

                Remarks:
                Construct a Buffer upon the provided input stream.

        ***********************************************************************/

        this (InputStream stream, size_t capacity)
        {
                set (new ubyte[capacity], 0);
                super (source = stream);
        }

        /***********************************************************************

                Attempt to share an upstream Buffer, and create an instance
                where there's not one available.

                Params:
                stream = an input stream

                Remarks:
                If an upstream Buffer instances is visible, it will be shared.
                Otherwise, a new instance is created based upon the bufferSize
                exposed by the stream endpoint (conduit).

        ***********************************************************************/

        static InputBuffer create (InputStream stream)
        {
                auto source = stream;
                auto conduit = source.conduit;
                while (cast(Mutator) source is null)
                      {
                      auto b = cast(InputBuffer) source;
                      if (b)
                          return b;
                      if (source is conduit)
                          break;
                      source = source.input;
                      assert (source);
                      }
                      
                return new BufferedInput (stream, conduit.bufferSize);
        }

        /***********************************************************************
        
                Place more data from the source stream into this buffer, and
                return the number of bytes added. This does not compress the
                current buffer content, so consider doing that explicitly.
                
                Returns: number of bytes added, which will be Eof when there
                         is no further input available. Zero is also a valid
                         response, meaning no data was actually added. 

        ***********************************************************************/

        final size_t populate ()
        {
                return writer (&input.read);
        }

        /***********************************************************************
        
                Return a void[] slice of the buffer from start to end, where
                end is exclusive

        ***********************************************************************/

        final void[] opSlice (size_t start, size_t end)
        {
                assert (start <= extent && end <= extent && start <= end);
                return data [start .. end];
        }

        /***********************************************************************

                Retrieve the valid content

                Returns:
                a void[] slice of the buffer

                Remarks:
                Return a void[] slice of the buffer, from the current position
                up to the limit of valid content. The content remains in the
                buffer for future extraction.

        ***********************************************************************/

        final void[] slice ()
        {
                return  data [index .. extent];
        }

        /***********************************************************************

                Access buffer content

                Params:
                size =  number of bytes to access
                eat =   whether to consume the content or not

                Returns:
                the corresponding buffer slice when successful, or
                null if there's not enough data available (Eof; Eob).

                Remarks:
                Read a slice of data from the buffer, loading from the
                conduit as necessary. The specified number of bytes is
                sliced from the buffer, and marked as having been read
                when the 'eat' parameter is set true. When 'eat' is set
                false, the read position is not adjusted.

                Note that the slice cannot be larger than the size of
                the buffer ~ use method fill(void[]) instead where you
                simply want the content copied, or use conduit.read()
                to extract directly from an attached conduit. Also note
                that if you need to retain the slice, then it should be
                .dup'd before the buffer is compressed or repopulated.

                Examples:
                ---
                // create a buffer with some content
                auto buffer = new Buffer ("hello world");

                // consume everything unread
                auto slice = buffer.slice (buffer.readable);
                ---

        ***********************************************************************/

        final void[] slice (size_t size, bool eat = true)
        {
                if (size > readable)
                   {
                   // make some space? This will try to leave as much content
                   // in the buffer as possible, such that entire records may
                   // be aliased directly from within.
                   if (size > (dimension - index))
                       if (size <= dimension)
                           compress;
                       else
                          conduit.error (underflow);

                   // populate tail of buffer with new content
                   do {
                      if (writer (&source.read) is Eof)
                          conduit.error (eofRead);
                      } while (size > readable);
                   }

                auto i = index;
                if (eat)
                    index += size;
                return data [i .. i + size];
        }

        /***********************************************************************

                Read directly from this buffer

                Params:
                dg = callback to provide buffer access to

                Returns:
                Returns whatever the delegate returns.

                Remarks:
                Exposes the raw data buffer at the current _read position. The
                delegate is provided with a void[] representing the available
                data, and should return zero to leave the current _read position
                intact.

                If the delegate consumes data, it should return the number of
                bytes consumed; or IConduit.Eof to indicate an error.

        ***********************************************************************/

        final size_t reader (size_t delegate (void[]) dg)
        {
                auto count = dg (data [index..extent]);

                if (count != Eof)
                   {
                   index += count;
                   assert (index <= extent);
                   }
                return count;
        }

        /***********************************************************************

                Write into this buffer

                Params:
                dg = the callback to provide buffer access to

                Returns:
                Returns whatever the delegate returns.

                Remarks:
                Exposes the raw data buffer at the current _write position,
                The delegate is provided with a void[] representing space
                available within the buffer at the current _write position.

                The delegate should return the appropriate number of bytes
                if it writes valid content, or IConduit.Eof on error.

        ***********************************************************************/

        public size_t writer (size_t delegate (void[]) dg)
        {
                auto count = dg (data [extent..dimension]);

                if (count != Eof)
                   {
                   extent += count;
                   assert (extent <= dimension);
                   }
                return count;
        }

        /***********************************************************************

                Transfer content into the provided dst

                Params:
                dst = destination of the content

                Returns:
                return the number of bytes read, which may be less than
                dst.length. Eof is returned when no further content is
                available.

                Remarks:
                Populates the provided array with content. We try to
                satisfy the request from the buffer content, and read
                directly from an attached conduit when the buffer is
                empty.

        ***********************************************************************/

        final override size_t read (void[] dst)
        {
                size_t content = readable;
                if (content)
                   {
                   if (content >= dst.length)
                       content = dst.length;

                   // transfer buffer content
                   dst [0 .. content] = data [index .. index + content];
                   index += content;
                   }
                else
                   // pathological cases read directly from conduit
                   if (dst.length > dimension)
                       content = source.read (dst);
                   else
                      {
                      if (writable is 0)
                          index = extent = 0;  // same as clear, without call-chain

                      // keep buffer partially populated
                      if ((content = writer (&source.read)) != Eof && content > 0)
                           content = read (dst);
                      }
                return content;
        }

        /**********************************************************************

                Fill the provided buffer. Returns the number of bytes
                actually read, which will be less that dst.length when
                Eof has been reached and Eof thereafter.

                Params:
                dst = where data should be placed 
                exact = whether to throw an exception when dst is not
                        filled (an Eof occurs first). Defaults to false

        **********************************************************************/

        final size_t fill (void[] dst, bool exact = false)
        {
                size_t len = 0;

                while (len < dst.length)
                      {
                      size_t i = read (dst [len .. $]);
                      if (i is Eof)
                         {
                         if (exact && len < dst.length)
                             conduit.error (eofRead);
                         return (len > 0) ? len : Eof;
                         }
                      len += i;
                      }
                return len;
        }

        /***********************************************************************

                Move the current read location

                Params:
                size = the number of bytes to move

                Returns:
                Returns true if successful, false otherwise.

                Remarks:
                Skip ahead by the specified number of bytes, streaming from
                the associated conduit as necessary.

                Can also reverse the read position by 'size' bytes, when size
                is negative. This may be used to support lookahead operations.
                Note that a negative size will fail where there is not sufficient
                content available in the buffer (can't _skip beyond the beginning).

        ***********************************************************************/

        final bool skip (int size)
        {
                if (size < 0)
                   {
                   size = -size;
                   if (index >= size)
                      {
                      index -= size;
                      return true;
                      }
                   return false;
                   }
                return slice(size) !is null;
        }

        /***********************************************************************

                Move the current read location

        ***********************************************************************/

        final override long seek (long offset, Anchor start = Anchor.Begin)
        {
                if (start is Anchor.Current)
                   {
                   // handle this specially because we know this is
                   // buffered - we should take into account the buffer
                   // position when seeking
                   offset -= readable;
                   auto bpos = offset + limit;

                   if (bpos >= 0 && bpos < limit)
                      {
                      // the new position is within the current
                      // buffer, skip to that position.
                      skip (cast(int) bpos - cast(int) position);

                      // see if we can return a valid offset
                      auto pos = source.seek (0, Anchor.Current);
                      if (pos != Eof)
                          return pos - readable;
                      return Eof;
                      }
                   // else, position is outside the buffer. Do a real
                   // seek using the adjusted position.
                   }

                clear;
                return source.seek (offset, start);
        }

        /***********************************************************************

                Iterator support

                Params:
                scan = the delegate to invoke with the current content

                Returns:
                Returns true if a token was isolated, false otherwise.

                Remarks:
                Upon success, the delegate should return the byte-based
                index of the consumed pattern (tail end of it). Failure
                to match a pattern should be indicated by returning an
                Eof

                Each pattern is expected to be stripped of the delimiter.
                An end-of-file condition causes trailing content to be
                placed into the token. Requests made beyond Eof result
                in empty matches (length is zero).

                Note that additional iterator and/or reader instances
                will operate in lockstep when bound to a common buffer.

        ***********************************************************************/

        final bool next (size_t delegate (void[]) scan)
        {
                while (reader(scan) is Eof)
                      {
                      // did we start at the beginning?
                      if (position)
                          // yep - move partial token to start of buffer
                          compress;
                      else
                         // no more space in the buffer?
                         if (writable is 0)
                             conduit.error ("BufferedInput.next :: input buffer is full");

                      // read another chunk of data
                      if (writer(&source.read) is Eof)
                          return false;
                      }
                return true;
        }

        /***********************************************************************

                Reserve the specified space within the buffer, compressing
                existing content as necessary to make room

                Returns the current read point, after compression if that
                was required

        ***********************************************************************/

        final size_t reserve (size_t space)
        {       
                assert (space < dimension);

                if ((dimension - index) < space)
                     compress;
                return index;
        }

        /***********************************************************************

                Compress buffer space

                Returns:
                the buffer instance

                Remarks:
                If we have some data left after an export, move it to
                front-of-buffer and set position to be just after the
                remains. This is for supporting certain conduits which
                choose to write just the initial portion of a request.

                Limit is set to the amount of data remaining. Position
                is always reset to zero.

        ***********************************************************************/

        final BufferedInput compress ()
        {       
                auto r = readable;

                if (index > 0 && r > 0)
                    // content may overlap ...
                    memcpy (&data[0], &data[index], r);

                index = 0;
                extent = r;
                return this;
        }

        /***********************************************************************

                Drain buffer content to the specific conduit

                Returns:
                Returns the number of bytes written, or Eof

                Remarks:
                Write as much of the buffer that the associated conduit
                can consume. The conduit is not obliged to consume all
                content, so some may remain within the buffer.

        ***********************************************************************/

        final size_t drain (OutputStream dst)
        {
                assert (dst);

                size_t ret = reader (&dst.write);
                compress;
                return ret;
        }

        /***********************************************************************

                Access buffer limit

                Returns:
                Returns the limit of readable content within this buffer.

                Remarks:
                Each buffer has a capacity, a limit, and a position. The
                capacity is the maximum content a buffer can contain, limit
                represents the extent of valid content, and position marks
                the current read location.

        ***********************************************************************/

        final size_t limit ()
        {
                return extent;
        }

        /***********************************************************************

                Access buffer capacity

                Returns:
                Returns the maximum capacity of this buffer

                Remarks:
                Each buffer has a capacity, a limit, and a position. The
                capacity is the maximum content a buffer can contain, limit
                represents the extent of valid content, and position marks
                the current read location.

        ***********************************************************************/

        final size_t capacity ()
        {
                return dimension;
        }

        /***********************************************************************

                Access buffer read position

                Returns:
                Returns the current read-position within this buffer

                Remarks:
                Each buffer has a capacity, a limit, and a position. The
                capacity is the maximum content a buffer can contain, limit
                represents the extent of valid content, and position marks
                the current read location.

        ***********************************************************************/

        final size_t position ()
        {
                return index;
        }

        /***********************************************************************

                Available content

                Remarks:
                Return count of _readable bytes remaining in buffer. This is
                calculated simply as limit() - position()

        ***********************************************************************/

        final size_t readable ()
        {
                return extent - index;
        }

        /***********************************************************************

                Cast to a target type without invoking the wrath of the
                runtime checks for misalignment. Instead, we truncate the
                array length

        ***********************************************************************/

        static T[] convert(T)(void[] x)
        {
                return (cast(T*) x.ptr) [0 .. (x.length / T.sizeof)];
        }

        /***********************************************************************

                Clear buffer content

                Remarks:
                Reset 'position' and 'limit' to zero. This effectively
                clears all content from the buffer.

        ***********************************************************************/

        final override BufferedInput flush ()
        {
                index = extent = 0;

                // clear the filter chain also
                if (source) 
                    super.flush;
                return this;
        }

        /***********************************************************************

                Set the input stream

        ***********************************************************************/

        final void input (InputStream source)
        {
                this.source = source;
        }

        /***********************************************************************

                Load the bits from a stream, up to an indicated length, and 
                return them all in an array. The function may consume more
                than the indicated size where additional data is available
                during a block read operation, but will not wait for more 
                than specified. An Eof terminates the operation.

                Returns an array representing the content, and throws
                IOException on error
                              
        ***********************************************************************/

        final override void[] load (size_t max = size_t.max)
        {
                load (super.input, super.conduit.bufferSize, max);
                return slice;
        }
                
        /***********************************************************************

                Import content from the specified conduit, expanding 
                as necessary up to the indicated maximum or until an 
                Eof occurs

                Returns the number of bytes contained.
        
        ***********************************************************************/

        private size_t load (InputStream src, size_t increment, size_t max)
        {
                size_t  len,
                        count;
                
                // make some room
                compress;

                // explicitly resize?
                if (max != max.max)
                    if ((len = writable) < max)
                         increment = max - len;
                        
                while (count < max)
                      {
                      if (! writable)
                         {
                         dimension += increment;
                         data.length = dimension;               
                         }
                      if ((len = writer(&src.read)) is Eof)
                           break;
                      else
                         count += len;
                      }
                return count;
        }       

        /***********************************************************************

                Reset the buffer content

                Params:
                data =          the backing array to buffer within
                readable =      the number of bytes within data considered
                                valid

                Returns:
                the buffer instance

                Remarks:
                Set the backing array with some content readable. Writing
                to this will either flush it to an associated conduit, or
                raise an Eof condition. Use clear() to reset the content
                (make it all writable).

        ***********************************************************************/

        private final BufferedInput set (void[] data, size_t readable)
        {
                this.data = data;
                this.extent = readable;
                this.dimension = data.length;

                // reset to start of input
                this.index = 0;

                return this;
        }

        /***********************************************************************

                Available space

                Remarks:
                Return count of _writable bytes available in buffer. This is
                calculated simply as capacity() - limit()

        ***********************************************************************/

        private final size_t writable ()
        {
                return dimension - extent;
        }
}



/*******************************************************************************

        Buffers the flow of data from a upstream output. A downstream 
        neighbour can locate and use this buffer instead of creating 
        another instance of their own.

        (note that upstream is closer to the source, and downstream is
        further away)

        Don't forget to flush() buffered content before closing.

*******************************************************************************/

class BufferedOutput : OutputFilter, OutputBuffer
{
        alias OutputFilter.output output;       /// access the sink

        private void[]        data;             // the raw data buffer
        private size_t        index;            // current read position
        private size_t        extent;           // limit of valid content
        private size_t        dimension;        // maximum extent of content

        /***********************************************************************

                Ensure the buffer remains valid between method calls

        ***********************************************************************/

        invariant()
        {
                assert (index <= extent);
                assert (extent <= dimension);
        }

        /***********************************************************************

                Construct a buffer

                Params:
                stream = an input stream
                capacity = desired buffer capacity

                Remarks:
                Construct a Buffer upon the provided input stream.

        ***********************************************************************/

        this (OutputStream stream)
        {
                assert (stream);
                this (stream, stream.conduit.bufferSize);
        }

        /***********************************************************************

                Construct a buffer

                Params:
                stream = an input stream
                capacity = desired buffer capacity

                Remarks:
                Construct a Buffer upon the provided input stream.

        ***********************************************************************/

        this (OutputStream stream, size_t capacity)
        {
                set (new ubyte[capacity], 0);
                super (sink = stream);
        }

        /***********************************************************************

                Attempts to share an upstream BufferedOutput, and creates a new
                instance where there's not a shared one available.

                Params:
                stream = an output stream

                Remarks:
                Where an upstream instance is visible it will be returned.
                Otherwise, a new instance is created based upon the bufferSize
                exposed by the associated conduit

        ***********************************************************************/

        static OutputBuffer create (OutputStream stream)
        {
                auto sink = stream;
                auto conduit = sink.conduit;
                while (cast(Mutator) sink is null)
                      {
                      auto b = cast(OutputBuffer) sink;
                      if (b)
                          return b;
                      if (sink is conduit)
                          break;
                      sink = sink.output;
                      assert (sink);
                      }
                      
                return new BufferedOutput (stream, conduit.bufferSize);
        }

        /***********************************************************************

                Retrieve the valid content

                Returns:
                a void[] slice of the buffer

                Remarks:
                Return a void[] slice of the buffer, from the current position
                up to the limit of valid content. The content remains in the
                buffer for future extraction.

        ***********************************************************************/

        final void[] slice ()
        {
                return data [index .. extent];
        }

        /***********************************************************************

                Emulate OutputStream.write()

                Params:
                src = the content to write

                Returns:
                return the number of bytes written, which may be less than
                provided (conceptually).

                Remarks:
                Appends src content to the buffer, flushing to an attached
                conduit as necessary. An IOException is thrown upon write
                failure.

        ***********************************************************************/

        final override size_t write (void[] src)
        {
                append (src.ptr, src.length);
                return src.length;
        }

        /***********************************************************************

                Append content

                Params:
                src = the content to _append

                Returns a chaining reference if all content was written.
                Throws an IOException indicating eof or eob if not.

                Remarks:
                Append an array to this buffer, and flush to the
                conduit as necessary. This is often used in lieu of
                a Writer.

        ***********************************************************************/

        final BufferedOutput append (void[] src)
        {
                return append (src.ptr, src.length);
        }

        /***********************************************************************

                Append content

                Params:
                src = the content to _append
                length = the number of bytes in src

                Returns a chaining reference if all content was written.
                Throws an IOException indicating eof or eob if not.

                Remarks:
                Append an array to this buffer, and flush to the
                conduit as necessary. This is often used in lieu of
                a Writer.

        ***********************************************************************/

        final BufferedOutput append (void* src, size_t length)
        {
                if (length > writable)
                   {
                   flush;

                   // check for pathological case
                   if (length > dimension)
                       do {
                          auto written = sink.write (src [0 .. length]);
                          if (written is Eof)
                              conduit.error (eofWrite);
                          length -= written;
                          src += written; 
                          } while (length > dimension);
                    }

                // avoid "out of bounds" test on zero length
                if (length)
                   {
                   // content may overlap ...
                   memcpy (&data[extent], src, length);
                   extent += length;
                   }
                return this;
        }

        /***********************************************************************

                Available space

                Remarks:
                Return count of _writable bytes available in buffer. This is
                calculated as capacity() - limit()

        ***********************************************************************/

        final size_t writable ()
        {
                return dimension - extent;
        }

        /***********************************************************************

                Access buffer limit

                Returns:
                Returns the limit of readable content within this buffer.

                Remarks:
                Each buffer has a capacity, a limit, and a position. The
                capacity is the maximum content a buffer can contain, limit
                represents the extent of valid content, and position marks
                the current read location.

        ***********************************************************************/

        final size_t limit ()
        {
                return extent;
        }

        /***********************************************************************

                Access buffer capacity

                Returns:
                Returns the maximum capacity of this buffer

                Remarks:
                Each buffer has a capacity, a limit, and a position. The
                capacity is the maximum content a buffer can contain, limit
                represents the extent of valid content, and position marks
                the current read location.

        ***********************************************************************/

        final size_t capacity ()
        {
                return dimension;
        }

        /***********************************************************************

                Truncate buffer content

                Remarks:
                Truncate the buffer within its extent. Returns true if
                the new length is valid, false otherwise.

        ***********************************************************************/

        final bool truncate (size_t length)
        {
                if (length <= data.length)
                   {
                   extent = length;
                   return true;
                   }
                return false;
        }

        /***********************************************************************

                Cast to a target type without invoking the wrath of the
                runtime checks for misalignment. Instead, we truncate the
                array length

        ***********************************************************************/

        static T[] convert(T)(void[] x)
        {
                return (cast(T*) x.ptr) [0 .. (x.length / T.sizeof)];
        }

        /***********************************************************************

                Flush all buffer content to the specific conduit

                Remarks:
                Flush the contents of this buffer. This will block until
                all content is actually flushed via the associated conduit,
                whereas drain() will not.

                Throws an IOException on premature Eof.

        ***********************************************************************/

        final override BufferedOutput flush ()
        {
                while (readable > 0)
                      {
                      auto ret = reader (&sink.write);
                      if (ret is Eof)
                          conduit.error (eofWrite);
                      }

                // flush the filter chain also
                clear;
                super.flush;
                return this;
        }

        /***********************************************************************

                Copy content via this buffer from the provided src
                conduit.

                Remarks:
                The src conduit has its content transferred through
                this buffer via a series of fill & drain operations,
                until there is no more content available. The buffer
                content should be explicitly flushed by the caller.

                Throws an IOException on premature eof

        ***********************************************************************/

        final override BufferedOutput copy (InputStream src, size_t max = -1)
        {
                size_t chunk,
                       copied;

                while (copied < max && (chunk = writer(&src.read)) != Eof)
                      {
                      copied += chunk;

                      // don't drain until we actually need to
                      if (writable is 0)
                          if (drain(sink) is Eof)
                              conduit.error (eofWrite);
                      }
                return this;
        }

        /***********************************************************************

                Drain buffer content to the specific conduit

                Returns:
                Returns the number of bytes written, or Eof

                Remarks:
                Write as much of the buffer that the associated conduit
                can consume. The conduit is not obliged to consume all
                content, so some may remain within the buffer.

        ***********************************************************************/

        final size_t drain (OutputStream dst)
        {
                assert (dst);

                size_t ret = reader (&dst.write);
                compress;
                return ret;
        }

        /***********************************************************************

                Clear buffer content

                Remarks:
                Reset 'position' and 'limit' to zero. This effectively
                clears all content from the buffer.

        ***********************************************************************/

        final BufferedOutput clear ()
        {
                index = extent = 0;
                return this;
        }

        /***********************************************************************

                Set the output stream

        ***********************************************************************/

        final void output (OutputStream sink)
        {
                this.sink = sink;
        }

        /***********************************************************************

                Seek within this stream. Any and all buffered output is 
                disposed before the upstream is invoked. Use an explicit
                flush() to emit content prior to seeking

        ***********************************************************************/

        final override long seek (long offset, Anchor start = Anchor.Begin)
        {       
                clear;
                return super.seek (offset, start);
        }

        /***********************************************************************

                Write into this buffer

                Params:
                dg = the callback to provide buffer access to

                Returns:
                Returns whatever the delegate returns.

                Remarks:
                Exposes the raw data buffer at the current _write position,
                The delegate is provided with a void[] representing space
                available within the buffer at the current _write position.

                The delegate should return the appropriate number of bytes
                if it writes valid content, or Eof on error.

        ***********************************************************************/

        final size_t writer (size_t delegate (void[]) dg)
        {
                auto count = dg (data [extent..dimension]);

                if (count != Eof)
                   {
                   extent += count;
                   assert (extent <= dimension);
                   }
                return count;
        }

        /***********************************************************************

                Read directly from this buffer

                Params:
                dg = callback to provide buffer access to

                Returns:
                Returns whatever the delegate returns.

                Remarks:
                Exposes the raw data buffer at the current _read position. The
                delegate is provided with a void[] representing the available
                data, and should return zero to leave the current _read position
                intact.

                If the delegate consumes data, it should return the number of
                bytes consumed; or Eof to indicate an error.

        ***********************************************************************/

        private final size_t reader (size_t delegate (void[]) dg)
        {
                auto count = dg (data [index..extent]);

                if (count != Eof)
                   {
                   index += count;
                   assert (index <= extent);
                   }
                return count;
        }

        /***********************************************************************

                Available content

                Remarks:
                Return count of _readable bytes remaining in buffer. This is
                calculated simply as limit() - position()

        ***********************************************************************/

        private final size_t readable ()
        {
                return extent - index;
        }

        /***********************************************************************

                Reset the buffer content

                Params:
                data =     the backing array to buffer within
                readable = the number of bytes within data considered
                           valid

                Returns:
                the buffer instance

                Remarks:
                Set the backing array with some content readable. Writing
                to this will either flush it to an associated conduit, or
                raise an Eof condition. Use clear() to reset the content
                (make it all writable).

        ***********************************************************************/

        private final BufferedOutput set (void[] data, size_t readable)
        {
                this.data = data;
                this.extent = readable;
                this.dimension = data.length;

                // reset to start of input
                this.index = 0;

                return this;
        }

        /***********************************************************************

                Compress buffer space

                Returns:
                the buffer instance

                Remarks:
                If we have some data left after an export, move it to
                front-of-buffer and set position to be just after the
                remains. This is for supporting certain conduits which
                choose to write just the initial portion of a request.

                Limit is set to the amount of data remaining. Position
                is always reset to zero.

        ***********************************************************************/

        private final BufferedOutput compress ()
        {       
                size_t r = readable;

                if (index > 0 && r > 0)
                    // content may overlap ...
                    memcpy (&data[0], &data[index], r);

                index = 0;
                extent = r;
                return this;
        }
}



/******************************************************************************

******************************************************************************/

debug (Buffered)
{
        void main()
        {
                auto input = new BufferedInput (null);
                auto output = new BufferedOutput (null);
        }
}
