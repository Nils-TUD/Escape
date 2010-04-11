/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Mar 2004: Initial release
                        Dec 2006: Outback release
        
        author:         Kris

*******************************************************************************/

module tango.io.model.IBuffer;

private import tango.io.model.IConduit;

/*******************************************************************************

        The premise behind this IO package is as follows:

        A central concept is that of a buffer. Each buffer acts
        as a queue (line) where items are removed from the front
        and new items are added to the back. Buffers are modeled 
        by tango.io.model.IBuffer, and a concrete implementation 
        is provided by this class.
        
        Buffers can be read and written directly, but Readers, 
        Iterators, and/or Writers are often leveraged to apply 
        structure to what might otherwise be simple raw data. 

        Readers & writers are bound to a buffer; often the same 
        buffer. It's also perfectly legitimate to bind multiple 
        readers to the same buffer; they will access buffer 
        content serially as one would expect. This also applies to
        multiple writers on the same buffer. Readers and writers
        support three styles of IO: put/get, the C++ style << &
        >> operators, and the () whisper style. All operations 
        can be chained.
        
        Any class can be made compatable with the reader/writer
        framework by implementing the IReadable and/or IWritable 
        interfaces. Each of these specify just a single method.
        Once compatable, the class can simply be passed to the 
        reader/writer as if it were native data. Structs can be
        made compatible in a similar manner by exposing an
        appropriate function signature.
        
        Buffers may also be tokenized by applying an Iterator. 
        This can be handy when one is dealing with text input, 
        and/or the content suits a more fluid format than most 
        typical readers & writers support. Iterator tokens
        are mapped directly onto buffer content (sliced), making 
        them quite efficient in practice. Like Readers, multiple
        iterators can be mapped onto a common buffer; access is
        serialized in a similar fashion.

        Conduits provide virtualized access to external content,
        and represent things like files or Internet connections.
        They are just a different kind of stream. Conduits are
        modelled by tango.io.model.IConduit, and implemented via
        classes FileConduit, SocketConduit, ConsoleConduit, and 
        so on. Additional conduit varieties are easy to construct: 
        one either subclasses tango.io.Conduit, or implements 
        tango.io.model.IConduit ~ depending upon which is the most 
        convenient to use. Each conduit reads and writes from/to 
        a buffer in big chunks (typically the entire buffer).

        Conduits may have one or more filters attached. These 
        will process content as it flows back and forth across
        the conduit. Examples of filters include compression, utf
        transcoding, and endian transformation. These filters
        apply to the entire scope of the conduit, rather than
        being specific to one data-type or another. Specific 
        data-type transformations are applied by readers and 
        writers instead, and include operations such as 
        endian-conversion.

        Buffers are sometimes memory-only, in which case there
        is nothing left to do when a reader (or iterator) hits
        end of buffer conditions. Other buffers are themselves 
        bound to a Conduit. When this is the case, a reader will 
        eventually cause the buffer to reload via its associated 
        conduit. Previous buffer content will thus be lost. The
        same approach is applied to writers, whereby they flush 
        the content of a full buffer to a bound conduit before 
        continuing. Another variation is that of a memory-mapped
        buffer, whereby the buffer content is mapped directly to
        virtual memory exposed via the OS. This can be used to 
        address large files as an array of content.

        Readers & writers may have a protocol attached. The role
        of a protocol is to format (and parse) data according to
        the specific protocol, and there are both binary and text
        oriented protocol to select from. 

        Direct buffer manipulation typically involves appending, 
        as in the following example:
        ---
        // create a small buffer
        auto buf = new Buffer (256);

        auto foo = "to write some D";

        // append some text directly to it
        buf.append("now is the time for all good men ").append(foo);
        ---

        Alternatively, one might use a Writer to append the buffer. 
        This is an example of the 'whisper' style supported by the
        IO package:
        ---
        auto write = new Writer (new Buffer(256));
        write ("now is the time for all good men "c) (foo);
        ---

        One might use a GrowBuffer instead, where one wishes to append
        beyond the specified size. 
        
        A common usage of a buffer is in conjunction with a conduit, 
        such as FileConduit. Each conduit exposes a preferred-size for 
        its associated buffers, utilized during buffer construction:
        ---
        auto file = new FileConduit ("file.name");
        auto buf = new Buffer (file);
        ---

        However, this is typically hidden by higher level constructors 
        such as those of Reader and Writer derivitives. For example:
        ---
        auto file = new FileConduit ("file.name");
        auto read = new Reader (file);
        ---

        There is indeed a buffer between the Reader and Conduit, but 
        explicit construction is unecessary in common cases. See both 
        Reader and Writer for examples of formatted IO.

        Stdout is a more specialized converter, attached to a conduit
        representing the console. However, all conduit operations are
        legitimate on Stdout and Stderr. For example:
        ---
        Stdout.conduit.copy (new FileConduit ("readme.txt"));
        ---

        Stdout also has support for both text conversions and formatted 
        output:
        ---
        Stdout ("now is the time for ") (3) (" good men ") (foo);

        Stdout.format ("now is the time for {0} good men {1}", 3, foo);
        ---

        Stdout is attached to a specific buffer, which in turn is attached 
        to a specific conduit. This buffer is known as Cout, and is attached 
        to a conduit representing the console. Cout can be used directly, 
        bypassing the Stdout formatting layer if so desired (it is lightweight)
        
        Cout has relatives named Cerr and Cin, which are attached to 
        the corresponding console conduits. Writer Stderr, and reader 
        Stdin are mapped onto Cerr and Cin respectively, ensuring 
        console IO is buffered in one common area. 
        ---
        Cout ("what is your name?") ();
        Cout ("hello ")(Cin.get).newline;
        ---

        An Iterator is constructed in a similar manner to a Reader; you
        provide it with a buffer or a conduit. There's a variety of 
        iterators available in the tango.text package, and they are each
        templated for utf8, utf16, and utf32 ~ this example uses a line 
        iterator to sweep a text file:
        ---
        auto file = new FileConduit ("file.name");
        foreach (line; new LineIterator (file))
                 Cout(line).newline;
        ---                 

        Buffers are useful for many purposes within Tango, but there
        are times when it may be more appropriate to sidestep them. For 
        such cases, conduit derivatives (such as FileConduit) support 
        direct array-based IO via a pair of read() and write() methods. 
        These alternate methods will also invoke any attached filters.

*******************************************************************************/

