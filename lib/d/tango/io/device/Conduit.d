/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Mar 2004: Initial release

        author:         Kris

*******************************************************************************/

module tango.io.device.Conduit;

private import tango.core.Thread,
               tango.core.Exception;

public  import tango.io.model.IConduit;

/*******************************************************************************

        Conduit abstract base-class, implementing interface IConduit.
        Only the conduit-specific read(), write(), detach() and 
        bufferSize() need to be implemented for a concrete conduit 
        implementation. See File for an example.

        Conduits provide virtualized access to external content, and
        represent things like files or Internet connections. Conduits
        expose a pair of streams, are modelled by tango.io.model.IConduit, 
        and are implemented via classes such as File & SocketConduit. 

        Additional kinds of conduit are easy to construct: one either
        subclasses tango.io.device.Conduit, or implements tango.io.model.IConduit.
        A conduit typically reads and writes from/to a Buffer in large
        chunks, typically the entire buffer. Alternatively, one can invoke
        input.read(dst[]) and/or output.write(src[]) directly.

*******************************************************************************/

class Conduit : IConduit
{
        protected Fiber.Scheduler scheduler;            // optional scheduler
        private   uint            duration = -1;        // scheduling timeout

        /***********************************************************************

                Test for asynchronous capability. This will be eligable
                for scheduling where (a) it is created within a fiber and
                (b) there is a scheduler attached to the fiber at the time
                this is invoked.

                Note that fibers may schedule just one outstanding I/O 
                request at a time

        ***********************************************************************/

        this ()
        {
                auto f = Fiber.getThis;
                if (f)
                    scheduler = f.event.scheduler;
        }

        /***********************************************************************

                Clean up when collected. See method detach()

        ***********************************************************************/

        ~this ()
        {
                detach;
        }

        /***********************************************************************
        
                Return the name of this conduit

        ***********************************************************************/

        abstract char[] toString (); 
                     
        /***********************************************************************

                Return a preferred size for buffering conduit I/O

        ***********************************************************************/

        abstract size_t bufferSize ();

        /***********************************************************************

                Read from conduit into a target array. The provided dst 
                will be populated with content from the conduit. 

                Returns the number of bytes read, which may be less than
                requested in dst. Eof is returned whenever an end-of-flow 
                condition arises.

        ***********************************************************************/

        abstract size_t read (void[] dst);

        /***********************************************************************

                Write to conduit from a source array. The provided src
                content will be written to the conduit.

                Returns the number of bytes written from src, which may
                be less than the quantity provided. Eof is returned when 
                an end-of-flow condition arises.

        ***********************************************************************/

        abstract size_t write (void [] src);

        /***********************************************************************

                Disconnect this conduit. Note that this may be invoked
                both explicitly by the user, and implicitly by the GC.
                Be sure to manage multiple detachment requests correctly:
                set a flag, or sentinel value as necessary

        ***********************************************************************/

        abstract void detach ();

        /***********************************************************************

                Set the active timeout period for IO calls (in milliseconds)

        ***********************************************************************/

        final void timeout (uint millisec)
        {
                duration = millisec;
        }

        /***********************************************************************

                Get the active timeout period for IO calls (in milliseconds)

        ***********************************************************************/

        final uint timeout ()
        {
                return duration;
        }

        /***********************************************************************

                Is the conduit alive? Default behaviour returns true

        ***********************************************************************/

        bool isAlive ()
        {
                return true;
        }

        /***********************************************************************
        
                Return the host. This is part of the Stream interface

        ***********************************************************************/

        final IConduit conduit ()
        {
                return this;
        }
                            
        /***********************************************************************

                Emit buffered output or reset buffered input

        ***********************************************************************/

        IOStream flush () 
        {
                return this;
        }

        /***********************************************************************

                Close this conduit
                
                Both input and output are detached, and are no longer usable

        ***********************************************************************/

        void close ()
        {
                this.detach;
        }

        /***********************************************************************

                Return the input stream 

        ***********************************************************************/
        
        final InputStream input ()
        {
                return this;
        }

        /***********************************************************************

                Return the output stream

        ***********************************************************************/
        
        final OutputStream output ()
        {
                return this;
        }

        /***********************************************************************

                Throw an IOException, with the provided message

        ***********************************************************************/

        final void error (char[] msg)
        {
                throw new IOException (msg);
        }

        /***********************************************************************

                Transfer the content of another conduit to this one. Returns
                the dst OutputStream, or throws IOException on failure.

        ***********************************************************************/

        OutputStream copy (InputStream src, size_t max = -1)
        {
                transfer (src, this, max);
                return this;
        }

        /***********************************************************************

                Load the bits from a stream, and return them all in an
                array. The dst array can be provided as an option, which
                will be expanded as necessary to consume the input.

                Returns an array representing the content, and throws
                IOException on error
                
        ***********************************************************************/

        void[] load (size_t max = -1)
        {
                return load (this, max);
        }

        /***********************************************************************
        
                Seek on this stream. Source conduits that don't support
                seeking will throw an IOException

        ***********************************************************************/

        long seek (long offset, Anchor anchor = Anchor.Begin)
        {
                error (this.toString ~ " does not support seek requests");
                return 0;
        }

