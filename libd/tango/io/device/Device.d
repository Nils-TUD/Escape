/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        May 2005: Initial release

        author:         Kris

*******************************************************************************/

module tango.io.device.Device;

private import  tango.sys.Common;

private import  tango.core.Exception;

public  import  tango.io.device.Conduit;

version (Escape)
{
	private import tango.stdc.io;
}

/*******************************************************************************

        Implements a means of reading and writing a file device. Conduits
        are the primary means of accessing external data, and this one is
        used as a superclass for the console, for files, sockets etc

*******************************************************************************/

class Device : Conduit, ISelectable
{
        /// expose superclass definition also
        public alias Conduit.error error;
            
        /***********************************************************************

                Throw an IOException noting the last error
        
        ***********************************************************************/

        final void error ()
        {
                error (this.toString ~ " :: " ~ SysError.lastMsg);
        }

        /***********************************************************************

                Return the name of this device

        ***********************************************************************/

        override char[] toString ()
        {
                return "<device>";
        }

        /***********************************************************************

                Return a preferred size for buffering conduit I/O

        ***********************************************************************/

        override size_t bufferSize ()
        {
                return 1024 * 16;
        }

        /***********************************************************************

                Windows-specific code

        ***********************************************************************/

        version (Win32)
        {
                protected IO io;
                struct IO
                {
                        OVERLAPPED      asynch; // must be the first attribute!!
                        Handle          handle;
                        bool            track;
                        void*           task;
                }

                /***************************************************************

                        Allow adjustment of standard IO handles

                ***************************************************************/

                protected void reopen (Handle handle)
                {
                        io.handle = handle;
                }

                /***************************************************************

                        Return the underlying OS handle of this Conduit

                ***************************************************************/

                final Handle fileHandle ()
                {
                        return io.handle;
                }

                /***************************************************************

                ***************************************************************/

                override void dispose ()
                {
                        if (io.handle != INVALID_HANDLE_VALUE)
                            if (scheduler)
                                scheduler.close (io.handle, toString);
                        detach();
                }

                /***************************************************************

                        Release the underlying file. Note that an exception
                        is not thrown on error, as doing so can induce some
                        spaggetti into error handling. Instead, we need to
                        change this to return a bool instead, so the caller
                        can decide what to do.                        

                ***************************************************************/

                override void detach ()
                {
                        if (io.handle != INVALID_HANDLE_VALUE)
                            CloseHandle (io.handle);

                        io.handle = INVALID_HANDLE_VALUE;
                }

                /***************************************************************

                        Read a chunk of bytes from the file into the provided
                        array. Returns the number of bytes read, or Eof where 
                        there is no further data.

                        Operates asynchronously where the hosting thread is
                        configured in that manner.

                ***************************************************************/

                override size_t read (void[] dst)
                {
                        DWORD bytes;

                        if (! ReadFile (io.handle, dst.ptr, dst.length, &bytes, &io.asynch))
                        //ReadFile (io.handle, dst.ptr, dst.length, &bytes, &io.asynch);
                              if ((bytes = wait (scheduler.Type.Read, bytes, timeout)) is Eof)
                                   return Eof;

                        // synchronous read of zero means Eof
                        if (bytes is 0 && dst.length > 0)
                            return Eof;

                        // update stream location?
                        if (io.track)
                           (*cast(long*) &io.asynch.Offset) += bytes;
                        return bytes;
                }

                /***************************************************************

                        Write a chunk of bytes to the file from the provided
                        array. Returns the number of bytes written, or Eof if 
                        the output is no longer available.

                        Operates asynchronously where the hosting thread is
                        configured in that manner.

                ***************************************************************/

                override size_t write (void[] src)
                {
                        DWORD bytes;

                        if (! WriteFile (io.handle, src.ptr, src.length, &bytes, &io.asynch))
                        //WriteFile (io.handle, src.ptr, src.length, &bytes, &io.asynch);
                        if ((bytes = wait (scheduler.Type.Write, bytes, timeout)) is Eof)
                             return Eof;

                        // update stream location?
                        if (io.track)
                           (*cast(long*) &io.asynch.Offset) += bytes;
                        return bytes;
                }

                /***************************************************************

                ***************************************************************/

                protected final size_t wait (scheduler.Type type, uint bytes, uint timeout)
                {
                        while (true)
                              {
                              auto code = GetLastError;
                              if (code is ERROR_HANDLE_EOF ||
                                  code is ERROR_BROKEN_PIPE)
                                  return Eof;

                              if (scheduler)
                                 {
                                 if (code is ERROR_SUCCESS || 
                                     code is ERROR_IO_PENDING || 
                                     code is ERROR_IO_INCOMPLETE)
                                    {
                                    if (code is ERROR_IO_INCOMPLETE)
                                        super.error ("timeout"); 

                                    io.task = cast(void*) tango.core.Thread.Fiber.getThis;
                                    scheduler.await (io.handle, type, timeout);
                                    if (GetOverlappedResult (io.handle, &io.asynch, &bytes, false))
                                        return bytes;
                                    }
                                 else
                                    error;
                                 }
                              else
                                 if (code is ERROR_SUCCESS)
                                     return bytes;
                                 else
                                    error;
                              }

                        // should never get here
                        assert(false);
                }
        }


