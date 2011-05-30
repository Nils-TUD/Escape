/*******************************************************************************

        copyright:      Copyright (c) 2008 Kris Bell. All rights reserved
        copyright:      Normalization & Patterns copyright (c) 2006-2009 
                        Max Samukha, Thomas Kühne, Grzegorz Adam Hankiewicz

        license:        BSD style: $(LICENSE)

        version:        Mar 2008: Initial version
                        Oct 2009: Added PathUtil code

        A more direct route to the file-system than FilePath. Use this 
        if you don't need path editing features. For example, if all you 
        want is to check some path exists, using this module would likely 
        be more convenient than FilePath. For example:
        ---
        if (exists ("some/file/path")) 
            ...
        ---

        These functions may be less efficient than FilePath because they 
        generally attach a null to the filename for each underlying O/S
        call. Use Path when you need pedestrian access to the file-system, 
        and are not manipulating the path components. Use FilePath where
        path editing or mutation is desired.

        We encourage the use of "scoped import" with this module, such as
        ---
        import Path = tango.io.Path;

        if (Path.exists ("some/file/path")) 
            ...
        ---

        Also residing here is a lightweight path-parser, which splits a 
        filepath into constituent components. See PathParser below:
        ---
        auto p = Path.parse ("some/file/path");
        auto path = p.path;
        auto name = p.name;
        auto suffix = p.suffix;
        ...
        ---

        Path normalization and pattern-matching is also hosted here, via
        the normalize() and pattern() functions. See the doc towards the
        end of this module.

        Compile with -version=Win32SansUnicode to enable Win95 & Win32s 
        file support.

*******************************************************************************/

module tango.io.Path;

private import  tango.sys.Common;

public  import  tango.time.Time : Time, TimeSpan;

private import  tango.io.model.IFile : FileConst, FileInfo;

public  import  tango.core.Exception : IOException, IllegalArgumentException;

private extern (C) void memmove (void* dst, void* src, uint bytes);


/*******************************************************************************

        Various imports

*******************************************************************************/

version (Win32)
{
	version (Win32SansUnicode)
    {
	    private extern (C) int strlen (char *s);
	    private alias WIN32_FIND_DATA FIND_DATA;
    }
 	else
    {
	    private extern (C) int wcslen (wchar *s);
	    private alias WIN32_FIND_DATAW FIND_DATA;
    }
}	
else version (Escape)
{
	private import tango.stdc.io;
	private import tango.stdc.stat;
	private import tango.stdc.dir;
}
else version (Posix)
{
	private import tango.stdc.stdio;
	private import tango.stdc.string;
	private import tango.stdc.posix.utime;
	private import tango.stdc.posix.dirent;
}

/*******************************************************************************

        Wraps the O/S specific calls with a D API. Note that these accept
        null-terminated strings only, which is why it's not public. We need 
        this declared first to avoid forward-reference issues

*******************************************************************************/

