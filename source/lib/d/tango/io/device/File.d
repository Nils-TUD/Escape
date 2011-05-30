/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Mar 2004: Initial release     
                        Dec 2006: Outback release
                        Nov 2008: relocated and simplified
                        
        author:         Kris, 
                        John Reimer, 
                        Anders F Bjorklund (Darwin patches),
                        Chris Sauls (Win95 file support)

*******************************************************************************/

module tango.io.device.File;

private import tango.sys.Common;

private import tango.io.device.Device;

private import stdc = tango.stdc.stringz;

/*******************************************************************************

        platform-specific functions

*******************************************************************************/

version (Win32)
	private import Utf = tango.text.convert.Utf;
else version (Escape)
{
	private import tango.stdc.io;
	private import tango.stdc.stat;
}
else
	private import tango.stdc.posix.unistd;


/*******************************************************************************

        Implements a means of reading and writing a generic file. Conduits
        are the primary means of accessing external data, and File
        extends the basic pattern by providing file-specific methods to
        set the file size, seek to a specific file position and so on. 
        
        Serial input and output is straightforward. In this example we
        copy a file directly to the console:
        ---
        // open a file for reading
        auto from = new File ("test.txt");

        // stream directly to console
        Stdout.copy (from);
        ---

        And here we copy one file to another:
        ---
        // open file for reading
        auto from = new File ("test.txt");

        // open another for writing
        auto to = new File ("copy.txt", File.WriteCreate);

        // copy file and close
        to.copy.close;
        from.close;
        ---
        
        You can use InputStream.load() to load a file directly into memory:
        ---
        auto file = new File ("test.txt");
        auto content = file.load;
        file.close;
        ---

        Or use a convenience static function within File:
        ---
        auto content = File.get ("test.txt");
        ---

        A more explicit version with a similar result would be:
        ---
        // open file for reading
        auto file = new File ("test.txt");

        // create an array to house the entire file
        auto content = new char [file.length];

        // read the file content. Return value is the number of bytes read
        auto bytes = file.read (content);
        file.close;
        ---

        Conversely, one may write directly to a File like so:
        ---
        // open file for writing
        auto to = new File ("text.txt", File.WriteCreate);

        // write an array of content to it
        auto bytes = to.write (content);
        ---

        There are equivalent static functions, File.set() and
        File.append(), which set or append file content respectively

        File can happily handle random I/O. Here we use seek() to
        relocate the file pointer:
        ---
        // open a file for reading and writing
        auto file = new File ("random.bin", File.ReadWriteCreate);

        // write some data
        file.write ("testing");

        // rewind to file start
        file.seek (0);

        // read data back again
        char[10] tmp;
        auto bytes = file.read (tmp);

        file.close;
        ---

        Note that File is unbuffered by default - wrap an instance within
        tango.io.stream.Buffered for buffered I/O.

        Compile with -version=Win32SansUnicode to enable Win95 & Win32s file 
        support.
        
*******************************************************************************/

class File : Device, Device.Seek, Device.Truncate
{
        public alias Device.read  read;
        public alias Device.write write;

        /***********************************************************************
        
                Fits into 32 bits ...

        ***********************************************************************/

         align(1) struct Style
        {
                Access          access;                 /// access rights
                Open            open;                   /// how to open
                Share           share;                  /// how to share
                Cache           cache;                  /// how to cache
        }

        /***********************************************************************

        ***********************************************************************/

        enum Access : ubyte     {
                                Read      = 0x01,       /// is readable
                                Write     = 0x02,       /// is writable
                                ReadWrite = 0x03,       /// both
                                }

        /***********************************************************************
        
        ***********************************************************************/

        enum Open : ubyte       {
                                Exists=0,               /// must exist
                                Create,                 /// create or truncate
                                Sedate,                 /// create if necessary
                                Append,                 /// create if necessary
                                New,                    /// can't exist
                                };

        /***********************************************************************
        
        ***********************************************************************/

        enum Share : ubyte      {
                                None=0,                 /// no sharing
                                Read,                   /// shared reading
                                ReadWrite,              /// open for anything
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

            Read an existing file
        
        ***********************************************************************/

        const Style ReadExisting = {Access.Read, Open.Exists};

        /***********************************************************************

            Read an existing file
        
        ***********************************************************************/

        const Style ReadShared = {Access.Read, Open.Exists, Share.Read};

        /***********************************************************************
        
                Write on an existing file. Do not create

        ***********************************************************************/

        const Style WriteExisting = {Access.Write, Open.Exists};

        /***********************************************************************
        
                Write on a clean file. Create if necessary

        ***********************************************************************/

        const Style WriteCreate = {Access.Write, Open.Create};