        /***********************************************************************

                 Escape-specific code.

        ***********************************************************************/

        version (Escape)
        {
                protected int handle = -1;

                /***************************************************************

                        Allow adjustment of standard IO handles

                ***************************************************************/

                protected void reopen (Handle handle)
                {
                        this.handle = handle;
                }

                /***************************************************************

                        Return the underlying OS handle of this Conduit

                ***************************************************************/

                final Handle fileHandle ()
                {
                        return cast(Handle) handle;
                }

                /***************************************************************

                        Release the underlying file

                ***************************************************************/

                override void detach ()
                {
                        if (handle >= 0)
                        	.close(handle);
                        handle = -1;
                }

                /***************************************************************

                        Read a chunk of bytes from the file into the provided
                        array. Returns the number of bytes read, or Eof where 
                        there is no further data.

                ***************************************************************/

                override size_t read (void[] dst)
                {
                        int res = .read (handle, dst.ptr, dst.length);
                        if (res is -1)
                            error;
                        else
                           if (res is 0 && dst.length > 0)
                               return Eof;
                        return res;
                }

                /***************************************************************

                        Write a chunk of bytes to the file from the provided
                        array. Returns the number of bytes written, or Eof if 
                        the output is no longer available.

                ***************************************************************/

                override size_t write (void[] src)
                {
                        int written = .write (handle, src.ptr, src.length);
                        if (written is -1)
                            error;
                        return written;
                }
        }


        /***********************************************************************

                 Unix-specific code.

        ***********************************************************************/

		else version (Posix)
        {
                protected int handle = -1;

                /***************************************************************

                        Allow adjustment of standard IO handles

                ***************************************************************/

                protected void reopen (Handle handle)
                {
                        this.handle = handle;
                }

                /***************************************************************

                        Return the underlying OS handle of this Conduit

                ***************************************************************/

                final Handle fileHandle ()
                {
                        return cast(Handle) handle;
                }

                /***************************************************************

                        Release the underlying file

                ***************************************************************/

                override void detach ()
                {
                        if (handle >= 0)
                           {
                           //if (scheduler)
                               // TODO Not supported on Posix
                               // scheduler.close (handle, toString);
                           posix.close (handle);
                           }
                        handle = -1;
                }

                /***************************************************************

                        Read a chunk of bytes from the file into the provided
                        array. Returns the number of bytes read, or Eof where 
                        there is no further data.

                ***************************************************************/

                override size_t read (void[] dst)
                {
                        int read = posix.read (handle, dst.ptr, dst.length);
                        if (read is -1)
                            error;
                        else
                           if (read is 0 && dst.length > 0)
                               return Eof;
                        return read;
                }

                /***************************************************************

                        Write a chunk of bytes to the file from the provided
                        array. Returns the number of bytes written, or Eof if 
                        the output is no longer available.

                ***************************************************************/

                override size_t write (void[] src)
                {
                        int written = posix.write (handle, src.ptr, src.length);
                        if (written is -1)
                            error;
                        return written;
                }
        }
}



