/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: March 2004      
                        Outback release: December 2006
                        
        author:         $(UL Kris)
                        $(UL John Reimer)
                        $(UL Anders F Bjorklund (Darwin patches))
                        $(UL Chris Sauls (Win95 file support))

*******************************************************************************/

module tango.io.FileConduit;

private import  tango.sys.Common;

public  import  tango.io.FileProxy;

public  import  tango.io.DeviceConduit;

private import  Utf = tango.text.convert.Utf;

/*******************************************************************************

        Other O/S functions

*******************************************************************************/

version (Win32)
	private extern (Windows) BOOL SetEndOfFile (HANDLE);
else version (Escape)
{
	private import tango.stdc.io;
}
else
{
	private extern (C) int ftruncate (int, int);
	private import tango.stdc.posix.fcntl;
}


/*******************************************************************************

        Implements a means of reading and writing a generic file. Conduits
        are the primary means of accessing external data and FileConduit
        extends the basic pattern by providing file-specific methods to
        set the file size, seek to a specific file position and so on. 
        
        Serial input and output is straightforward. In this example we
        copy a file directly to the console:

        ---
        // open a file for reading
        auto from = new FileConduit ("test.txt");

        // stream directly to console
        Stdout.conduit.copy (from);
        ---

        And here we copy one file to another:

        ---
        // open a file for reading
        auto from = new FileConduit ("test.txt");

        // open another for writing
        auto to = new FileConduit ("copy.txt", FileConduit.WriteTruncate);

        // copy file
        to.copy (from);
        ---
        
        To load a file directly into memory one might do this:

        ---
        // open file for reading
        auto fc = new FileConduit ("test.txt");

        // create an array to house the entire file
        auto content = new char[fc.length];

        // read the file content. Return value is the number of bytes read
        auto bytesRead = fc.read (content);
        ---

        Conversely, one may write directly to a FileConduit, like so:

        ---
        // open file for writing
        auto to = new FileConduit ("text.txt", FileConduit.WriteTruncate);

        // write an array of content to it
        auto bytesWritten = to.write (content);
        ---


        FileConduit can just as easily handle random IO. Here we use seek()
        to relocate the file pointer and, for variation, apply a protocol to
        perform simple input and output:

        ---
        // open a file for reading
        auto fc = new FileConduit ("random.bin", FileConduit.ReadWriteCreate);

        // construct (binary) reader & writer upon this conduit
        auto read = new Reader (fc);
        auto write = new Writer (fc);

        int x=10, y=20;

        // write some data, and flush output since protocol IO is buffered
        write (x) (y) ();

        // rewind to file start
        fc.seek (0);

        // read data back again
        read (x) (y);

        fc.close();
        ---


        See File, FilePath, FileProxy, FileConst, FileScan, and FileSystem for 
        additional functionality related to file manipulation. 

        Compile with -version=Win32SansUnicode to enable Win95 & Win32s file 
        support.
        
*******************************************************************************/

class FileConduit : DeviceConduit, DeviceConduit.Seek
{
        /***********************************************************************
        
                Fits into 32 bits ...

        ***********************************************************************/

        struct Style
        {
                align (1):

                Access          access;                 /// access rights
                Open            open;                   /// how to open
                Share           share;                  /// how to share
                Cache           cache;                  /// how to cache
        }

        /***********************************************************************
        
        ***********************************************************************/

        enum Open : ubyte       {
                                Exists=0,               /// must exist
                                Create,                 /// create always
                                Truncate,               /// must exist
                                Append,                 /// create if necessary
                                };

        /***********************************************************************
        
        ***********************************************************************/

        enum Share : ubyte      {
                                Read=0,                 /// shared reading
                                Write,                  /// shared writing
                                ReadWrite,              /// both
                                };

        /***********************************************************************
        
        ***********************************************************************/

        enum Cache : ubyte      {
                                None      = 0x00,       /// don't optimize
                                Random    = 0x01,       /// optimize for random
                                Stream    = 0x02,       /// optimize for stream
                                WriteThru = 0x04,       /// backing-cache flag
                                };

        /***********************************************************************

            Predefined styles
        
        ***********************************************************************/

        const Style ReadExisting = {DeviceConduit.Access.Read, Open.Exists};

        /***********************************************************************
        
        ***********************************************************************/

        const Style WriteTruncate = {DeviceConduit.Access.Write, Open.Truncate};

        /***********************************************************************
        
        ***********************************************************************/

        const Style WriteAppending = {DeviceConduit.Access.Write, Open.Append};

        /***********************************************************************
        
        ***********************************************************************/

        const Style ReadWriteCreate = {DeviceConduit.Access.ReadWrite, Open.Create}; 

