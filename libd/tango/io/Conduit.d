/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: March 2004

        author:         Kris

*******************************************************************************/

module tango.io.Conduit;

private import  tango.core.Exception;

private import  tango.io.model.IConduit;

/*******************************************************************************

        Conduit abstract base-class, implementing interface IConduit.
        Only the conduit-specific reader(), writer(), and bufferSize()
        need to be implemented for a concrete conduit implementation.
        See FileConduit for an example.

        Conduits provide virtualized access to external content, and
        represent things like files or Internet connections. Conduits
        are modelled by tango.io.model.IConduit, and implemented via
        classes FileConduit, SocketConduit, etc

        Additional kinds of conduit are easy to construct: one either
        subclasses tango.io.Conduit, or implements tango.io.model.IConduit.
        A conduit typically reads and writes from/to an IBuffer in large
        chunks, typically the entire buffer. Alternatively, one can invoke
        read(dst[]) and/or write(src[]) directly.

*******************************************************************************/

class Conduit : IConduit, IConduitFilter
{
        private Access                  access;
        private IConduitFilter          filter;
        private bool                    seekable;

        enum Access : ubyte             {
                                        Read      = 0x01,       // is readable
                                        Write     = 0x02,       // is writable
                                        ReadWrite = 0x03,       // both
                                        };


        /***********************************************************************

                Return a preferred size for buffering conduit I/O

        ***********************************************************************/

        abstract uint bufferSize ();

        /***********************************************************************

                Models a handle-oriented device. We need to revisit this

                TODO: figure out how to avoid exposing this in the general
                case

        ***********************************************************************/

        abstract Handle fileHandle ();

        /***********************************************************************

                conduit-specific reader

        ***********************************************************************/

        protected abstract uint reader (void[] dst);

        /***********************************************************************

                conduit-specific writer

        ***********************************************************************/

        protected abstract uint writer (void[] src);

        /***********************************************************************

                Construct a conduit with the given style and seek abilities.
                Conduits are either seekable or non-seekable.

        ***********************************************************************/

        this (Access access, bool seekable = false)
        {
                filter = this;
                this.access = access;
                this.seekable = seekable;
        }

        /***********************************************************************

                Method to close the filters. This is invoked from the
                Resource base-class when the resource is being closed.
                You should ensure that a subclass invokes this as part
                of its closure mechanics.

        ***********************************************************************/

        void close ()
        {
                filter.unbind ();
                filter = this;
        }

        /***********************************************************************

                flush provided content to the conduit

        ***********************************************************************/

        bool flush (void[] src)
        {
                int len = src.length;

                for (int i, written; written < len;)
                     if ((i = write (src[written..len])) != Eof)
                          written += i;
                     else
                        return false;
                return true;
        }

        /***********************************************************************

                Please refer to IConduit.attach for details

        ***********************************************************************/

        void attach (IConduitFilter filter)
        {
                // hook new filter to current one
                filter.bind (this, this.filter);

                // make this the head filter
                this.filter = filter;
        }

        /***********************************************************************

        ***********************************************************************/

        protected void bind (IConduit conduit, IConduitFilter next)
        {
        }

        /***********************************************************************

        ***********************************************************************/

        protected void unbind ()
        {
        }

        /***********************************************************************

                read from conduit into a target buffer

        ***********************************************************************/

        uint read (void[] dst)
        {
                return filter.reader (dst);
        }

        /***********************************************************************

                write to conduit from a source buffer

        ***********************************************************************/

        uint write (void [] src)
        {
                return filter.writer (src);
        }

        /***********************************************************************

                Returns true if this conduit is seekable (whether it
                implements ISeekable)

        ***********************************************************************/

        bool isSeekable ()
        {
                return seekable;
        }

        /***********************************************************************

                Returns true is this conduit can be read from

        ***********************************************************************/

        bool isReadable ()
        {
                return (access & Access.Read) != 0;
        }

        /***********************************************************************

                Returns true if this conduit can be written to

        ***********************************************************************/

        bool isWritable ()
        {
                return (access & Access.Write) != 0;
        }

        /***********************************************************************

                Is the conduit alive?

        ***********************************************************************/

        bool isAlive ()
        {
                return true;
        }

        /***********************************************************************

                Transfer the content of another conduit to this one. Returns
                a reference to this class, and throws IOException on failure.

        ***********************************************************************/

        IConduit copy (IConduit source)
        {
                auto buffer = new byte[bufferSize];

                uint i;
                while ((i = source.read (buffer)) != Eof)
                        if (! flush (buffer [0..i]))
                              exception ("target Eof while copying conduit");
                
                delete buffer;
                return this;
        }

        /**********************************************************************

                Fill the provided buffer. Returns the number of bytes
                actually read, which will be less that dst.length when
                Eof has been reached and zero thereafter

        **********************************************************************/

        uint fill (void[] dst)
        {
                uint len;

                do {
                   int i = read (dst [len .. $]);
                   if (i is Eof)
                       return len;

                   len += i;
                   } while (len < dst.length);

                return len;
        }

        /***********************************************************************

                Return the access-style used when creating this conduit

        ***********************************************************************/

        Access getAccess ()
        {
                return access;
        }

        /***********************************************************************

                Is the application terminating?

        ***********************************************************************/

        bool isHalting ()
        {
                return halting;
        }

        /***********************************************************************

        ***********************************************************************/

        protected static void exception (char[] msg)
        {
                throw new IOException (msg);
        }
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

class ConduitFilter : IConduitFilter
{
        protected IConduitFilter next;

        /***********************************************************************

        ***********************************************************************/

        uint reader (void[] dst)
        {
                return next.reader (dst);
        }

        /***********************************************************************

        ***********************************************************************/

        uint writer (void[] src)
        {
                return next.writer (src);
        }

        /***********************************************************************

        ***********************************************************************/

        protected void bind (IConduit conduit, IConduitFilter next)
        {
                this.next = next;
        }

        /***********************************************************************

        ***********************************************************************/

        protected void unbind ()
        {
                next.unbind ();
        }
}


/*******************************************************************************

        Set a flag when the application is halting. This is used to avoid
        closure mechanics while the object pool may be in a state of flux.

*******************************************************************************/

private static bool halting;

private static ~this()
{
        halting = true;
}