        /***********************************************************************
        
                Write at the end of the file

        ***********************************************************************/

        const Style WriteAppending = {Access.Write, Open.Append};

        /***********************************************************************
        
                Read and write an existing file

        ***********************************************************************/

        const Style ReadWriteExisting = {Access.ReadWrite, Open.Exists}; 

        /***********************************************************************
        
                Read & write on a clean file. Create if necessary

        ***********************************************************************/

        const Style ReadWriteCreate = {Access.ReadWrite, Open.Create}; 

        /***********************************************************************
        
                Read and Write. Use existing file if present

        ***********************************************************************/

        const Style ReadWriteOpen = {Access.ReadWrite, Open.Sedate}; 




        // the file we're working with 
        private char[]  path_;

        // the style we're opened with
        private Style   style_;

        /***********************************************************************
        
                Create a File for use with open()

                Note that File is unbuffered by default - wrap an instance 
                within tango.io.stream.Buffered for buffered I/O

        ***********************************************************************/

        this ()
        {
        }

        /***********************************************************************
        
                Create a File with the provided path and style.

                Note that File is unbuffered by default - wrap an instance 
                within tango.io.stream.Buffered for buffered I/O

        ***********************************************************************/

        this (char[] path, Style style = ReadExisting)
        {
                open (path, style);
        }

        /***********************************************************************
        
                Return the Style used for this file.

        ***********************************************************************/

        Style style ()
        {
                return style_;
        }               

        /***********************************************************************
        
                Return the path used by this file.

        ***********************************************************************/

        override char[] toString ()
        {
                return path_;
        }               

        /***********************************************************************

                Convenience function to return the content of a file.
                Returns a slice of the provided output buffer, where
                that has sufficient capacity, and allocates from the
                heap where the file content is larger.

                Content size is determined via the file-system, per
                File.length, although that may be misleading for some
                *nix systems. An alternative is to use File.load which
                loads content until an Eof is encountered

        ***********************************************************************/

        static void[] get (char[] path, void[] dst = null)
        {
                scope file = new File (path);  

                // allocate enough space for the entire file
                auto len = cast(size_t) file.length;
                if (dst.length < len)
                    dst.length = len;

                //read the content
                len = file.read (dst);
                if (len is file.Eof)
                    file.error ("File.read :: unexpected eof");

                return dst [0 .. len];
        }

        /***********************************************************************

                Convenience function to set file content and length to 
                reflect the given array

        ***********************************************************************/

        static void set (char[] path, void[] content)
        {
                scope file = new File (path, ReadWriteCreate);  
                file.write (content);
        }

        /***********************************************************************

                Convenience function to append content to a file

        ***********************************************************************/

        static void append (char[] path, void[] content)
        {
                scope file = new File (path, WriteAppending);  
                file.write (content);
        }


        /***********************************************************************

                Windows-specific code
        
        ***********************************************************************/

        version(Win32)
        {
                /***************************************************************
                  
                    Low level open for sub-classes that need to apply specific
                    attributes.

                    Return:
                        false in case of failure

                ***************************************************************/

                protected bool open (char[] path, Style style, DWORD addattr)
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
                                        CREATE_ALWAYS,          // truncate always
                                        OPEN_ALWAYS,            // create if needed
                                        OPEN_ALWAYS,            // (for appending)
                                        CREATE_NEW              // can't exist
                                        ];
                                                
                        static const Flags Share =   
                                        [
                                        0,
                                        FILE_SHARE_READ,
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

                        // remember our settings
                        assert(path);
                        path_ = path;
                        style_ = style;

                        attr   = Attr[style.cache] | addattr;
                        share  = Share[style.share];
                        create = Create[style.open];
                        access = Access[style.access];

                        if (scheduler)
                            attr |= FILE_FLAG_OVERLAPPED;// + FILE_FLAG_NO_BUFFERING;

                        // zero terminate the path
                        char[512] zero = void;
                        auto name = stdc.toStringz (path, zero);

                        version (Win32SansUnicode)
                                 io.handle = CreateFileA (name, access, share, 
                                                          null, create, 
                                                          attr | FILE_ATTRIBUTE_NORMAL,
                                                          null);
                             else
                                {
                                // convert to utf16
                                wchar[512] convert = void;
                                auto wide = Utf.toString16 (name[0..path.length+1], convert);

                                // open the file
                                io.handle = CreateFileW (wide.ptr, access, share,
                                                         null, create, 
                                                         attr | FILE_ATTRIBUTE_NORMAL,
                                                         null);
                                }

                        if (io.handle is INVALID_HANDLE_VALUE)
                            return false;

                        // reset extended error 
                        SetLastError (ERROR_SUCCESS);

                        // move to end of file?
                        if (style.open is Open.Append)
                            *(cast(long*) &io.asynch.Offset) = -1;
                        else
                           io.track = true;

                        // monitor this handle for async I/O?
                        if (scheduler)
                            scheduler.open (io.handle, toString);

                        return true;
                }