        /***********************************************************************
        
        ***********************************************************************/

        const Style ReadWriteExisting = {DeviceConduit.Access.ReadWrite, Open.Exists}; 


        // the file we're working with 
        private FilePath path;

        // expose deviceconduit.copy() methods also 
        alias DeviceConduit.copy copy;

        /***********************************************************************
        
                Create a FileConduit with the provided path and style.

        ***********************************************************************/

        this (char[] name, Style style = ReadExisting)
        {
                this (new FilePath(name), style);
        }

        /***********************************************************************
        
                Create a FileConduit from the provided proxy and style.

        ***********************************************************************/

        this (FileProxy proxy, Style style = ReadExisting)
        {
                this (proxy.getPath(), style);
        }

        /***********************************************************************
        
                Create a FileConduit with the provided path and style.

        ***********************************************************************/

        this (FilePath path, Style style = ReadExisting)
        {
                // say we're seekable
                super (style.access, true);
                
                // remember who we are
                this.path = path;

                // open the file
                open (style);
        }    

        /***********************************************************************
        
                Return the FilePath used by this file.

        ***********************************************************************/

        FilePath getPath ()
        {
                return path;
        }               

        /***********************************************************************
        
                Return the name of the FilePath used by this file.

        ***********************************************************************/

        override char[] toUtf8 ()
        {
                return path.toUtf8;
        }               

        /***********************************************************************
                
                Return the current file position.
                
        ***********************************************************************/

        ulong getPosition ()
        {
                return seek (0, Seek.Anchor.Current);
        }               

        /***********************************************************************
        
                Return the total length of this file.

        ***********************************************************************/

        ulong length ()
        {
                ulong   pos,    
                        ret;
                        
                pos = getPosition ();
                ret = seek (0, Seek.Anchor.End);
                seek (pos);
                return ret;
        }               

        /***********************************************************************

                Transfer the content of another file to this one. Returns a
                reference to this class on success, or throws an IOException 
                upon failure.
        
        ***********************************************************************/

        FileConduit copy (FilePath source)
        {
                auto fc = new FileConduit (source);
                scope (exit)
                       fc.close;

                super.copy (fc);
                return this;
        }               

        /***********************************************************************
        
                Return the name used by this file.

        ***********************************************************************/

        protected override char[] getName ()
        {
                return path.toUtf8;
        }               


        /***********************************************************************

                Windows-specific code
        
        ***********************************************************************/

        version(Win32)
        {
                private bool appending;

                /***************************************************************

                        Open a file with the provided style.

                ***************************************************************/

                protected void open (Style style)
                {
                        DWORD   attr,
                                share,
                                access,
                                create;

                        alias DWORD[] Flags;

                        static const Flags Access =  
                                        [
                                        0,                      // invalid
                                        GENERIC_READ,
                                        GENERIC_WRITE,
                                        GENERIC_READ | GENERIC_WRITE,
                                        ];
                                                
                        static const Flags Create =  
                                        [
                                        OPEN_EXISTING,          // must exist
                                        CREATE_ALWAYS,          // create always
                                        TRUNCATE_EXISTING,      // must exist
                                        OPEN_ALWAYS,            // (for appending)
                                        ];
                                                
                        static const Flags Share =   
                                        [
                                        FILE_SHARE_READ,
                                        FILE_SHARE_WRITE,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        ];
                                                
                        static const Flags Attr =   
                                        [
                                        0,
                                        FILE_FLAG_RANDOM_ACCESS,
                                        FILE_FLAG_SEQUENTIAL_SCAN,
                                        0,
                                        FILE_FLAG_WRITE_THROUGH,
                                        ];

                        attr = Attr[style.cache];
                        share = Share[style.share];
                        create = Create[style.open];
                        access = Access[style.access];

                        version (Win32SansUnicode)
                                 handle = CreateFileA (path.cString.ptr, access, share, 
                                                       null, create, 
                                                       attr | FILE_ATTRIBUTE_NORMAL,
                                                       cast(HANDLE) null);
                             else
                                {
                                wchar[256] tmp = void;
                                auto name = Utf.toUtf16 (path.cString, tmp);
                                handle = CreateFileW (name.ptr, access, share,
                                                      null, create, 
                                                      attr | FILE_ATTRIBUTE_NORMAL,
                                                      cast(HANDLE) null);
                                }

                        if (handle is INVALID_HANDLE_VALUE)
                            error ();

                        // move to end of file?
                        if (style.open is Open.Append)
                            appending = true;
                }
                
                /***************************************************************

                        Write a chunk of bytes to the file from the provided
                        array (typically that belonging to an IBuffer)

                ***************************************************************/

                protected override uint writer (void[] src)
                {
                        DWORD written;

                        // try to emulate the Unix O_APPEND mode
                        if (appending)
                            SetFilePointer (handle, 0, null, Seek.Anchor.End);
                        
                        return super.writer (src);
                }
            
                /***************************************************************

                        Set the file size to be that of the current seek 
                        position. The file must be writable for this to
                        succeed.

                ***************************************************************/

                void truncate ()
                {
                        // must have Generic_Write access
                        if (! SetEndOfFile (handle))
                              error ();                            
                }               

                /***************************************************************

                        Set the file seek position to the specified offset
                        from the given anchor. 

                ***************************************************************/

                ulong seek (ulong offset, Seek.Anchor anchor = Seek.Anchor.Begin)
                {
                        LONG high = cast(LONG) (offset >> 32);
                        ulong result = SetFilePointer (handle, cast(LONG) offset, 
                                                       &high, anchor);

                        if (result is -1 && 
                            GetLastError() != ERROR_SUCCESS)
                            error ();

                        return result + (cast(ulong) high << 32);
                }               
        }


