/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: March 2004

        author:         $(UL Kris)
                        $(UL Brad Anderson)
                        $(UL teqdruid)
                        $(UL Anders (Darwin support))
                        $(UL Chris Sauls (Win95 file support))

*******************************************************************************/

module tango.io.FileProxy;

private import  tango.sys.Common;

public  import  tango.io.FilePath;

private import  tango.core.Exception;

/*******************************************************************************

*******************************************************************************/

version (Win32)
        {
        private import Utf = tango.text.convert.Utf;

        extern (Windows) BOOL   MoveFileExA (LPCSTR,LPCSTR,DWORD);
        extern (Windows) BOOL   MoveFileExW (LPCWSTR,LPCWSTR,DWORD);

        private const DWORD     REPLACE_EXISTING   = 1,
                                COPY_ALLOWED       = 2,
                                DELAY_UNTIL_REBOOT = 4,
                                WRITE_THROUGH      = 8;

        version (Win32SansUnicode)
                {
                alias char T;
                private extern (C) int strlen (char *s);
                private alias WIN32_FIND_DATA FIND_DATA;
                }
             else
                {
                alias wchar T;
                private extern (C) int wcslen (wchar *s);
                private alias WIN32_FIND_DATAW FIND_DATA;
                }
        }


version (Posix)
        {
        private import tango.stdc.stdio;
        private import tango.stdc.string;
        private import tango.stdc.posix.dirent;
        }

version (Escape)
{
	private import tango.stdc.stdio;
	private import tango.stdc.string;
	private import tango.stdc.io;
	private import tango.stdc.stat;
	private import tango.stdc.dir;
}


/*******************************************************************************

        Models a generic file. Use this to manipulate files and directories
        in conjunction with FilePath, FileSystem and FileConduit.

        Compile with -version=Win32SansUnicode to enable Win95 & Win32s file
        support.

*******************************************************************************/

class FileProxy
{
        private FilePath path;

        /***********************************************************************

                Construct a FileProxy from the provided FilePath

        ***********************************************************************/

        this (FilePath path)
        {
                this.path = path;
        }

        /***********************************************************************

                Construct a FileProxy from a text string

        ***********************************************************************/

        this (char[] path)
        {
                this (new FilePath (path));
        }

        /***********************************************************************

                Return the FilePath associated with this FileProxy

        ***********************************************************************/

        FilePath getPath ()
        {
                return path;
        }

        /***********************************************************************

                Return the name of the associated path

        ***********************************************************************/

        char[] toUtf8 ()
        {
                return path.toUtf8;
        }

        /***********************************************************************

                Does this path currently exist?

        ***********************************************************************/

        bool isExisting ()
        {
                try {
                    getSize();
                    return true;
                    } catch (IOException){}
                return false;
        }

        /***********************************************************************

                List the set of filenames within this directory. All
                filenames are null terminated, though the null itself
                is hidden at the end of each name (not exposed by the
                length property)

                Each filename includes the parent prefix

        ***********************************************************************/

        char[][] toList ()
        {
                int      i;
                char[][] list;

                void add (char[] prefix, char[] name, bool dir)
                {
                        if (i >= list.length)
                            list.length = list.length * 2;

                        // duplicate the path, including the null. Note that
                        // the saved length *excludes* the terminator
                        list[i++] = (prefix ~ name)[0 .. $-1];
                }

                list = new char[][1024];
                toList (&add);
                return list [0 .. i];
        }

        /***********************************************************************

                Throw an exception using the last known error

        ***********************************************************************/

        private void exception ()
        {
                throw new IOException (path.toUtf8 ~ ": " ~ SysError.lastMsg);
        }

        /***********************************************************************

        ***********************************************************************/