                /***************************************************************

                        Open a file with the provided style.

                ***************************************************************/

                void open (char[] path, Style style = ReadExisting)
                {
                    if (! open (path, style, 0))
                          error;
                }

                /***************************************************************

                        Set the file size to be that of the current seek 
                        position. The file must be writable for this to
                        succeed.

                ***************************************************************/

                void truncate ()
                {
                        truncate (position);
                }               

                /***************************************************************

                        Set the file size to be the specified length. The 
                        file must be writable for this to succeed. 

                ***************************************************************/

                override void truncate (long size)
                {
                        auto s = seek (size);
                        assert (s is size);

                        // must have Generic_Write access
                        if (! SetEndOfFile (io.handle))
                              error;                            
                }               

                /***************************************************************

                        Set the file seek position to the specified offset
                        from the given anchor. 

                ***************************************************************/

                override long seek (long offset, Anchor anchor = Anchor.Begin)
                {
                        long newOffset; 

                        // hack to ensure overlapped.Offset and file location 
                        // are correctly in synch ...
                        if (anchor is Anchor.Current)
                            SetFilePointerEx (io.handle, 
                                              *cast(LARGE_INTEGER*) &io.asynch.Offset, 
                                              cast(PLARGE_INTEGER) &newOffset, 0);

                        if (! SetFilePointerEx (io.handle, *cast(LARGE_INTEGER*) 
                                                &offset, cast(PLARGE_INTEGER) 
                                                &newOffset, anchor)) 
                              error;

                        return (*cast(long*) &io.asynch.Offset) = newOffset;
                } 
                              
                /***************************************************************
                
                        Return the current file position.
                
                ***************************************************************/

                long position ()
                {
                        return *cast(long*) &io.asynch.Offset;
                }               

                /***************************************************************
        
                        Return the total length of this file.

                ***************************************************************/

                long length ()
                {
                        long len;

                        if (! GetFileSizeEx (io.handle, cast(PLARGE_INTEGER) &len))
                              error;
                        return len;
                }               
        }


        /***********************************************************************

                 Escape-specific code. Note that some methods are 32bit only
        
        ***********************************************************************/

        version (Escape)
        {
                /***************************************************************

                    Low level open for sub-classes that need to apply specific
                    attributes.

                    Return:
                        false in case of failure

                ***************************************************************/

                protected bool open (char[] path, Style style,
                                     int addflags, int access = 0666)
                {
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
                                        0,                      	// open existing
                                        IO_CREATE | IO_TRUNCATE,    // truncate always
                                        IO_CREATE,                	// create if needed
                                        IO_APPEND | IO_CREATE,      // append
                                        IO_CREATE,       			// can't exist
	                                    ];

                        // remember our settings
                        assert(path);
                        path_ = path;
                        style_ = style;
                        
                        // zero terminate and convert to utf16
                        char[512] zero = void;
                        auto name = stdc.toStringz (path, zero);
                        auto mode = Access[style.access] | Create[style.open];
	
	                    // TODO note that we ignore locks here since its not available
	                    
	                    handle = .open (name, mode | addflags);
	                    if (handle < 0)
                            return false;
                        return true;
                }

                /***************************************************************

                        Open a file with the provided style.

                        Note that files default to no-sharing. That is, 
                        they are locked exclusively to the host process 
                        unless otherwise stipulated. We do this in order
                        to expose the same default behaviour as Win32

                        NO FILE LOCKING FOR BORKED POSIX

                ***************************************************************/

                void open (char[] path, Style style = ReadExisting)
                {
                    if (! open (path, style, 0))
                          error;
                }


                /***************************************************************

                        Set the file size to be that of the current seek 
                        position. The file must be writable for this to
                        succeed.

                ***************************************************************/

                void truncate ()
                {
                        truncate (position);
                }               

                /***************************************************************

                        Set the file size to be the specified length. The 
                        file must be writable for this to succeed.

                ***************************************************************/

                override void truncate (long size)
                {
                		// TODO not implemented
                		assert(0);
                }
                
                /***************************************************************

                        Set the file seek position to the specified offset
                        from the given anchor. 

                ***************************************************************/