abstract class IBuffer // could be an interface, but that causes poor codegen
{
        typedef uint delegate (void* dst, uint count, uint type) Converter;

        alias append opCall;
        alias flush  opCall;
      
        /***********************************************************************
                
                Return the backing array

        ***********************************************************************/

        abstract void[] getContent ();

        /***********************************************************************
        
                Return a void[] slice of the buffer up to the limit of
                valid content.

        ***********************************************************************/

        abstract void[] slice ();

        /***********************************************************************
        
                Set the backing array with all content readable. Writing
                to this will either flush it to an associated conduit, or
                raise an Eof condition. Use IBuffer.clear() to reset the
                content (make it all writable).

        ***********************************************************************/

        abstract IBuffer setContent (void[] data);

        /***********************************************************************
        
                Set the backing array with some content readable. Writing
                to this will either flush it to an associated conduit, or
                raise an Eof condition. Use IBuffer.clear() to reset the
                content (make it all writable).

        ***********************************************************************/

        abstract IBuffer setContent (void[] data, uint readable);

        /***********************************************************************

                Append an array of data into this buffer, and flush to the
                conduit as necessary. Returns a chaining reference if all 
                data was written; throws an IOException indicating eof or 
                eob if not.

                This is often used in lieu of a Writer.

        ***********************************************************************/

        abstract IBuffer append (void* content, uint length);

        /***********************************************************************

                Append an array of data into this buffer, and flush to the
                conduit as necessary. Returns a chaining reference if all 
                data was written; throws an IOException indicating eof or 
                eob if not.

                This is often used in lieu of a Writer.

        ***********************************************************************/

        abstract IBuffer append (void[] content);

        /***********************************************************************
        
                Append another buffer to this one, and flush to the
                conduit as necessary. Returns a chaining reference if all 
                data was written; throws an IOException indicating eof or 
                eob if not.

                This is often used in lieu of a Writer.

        ***********************************************************************/

        abstract IBuffer append (IBuffer other);

        /***********************************************************************
        
                Consume content from a producer

                Params:
                dg = the producing delegate, which should itself accept
                a callback for consuming char[] content

                Returns:
                Returns a chaining reference if all content was written. 
                Throws an IOException indicating eof or eob if not.

                Remarks:
                Invokes the provided 

                This is often used in lieu of a Writer, and enables simple
                classes, such as FilePath and Uri, to emit content directly
                into a buffer (thus avoiding potential for heap activity)

                Examples:
                ---
                auto uri = new Uri (someuri);

                uri.produce (&buffer.consume);
                ---

        ***********************************************************************/

        abstract void consume (void[] src);

        /***********************************************************************

                Read a chunk of data from the buffer, loading from the
                conduit as necessary. The requested number of bytes are
                loaded into the buffer, and marked as having been read 
                when the 'eat' parameter is set true. When 'eat' is set
                false, the read position is not adjusted.

                Returns the corresponding buffer slice when successful, 
                or null if there's not enough data available (Eof; Eob).

        ***********************************************************************/

        abstract void[] slice (uint size, bool eat = true);

        /***********************************************************************

                Access buffer content

                Params: 
                dst = destination of the content

                Returns:
                return the number of bytes read, which will be less than
                dst.length when the content has been consumed (Eof, Eob)
                and zero thereafter.

                Remarks:
                Fill the provided array with content. We try to satisfy 
                the request from the buffer content, and read directly
                from an attached conduit where more is required.

        ***********************************************************************/

        abstract uint fill (void[] dst);

        /***********************************************************************

                Access buffer content

                Params: 
                dst = destination of the content
                bytes = size of dst

                Returns:
                A reference to the populated content

                Remarks:
                Fill the provided array with content. We try to satisfy 
                the request from the buffer content, and read directly
                from an attached conduit where more is required.

        ***********************************************************************/

        void[] extract (void* dst, uint bytes);
        
