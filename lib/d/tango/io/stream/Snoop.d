/*******************************************************************************

        copyright:      Copyright (c) 2007 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: Oct 2007

        author:         Kris

*******************************************************************************/

module tango.io.stream.Snoop;

private import  tango.io.Console,
                tango.io.device.Conduit;

private import  tango.text.convert.Format;

private alias void delegate(char[]) Snoop;

/*******************************************************************************

        Stream to expose call behaviour. By default, activity trace is
        sent to Cerr

*******************************************************************************/

class SnoopInput : InputStream
{
        private InputStream     host;
        private Snoop           snoop;

        /***********************************************************************

                Attach to the provided stream

        ***********************************************************************/

        this (InputStream host, Snoop snoop = null)
        {
                assert (host);
                this.host = host;
                this.snoop = snoop ? snoop : &snooper;
        }

        /***********************************************************************

                Return the upstream host of this filter
                        
        ***********************************************************************/

        InputStream input ()
        {
                return host;
        }            

        /***********************************************************************

                Return the hosting conduit

        ***********************************************************************/

        final IConduit conduit ()
        {
                return host.conduit;
        }

        /***********************************************************************

                Read from conduit into a target array. The provided dst 
                will be populated with content from the conduit. 

                Returns the number of bytes read, which may be less than
                requested in dst

        ***********************************************************************/

        final size_t read (void[] dst)
        {
                auto x = host.read (dst);
                trace ("{}: read {} bytes", host.conduit, x is -1 ? 0 : x);
                return x;
        }

        /***********************************************************************

                Load the bits from a stream, and return them all in an
                array. The dst array can be provided as an option, which
                will be expanded as necessary to consume the input.

                Returns an array representing the content, and throws
                IOException on error
                              
        ***********************************************************************/

        void[] load (size_t max=-1)
        {
                auto x = host.load (max);
                trace ("{}: loaded {} bytes", x.length);
                return x;
        }

        /***********************************************************************

                Clear any buffered content

        ***********************************************************************/

        final InputStream flush ()
        {
                host.flush;
                trace ("{}: flushed/cleared", host.conduit);
                return this;
        }

        /***********************************************************************

                Close the input

        ***********************************************************************/

        final void close ()
        {
                host.close;
                trace ("{}: closed", host.conduit);
        }

        /***********************************************************************
        
                Seek on this stream. Target conduits that don't support
                seeking will throw an IOException

        ***********************************************************************/

        final long seek (long offset, Anchor anchor = Anchor.Begin)
        {
                auto s = host.seek (offset, anchor);
                trace ("{}: seek at offset {} from anchor {}", host.conduit, offset, anchor);
                return s;
        }

        /***********************************************************************

                Internal trace handler

        ***********************************************************************/

        private void snooper (char[] x)
        {
                Cerr(x).newline;
        }

        /***********************************************************************

                Internal trace handler

        ***********************************************************************/

        private void trace (char[] format, ...)
        {
                char[256] tmp = void;
                snoop (Format.vprint (tmp, format, _arguments, _argptr));
        }
}


/*******************************************************************************

        Stream to expose call behaviour. By default, activity trace is
        sent to Cerr

*******************************************************************************/

class SnoopOutput : OutputStream
{
        private OutputStream    host;
        private Snoop           snoop;

        /***********************************************************************

                Attach to the provided stream

        ***********************************************************************/

        this (OutputStream host, Snoop snoop = null)
        {
                assert (host);
                this.host = host;
                this.snoop = snoop ? snoop : &snooper;
        }

        /***********************************************************************
        
                Return the upstream host of this filter
                        
        ***********************************************************************/

        OutputStream output ()
        {
                return host;
        }              

        /***********************************************************************

                Write to conduit from a source array. The provided src
                content will be written to the conduit.

                Returns the number of bytes written from src, which may
                be less than the quantity provided

        ***********************************************************************/

        final size_t write (void[] src)
        {
                auto x = host.write (src);
                trace ("{}: wrote {} bytes", host.conduit, x is -1 ? 0 : x);
                return x;
        }

        /***********************************************************************

                Return the hosting conduit

        ***********************************************************************/

        final IConduit conduit ()
        {
                return host.conduit;
        }

        /***********************************************************************

                Emit/purge buffered content

        ***********************************************************************/

        final OutputStream flush ()
        {
                host.flush;
                trace ("{}: flushed", host.conduit);
                return this;
        }

        /***********************************************************************

                Close the output

        ***********************************************************************/

        final void close ()
        {
                host.close;
                trace ("{}: closed", host.conduit);
        }

        /***********************************************************************

                Transfer the content of another conduit to this one. Returns
                a reference to this class, or throws IOException on failure.

        ***********************************************************************/

        final OutputStream copy (InputStream src, size_t max=-1)
        {
                host.copy (src, max);
                trace("{}: copied from {}", host.conduit, src.conduit);
                return this;
        }

        /***********************************************************************
        
                Seek on this stream. Target conduits that don't support
                seeking will throw an IOException

        ***********************************************************************/

        final long seek (long offset, Anchor anchor = Anchor.Begin)
        {
                auto s = host.seek (offset, anchor);
                trace ("{}: seek at offset {} from anchor {}", host.conduit, offset, anchor);
                return s;
        }

        /***********************************************************************

                Internal trace handler

        ***********************************************************************/

        private void snooper (char[] x)
        {
                Cerr(x).newline;
        }

        /***********************************************************************

                Internal trace handler

        ***********************************************************************/

        private void trace (char[] format, ...)
        {
                char[256] tmp = void;
                snoop (Format.vprint (tmp, format, _arguments, _argptr));
        }
}



debug (Snoop)
{
        void main()
        {
                auto s = new SnoopInput (null);
                auto o = new SnoopOutput (null);
        }
}