                override long seek (long offset, Anchor anchor = Anchor.Begin)
                {
	            		uint whence;
	            		switch(anchor)
	            		{
	            			case Anchor.Begin:
	            				whence = SEEK_SET;
	            				break;
	            			case Anchor.End:
	            				whence = SEEK_END;
	            				break;
	            			case Anchor.Current:
	            				whence = SEEK_CUR;
	            				break;
	            			default:
	            				assert(0);
	            		}
	            		
                        long result = .seek (handle, cast(uint)offset, whence);
                        if (result < 0)
                            error;
                        return result;
                }               

                /***************************************************************
                
                        Return the current file position.
                
                ***************************************************************/

                long position ()
                {
                        return seek (0, Anchor.Current);
                }               

                /***************************************************************
        
                        Return the total length of this file. 

                ***************************************************************/

                long length ()
                {
                        stat_t stats = void;
                        if (.fstat(handle,&stats) < 0)
                            error;
                        return cast(long) stats.st_size;
                }
        }


        /***********************************************************************

                 Unix-specific code. Note that some methods are 32bit only
        
        ***********************************************************************/

		else version (Posix)
        {
                /***************************************************************

                    Low level open for sub-classes that need to apply specific
                    attributes.

                    Return:
                        false in case of failure

                ***************************************************************/

                protected bool open (char[] path, Style style,
                                     int addflags, int access = 0666)
                {
                        alias int[] Flags;

                        const O_LARGEFILE = 0x8000;

                        static const Flags Access =  
                                        [
                                        0,                      // invalid
                                        O_RDONLY,
                                        O_WRONLY,
                                        O_RDWR,
                                        ];
                                                
                        static const Flags Create =  
                                        [
                                        0,                      // open existing
                                        O_CREAT | O_TRUNC,      // truncate always
                                        O_CREAT,                // create if needed
                                        O_APPEND | O_CREAT,     // append
                                        O_CREAT | O_EXCL,       // can't exist
                                        ];

                        static const short[] Locks =   
                                        [
                                        F_WRLCK,                // no sharing
                                        F_RDLCK,                // shared read
                                        ];
                                                
                        // remember our settings
                        assert(path);
                        path_ = path;
                        style_ = style;

                        // zero terminate and convert to utf16
                        char[512] zero = void;
                        auto name = stdc.toStringz (path, zero);
                        auto mode = Access[style.access] | Create[style.open];

                        // always open as a large file
                        handle = posix.open (name, mode | O_LARGEFILE | addflags, 
                                             access);
                        if (handle is -1)
                            return false;

                        return true;
                }

                /***************************************************************

                        Open a file with the provided style.

                        Note that files default to no-sharing. That is, 
                        they are locked exclusively to the host process 
                        unless otherwise stipulated. We do this in order
                        to expose the same default behaviour as Win32

                        NO FILE LOCKING FOR BORKED POSIX

                ***************************************************************/

                void open (char[] path, Style style = ReadExisting)
                {
                    if (! open (path, style, 0))
                          error;
                }

                /***************************************************************

                        Set the file size to be that of the current seek 
                        position. The file must be writable for this to
                        succeed.

                ***************************************************************/

                void truncate ()
                {
                        truncate (position);
                }               

                /***************************************************************

                        Set the file size to be the specified length. The 
                        file must be writable for this to succeed.

                ***************************************************************/

                override void truncate (long size)
                {
                        // set filesize to be current seek-position
                        if (posix.ftruncate (handle, cast(off_t) size) is -1)
                            error;
                }               

                /***************************************************************

                        Set the file seek position to the specified offset
                        from the given anchor. 

                ***************************************************************/

                override long seek (long offset, Anchor anchor = Anchor.Begin)
                {
                        long result = posix.lseek (handle, cast(off_t) offset, anchor);
                        if (result is -1)
                            error;
                        return result;
                }               

                /***************************************************************
                
                        Return the current file position.
                
                ***************************************************************/

                long position ()
                {
                        return seek (0, Anchor.Current);
                }               

                /***************************************************************
        
                        Return the total length of this file. 

                ***************************************************************/

                long length ()
                {
                        stat_t stats = void;
                        if (posix.fstat (handle, &stats))
                            error;
                        return cast(long) stats.st_size;
                }               
        }
}


debug (File)
{
        import tango.io.Stdout;

        void main()
        {
                char[10] ff;

                auto file = new File("file.d");
                auto content = cast(char[]) file.load (file);
                assert (content.length is file.length);
                assert (file.read(ff) is file.Eof);
                assert (file.position is content.length);
                file.seek (0);
                assert (file.position is 0);
                assert (file.read(ff) is 10);
                assert (file.position is 10);
                assert (file.seek(0, file.Anchor.Current) is 10);
                assert (file.seek(0, file.Anchor.Current) is 10);
        }
}
