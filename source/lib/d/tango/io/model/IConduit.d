/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: March 2004      
                        Outback release: December 2006
        
        author:         Kris

*******************************************************************************/

module tango.io.model.IConduit;

/*******************************************************************************

        Conduits provide virtualized access to external content, and 
        represent things like files or Internet connections. Conduits 
        expose a pair of streams, are modelled by tango.io.model.IConduit, 
        and are implemented via classes such as File & SocketConduit. 
        
        Additional kinds of conduit are easy to construct: one either 
        subclasses tango.io.device.Conduit, or implements tango.io.model.IConduit. 
        A conduit typically reads and writes from/to an IBuffer in large 
        chunks, typically the entire buffer. Alternatively, one can invoke 
        input.read(dst[]) and/or output.write(src[]) directly.

*******************************************************************************/

interface IConduit : InputStream, OutputStream
{
        /***********************************************************************
        
                Return a preferred size for buffering conduit I/O

        ***********************************************************************/

        abstract size_t bufferSize (); 
                     
        /***********************************************************************
        
                Return the name of this conduit

        ***********************************************************************/

        abstract char[] toString (); 

        /***********************************************************************

                Is the conduit alive?

        ***********************************************************************/

        abstract bool isAlive ();

        /***********************************************************************
                
                Release external resources

        ***********************************************************************/

        abstract void detach ();

        /***********************************************************************

                Throw a generic IO exception with the provided msg

        ***********************************************************************/

        abstract void error (char[] msg);

        /***********************************************************************

                All streams now support seek(), so this is used to signal
                a seekable conduit instead

        ***********************************************************************/

        interface Seek {}

        /***********************************************************************

                Indicates the conduit supports resize/truncation

        ***********************************************************************/

        interface Truncate 
        {
                void truncate (long size);
        }
}


/*******************************************************************************

        Describes how to make an IO entity usable with selectors
        
*******************************************************************************/

interface ISelectable
{     
        version (Windows) 
                 alias void* Handle;   /// opaque OS file-handle         
             else
                typedef int Handle = -1;        /// opaque OS file-handle        

        /***********************************************************************

                Models a handle-oriented device. 

                TODO: figure out how to avoid exposing this in the general
                case

        ***********************************************************************/

        Handle fileHandle ();
}


/*******************************************************************************
        
        The common attributes of streams

*******************************************************************************/

interface IOStream 
{
        const Eof = -1;         /// the End-of-Flow identifer

        /***********************************************************************
        
                The anchor positions supported by seek()

        ***********************************************************************/

        enum Anchor {
                    Begin   = 0,
                    Current = 1,
                    End     = 2,
                    };

        /***********************************************************************
                
                Move the stream position to the given offset from the 
                provided anchor point, and return adjusted position.

                Those conduits which don't support seeking will throw
                an IOException (and don't implement IConduit.Seek)

        ***********************************************************************/

        long seek (long offset, Anchor anchor = Anchor.Begin);

        /***********************************************************************
        
                Return the host conduit

        ***********************************************************************/

        IConduit conduit ();
                          
        /***********************************************************************
        
                Flush buffered content. For InputStream this is equivalent
                to clearing buffered content

        ***********************************************************************/

        IOStream flush ();               
        
        /***********************************************************************
        
                Close the input

        ***********************************************************************/

        void close ();               


        /***********************************************************************
        
                Marks a stream that performs read/write mutation, rather than 
                generic decoration. This is used to identify those stream that
                should explicitly not share an upstream buffer with downstream
                siblings.
        
                Many streams add simple decoration (such as DataStream) while
                others are merely template aliases. However, streams such as
                EndianStream mutate content as it passes through the read and
                write methods, which must be respected. On one hand we wish
                to share a single buffer instance, while on the other we must
                ensure correct data flow through an arbitrary combinations of  
                streams. 

                There are two stream variations: one which operate directly 
                upon memory (and thus must have access to a buffer) and another 
                that prefer to have buffered input (for performance reasons) but 
                can operate without. EndianStream is an example of the former, 
                while DataStream represents the latter.
        
                In order to sort out who gets what, each stream makes a request
                for an upstream buffer at construction time. The request has an
                indication of the intended purpose (array-based access, or not). 

        ***********************************************************************/

        interface Mutator {}
}


/*******************************************************************************
        
        The Tango input stream

*******************************************************************************/

interface InputStream : IOStream
{
        /***********************************************************************
        
                Read from stream into a target array. The provided dst 
                will be populated with content from the stream. 

                Returns the number of bytes read, which may be less than
                requested in dst. Eof is returned whenever an end-of-flow 
                condition arises.

        ***********************************************************************/

        size_t read (void[] dst);               
                        
        /***********************************************************************

                Load the bits from a stream, and return them all in an
                array. The dst array can be provided as an option, which
                will be expanded as necessary to consume the input.

                Returns an array representing the content, and throws
                IOException on error
                              
        ***********************************************************************/

        void[] load (size_t max = -1);
        
        /***********************************************************************
        
                Return the upstream source

        ***********************************************************************/

        InputStream input ();               
}


/*******************************************************************************
        
        The Tango output stream

*******************************************************************************/

interface OutputStream : IOStream
{
        /***********************************************************************
        
                Write to stream from a source array. The provided src
                content will be written to the stream.

                Returns the number of bytes written from src, which may
                be less than the quantity provided. Eof is returned when 
                an end-of-flow condition arises.

        ***********************************************************************/

        size_t write (void[] src);     
        
        /***********************************************************************

                Transfer the content of another stream to this one. Returns
                a reference to this class, and throws IOException on failure.

        ***********************************************************************/

        OutputStream copy (InputStream src, size_t max = -1);
                          
        /***********************************************************************
        
                Return the upstream sink

        ***********************************************************************/

        OutputStream output ();               
}


/*******************************************************************************
        
        A buffered input stream

*******************************************************************************/

interface InputBuffer : InputStream
{
        void[] slice ();

        bool next (size_t delegate(void[]) scan);

        size_t reader (size_t delegate(void[]) consumer);
}

/*******************************************************************************
        
        A buffered output stream

*******************************************************************************/

interface OutputBuffer : OutputStream
{
        alias append opCall;

        void[] slice ();
        
        OutputBuffer append (void[]);

        size_t writer (size_t delegate(void[]) producer);
}