        /***********************************************************************

                Exposes the raw data buffer at the current write position, 
                The delegate is provided with a void[] representing space
                available within the buffer at the current write position.

                The delegate should return the approriate number of bytes 
                if it writes valid content, or IConduit.Eof on error.

                Returns whatever the delegate returns.

        ***********************************************************************/

        abstract uint write (uint delegate (void[]) writer);

        /***********************************************************************

                Exposes the raw data buffer at the current read position. The
                delegate is provided with a void[] representing the available
                data, and should return zero to leave the current read position
                intact. 
                
                If the delegate consumes data, it should return the number of 
                bytes consumed; or IConduit.Eof to indicate an error.

                Returns whatever the delegate returns.

        ***********************************************************************/

        abstract uint read (uint delegate (void[]) reader);

        /***********************************************************************

                If we have some data left after an export, move it to 
                front-of-buffer and set position to be just after the 
                remains. This is for supporting certain conduits which 
                choose to write just the initial portion of a request.
                            
                Limit is set to the amount of data remaining. Position 
                is always reset to zero.

        ***********************************************************************/

        abstract IBuffer compress ();

        /***********************************************************************
        
                Skip ahead by the specified number of bytes, streaming from 
                the associated conduit as necessary.
        
                Can also reverse the read position by 'size' bytes. This may
                be used to support lookahead-type operations.

                Returns true if successful, false otherwise.

        ***********************************************************************/

        abstract bool skip (int size);

        /***********************************************************************

                Support for tokenizing iterators. 
                
                Upon success, the delegate should return the byte-based 
                index of the consumed pattern (tail end of it). Failure
                to match a pattern should be indicated by returning an
                IConduit.Eof

                Each pattern is expected to be stripped of the delimiter.
                An end-of-file condition causes trailing content to be 
                placed into the token. Requests made beyond Eof result
                in empty matches (length == zero).

                Note that additional iterator and/or reader instances
                will stay in lockstep when bound to a common buffer.

                Returns true if a token was isolated, false otherwise.

        ***********************************************************************/

        abstract bool next (uint delegate (void[]));

        /***********************************************************************

                Try to fill the available buffer with content from the 
                specified conduit. In particular, we will never ask to 
                read less than 32 bytes. This permits conduit-filters 
                to operate within a known environment.

                Returns the number of bytes read, or throws an underflow
                error if there nowhere to read from
        
        ***********************************************************************/

        abstract uint fill ();

        /***********************************************************************

                Try to fill the available buffer with content from the 
                specified conduit. In particular, we will never ask to 
                read less than 32 bytes. This permits conduit-filters 
                to operate within a known environment.

                Returns the number of bytes read, or Conduit.Eof
        
        ***********************************************************************/

        abstract uint fill (IConduit conduit);

        /***********************************************************************

                Write as much of the buffer that the associated conduit
                can consume.

                Returns the number of bytes written, or Conduit.Eof
        
        ***********************************************************************/

        abstract uint drain ();

        /***********************************************************************
        
                flush the contents of this buffer to the related conduit.
                Throws an IOException on premature eof.

        ***********************************************************************/

        abstract IBuffer flush ();

        /***********************************************************************
        
                Reset position and limit to zero.

        ***********************************************************************/

        abstract IBuffer clear ();               

        /***********************************************************************
        
                Truncate the buffer within its extend. Returns true if
                the new 'extent' is valid, false otherwise.

        ***********************************************************************/

        abstract bool truncate (uint extent);

        /***********************************************************************
        
                return count of readable bytes remaining in buffer. This is 
                calculated simply as limit() - position()

        ***********************************************************************/

        abstract uint readable ();               

        /***********************************************************************
        
                Return count of writable bytes available in buffer. This is 
                calculated simply as capacity() - limit()

        ***********************************************************************/

        abstract uint writable ();

        /***********************************************************************
        
                returns the limit of readable content within this buffer

        ***********************************************************************/

        abstract uint limit ();               

        /***********************************************************************
        
                returns the total capacity of this buffer

        ***********************************************************************/

        abstract uint capacity ();               

        /***********************************************************************
        
                returns the current position within this buffer

        ***********************************************************************/

        abstract uint position ();               

        /***********************************************************************

                make some room in the buffer
                        
        ***********************************************************************/

        abstract uint makeRoom (uint space);

        /***********************************************************************
        
                Returns the conduit associated with this buffer. Returns 
                null if the buffer is purely memory based; that is, it's
                not backed by some external conduit.

                Buffers do not require a conduit to operate, but it can
                be convenient to associate one. For example, the IReader
                and IWriter classes use this to import/export content as
                necessary.

        ***********************************************************************/

        abstract IConduit conduit ();               

        /***********************************************************************
        
                Sets the external conduit associated with this buffer.

                Buffers do not require an external conduit to operate, but 
                it can be convenient to associate one. For example, methods
                read and write use it to import/export content as necessary.

        ***********************************************************************/

        abstract IBuffer setConduit (IConduit conduit);

        /***********************************************************************
        
                Throw an exception with the provided message

        ***********************************************************************/

        abstract void error (char[] msg);
}