        /***********************************************************************

                Load the bits from a stream, and return them all in an
                array. The dst array can be provided as an option, which
                will be expanded as necessary to consume input.

                Returns an array representing the content, and throws
                IOException on error
                
        ***********************************************************************/

        static void[] load (InputStream src, size_t max=-1)
        {
                void[]  dst;
                size_t  i,
                        len,
                        chunk;

                if (max != -1)
                    chunk = max;
                else
                   chunk = src.conduit.bufferSize;

                while (len < max)
                      {
                      if (dst.length - len is 0)
                          dst.length = len + chunk;

                      if ((i = src.read (dst[len .. $])) is Eof)
                           break;
                      len += i;
                      }

                return dst [0 .. len];
        }

        /***********************************************************************
                
                Low-level data transfer, where max represents the maximum
                number of bytes to transfer. 

                Returns Eof on failure, number of bytes copied on success

        ***********************************************************************/

        static size_t transfer (InputStream src, OutputStream dst, size_t max=-1)
        {
                byte[8192] tmp;
                size_t     done;

                while (max)
                      {
                      auto len = max;
                      if (len > tmp.length)
                          len = tmp.length;

                      if ((len = src.read(tmp[0 .. len])) is Eof)
                           max = 0;
                      else
                         {
                         max -= len;
                         done += len;
                         auto p = tmp.ptr;
                         for (auto j=0; len > 0; len -= j, p += j)
                              if ((j = dst.write (p[0 .. len])) is Eof)
                                   return Eof;
                         }
                      }

                return done;
        }
}


/*******************************************************************************

        Base class for input stream filtering

*******************************************************************************/

class InputFilter : InputStream
{
        protected InputStream source;

        /***********************************************************************

                Attach to the provided stream. The provided source stream 
                should generally never be null, though some filters have a
                need to set this lazily

        ***********************************************************************/

        this (InputStream source)
        {
                this.source = source;
        }

        /***********************************************************************

                Return the hosting conduit

        ***********************************************************************/

        IConduit conduit ()
        {
                return source.conduit;
        }

        /***********************************************************************

                Read from conduit into a target array. The provided dst 
                will be populated with content from the conduit. 

                Returns the number of bytes read, which may be less than
                requested in dst. Eof is returned whenever an end-of-flow 
                condition arises.

        ***********************************************************************/

        size_t read (void[] dst)
        {
                return source.read (dst);
        }

        /***********************************************************************

                Load the bits from a stream, and return them all in an
                array. The dst array can be provided as an option, which
                will be expanded as necessary to consume the input.

                Returns an array representing the content, and throws
                IOException on error
                              
        ***********************************************************************/

        void[] load (size_t max = -1)
        {
                return Conduit.load (this, max);
        }

        /***********************************************************************

                Clear any buffered content

        ***********************************************************************/

        IOStream flush ()
        {
                source.flush;
                return this;
        }

        /***********************************************************************
        
                Seek on this stream. Target conduits that don't support
                seeking will throw an IOException

        ***********************************************************************/

        long seek (long offset, Anchor anchor = Anchor.Begin)
        {
                return source.seek (offset, anchor);
        }

        /***********************************************************************

                Return the upstream host of this filter
                        
        ***********************************************************************/

        InputStream input ()
        {
                return source;
        }            

        /***********************************************************************

                Close the input

        ***********************************************************************/

        void close ()
        {
                source.close;
        }
}


/*******************************************************************************

         Base class for output stream filtering. The provided sink stream 
                should generally never be null, though some filters have a
                need to set this lazily

*******************************************************************************/

class OutputFilter : OutputStream
{
        protected OutputStream sink;

        /***********************************************************************

                Attach to the provided stream

        ***********************************************************************/

        this (OutputStream sink)
        {
                this.sink = sink;
        }

        /***********************************************************************

                Return the hosting conduit

        ***********************************************************************/

        IConduit conduit ()
        {
                return sink.conduit;
        }

        /***********************************************************************

                Write to conduit from a source array. The provided src
                content will be written to the conduit.

                Returns the number of bytes written from src, which may
                be less than the quantity provided. Eof is returned when 
                an end-of-flow condition arises.

        ***********************************************************************/

        size_t write (void[] src)
        {
                return sink.write (src);
        }

        /***********************************************************************

                Transfer the content of another conduit to this one. Returns
                a reference to this class, or throws IOException on failure.

        ***********************************************************************/

        OutputStream copy (InputStream src, size_t max = -1)
        {
                Conduit.transfer (src, this, max);
                return this;
        }

        /***********************************************************************

                Emit/purge buffered content

        ***********************************************************************/

        IOStream flush ()
        {
                sink.flush;
                return this;
        }

        /***********************************************************************
        
                Seek on this stream. Target conduits that don't support
                seeking will throw an IOException

        ***********************************************************************/

        long seek (long offset, Anchor anchor = Anchor.Begin)
        {
                return sink.seek (offset, anchor);
        }

        /***********************************************************************
        
                Return the upstream host of this filter
                        
        ***********************************************************************/

        OutputStream output ()
        {
                return sink;
        }              

        /***********************************************************************

                Close the output

        ***********************************************************************/

        void close ()
        {
                sink.close;
        }
}