package struct FS
{
        /***********************************************************************

                TimeStamp information. Accurate to whatever the F/S supports

        ***********************************************************************/

        struct Stamps
        {
                Time    created,        /// time created
                        accessed,       /// last time accessed
                        modified;       /// last time modified
        }

        /***********************************************************************

                Some fruct glue for directory listings

        ***********************************************************************/

        struct Listing
        {
                char[] folder;
                bool   allFiles;
                
                int opApply (int delegate(ref FileInfo) dg)
                {
                        char[256] tmp = void;
                        auto path = strz (folder, tmp);

                        // sanity check on Win32 ...
                        version (Win32)
                                {
                                bool kosher(){foreach (c; path) if (c is '\\') return false; return true;};
                                assert (kosher, "attempting to use non-standard '\\' in a path for a folder listing");
                                }

                        return list (path, dg, allFiles);
                }
        }

        /***********************************************************************

                Throw an exception using the last known error

        ***********************************************************************/

        static void exception (char[] filename)
        {
                exception (filename[0..$-1] ~ ": ", SysError.lastMsg);
        }

        /***********************************************************************

                Throw an IO exception 

        ***********************************************************************/

        static void exception (char[] prefix, char[] error)
        {
                throw new IOException (prefix ~ error);
        }

        /***********************************************************************

                Return an adjusted path such that non-empty instances always
                have a trailing separator.

                Note: allocates memory where path is not already terminated

        ***********************************************************************/

        static char[] padded (char[] path, char c = '/')
        {
                if (path.length && path[$-1] != c)
                    path = path ~ c;
                return path;
        }

        /***********************************************************************

                Return an adjusted path such that non-empty instances do not
                have a trailing separator

        ***********************************************************************/

        static char[] stripped (char[] path, char c = '/')
        {
                if (path.length && path[$-1] is c)
                    path = path [0 .. $-1];
                return path;
        }

        /***********************************************************************

                Join a set of path specs together. A path separator is
                potentially inserted between each of the segments.

                Note: allocates memory

        ***********************************************************************/

        static char[] join (char[][] paths...)
        {
                char[] result;

                foreach (path; paths)
                         result ~= padded (path);

                return result.length ? result [0 .. $-1] : "";
        }

        /***********************************************************************

                Append a terminating null onto a string, cheaply where 
                feasible

                Note: allocates memory where the dst is too small

        ***********************************************************************/

        static char[] strz (char[] src, char[] dst)
        {
                auto i = src.length + 1;
                if (dst.length < i)
                    dst.length = i;
                dst [0 .. i-1] = src;
                dst[i-1] = 0;
                return dst [0 .. i];
        }

        /***********************************************************************

                Win32 API code

        ***********************************************************************/

        version (Win32)
        {
                /***************************************************************

                        return a wchar[] instance of the path

                ***************************************************************/

                private static wchar[] toString16 (wchar[] tmp, char[] path)
                {
                        auto i = MultiByteToWideChar (CP_UTF8, 0,
                                                      cast(PCHAR)path.ptr, path.length,
                                                      tmp.ptr, tmp.length);
                        return tmp [0..i];
                }

                /***************************************************************

                        return a char[] instance of the path

                ***************************************************************/

                private static char[] toString (char[] tmp, wchar[] path)
                {
                        auto i = WideCharToMultiByte (CP_UTF8, 0, path.ptr, path.length,
                                                      cast(PCHAR)tmp.ptr, tmp.length, null, null);
                        return tmp [0..i];
                }

                /***************************************************************

                        Get info about this path

                ***************************************************************/

                private static bool fileInfo (char[] name, ref WIN32_FILE_ATTRIBUTE_DATA info)
                {
                        version (Win32SansUnicode)
                                {
                                if (! GetFileAttributesExA (name.ptr, GetFileInfoLevelStandard, &info))
                                      return false;
                                }
                             else
                                {
                                wchar[MAX_PATH] tmp = void;
                                if (! GetFileAttributesExW (toString16(tmp, name).ptr, GetFileInfoLevelStandard, &info))
                                      return false;
                                }

                        return true;
                }

                /***************************************************************

                        Get info about this path

                ***************************************************************/

                private static DWORD getInfo (char[] name, ref WIN32_FILE_ATTRIBUTE_DATA info)
                {
                        if (! fileInfo (name, info))
                              exception (name);
                        return info.dwFileAttributes;
                }

                /***************************************************************

                        Get flags for this path

                ***************************************************************/

                private static DWORD getFlags (char[] name)
                {
                        WIN32_FILE_ATTRIBUTE_DATA info = void;

                        return getInfo (name, info);
                }

                /***************************************************************

                        Return whether the file or path exists

                ***************************************************************/

                static bool exists (char[] name)
                {
                        WIN32_FILE_ATTRIBUTE_DATA info = void;

                        return fileInfo (name, info);
                }

                /***************************************************************

                        Return the file length (in bytes)

                ***************************************************************/

                static ulong fileSize (char[] name)
                {
                        WIN32_FILE_ATTRIBUTE_DATA info = void;

                        getInfo (name, info);
                        return (cast(ulong) info.nFileSizeHigh << 32) +
                                            info.nFileSizeLow;
                }

                /***************************************************************

                        Is this file writable?

                ***************************************************************/

                static bool isWritable (char[] name)
                {
                        return (getFlags(name) & FILE_ATTRIBUTE_READONLY) is 0;
                }

                /***************************************************************

                        Is this file actually a folder/directory?

                ***************************************************************/

                static bool isFolder (char[] name)
                {
                        return (getFlags(name) & FILE_ATTRIBUTE_DIRECTORY) != 0;
                }

                /***************************************************************

                        Is this a normal file?

                ***************************************************************/

                static bool isFile (char[] name)
                {
                        return (getFlags(name) & FILE_ATTRIBUTE_DIRECTORY) == 0;
                }

                /***************************************************************

                        Return timestamp information

                        Timestamps are returns in a format dictated by the 
                        file-system. For example NTFS keeps UTC time, 
                        while FAT timestamps are based on the local time

                ***************************************************************/

                static Stamps timeStamps (char[] name)
                {
                        static Time convert (FILETIME time)
                        {
                                return Time (TimeSpan.Epoch1601 + *cast(long*) &time);
                        }

                        WIN32_FILE_ATTRIBUTE_DATA info = void;
                        Stamps                    time = void;

                        getInfo (name, info);
                        time.modified = convert (info.ftLastWriteTime);
                        time.accessed = convert (info.ftLastAccessTime);
                        time.created  = convert (info.ftCreationTime);
                        return time;
                }

                /***************************************************************

                        Set the accessed and modified timestamps of the
                        specified file

                ***************************************************************/

                static void timeStamps (char[] name, Time accessed, Time modified)
                {
                        void set (HANDLE h)
                        {
                                FILETIME m1, a1;
                                auto m = modified - Time.epoch1601;
                                auto a = accessed - Time.epoch1601;
                                *cast(long*) &a1.dwLowDateTime = m.ticks;
                                *cast(long*) &m1.dwLowDateTime = m.ticks;
                                if (SetFileTime (h, null, &a1, &m1) is 0)
                                    exception (name);
                        }
                                                
                        createFile (name, &set);
                }

                /***************************************************************

                        Transfer the content of another file to this one. 
                        Returns a reference to this class on success, or 
                        throws an IOException upon failure.

                ***************************************************************/

                static void copy (char[] src, char[] dst)
                {
                        version (Win32SansUnicode)
                                {
                                if (! CopyFileA (src.ptr, dst.ptr, false))
                                      exception (src);
                                }
                             else
                                {
                                wchar[MAX_PATH+1] tmp1 = void;
                                wchar[MAX_PATH+1] tmp2 = void;

                                if (! CopyFileW (toString16(tmp1, src).ptr, toString16(tmp2, dst).ptr, false))
                                      exception (src);
                                }
                }

                /***************************************************************

                        Remove the file/directory from the file-system.
                        Returns true on success - false otherwise

                ***************************************************************/

                static bool remove (char[] name)
                {
                        if (isFolder(name))
                           {
                           version (Win32SansUnicode)
                                    return RemoveDirectoryA (name.ptr) != 0;
                                else
                                   {
                                   wchar[MAX_PATH] tmp = void;
                                   return RemoveDirectoryW (toString16(tmp, name).ptr) != 0;
                                   }
                           }
                        else
                           version (Win32SansUnicode)
                                    return DeleteFileA (name.ptr) != 0;
                                else
                                   {
                                   wchar[MAX_PATH] tmp = void;
                                   return DeleteFileW (toString16(tmp, name).ptr) != 0;
                                   }
                }

                /***************************************************************

                       change the name or location of a file/directory, and
                       adopt the provided Path

                ***************************************************************/

                static void rename (char[] src, char[] dst)
                {
                        const int Typical = MOVEFILE_REPLACE_EXISTING +
                                            MOVEFILE_COPY_ALLOWED     +
                                            MOVEFILE_WRITE_THROUGH;

                        int result;
                        version (Win32SansUnicode)
                                 result = MoveFileExA (src.ptr, dst.ptr, Typical);
                             else
                                {
                                wchar[MAX_PATH] tmp1 = void;
                                wchar[MAX_PATH] tmp2 = void;
                                result = MoveFileExW (toString16(tmp1, src).ptr, toString16(tmp2, dst).ptr, Typical);
                                }

                        if (! result)
                              exception (src);
                }

                /***************************************************************

                        Create a new file

                ***************************************************************/

                static void createFile (char[] name)
                {
                        createFile (name, null);
                }

                /***************************************************************

                        Create a new directory

                ***************************************************************/

                static void createFolder (char[] name)
                {
                        version (Win32SansUnicode)
                                {
                                if (! CreateDirectoryA (name.ptr, null))
                                      exception (name);
                                }
                             else
                                {
                                wchar[MAX_PATH] tmp = void;
                                if (! CreateDirectoryW (toString16(tmp, name).ptr, null))
                                      exception (name);
                                }
                }

                /***************************************************************

                        List the set of filenames within this folder.

                        Each path and filename is passed to the provided
                        delegate, along with the path prefix and whether
                        the entry is a folder or not.

                        Note: allocates a small memory buffer

                ***************************************************************/

                static int list (char[] folder, int delegate(ref FileInfo) dg, bool all=false)
                {
                        HANDLE                  h;
                        int                     ret;
                        char[]                  prefix;
                        char[MAX_PATH+1]        tmp = void;
                        FIND_DATA               fileinfo = void;
                        
                        version (Win32SansUnicode)
                                 alias char T;
                              else
                                 alias wchar T;

                        int next()
                        {
                                version (Win32SansUnicode)
                                         return FindNextFileA (h, &fileinfo);
                                   else
                                      return FindNextFileW (h, &fileinfo);
                        }

                        static T[] padded (T[] s, T[] ext)
                        {
                                if (s.length && s[$-1] is '/')
                                    return s ~ ext;
                                return s ~ "/" ~ ext;
                        }

                        version (Win32SansUnicode)
                                 h = FindFirstFileA (padded(folder[0..$-1], "*\0").ptr, &fileinfo);
                             else
                                {
                                wchar[MAX_PATH] host = void;
                                h = FindFirstFileW (padded(toString16(host, folder[0..$-1]), "*\0").ptr, &fileinfo);
                                }

                        if (h is INVALID_HANDLE_VALUE)
                            return ret; 

                        scope (exit)
                               FindClose (h);

                        prefix = FS.padded (folder[0..$-1]);
                        do {
                           version (Win32SansUnicode)
                                   {
                                   auto len = strlen (fileinfo.cFileName.ptr);
                                   auto str = fileinfo.cFileName.ptr [0 .. len];
                                   }
                                else
                                   {
                                   auto len = wcslen (fileinfo.cFileName.ptr);
                                   auto str = toString (tmp, fileinfo.cFileName [0 .. len]);
                                   }

                           // skip hidden/system files
                           if (all || (fileinfo.dwFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) is 0)
                              {
                              FileInfo info = void;
                              info.name   = str;
                              info.path   = prefix;
                              info.bytes  = (cast(ulong) fileinfo.nFileSizeHigh << 32) + fileinfo.nFileSizeLow;
                              info.folder = (fileinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                              info.hidden = (fileinfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;
                              info.system = (fileinfo.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0;

                              // skip "..." names
                              if (str.length > 3 || str != "..."[0 .. str.length])
                                  if ((ret = dg(info)) != 0)
                                       break;
                              }
                           } while (next);

                        return ret;
                }

                /***************************************************************

                        Create a new file

                ***************************************************************/

                private static void createFile (char[] name, void delegate(HANDLE) dg)
                {
                        HANDLE h;

                        auto flags = dg.ptr ? OPEN_EXISTING : CREATE_ALWAYS;
                        version (Win32SansUnicode)
                                 h = CreateFileA (name.ptr, GENERIC_WRITE,
                                                  0, null, flags, FILE_ATTRIBUTE_NORMAL, 
                                                  cast(HANDLE) 0);
                             else
                                {
                                wchar[MAX_PATH] tmp = void;
                                h = CreateFileW (toString16(tmp, name).ptr, GENERIC_WRITE,
                                                 0, null, flags, FILE_ATTRIBUTE_NORMAL, 
                                                 cast(HANDLE) 0);
                                }

                        if (h is INVALID_HANDLE_VALUE)
                            exception (name);

                        if (dg.ptr)
                            dg(h);

                        if (! CloseHandle (h))
                              exception (name);
                }
        }

        /***********************************************************************

                Escape-specific code

        ***********************************************************************/

        version (Escape)
        {
                /***************************************************************

                        Get info about this path

                ***************************************************************/

                private static uint getInfo (char[] name, ref stat_t stats)
                {
                        if (.stat (name.ptr, &stats))
                            exception (name);

                        return stats.st_mode;
                }

                /***************************************************************

                        Return whether the file or path exists

                ***************************************************************/

                static bool exists (char[] name)
                {
                        stat_t stats = void;
                        return .stat (name.ptr, &stats) is 0;
                }

                /***************************************************************

                        Return the file length (in bytes)

                ***************************************************************/

                static ulong fileSize (char[] name)
                {
                        stat_t stats = void;

                        getInfo (name, stats);
                        return cast(ulong) stats.st_size;
                }

                /***************************************************************

                        Is this file writable?

                ***************************************************************/

                static bool isWritable (char[] name)
                {
                		// TODO always writable atm
                		return true;
                }

                /***************************************************************

                        Is this file actually a folder/directory?

                ***************************************************************/

                static bool isFolder (char[] name)
                {
                        stat_t stats = void;

                        return S_ISDIR(cast(mode_t)getInfo(name, stats));
                }

                /***************************************************************

                        Is this a normal file?

                ***************************************************************/

                static bool isFile (char[] name)
                {
                        stat_t stats = void;

                        return S_ISREG(cast(mode_t)getInfo(name, stats));
                }

                /***************************************************************

                        Return timestamp information

                        Timestamps are returns in a format dictated by the 
                        file-system. For example NTFS keeps UTC time, 
                        while FAT timestamps are based on the local time

                ***************************************************************/

                static Stamps timeStamps (char[] name)
                {
                        static Time convert (typeof(stat_t.st_mtime) secs)
                        {
                                return Time.epoch1970 +
                                       TimeSpan.fromSeconds(secs);
                        }
                        
                        stat_t stats = void;
                        Stamps time  = void;

                        getInfo (name, stats);

                        time.modified = convert (stats.st_mtime);
                        time.accessed = convert (stats.st_atime);
                        time.created  = convert (stats.st_ctime);
                        return time;
                }

                /***************************************************************

                        Set the accessed and modified timestamps of the
                        specified file

                ***************************************************************/

                static void timeStamps (char[] name, Time accessed, Time modified)
                {
                    	// TODO not available
                        assert(0);
                }

                /***********************************************************************

                        Transfer the content of another file to this one. Returns a
                        reference to this class on success, or throws an IOException
                        upon failure.

                        Note: allocates a memory buffer

                ***********************************************************************/

                static void copy (char[] source, char[] dest)
                {
                        auto src = .open (source.ptr, IO_READ);
                        scope (exit)
                               if (src != -1)
                                   .close (src);

                        auto dst = .open (dest.ptr, IO_CREATE | IO_READ | IO_WRITE);
                        scope (exit)
                               if (dst != -1)
                                   .close (dst);

                        if (src < 0 || dst < 0)
                            exception (source);

                        // copy content
                        ubyte[] buf = new ubyte [16 * 1024];
                        int read = .read (src, buf.ptr, buf.length);
                        while (read > 0)
                              {
                              auto p = buf.ptr;
                              do {
                                 int written = .write (dst, p, read);
                                 p += written;
                                 read -= written;
                                 if (written < 0)
                                     exception (dest);
                                 } while (read > 0);
                              read = .read (src, buf.ptr, buf.length);
                              }
                        if (read < 0)
                            exception (source);

                        // TODO no timestamp-copy here
                }

                /***************************************************************

                        Remove the file/directory from the file-system. 
                        Returns true on success - false otherwise.

                ***************************************************************/

                static bool remove (char[] name)
                {
                        return .unlink(name.ptr) != -1;
                }

                /***************************************************************

                       change the name or location of a file/directory, and
                       adopt the provided FilePath

                ***************************************************************/

                static void rename (char[] src, char[] dst)
                {
                		// TODO not supported
                		assert(0);
                }

                /***************************************************************

                        Create a new file

                ***************************************************************/

                static void createFile (char[] name)
                {
                        int fd;

                        fd = .open (name.ptr, IO_CREATE | IO_WRITE | IO_TRUNCATE);
                        if (fd < 0)
                            exception (name);
                        .close(fd);
                }

                /***************************************************************

                        Create a new directory

                ***************************************************************/

                static void createFolder (char[] name)
                {
                        if (.mkdir (name.ptr))
                            exception (name);
                }
                /***************************************************************

                        List the set of filenames within this folder.

                        Each path and filename is passed to the provided
                        delegate, along with the path prefix and whether
                        the entry is a folder or not.

                        Note: allocates and reuses a small memory buffer

                ***************************************************************/

                static int list (char[] folder, int delegate(ref FileInfo) dg, bool all=false)
                {
                        int             ret;
                        DIR*            dir;
                        dirent          entry;
                        stat_t          sbuf;
                        char[]          prefix;
                        char[]          sfnbuf;

                        dir = .opendir (folder.ptr);
                        if (! dir)
                              return ret;

                        scope (exit)
                               .closedir (dir);

                        // ensure a trailing '/' is present
                        prefix = FS.padded (folder[0..$-1]);

                        // prepare our filename buffer
                        sfnbuf = prefix.dup;
                        
                        while (.readdir(dir,&entry))
                              {
                              auto len = tango.stdc.string.strlen (entry.name.ptr);
                              auto str = entry.name.ptr [0 .. len];
                              ++len;  // include the null

                              // resize the buffer as necessary ...
                              if (sfnbuf.length < prefix.length + len)
                                  sfnbuf.length = prefix.length + len;

                              sfnbuf [prefix.length .. prefix.length + len]
                                      = entry.name.ptr [0 .. len];

                              // skip "..." names
                              if (str.length > 3 || str != "..."[0 .. str.length])
                                 {
                                 FileInfo info = void;
                                 info.bytes  = 0;
                                 info.name   = str;
                                 info.path   = prefix;
                                 info.hidden = str[0] is '.';
                                 info.folder = info.system = false;
                                 
                                 if (! .stat (sfnbuf.ptr, &sbuf))
                                    {
                                    info.folder = (sbuf.st_mode & S_IFDIR) != 0;
                                    if (info.folder is false)
                                        if ((sbuf.st_mode & S_IFREG) is 0)
                                             info.system = true;
                                        else
                                           info.bytes = cast(ulong) sbuf.st_size;
                                    }
                                 if (all || (info.hidden | info.system) is false)
                                     if ((ret = dg(info)) != 0)
                                          break;
                                 }
                              }
                        return ret;
                }
        }

        /***********************************************************************

                Posix-specific code

        ***********************************************************************/

		else version (Posix)
        {
                /***************************************************************

                        Get info about this path

                ***************************************************************/

                private static uint getInfo (char[] name, ref stat_t stats)
                {
                        if (posix.stat (name.ptr, &stats))
                            exception (name);

                        return stats.st_mode;
                }

                /***************************************************************

                        Return whether the file or path exists

                ***************************************************************/

                static bool exists (char[] name)
                {
                        stat_t stats = void;
                        return posix.stat (name.ptr, &stats) is 0;
                }

                /***************************************************************

                        Return the file length (in bytes)

                ***************************************************************/

                static ulong fileSize (char[] name)
                {
                        stat_t stats = void;

                        getInfo (name, stats);
                        return cast(ulong) stats.st_size;
                }

                /***************************************************************

                        Is this file writable?

                ***************************************************************/

                static bool isWritable (char[] name)
                {
                        stat_t stats = void;

                        return (getInfo(name, stats) & O_RDONLY) is 0;
                }

                /***************************************************************

                        Is this file actually a folder/directory?

                ***************************************************************/

                static bool isFolder (char[] name)
                {
                        stat_t stats = void;

                        return (getInfo(name, stats) & S_IFMT) is S_IFDIR;
                }

                /***************************************************************

                        Is this a normal file?

                ***************************************************************/

                static bool isFile (char[] name)
                {
                        stat_t stats = void;

                        return (getInfo(name, stats) & S_IFMT) is S_IFREG;
                }

                /***************************************************************

                        Return timestamp information

                        Timestamps are returns in a format dictated by the 
                        file-system. For example NTFS keeps UTC time, 
                        while FAT timestamps are based on the local time

                ***************************************************************/

                static Stamps timeStamps (char[] name)
                {
                        static Time convert (typeof(stat_t.st_mtime) secs)
                        {
                                return Time.epoch1970 +
                                       TimeSpan.fromSeconds(secs);
                        }

                        stat_t stats = void;
                        Stamps time  = void;

                        getInfo (name, stats);

                        time.modified = convert (stats.st_mtime);
                        time.accessed = convert (stats.st_atime);
                        time.created  = convert (stats.st_ctime);
                        return time;
                }

                /***************************************************************

                        Set the accessed and modified timestamps of the
                        specified file

                ***************************************************************/

                static void timeStamps (char[] name, Time accessed, Time modified)
                {
                        utimbuf time = void;
                        time.actime = (accessed - Time.epoch1970).seconds;
                        time.modtime = (modified - Time.epoch1970).seconds;
                        if (utime (name.ptr, &time) is -1)
                            exception (name);
                }

                /***********************************************************************

                        Transfer the content of another file to this one. Returns a
                        reference to this class on success, or throws an IOException
                        upon failure.

                        Note: allocates a memory buffer

                ***********************************************************************/

                static void copy (char[] source, char[] dest)
                {
                        auto src = posix.open (source.ptr, O_RDONLY, 0640);
                        scope (exit)
                               if (src != -1)
                                   posix.close (src);

                        auto dst = posix.open (dest.ptr, O_CREAT | O_RDWR, 0660);
                        scope (exit)
                               if (dst != -1)
                                   posix.close (dst);

                        if (src is -1 || dst is -1)
                            exception (source);

                        // copy content
                        ubyte[] buf = new ubyte [16 * 1024];
                        int read = posix.read (src, buf.ptr, buf.length);
                        while (read > 0)
                              {
                              auto p = buf.ptr;
                              do {
                                 int written = posix.write (dst, p, read);
                                 p += written;
                                 read -= written;
                                 if (written is -1)
                                     exception (dest);
                                 } while (read > 0);
                              read = posix.read (src, buf.ptr, buf.length);
                              }
                        if (read is -1)
                            exception (source);

                        // copy timestamps
                        stat_t stats;
                        if (posix.stat (source.ptr, &stats))
                            exception (source);

                        utimbuf utim;
                        utim.actime = stats.st_atime;
                        utim.modtime = stats.st_mtime;
                        if (utime (dest.ptr, &utim) is -1)
                            exception (dest);
                }

                /***************************************************************

                        Remove the file/directory from the file-system. 
                        Returns true on success - false otherwise.

                ***************************************************************/

                static bool remove (char[] name)
                {
                        return tango.stdc.stdio.remove(name.ptr) != -1;
                }

                /***************************************************************

                       change the name or location of a file/directory, and
                       adopt the provided FilePath

                ***************************************************************/

                static void rename (char[] src, char[] dst)
                {
                        if (tango.stdc.stdio.rename (src.ptr, dst.ptr) is -1)
                            exception (src);
                }

                /***************************************************************

                        Create a new file

                ***************************************************************/

                static void createFile (char[] name)
                {
                        int fd;

                        fd = posix.open (name.ptr, O_CREAT | O_WRONLY | O_TRUNC, 0660);
                        if (fd is -1)
                            exception (name);

                        if (posix.close(fd) is -1)
                            exception (name);
                }

                /***************************************************************

                        Create a new directory

                ***************************************************************/

                static void createFolder (char[] name)
                {
                        if (posix.mkdir (name.ptr, 0777))
                            exception (name);
                }
                /***************************************************************

                        List the set of filenames within this folder.

                        Each path and filename is passed to the provided
                        delegate, along with the path prefix and whether
                        the entry is a folder or not.

                        Note: allocates and reuses a small memory buffer

                ***************************************************************/

                static int list (char[] folder, int delegate(ref FileInfo) dg, bool all=false)
                {
                        int             ret;
                        DIR*            dir;
                        dirent          entry;
                        dirent*         pentry;
                        stat_t          sbuf;
                        char[]          prefix;
                        char[]          sfnbuf;

                        dir = tango.stdc.posix.dirent.opendir (folder.ptr);
                        if (! dir)
                              return ret;

                        scope (exit)
                               tango.stdc.posix.dirent.closedir (dir);

                        // ensure a trailing '/' is present
                        prefix = FS.padded (folder[0..$-1]);

                        // prepare our filename buffer
                        sfnbuf = prefix.dup;
                        
                        while (true)
                              {
                              // pentry is null at end of listing, or on an error 
                              readdir_r (dir, &entry, &pentry);
                              if (pentry is null)
                                  break;

                              auto len = tango.stdc.string.strlen (entry.d_name.ptr);
                              auto str = entry.d_name.ptr [0 .. len];
                              ++len;  // include the null

                              // resize the buffer as necessary ...
                              if (sfnbuf.length < prefix.length + len)
                                  sfnbuf.length = prefix.length + len;

                              sfnbuf [prefix.length .. prefix.length + len]
                                      = entry.d_name.ptr [0 .. len];

                              // skip "..." names
                              if (str.length > 3 || str != "..."[0 .. str.length])
                                 {
                                 FileInfo info = void;
                                 info.bytes  = 0;
                                 info.name   = str;
                                 info.path   = prefix;
                                 info.hidden = str[0] is '.';
                                 info.folder = info.system = false;
                                 
                                 if (! stat (sfnbuf.ptr, &sbuf))
                                    {
                                    info.folder = (sbuf.st_mode & S_IFDIR) != 0;
                                    if (info.folder is false)
                                        if ((sbuf.st_mode & S_IFREG) is 0)
                                             info.system = true;
                                        else
                                           info.bytes = cast(ulong) sbuf.st_size;
                                    }
                                 if (all || (info.hidden | info.system) is false)
                                     if ((ret = dg(info)) != 0)
                                          break;
                                 }
                              }
                        return ret;
                }
        }
}


/*******************************************************************************

        Parse a file path

        File paths containing non-ansi characters should be UTF-8 encoded.
        Supporting Unicode in this manner was deemed to be more suitable
        than providing a wchar version of PathParser, and is both consistent
        & compatible with the approach taken with the Uri class.

        Note that patterns of adjacent '.' separators are treated specially
        in that they will be assigned to the name where there is no distinct
        suffix. In addition, a '.' at the start of a name signifies it does 
        not belong to the suffix i.e. ".file" is a name rather than a suffix.
        Patterns of intermediate '.' characters will otherwise be assigned
        to the suffix, such that "file....suffix" includes the dots within
        the suffix itself. See method ext() for a suffix without dots.

        Note also that normalization of path-separators does *not* occur by 
        default. This means that usage of '\' characters should be explicitly
        converted beforehand into '/' instead (an exception is thrown in those
        cases where '\' is present). On-the-fly conversion is avoided because
        (a) the provided path is considered immutable and (b) we avoid taking
        a copy of the original path. Module FilePath exists at a higher level, 
        without such contraints.

*******************************************************************************/

struct PathParser
{       
        package char[]  fp;                     // filepath with trailing
        package int     end_,                   // before any trailing 0
                        ext_,                   // after rightmost '.'
                        name_,                  // file/dir name
                        folder_,                // path before name
                        suffix_;                // including leftmost '.'

        /***********************************************************************

                Parse the path spec

        ***********************************************************************/

        PathParser parse (char[] path)
        {
                return parse (path, path.length);
        }

        /***********************************************************************

                Duplicate this path

                Note: allocates memory for the path content

        ***********************************************************************/

        PathParser dup ()
        {
                auto ret = *this;
                ret.fp = fp.dup;
                return ret;
        }

        /***********************************************************************

                Return the complete text of this filepath

        ***********************************************************************/

        char[] toString ()
        {
                return fp [0 .. end_];
        }

        /***********************************************************************

                Return the root of this path. Roots are constructs such as
                "c:"

        ***********************************************************************/

        char[] root ()
        {
                return fp [0 .. folder_];
        }

        /***********************************************************************

                Return the file path. Paths may start and end with a "/".
                The root path is "/" and an unspecified path is returned as
                an empty string. Directory paths may be split such that the
                directory name is placed into the 'name' member; directory
                paths are treated no differently than file paths

        ***********************************************************************/

        char[] folder ()
        {
                return fp [folder_ .. name_];
        }

        /***********************************************************************

                Returns a path representing the parent of this one. This
                will typically return the current path component, though
                with a special case where the name component is empty. In 
                such cases, the path is scanned for a prior segment:
                ---
                normal:  /x/y/z => /x/y
                special: /x/y/  => /x
                ---

                Note that this returns a path suitable for splitting into
                path and name components (there's no trailing separator).

        ***********************************************************************/

        char[] parent ()
        {
                auto p = path;
                if (name.length is 0)
                    for (int i=p.length-1; --i > 0;)
                         if (p[i] is FileConst.PathSeparatorChar)
                            {
                            p = p[0 .. i];
                            break;
                            }
                return FS.stripped (p);
        }

        /***********************************************************************

                Return the name of this file, or directory.

        ***********************************************************************/

        char[] name ()
        {
                return fp [name_ .. suffix_];
        }

        /***********************************************************************

                Ext is the tail of the filename, rightward of the rightmost
                '.' separator e.g. path "foo.bar" has ext "bar". Note that
                patterns of adjacent separators are treated specially - for
                example, ".." will wind up with no ext at all

        ***********************************************************************/

        char[] ext ()
        {
                auto x = suffix;
                if (x.length)
                   {
                   if (ext_ is 0)
                       foreach (c; x)
                                if (c is '.')
                                    ++ext_;
                                else
                                   break;
                   x = x [ext_ .. $];
                   }
                return x;
        }

        /***********************************************************************

                Suffix is like ext, but includes the separator e.g. path
                "foo.bar" has suffix ".bar"

        ***********************************************************************/

        char[] suffix ()
        {
                return fp [suffix_ .. end_];
        }

        /***********************************************************************

                return the root + folder combination

        ***********************************************************************/

        char[] path ()
        {
                return fp [0 .. name_];
        }

        /***********************************************************************

                return the name + suffix combination

        ***********************************************************************/

        char[] file ()
        {
                return fp [name_ .. end_];
        }

        /***********************************************************************

                Returns true if this path is *not* relative to the
                current working directory

        ***********************************************************************/

        bool isAbsolute ()
        {
                return (folder_ > 0) ||
                       (folder_ < end_ && fp[folder_] is FileConst.PathSeparatorChar);
        }

        /***********************************************************************

                Returns true if this FilePath is empty

        ***********************************************************************/

        bool isEmpty ()
        {
                return end_ is 0;
        }

        /***********************************************************************

                Returns true if this path has a parent. Note that a
                parent is defined by the presence of a path-separator in
                the path. This means 'foo' within "/foo" is considered a
                child of the root

        ***********************************************************************/

        bool isChild ()
        {
                return folder.length > 0;
        }

        /***********************************************************************

                Does this path equate to the given text?

        ***********************************************************************/

        int opEquals (char[] s)
        {
                return toString == s;
        }

        /***********************************************************************

                Parse the path spec with explicit end point. A '\' is 
                considered illegal in the path.

        ***********************************************************************/

        package PathParser parse (char[] path, size_t end)
        {
                end_ = end;
                fp = path;
                folder_ = 0;
                name_ = suffix_ = -1;

                for (int i=end_; --i >= 0;)
                     switch (fp[i])
                            {
                            case FileConst.FileSeparatorChar:
                                 if (name_ < 0)
                                     if (suffix_ < 0 && i && fp[i-1] != '.')
                                         suffix_ = i;
                                 break;

                            case FileConst.PathSeparatorChar:
                                 if (name_ < 0)
                                     name_ = i + 1;
                                 break;

                            // Windows file separators are illegal. Use
                            // standard() or equivalent to convert first
                            case '\\':
                                 FS.exception ("unexpected '\\' character in path: ", path[0..end]);

                            version (Win32)
                            {
                            case ':':
                                 folder_ = i + 1;
                                 break;
                            }

                            default:
                                 break;
                            }

                if (name_ < 0)
                    name_ = folder_;

                if (suffix_ < 0 || suffix_ is name_)
                    suffix_ = end_;

                return *this;
        }
}


/*******************************************************************************

        Does this path currently exist?

*******************************************************************************/

bool exists (char[] name)
{
        char[512] tmp = void;
        return FS.exists (FS.strz(name, tmp));
}

/*******************************************************************************

        Returns the time of the last modification. Accurate
        to whatever the F/S supports, and in a format dictated
        by the file-system. For example NTFS keeps UTC time, 
        while FAT timestamps are based on the local time. 

*******************************************************************************/

Time modified (char[] name)
{       
        return timeStamps(name).modified;
}

/*******************************************************************************

        Returns the time of the last access. Accurate to
        whatever the F/S supports, and in a format dictated
        by the file-system. For example NTFS keeps UTC time, 
        while FAT timestamps are based on the local time.

*******************************************************************************/

Time accessed (char[] name)
{
        return timeStamps(name).accessed;
}

/*******************************************************************************

        Returns the time of file creation. Accurate to
        whatever the F/S supports, and in a format dictated
        by the file-system. For example NTFS keeps UTC time,  
        while FAT timestamps are based on the local time.

*******************************************************************************/

Time created (char[] name)
{
        return timeStamps(name).created;
}

/*******************************************************************************

        Return the file length (in bytes)

*******************************************************************************/

ulong fileSize (char[] name)
{
        char[512] tmp = void;
        return FS.fileSize (FS.strz(name, tmp));
}

/*******************************************************************************

        Is this file writable?

*******************************************************************************/

bool isWritable (char[] name)
{
        char[512] tmp = void;
        return FS.isWritable (FS.strz(name, tmp));
}

/*******************************************************************************

        Is this file actually a folder/directory?

*******************************************************************************/

bool isFolder (char[] name)
{
        char[512] tmp = void;
        return FS.isFolder (FS.strz(name, tmp));
}

/*******************************************************************************

        Is this file actually a normal file?
        Not a directory or (on unix) a device file or link.

*******************************************************************************/

bool isFile (char[] name)
{
        char[512] tmp = void;
        return FS.isFile (FS.strz(name, tmp));
}

/*******************************************************************************

        Return timestamp information

        Timestamps are returns in a format dictated by the 
        file-system. For example NTFS keeps UTC time, 
        while FAT timestamps are based on the local time

*******************************************************************************/

FS.Stamps timeStamps (char[] name)
{
        char[512] tmp = void;
        return FS.timeStamps (FS.strz(name, tmp));
}

/*******************************************************************************

        Set the accessed and modified timestamps of the specified file

        Since 0.99.9

*******************************************************************************/

void timeStamps (char[] name, Time accessed, Time modified)
{
        char[512] tmp = void;
        FS.timeStamps (FS.strz(name, tmp), accessed, modified);
}

/*******************************************************************************

        Remove the file/directory from the file-system. Returns true if
        successful, false otherwise

*******************************************************************************/

bool remove (char[] name)
{      
        char[512] tmp = void;
        return FS.remove (FS.strz(name, tmp));
}

/*******************************************************************************

        Remove the files and folders listed in the provided paths. Where
        folders are listed, they should be preceded by their contained
        files in order to be successfuy removed. Returns a set of paths
        that failed to be removed (where .length is zero upon success).

        The collate() function can be used to provide the input paths:
        ---
        remove (collate (".", "*.d", true));
        ---

        Use with great caution

        Note: may allocate memory

        Since: 0.99.9

*******************************************************************************/

char[][] remove (char[][] paths)
{     
        char[][] failed;
        foreach (path; paths)
                 if (! remove (path))
                       failed ~= path;
        return failed;
}

/*******************************************************************************

        Create a new file

*******************************************************************************/

void createFile (char[] name)
{
        char[512] tmp = void;
        FS.createFile (FS.strz(name, tmp));
}

/*******************************************************************************

        Create a new directory

*******************************************************************************/

void createFolder (char[] name)
{
        char[512] tmp = void;
        FS.createFolder (FS.strz(name, tmp));
}

/*******************************************************************************

        Create an entire path consisting of this folder along with
        all parent folders. The path should not contain '.' or '..'
        segments, which can be removed via the normalize() function.

        Note that each segment is created as a folder, including the
        trailing segment.

        Throws: IOException upon system errors

        Throws: IllegalArgumentException if a segment exists but as a 
        file instead of a folder

*******************************************************************************/

void createPath (char[] path)
{
        void test (char[] segment)
        {
                if (segment.length)
                    if (! exists (segment))
                          createFolder (segment);
                    else
                       if (! isFolder (segment))
                             throw new IllegalArgumentException ("Path.createPath :: file/folder conflict: " ~ segment);
        }

        foreach (i, char c; path)
                 if (c is '/')
                     test (path [0 .. i]);
        test (path);
}

/*******************************************************************************

       change the name or location of a file/directory

*******************************************************************************/

void rename (char[] src, char[] dst)
{
        char[512] tmp1 = void;
        char[512] tmp2 = void;
        FS.rename (FS.strz(src, tmp1), FS.strz(dst, tmp2));
}

/*******************************************************************************

        Transfer the content of one file to another. Throws 
        an IOException upon failure.

*******************************************************************************/

void copy (char[] src, char[] dst)
{
        char[512] tmp1 = void;
        char[512] tmp2 = void;
        FS.copy (FS.strz(src, tmp1), FS.strz(dst, tmp2));
}

/*******************************************************************************

        Provides foreach support via a fruct, as in
        ---
        foreach (info; children("myfolder"))
                 ...
        ---

        Each path and filename is passed to the foreach
        delegate, along with the path prefix and whether
        the entry is a folder or not. The info construct
        exposes the following attributes:
        ---
        char[]  path
        char[]  name
        ulong   bytes
        bool    folder
        ---

        Argument 'all' controls whether hidden and system 
        files are included - these are ignored by default

*******************************************************************************/

FS.Listing children (char[] path, bool all=false)
{
        return FS.Listing (path, all);
}

/*******************************************************************************

        Collate all files and folders from the given path whose name matches
        the given pattern. Folders will be traversed where recurse is enabled, 
        and a set of matching names is returned as filepaths (including those 
        folders which match the pattern)

        Note: allocates memory for returned paths

        Since: 0.99.9

*******************************************************************************/

char[][] collate (char[] path, char[] pattern, bool recurse=false)
{      
        char[][] list;

        foreach (info; children (path))
                {
                if (info.folder && recurse)
                    list ~= collate (join(info.path, info.name), pattern, true);

                if (patternMatch (info.name, pattern))
                    list ~= join (info.path, info.name);
                }
        return list;
}

/*******************************************************************************

        Join a set of path specs together. A path separator is
        potentially inserted between each of the segments.

        Note: may allocate memory

*******************************************************************************/

char[] join (char[][] paths...)
{
        return FS.join (paths);
}

/*******************************************************************************

        Convert path separators to a standard format, using '/' as
        the path separator. This is compatible with Uri and all of 
        the contemporary O/S which Tango supports. Known exceptions
        include the Windows command-line processor, which considers
        '/' characters to be switches instead. Use the native()
        method to support that.

        Note: mutates the provided path.

*******************************************************************************/

char[] standard (char[] path)
{
        return replace (path, '\\', '/');
}

/*******************************************************************************

        Convert to native O/S path separators where that is required,
        such as when dealing with the Windows command-line. 
        
        Note: mutates the provided path. Use this pattern to obtain a 
        copy instead: native(path.dup);

*******************************************************************************/

char[] native (char[] path)
{
        version (Win32)
                 replace (path, '/', '\\');
        return path;
}

/*******************************************************************************

        Returns a path representing the parent of this one, with a special 
        case concerning a trailing '/':
        ---
        normal:  /x/y/z => /x/y
        special: /x/y/  => /x
        final:   /x     => empty
        ---

*******************************************************************************/

char[] pop (char[] path)
{
        path = FS.stripped (path);
        int i = path.length;
        while (i && path[--i] != '/') {}
        return path [0..i];
}

/*******************************************************************************

        Break a path into "head" and "tail" components. For example: 
        ---
        "/a/b/c" -> "/a","b/c" 
        "a/b/c" -> "a","b/c" 
        ---

*******************************************************************************/

char[] split (char[] path, out char[] head, out char[] tail)
{
        head = path;
        if (path.length > 1)
            foreach (i, char c; path[1..$])
                     if (c is '/')
                        {
                        head = path [0 .. i+1];
                        tail = path [i+2 .. $];
                        break;
                        }
        return path;
}

/*******************************************************************************

        Replace all path 'from' instances with 'to', in place (overwrites
        the provided path)

*******************************************************************************/

char[] replace (char[] path, char from, char to)
{
        foreach (ref char c; path)
                 if (c is from)
                     c = to;
        return path;
}

/*******************************************************************************

        Parse a path into its constituent components. 
        
        Note that the provided path is not duplicated

*******************************************************************************/

PathParser parse (char[] path)
{
        PathParser p;
        
        p.parse (path);
        return p;
}

/*******************************************************************************

*******************************************************************************/

debug(UnitTest)
{
        unittest
        {
                assert(exists("Path.d"));
                    
                auto p = parse ("d:/foo/bar/file.ext");
                assert (p == "d:/foo/bar/file.ext");
                assert (p.root == "d:");
                assert (p.folder == "/foo/bar/");
                assert (p.path == "d:/foo/bar/");
                assert (p.file == "file.ext");
                assert (p.name == "file");
                assert (p.suffix == ".ext");
                assert (p.ext == "ext");
                assert (p.isChild == true);
                assert (p.isEmpty == false);
                assert (p.isAbsolute == true);
        }
}


/******************************************************************************

        Matches a pattern against a filename.

        Some characters of pattern have special a meaning (they are
        <i>meta-characters</i>) and <b>can't</b> be escaped. These are:
        <p><table>
        <tr><td><b>*</b></td>
        <td>Matches 0 or more instances of any character.</td></tr>
        <tr><td><b>?</b></td>
        <td>Matches exactly one instances of any character.</td></tr>
        <tr><td><b>[</b><i>chars</i><b>]</b></td>
        <td>Matches one instance of any character that appears
        between the brackets.</td></tr>
        <tr><td><b>[!</b><i>chars</i><b>]</b></td>
        <td>Matches one instance of any character that does not appear
        between the brackets after the exclamation mark.</td></tr>
        </table><p>
        Internally individual character comparisons are done calling
        charMatch(), so its rules apply here too. Note that path
        separators and dots don't stop a meta-character from matching
        further portions of the filename.

        Returns: true if pattern matches filename, false otherwise.

        Throws: Nothing.
        -----
        version (Win32)
                {
                patternMatch("foo.bar", "*") // => true
                patternMatch(r"foo/foo\bar", "f*b*r") // => true
                patternMatch("foo.bar", "f?bar") // => false
                patternMatch("Goo.bar", "[fg]???bar") // => true
                patternMatch(r"d:\foo\bar", "d*foo?bar") // => true
                }
        version (Posix)
                {
                patternMatch("Go*.bar", "[fg]???bar") // => false
                patternMatch("/foo*home/bar", "?foo*bar") // => true
                patternMatch("foobar", "foo?bar") // => true
                }
        -----
    
******************************************************************************/

bool patternMatch (char[] filename, char[] pattern)
in
{
        // Verify that pattern[] is valid
        bool inbracket = false;
        for (auto i=0; i < pattern.length; i++)
            {
            switch (pattern[i])
                   {
                   case '[':
                        assert(!inbracket);
                        inbracket = true;
                        break;
                   case ']':
                        assert(inbracket);
                        inbracket = false;
                        break;
                   default:
                        break;
                   }
            }
}
body
{
        int pi;
        int ni;
        char pc;
        char nc;
        int j;
        int not;
        int anymatch;

        bool charMatch (char c1, char c2)
        {
        version (Win32)
                {
                if (c1 != c2)
                    return ((c1 >= 'a' && c1 <= 'z') ? c1 - ('a' - 'A') : c1) ==
                           ((c2 >= 'a' && c2 <= 'z') ? c2 - ('a' - 'A') : c2);
                return true;
                }
        else version (Escape)
                 return c1 == c2;
        else version (Posix)
                 return c1 == c2;
        }

        ni = 0;
        for (pi = 0; pi < pattern.length; pi++)
            {
            pc = pattern [pi];
            switch (pc)
                   {
                   case '*':
                        if (pi + 1 == pattern.length)
                            goto match;
                        for (j = ni; j < filename.length; j++)
                            {
                            if (patternMatch(filename[j .. filename.length],
                                pattern[pi + 1 .. pattern.length]))
                               goto match;
                            }
                        goto nomatch;

                   case '?':
                        if (ni == filename.length)
                            goto nomatch;
                        ni++;
                        break;

                   case '[':
                        if (ni == filename.length)
                            goto nomatch;
                        nc = filename[ni];
                        ni++;
                        not = 0;
                        pi++;
                        if (pattern[pi] == '!')
                           {
                           not = 1;
                           pi++;
                           }
                        anymatch = 0;
                        while (1)
                              {
                              pc = pattern[pi];
                              if (pc == ']')
                                  break;
                              if (!anymatch && charMatch(nc, pc))
                                   anymatch = 1;
                              pi++;
                              }
                        if (!(anymatch ^ not))
                              goto nomatch;
                        break;

                   default:
                        if (ni == filename.length)
                            goto nomatch;
                        nc = filename[ni];
                        if (!charMatch(pc, nc))
                             goto nomatch;
                        ni++;
                        break;
                   }
            }
        if (ni < filename.length)
            goto nomatch;

        match:
            return true;

        nomatch:
            return false;
}

/*******************************************************************************

*******************************************************************************/

debug (UnitTest)
{
        unittest
        {
        version (Win32)
        	assert(patternMatch("foo", "Foo"));
        else version (Escape)
        	assert(!patternMatch("foo", "Foo"));
        else version (Posix)
        	assert(!patternMatch("foo", "Foo"));
        
        assert(patternMatch("foo", "*"));
        assert(patternMatch("foo.bar", "*"));
        assert(patternMatch("foo.bar", "*.*"));
        assert(patternMatch("foo.bar", "foo*"));
        assert(patternMatch("foo.bar", "f*bar"));
        assert(patternMatch("foo.bar", "f*b*r"));
        assert(patternMatch("foo.bar", "f???bar"));
        assert(patternMatch("foo.bar", "[fg]???bar"));
        assert(patternMatch("foo.bar", "[!gh]*bar"));
        
        assert(!patternMatch("foo", "bar"));
        assert(!patternMatch("foo", "*.*"));
        assert(!patternMatch("foo.bar", "f*baz"));
        assert(!patternMatch("foo.bar", "f*b*x"));
        assert(!patternMatch("foo.bar", "[gh]???bar"));
        assert(!patternMatch("foo.bar", "[!fg]*bar"));
        assert(!patternMatch("foo.bar", "[fg]???baz"));
        }
}


/*******************************************************************************

        Normalizes a path component
        ---
        . segments are removed
        <segment>/.. are removed
        ---

        Multiple consecutive forward slashes are replaced with a single 
        forward slash. On Windows, \ will be converted to / prior to any
        normalization.

        Note that any number of .. segments at the front is ignored,
        unless it is an absolute path, in which case they are removed.

        The input path is copied into either the provided buffer, or a heap
        allocated array if no buffer was provided. Normalization modifies
        this copy before returning the relevant slice.
        -----
        normalize("/home/foo/./bar/../../john/doe"); // => "/home/john/doe"
        -----

        Note: allocates memory

*******************************************************************************/

char[] normalize (char[] path, char[] buf = null)
{
        size_t  idx;            // Current position
        size_t  moveTo;         // Position to move
        bool    isAbsolute;     // Whether the path is absolute
        enum    {NodeStackLength = 64}

        // Starting positions of regular path segments are pushed 
        // on this stack to avoid backward scanning when .. segments 
        // are encountered
        size_t[NodeStackLength] nodeStack;
        size_t nodeStackTop;

        // Moves the path tail starting at the current position to 
        // moveTo. Then sets the current position to moveTo.
        void move ()
        {
                auto len = path.length - idx;
                memmove (path.ptr + moveTo, path.ptr + idx, len);
                path = path[0..moveTo + len];
                idx = moveTo;
        }

        // Checks if the character at the current position is a 
        // separator. If true, normalizes the separator to '/' on 
        // Windows and advances the current position to the next 
        // character.
        bool isSep (ref size_t i)
        {
                char c = path[i];
                version (Windows)
                        {
                        if (c == '\\')
                                path[i] = '/';
                        else if (c != '/')
                                return false;
                        }
                     else
                        {
                        if (c != '/')
                                return false;
                        }
                i++;
                return true;
        }

        if (buf is null)
            path = path.dup;
        else
           path = buf[0..path.length] = path;

        version (Windows)
        {
                // Skip Windows drive specifiers
                if (path.length >= 2 && path[1] == ':')
                   {
                   auto c = path[0];

                   if (c >= 'a' && c <= 'z')
                      {
                      path[0] = c - 32;
                      idx = 2;
                      }
                   else 
                      if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z')
                          idx = 2;
                   }
        }

        if (idx == path.length)
            return path;

        moveTo = idx;
        if (isSep(idx))
           {
           moveTo++; // preserve root separator.
           isAbsolute = true;
           }

        while (idx < path.length)
              {
              // Skip duplicate separators
              if (isSep(idx))
                  continue;

              if (path[idx] == '.')
                 {
                 // leave the current position at the start of 
                 // the segment
                 auto i = idx + 1;
                 if (i < path.length && path[i] == '.')
                    {
                    i++;
                    if (i == path.length || isSep(i))
                       {
                       // It is a '..' segment. If the stack is not 
                       // empty, set moveTo and the current position
                       // to the start position of the last found 
                       // regular segment
                       if (nodeStackTop > 0)
                           moveTo = nodeStack[--nodeStackTop];

                       // If no regular segment start positions on the 
                       // stack, drop the .. segment if it is absolute 
                       // path or, otherwise, advance moveTo and the 
                       // current position to the character after the 
                       // '..' segment
                       else 
                          if (!isAbsolute)
                             {
                             if (moveTo != idx)
                                {
                                i -= idx - moveTo;
                                move();
                                }
                             moveTo = i;
                             }
        
                       idx = i;
                       continue;
                       }
                    }
        
                 // If it is '.' segment, skip it.
                 if (i == path.length || isSep(i))
                    {
                    idx = i;
                    continue;
                    }
                 }

              // Remove excessive '/', '.' and/or '..' preceeding the 
              // segment
              if (moveTo != idx)
                  move();

              // Push the start position of the regular segment on the 
              // stack
              assert (nodeStackTop < NodeStackLength);
              nodeStack[nodeStackTop++] = idx;

              // Skip the regular segment and set moveTo to the position 
              // after the segment (including the trailing '/' if present)
              for (; idx < path.length && !isSep(idx); idx++) 
                  {}
              moveTo = idx;
              }

        if (moveTo != idx)
            move();
        return path;
}

/*******************************************************************************

*******************************************************************************/

debug (UnitTest)
{
        unittest
        {
        assert (normalize ("") == "");
        assert (normalize ("/home/../john/../.tango/.htaccess") == "/.tango/.htaccess");
        assert (normalize ("/home/../john/../.tango/foo.conf") == "/.tango/foo.conf");
        assert (normalize ("/home/john/.tango/foo.conf") == "/home/john/.tango/foo.conf");
        assert (normalize ("/foo/bar/.htaccess") == "/foo/bar/.htaccess");
        assert (normalize ("foo/bar/././.") == "foo/bar/");
        assert (normalize ("././foo/././././bar") == "foo/bar");
        assert (normalize ("/foo/../john") == "/john");
        assert (normalize ("foo/../john") == "john");
        assert (normalize ("foo/bar/..") == "foo/");
        assert (normalize ("foo/bar/../john") == "foo/john");
        assert (normalize ("foo/bar/doe/../../john") == "foo/john");
        assert (normalize ("foo/bar/doe/../../john/../bar") == "foo/bar");
        assert (normalize ("./foo/bar/doe") == "foo/bar/doe");
        assert (normalize ("./foo/bar/doe/../../john/../bar") == "foo/bar");
        assert (normalize ("./foo/bar/../../john/../bar") == "bar");
        assert (normalize ("foo/bar/./doe/../../john") == "foo/john");
        assert (normalize ("../../foo/bar") == "../../foo/bar");
        assert (normalize ("../../../foo/bar") == "../../../foo/bar");
        assert (normalize ("d/") == "d/");
        assert (normalize ("/home/john/./foo/bar.txt") == "/home/john/foo/bar.txt");
        assert (normalize ("/home//john") == "/home/john");

        assert (normalize("/../../bar/") == "/bar/");
        assert (normalize("/../../bar/../baz/./") == "/baz/");
        assert (normalize("/../../bar/boo/../baz/.bar/.") == "/bar/baz/.bar/");
        assert (normalize("../..///.///bar/..//..//baz/.//boo/..") == "../../../baz/");
        assert (normalize("./bar/./..boo/./..bar././/") == "bar/..boo/..bar./");
        assert (normalize("/bar/..") == "/");
        assert (normalize("bar/") == "bar/");
        assert (normalize(".../") == ".../");
        assert (normalize("///../foo") == "/foo");
        assert (normalize("./foo") == "foo");
        auto buf = new char[100];
        auto ret = normalize("foo/bar/./baz", buf);
        assert (ret.ptr == buf.ptr);
        assert (ret == "foo/bar/baz");

        version (Windows) 
                {
                assert (normalize ("\\foo\\..\\john") == "/john");
                assert (normalize ("foo\\..\\john") == "john");
                assert (normalize ("foo\\bar\\..") == "foo/");
                assert (normalize ("foo\\bar\\..\\john") == "foo/john");
                assert (normalize ("foo\\bar\\doe\\..\\..\\john") == "foo/john");
                assert (normalize ("foo\\bar\\doe\\..\\..\\john\\..\\bar") == "foo/bar");
                assert (normalize (".\\foo\\bar\\doe") == "foo/bar/doe");
                assert (normalize (".\\foo\\bar\\doe\\..\\..\\john\\..\\bar") == "foo/bar");
                assert (normalize (".\\foo\\bar\\..\\..\\john\\..\\bar") == "bar");
                assert (normalize ("foo\\bar\\.\\doe\\..\\..\\john") == "foo/john");
                assert (normalize ("..\\..\\foo\\bar") == "../../foo/bar");
                assert (normalize ("..\\..\\..\\foo\\bar") == "../../../foo/bar");
                assert (normalize(r"C:") == "C:");
                assert (normalize(r"C") == "C");
                assert (normalize(r"c:\") == "C:/");
                assert (normalize(r"C:\..\.\..\..\") == "C:/");
                assert (normalize(r"c:..\.\boo\") == "C:../boo/");
                assert (normalize(r"C:..\..\boo\foo\..\.\..\..\bar") == "C:../../../bar");
                assert (normalize(r"C:boo\..") == "C:");
                }
        }
}


/*******************************************************************************

*******************************************************************************/

debug (Path)
{
        import tango.io.Stdout;

        void main()
        { 
                foreach (file; collate (".", "*.d", true))
                         Stdout (file).newline;      
        }
}