        /***********************************************************************

                 Unix-specific code. Note that some methods are 32bit only
        
        ***********************************************************************/

        version (Posix)
        {
                /***************************************************************

                        Open a file with the provided style.

                ***************************************************************/

                protected void open (Style style)
                {
                        int     share, 
                                access;

                        alias int[] Flags;

                        static const Flags Access =  
                                        [
                                        0,              // invalid
                                        O_RDONLY,
                                        O_WRONLY,
                                        O_RDWR,
                                        ];
                                                
                        static const Flags Create =  
                                        [
                                        0,              // open existing
                                        O_CREAT,        // create always
                                        O_TRUNC,        // must exist
                                        O_APPEND | O_CREAT, 
                                        ];

                        // this is not the same as Windows sharing,
                        // but it's perhaps a reasonable approximation
                        static const Flags Share =   
                                        [
                                        0640,           // read access
                                        0620,           // write access
                                        0660,           // read & write
                                        ];
                                                
                        share = Share[style.share];
                        access = Access[style.access] | Create[style.open];

                        handle = posix.open (path.cString.ptr, access, share);
                        if (handle is -1)
                            error ();
                }

                /***************************************************************

                        32bit only ...

                        Set the file size to be that of the current seek 
                        position. The file must be writable for this to
                        succeed.

                ***************************************************************/

                void truncate ()
                {
                        // set filesize to be current seek-position
                        if (ftruncate (handle, getPosition()) is -1)
                            error ();
                }               

                /***************************************************************

                        32bit only ...

                        Set the file seek position to the specified offset
                        from the given anchor. 

                ***************************************************************/

                ulong seek (ulong offset, Seek.Anchor anchor = Seek.Anchor.Begin)
                {
                        uint result = posix.lseek (handle, offset, anchor);
                        if (result is -1)
                            error ();
                        return result;
                }               
        }


        /***********************************************************************

                 Escape-specific code
        
        ***********************************************************************/

        version (Escape)
        {
                /***************************************************************

                        Open a file with the provided style.

                ***************************************************************/

                protected void open (Style style)
                {
                        ubyte access;

                        alias int[] Flags;

                        static const Flags Access =  
                                        [
                                        0,              // invalid
                                        IO_READ,
                                        IO_WRITE,
                                        IO_READ | IO_WRITE,
                                        ];
                                                
                        static const Flags Create =  
                                        [
                                        0,              // open existing
                                        IO_CREATE,        // create always
                                        IO_TRUNCATE,        // must exist
                                        IO_APPEND | IO_CREATE, 
                                        ];

                        access = Access[style.access] | Create[style.open];

                        // TODO note that we ignore style.share here since its not available
                        
                        handle = .open (path.cString.ptr, access);
                        if (handle < 0)
                            error ();
                }

                /***************************************************************

                        32bit only ...

                        Set the file seek position to the specified offset
                        from the given anchor. 

                ***************************************************************/

                ulong seek (ulong offset, Seek.Anchor anchor = Seek.Anchor.Begin)
                {
                		uint whence;
                		switch(anchor)
                		{
                			case Seek.Anchor.Begin:
                				whence = SEEK_SET;
                				break;
                			case Seek.Anchor.End:
                				whence = SEEK_END;
                				break;
                			case Seek.Anchor.Current:
                				whence = SEEK_CUR;
                				break;
                			default:
                				assert(0);
                		}
                	 
                        int result = .seek (handle, offset, whence);
                        if (result < 0)
                            error ();
                        return result;
                }
        }
}