        version (Win32)
        {
                // have to convert the filepath to utf16
                private wchar[]         widepath;
                private wchar[MAX_PATH] widepathsink = void;

                /***************************************************************

                        Cache a wchar[] instance of the filepath

                ***************************************************************/

                private wchar[] name16()
                {
                        if (widepath is null)
                            widepath = Utf.toUtf16 (path.cString, widepathsink);
                        return widepath;
                }

                /***************************************************************

                        Get info about this path

                ***************************************************************/

                private DWORD getInfo (inout WIN32_FILE_ATTRIBUTE_DATA info)
                {
                        version (Win32SansUnicode)
                                {
                                if (! GetFileAttributesExA (path.cString.ptr, GetFileInfoLevelStandard, &info))
                                      exception;
                                }
                             else
                                {
                                if (! GetFileAttributesExW (name16.ptr, GetFileInfoLevelStandard, &info))
                                      exception;
                                }

                        return info.dwFileAttributes;
                }

                /***************************************************************

                        Get flags for this path

                ***************************************************************/

                private DWORD getFlags ()
                {
                        WIN32_FILE_ATTRIBUTE_DATA info;

                        return getInfo (info);
                }

                /***************************************************************

                        Return the file length (in bytes)

                ***************************************************************/

                ulong getSize ()
                {
                        WIN32_FILE_ATTRIBUTE_DATA info;

                        getInfo (info);
                        return (cast(ulong) info.nFileSizeHigh << 32) +
                                            info.nFileSizeLow;
                }

                /***************************************************************

                        Is this file writable?

                ***************************************************************/

                bool isWritable ()
                {
                        return (getFlags & FILE_ATTRIBUTE_READONLY) == 0;
                }

                /***************************************************************

                        Is this file really a directory?

                ***************************************************************/

                bool isDirectory ()
                {
                        return (getFlags & FILE_ATTRIBUTE_DIRECTORY) != 0;
                }

                /***************************************************************

                        Return the time when the file was last modified

                ***************************************************************/

                ulong getModifiedTime ()
                {
                        WIN32_FILE_ATTRIBUTE_DATA info;

                        getInfo (info);
                        return (cast(ulong) info.ftLastWriteTime.dwHighDateTime << 32) +
                                            info.ftLastWriteTime.dwLowDateTime;
                }

                /***************************************************************

                        Return the time when the file was last accessed

                ***************************************************************/

                ulong getAccessedTime ()
                {
                        WIN32_FILE_ATTRIBUTE_DATA info;

                        getInfo (info);
                        return (cast(ulong) info.ftLastAccessTime.dwHighDateTime << 32) +
                                            info.ftLastAccessTime.dwLowDateTime;
                }

                /***************************************************************

                        Return the time when the file was created

                ***************************************************************/

                ulong getCreatedTime ()
                {
                        WIN32_FILE_ATTRIBUTE_DATA info;

                        getInfo (info);
                        return (cast(ulong) info.ftCreationTime.dwHighDateTime << 32) +
                                            info.ftCreationTime.dwLowDateTime;
                }

                /***************************************************************

                        Remove the file/directory from the file-system

                ***************************************************************/

                FileProxy remove ()
                {
                        if (isDirectory)
                           {
                           version (Win32SansUnicode)
                                   {
                                   if (! RemoveDirectoryA (path.cString.ptr))
                                         exception();
                                   }
                                else
                                   {
                                   if (! RemoveDirectoryW (name16.ptr))
                                         exception();
                                   }
                           }
                        else
                           version (Win32SansUnicode)
                                   {
                                   if (! DeleteFileA (path.cString.ptr))
                                         exception();
                                   }
                                else
                                   {
                                   if (! DeleteFileW (name16.ptr))
                                         exception();
                                   }

                        return this;
                }

                /***************************************************************

                       change the name or location of a file/directory, and
                       adopt the provided FilePath

                ***************************************************************/

                FileProxy rename (FilePath dst)
                {
                        const int Typical = REPLACE_EXISTING + COPY_ALLOWED +
                                                               WRITE_THROUGH;

                        int result;

                        version (Win32SansUnicode)
                                 result = MoveFileExA (path.cString.ptr, dst.cString.ptr, Typical);
                             else
                                result = MoveFileExW (name16.ptr, Utf.toUtf16(dst.cString).ptr, Typical);

                        if (! result)
                              exception;

                        path = dst;
                        return this;
                }

                /***************************************************************

                        Create a new file

                ***************************************************************/

                FileProxy createFile ()
                {
                        HANDLE h;

                        version (Win32SansUnicode)
                                 h = CreateFileA (path.cString.ptr, GENERIC_WRITE,
                                                  0, null, CREATE_ALWAYS,
                                                  FILE_ATTRIBUTE_NORMAL, cast(HANDLE) 0);
                             else
                                h = CreateFileW (name16.ptr, GENERIC_WRITE,
                                                 0, null, CREATE_ALWAYS,
                                                 FILE_ATTRIBUTE_NORMAL, cast(HANDLE) 0);

                        if (h == INVALID_HANDLE_VALUE)
                            exception;

                        if (! CloseHandle (h))
                              exception;

                        return this;
                }

                /***************************************************************

                        Create a new directory

                ***************************************************************/

                FileProxy createFolder ()
                {
                        version (Win32SansUnicode)
                                {
                                if (! CreateDirectoryA (path.cString.ptr, null))
                                      exception;
                                }
                             else
                                {
                                if (! CreateDirectoryW (name16.ptr, null))
                                      exception;
                                }
                        return this;
                }

                /***************************************************************

                        List the set of filenames within this directory.

                        All filenames are null terminated and are passed
                        to the provided delegate as such, along with the
                        path prefix and whether the entry is a directory
                        or not.

                ***************************************************************/

                void toList (void delegate (char[], char[], bool) dg)
                {
                        HANDLE                  h;
                        char[]                  prefix;
                        FIND_DATA               fileinfo;
                        char[MAX_PATH+1]        tmp = void;

                        int next()
                        {
                                version (Win32SansUnicode)
                                         return FindNextFileA (h, &fileinfo);
                                   else
                                      return FindNextFileW (h, &fileinfo);
                        }

                        static T[] padded (T[] s, T[] ext)
                        {
                                if (s.length is 0 || s[$-1] != '\\')
                                    return s ~ "\\" ~ ext;
                                return s ~ ext;
                        }

                        version (Win32SansUnicode)
                                 h = FindFirstFileA (padded(path.toUtf8, "*\0").ptr, &fileinfo);
                             else
                                h = FindFirstFileW (padded(name16[0..$-1], "*\0").ptr, &fileinfo);

                        if (h is INVALID_HANDLE_VALUE)
                            exception;

                        scope (exit)
                               FindClose (h);

                        prefix = FilePath.asPadded(path.toUtf8);
                        do {
                           version (Win32SansUnicode)
                                   {
                                   // ensure we include the null
                                   auto len = strlen (fileinfo.cFileName.ptr) + 1;
                                   auto str = fileinfo.cFileName.ptr [0 .. len];
                                   }
                                else
                                   {
                                   // ensure we include the null
                                   auto len = wcslen (fileinfo.cFileName.ptr) + 1;
                                   auto str = Utf.toUtf8 (fileinfo.cFileName [0 .. len], tmp);
                                   }

                           // skip hidden/system files
                           if ((fileinfo.dwFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) is 0)
                                dg (prefix, str, (fileinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);

                           } while (next);
                }
        }


        /***********************************************************************

        ***********************************************************************/

        version (Posix)
        {
                /***************************************************************

                        Get info about this path

                ***************************************************************/

                private uint getInfo (inout stat_t stats)
                {
                        if (posix.stat (path.cString.ptr, &stats))
                            exception;

                        return stats.st_mode;
                }

                /***************************************************************

                        Return the file length (in bytes)

                ***************************************************************/

                ulong getSize ()
                {
                        stat_t stats;

                        getInfo (stats);
                        return cast(ulong) stats.st_size;    // 32 bits only
                }

                /***************************************************************

                        Is this file writable?

                ***************************************************************/

                bool isWritable ()
                {
                        stat_t stats;

                        return (getInfo(stats) & O_RDONLY) == 0;
                }

                /***************************************************************

                        Is this file really a directory?

                ***************************************************************/

                bool isDirectory ()
                {
                        stat_t stats;

                        return (getInfo(stats) & S_IFDIR) != 0;
                }

                /***************************************************************

                        Return the time when the file was last modified

                ***************************************************************/

                ulong getModifiedTime ()
                {
                        stat_t stats;

                        getInfo (stats);
                        return cast(ulong) stats.st_mtime;
                }

                /***************************************************************

                        Return the time when the file was last accessed

                ***************************************************************/

                ulong getAccessedTime ()
                {
                        stat_t stats;

                        getInfo (stats);
                        return cast(ulong) stats.st_atime;
                }

                /***************************************************************

                        Return the time when the file was created

                ***************************************************************/

                ulong getCreatedTime ()
                {
                        stat_t stats;

                        getInfo (stats);
                        return cast(ulong) stats.st_ctime;
                }

                /***************************************************************

                        Remove the file/directory from the file-system

                ***************************************************************/

                FileProxy remove ()
                {
                        if (isDirectory)
                           {
                           if (posix.rmdir (path.cString.ptr))
                               exception;
                           }
                        else
                           if (tango.stdc.stdio.remove (path.cString.ptr) == -1)
                               exception;

                        return this;
                }

                /***************************************************************

                       change the name or location of a file/directory, and
                       adopt the provided FilePath

                ***************************************************************/

                FileProxy rename (FilePath dst)
                {
                        if (tango.stdc.stdio.rename (path.cString.ptr, dst.cString.ptr) == -1)
                            exception;

                        path = dst;
                        return this;
                }

                /***************************************************************

                        Create a new file

                ***************************************************************/

                FileProxy createFile ()
                {
                        int fd;

                        fd = posix.open (path.cString.ptr, O_CREAT | O_WRONLY | O_TRUNC, 0660);
                        if (fd == -1)
                            exception;

                        if (posix.close(fd) == -1)
                            exception;

                        return this;
                }

                /***************************************************************

                        Create a new directory

                ***************************************************************/

                FileProxy createFolder ()
                {
                        if (posix.mkdir (path.cString.ptr, 0777))
                            exception;

                        return this;
                }

                /***************************************************************

                        List the set of filenames within this directory.

                        All filenames are null terminated and are passed
                        to the provided delegate as such, along with the
                        path prefix and whether the entry is a directory
                        or not.

                ***************************************************************/

                void toList (void delegate (char[], char[], bool) dg)
                {
                        DIR*            dir;
                        dirent*         entry;
                        char[]          prefix;

                        dir = tango.stdc.posix.dirent.opendir (path.cString.ptr);
                        if (! dir)
                              exception;

                        scope (exit)
                               tango.stdc.posix.dirent.closedir (dir);

                        prefix = FilePath.asPadded (path.toUtf8);

                        while ((entry = tango.stdc.posix.dirent.readdir(dir)) != null)
                              {
                              // ensure we include the terminating null ...
                              auto len = tango.stdc.string.strlen (entry.d_name.ptr)+1;
                              auto str = entry.d_name.ptr [0 .. len];

                              dg (prefix, str, entry.d_type is DT_DIR);
                              }
                }
        }


        /***********************************************************************

        ***********************************************************************/

        version (Escape)
        {
                /***************************************************************

                        Get info about this path

                ***************************************************************/

                private uint getInfo (inout stat_t stats)
                {
                        if (.stat (path.cString.ptr, &stats))
                            exception;

                        return stats.st_mode;
                }

                /***************************************************************

                        Return the file length (in bytes)

                ***************************************************************/

                ulong getSize ()
                {
                        stat_t stats;

                        getInfo (stats);
                        return cast(ulong) stats.st_size;    // 32 bits only
                }

                /***************************************************************

                        Is this file writable?

                ***************************************************************/

                bool isWritable ()
                {
                		// TODO atm always true
                	    return true;
                }

                /***************************************************************

                        Is this file really a directory?

                ***************************************************************/

                bool isDirectory ()
                {
                        stat_t stats;

                        return (getInfo(stats) & S_IFDIR) != 0;
                }

                /***************************************************************

                        Return the time when the file was last modified

                ***************************************************************/

                ulong getModifiedTime ()
                {
                        stat_t stats;

                        getInfo (stats);
                        return cast(ulong) stats.st_mtime;
                }

                /***************************************************************

                        Return the time when the file was last accessed

                ***************************************************************/

                ulong getAccessedTime ()
                {
                        stat_t stats;

                        getInfo (stats);
                        return cast(ulong) stats.st_atime;
                }

                /***************************************************************

                        Return the time when the file was created

                ***************************************************************/

                ulong getCreatedTime ()
                {
                        stat_t stats;

                        getInfo (stats);
                        return cast(ulong) stats.st_ctime;
                }

                /***************************************************************

                        Remove the file/directory from the file-system

                ***************************************************************/

                FileProxy remove ()
                {
                        if (isDirectory)
                           {
                           if (.rmdir (path.cString.ptr))
                               exception;
                           }
                        else
                           if (.unlink (path.cString.ptr) == -1)
                               exception;

                        return this;
                }

                /***************************************************************

                        Create a new file

                ***************************************************************/

                FileProxy createFile ()
                {
                        int fd;

                        fd = .open (path.cString.ptr, IO_CREATE | IO_WRITE | IO_TRUNCATE);
                        if (fd == -1)
                            exception;

                        .close(fd);
                        return this;
                }

                /***************************************************************

                        Create a new directory

                ***************************************************************/

                FileProxy createFolder ()
                {
                        if (.mkdir (path.cString.ptr))
                            exception;

                        return this;
                }

                /***************************************************************

                        List the set of filenames within this directory.

                        All filenames are null terminated and are passed
                        to the provided delegate as such, along with the
                        path prefix and whether the entry is a directory
                        or not.

                ***************************************************************/

                void toList (void delegate (char[], char[], bool) dg)
                {
                        int             dir;
                        dirent         entry;
                        char[]          prefix;
                        stat_t			st;

                        dir = .opendir (path.cString.ptr);
                        if (! dir)
                              exception;

                        scope (exit)
                               .closedir (dir);

                        prefix = FilePath.asPadded (path.toUtf8);

                        while (.readdir(&entry,dir))
                        {
	                        // ensure we include the terminating null ...
	                        auto len = tango.stdc.string.strlen (entry.name)+1;
	                        auto str = entry.name [0 .. len];
	
	                        stat((prefix ~ str).ptr, &st);
	                        dg (prefix, str, S_ISDIR(st.st_mode));
                        }
                }
        }
}

