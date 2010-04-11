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
        are modelled by tango.io.model.IConduit, and implemented via 
        classes FileConduit and SocketConduit. 
        
        Additional kinds of conduit are easy to construct: one either 
        subclasses tango.io.Conduit, or implements tango.io.model.IConduit. 
        A conduit typically reads and writes from/to an IBuffer in large 
        chunks, typically the entire buffer. Alternatively, one can invoke 
        read(dst[]) and/or write(src[]) directly.

*******************************************************************************/

abstract class IConduit : ISelectable
{
        /***********************************************************************
        
                Declare the End Of File identifer

        ***********************************************************************/

        enum {Eof = uint.max};

        /***********************************************************************
                
                read from conduit into a target array

        ***********************************************************************/

        abstract uint read (void[] dst);               

        /***********************************************************************
        
                write to conduit from a source array

        ***********************************************************************/

        abstract uint write (void[] src);               

        /***********************************************************************
        
                flush provided content to the conduit

        ***********************************************************************/

        abstract bool flush (void[] src);

        /***********************************************************************
        
                Transfer the content of this conduit to another one. 
                Returns true if all content was successfully copied.
        
        ***********************************************************************/

        abstract IConduit copy (IConduit source);

        /**********************************************************************
        
                Fill the provided buffer. Returns the number of bytes 
                actually read, which will be less that dst.length when 
                Eof has been reached and zero thereafter

        **********************************************************************/
        
        abstract uint fill (void[] dst);

        /***********************************************************************
        
                Attach a filter to this conduit: see IConduitFilter

        ***********************************************************************/

        abstract void attach (IConduitFilter filter);

        /***********************************************************************
        
                Return a preferred size for buffering conduit I/O

        ***********************************************************************/

        abstract uint bufferSize (); 
                     
        /***********************************************************************
        
                Returns true is this conduit can be read from

        ***********************************************************************/

        abstract bool isReadable ();

        /***********************************************************************
        
                Returns true if this conduit can be written to

        ***********************************************************************/

        abstract bool isWritable ();

        /***********************************************************************
        
                Returns true if this conduit is seekable (whether it 
                implements ISeekable)

        ***********************************************************************/

        abstract bool isSeekable ();

        /***********************************************************************

                Is the conduit alive?

        ***********************************************************************/

        abstract bool isAlive ();

        /***********************************************************************

                Are we shutting down?

        ***********************************************************************/

        abstract bool isHalting ();

        /***********************************************************************
                
                Release external resources

        ***********************************************************************/

        abstract void close ();

        /***********************************************************************

                Models a handle-oriented device. 

                TODO: figure out how to avoid exposing this in the general 
                case

        ***********************************************************************/
        
        abstract Handle fileHandle ();

        /***********************************************************************

                Models the ability to seek within a conduit

        ***********************************************************************/

        interface Seek
        {
                /***************************************************************
        
                        The anchor positions supported by ISeekable

                ***************************************************************/

                enum Anchor     {
                                Begin   = 0,
                                Current = 1,
                                End     = 2,
                                };

                /***************************************************************
                        
                        Return current conduit position (e.g. file position)
                
                ***************************************************************/

                ulong getPosition ();

                /***************************************************************
                
                        Move the file position to the given offset from the 
                        provided anchor point, and return adjusted position.

                ***************************************************************/

                ulong seek (ulong offset, Anchor anchor = Anchor.Begin);
        }
}


/*******************************************************************************

        Describes how to make an IO entity usable with selectors
        
*******************************************************************************/

interface ISelectable
{
        /// opaque OS file-handle        
        typedef int Handle = -1;        

        /***********************************************************************

                Models a handle-oriented device. 

                TODO: figure out how to avoid exposing this in the general
                case

        ***********************************************************************/

        Handle fileHandle ();
}


/*******************************************************************************
        
        Define a conduit filter base-class. The filter is invoked
        via its reader() method whenever a block of content is 
        being read, and by its writer() method whenever content is
        being written. 

        The filter should return the number of bytes it has actually
        produced: less than or equal to the length of the provided 
        array. 

        Filters are chained together such that the last filter added
        is the first one invoked. It is the responsibility of each
        filter to invoke the next link in the chain; for example:

        ---
        class MungingFilter : ConduitFilter
        {
                int reader (void[] dst)
                {
                        // read the next X bytes
                        int count = next.reader (dst);

                        // set everything to '*' !
                        dst[0..count] = '*';

                        // say how many we read
                        return count;
                }

                int writer (void[] src)
                {
                        byte[] tmp = new byte[src.length];

                        // set everything to '*'
                        tmp = '*';

                        // write the munged output
                        return next.writer (tmp);
                }
        }
        ---

        Notice how this filter invokes the 'next' instance before 
        munging the content ... the far end of the chain is where
        the original IConduit reader is attached, so it will get
        invoked eventually assuming each filter invokes 'next'. 
        If the next reader fails it will return IConduit.Eof, as
        should your filter (or throw an IOException). From a client 
        perspective, filters are attached like this:

        ---
        FileConduit fc = new FileConduit (...);

        fc.attach (new ZipFilter);
        fc.attach (new MungingFilter);
        ---

        Again, the last filter attached is the first one invoked 
        when a block of content is actually read. Each filter has
        two additional methods that it may use to control behavior:

        ---
        class ConduitFilter : IConduitFilter
        {
                protected IConduitFilter next;

                void bind (IConduit conduit, IConduitFilter next)
                {
                        this.next = next;
                }

                void unbind ()
                {
                }
        }
        ---

        The first method is invoked when the filter is attached to a 
        conduit, while the second is invoked just before the conduit 
        is closed. Both of these may be overridden by the filter for
        whatever purpose desired.

        Note that a conduit filter can choose to sidestep reading from 
        the conduit (per the usual case), and produce its input from 
        somewhere else entirely. This mechanism supports the notion
        of 'piping' between multiple conduits, or between a conduit 
        and something else entirely; it's a bridging mechanism.

*******************************************************************************/

interface IConduitFilter
{
        /***********************************************************************
        
                filter-specific reader

        ***********************************************************************/

        uint reader (void[] dst);               
                             
        /***********************************************************************
        
                filter-specific writer

        ***********************************************************************/

        uint writer (void[] dst);               
                             
        /***********************************************************************
        
        ***********************************************************************/

        void bind (IConduit conduit, IConduitFilter next);                       
                              
        /***********************************************************************
        
        ***********************************************************************/

        void unbind ();
}


