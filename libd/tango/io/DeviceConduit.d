/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: May 2005

        author:         Kris

*******************************************************************************/

module tango.io.DeviceConduit;

private import  tango.sys.Common;

public  import  tango.io.Conduit;

private import  tango.core.Exception;

version (Escape)
{
	private import tango.stdc.io;
}

/*******************************************************************************

        Implements a means of reading and writing a file device. Conduits
        are the primary means of accessing external data, and are usually
        routed through a Buffer.

*******************************************************************************/

class DeviceConduit : Conduit
{
        /***********************************************************************

                Construct a conduit with the given style and seek abilities.
                Conduits are either seekable or non-seekable.

        ***********************************************************************/

        this (Access access, bool seekable)
        {
                super (access, seekable);
        }

        /***********************************************************************

                Create a FileConduit on the provided FileDevice. This is
                strictly for adapting existing devices such as Stdout and
                friends.

        ***********************************************************************/

        this (FileDevice device)
        {
                // say we're not seekable
                super (device.access, false);

                // open the file
                reopen (device);
        }

        /***********************************************************************

                Callback to close the file. This is invoked from the Resource
                base-class when the resource is being closed.

        ***********************************************************************/

        override void close ()
        {
                super.close ();
                _close ();
        }

        /***********************************************************************

                Make a reasonable attempt to clean up

        ***********************************************************************/

        ~this ()
        {
                if (! isHalting)
                      _close ();
        }

        /***********************************************************************

                Return a preferred size for buffering conduit I/O

        ***********************************************************************/

        uint bufferSize ()
        {
                return 1024 * 16;
        }

        /***********************************************************************

                Return the name of this device

        ***********************************************************************/

        protected char[] getName()
        {
                return "<device>";
        }

        /***********************************************************************

                Throw an IOException noting the last error
        
        ***********************************************************************/

        final void error ()
        {
                super.exception (getName() ~ ": " ~ SysError.lastMsg);
        }

        /***********************************************************************

                Windows-specific code

        ***********************************************************************/

        version (Win32)
        {
                protected HANDLE handle;

                /***************************************************************

                        Return the underlying OS handle of this Conduit

                ***************************************************************/

                final Handle fileHandle ()
                {
                        return cast(Handle) handle;
                }

                /***************************************************************

                        Gain access to the standard IO handles (console etc).

                ***************************************************************/

                protected void reopen (FileDevice device)
                {
                        handle = cast(HANDLE) device.id;
                }

                /***************************************************************

                        Close the underlying file

                ***************************************************************/

                protected void _close ()
                {
                        if (handle)
                            if (! CloseHandle (handle))
                                  error ();
                        handle = cast(HANDLE) null;
                }

                /***************************************************************

                        Read a chunk of bytes from the file into the provided
                        array (typically that belonging to an IBuffer)

                ***************************************************************/

                protected override uint reader (void[] dst)
                {
                        DWORD read;
                        void *p = dst.ptr;

                        if (! ReadFile (handle, p, dst.length, &read, null))
                              error ();

                        if (read == 0 && dst.length > 0)
                            return Eof;
                        return read;
                }

                /***************************************************************

                        Write a chunk of bytes to the file from the provided
                        array (typically that belonging to an IBuffer)

                ***************************************************************/

                protected override uint writer (void[] src)
                {
                        DWORD written;

                        if (! WriteFile (handle, src.ptr, src.length, &written, null))
                              error ();

                        return written;
                }
        }


        /***********************************************************************

                 Unix-specific code.

        ***********************************************************************/

        version (Posix)
        {
                protected int handle = -1;

                /***************************************************************

                        Return the underlying OS handle of this Conduit

                ***************************************************************/

                final Handle fileHandle ()
                {
                        return cast(Handle) handle;
                }

                /***************************************************************

                        Gain access to the standard IO handles (console etc).

                ***************************************************************/

                protected void reopen (FileDevice device)
                {
                        handle = device.id;
                }

                /***************************************************************

                        Close the underlying file

                ***************************************************************/

                protected void _close ()
                {
                        if (handle)
                            if (posix.close (handle) == -1)
                                error ();
                        handle = 0;
                }

                /***************************************************************

                        Read a chunk of bytes from the file into the provided
                        array (typically that belonging to an IBuffer)

                ***************************************************************/

                protected override uint reader (void[] dst)
                {
                        int read = posix.read (handle, dst.ptr, dst.length);
                        if (read == -1)
                            error ();
                        else
                           if (read == 0 && dst.length > 0)
                               return Eof;
                        return read;
                }

                /***************************************************************

                        Write a chunk of bytes to the file from the provided
                        array (typically that belonging to an IBuffer)

                ***************************************************************/

                protected override uint writer (void[] src)
                {
                        int written = posix.write (handle, src.ptr, src.length);
                        if (written == -1)
                            error ();
                        return written;
                }
        }


        /***********************************************************************

                 Escape-specific code.

        ***********************************************************************/

        version (Escape)
        {
                protected int handle = -1;

                /***************************************************************

                        Return the underlying OS handle of this Conduit

                ***************************************************************/

                final Handle fileHandle ()
                {
                        return cast(Handle) handle;
                }

                /***************************************************************

                        Gain access to the standard IO handles (console etc).

                ***************************************************************/

                protected void reopen (FileDevice device)
                {
                        handle = device.id;
                }

                /***************************************************************

                        Close the underlying file

                ***************************************************************/

                protected void _close ()
                {
                        if (handle)
                            .close (handle);
                        handle = 0;
                }

                /***************************************************************

                        Read a chunk of bytes from the file into the provided
                        array (typically that belonging to an IBuffer)

                ***************************************************************/

                protected override uint reader (void[] dst)
                {
                        int read = .read (handle, dst.ptr, dst.length);
                        if (read == -1)
                            error ();
                        else
                           if (read == 0 && dst.length > 0)
                               return Eof;
                        return read;
                }

                /***************************************************************

                        Write a chunk of bytes to the file from the provided
                        array (typically that belonging to an IBuffer)

                ***************************************************************/

                protected override uint writer (void[] src)
                {
                        int written = .write (handle, src.ptr, src.length);
                        if (written == -1)
                            error ();
                        return written;
                }
        }
}


/*******************************************************************************

        Class used to wrap an existing file-oriented handle, such as Stdout
        and its cohorts.

*******************************************************************************/

package class FileDevice
{
        private uint             _id;
        private Conduit.Access  access;

        package this (uint id, Conduit.Access access)
        {
                this.access = access;
                this._id = id;
        }

        int id()
        {
              return _id;
        }
}


