/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD:  
                        AFL 3.0: 

        version:        Mar 2004: Initial release
        version:        Feb 2007: Now using mutating paths

        author:         Kris, Chris Sauls (Win95 file support)

*******************************************************************************/

module tango.io.FileSystem;

private import tango.sys.Common;

private import tango.io.FilePath;

private import tango.core.Exception;

private import tango.io.Path : standard, native;

/*******************************************************************************

*******************************************************************************/

version (Win32)
        {
        private import Text = tango.text.Util;
        private extern (Windows) DWORD GetLogicalDriveStringsA (DWORD, LPSTR);
        private import tango.stdc.stringz : fromString16z, fromStringz;

        enum {        
            FILE_DEVICE_DISK = 7,
            IOCTL_DISK_BASE = FILE_DEVICE_DISK,
            METHOD_BUFFERED = 0,
            FILE_READ_ACCESS = 1
        }
        uint CTL_CODE(uint t, uint f, uint m, uint a) {
            return (t << 16) | (a << 14) | (f << 2) | m;
        }

        const IOCTL_DISK_GET_LENGTH_INFO = CTL_CODE(IOCTL_DISK_BASE,0x17,METHOD_BUFFERED,FILE_READ_ACCESS);
        }

version (Posix)
{
	private import tango.stdc.string;
	private import tango.stdc.posix.unistd,
	               tango.stdc.posix.sys.statvfs;
	
	private import tango.io.device.File;
	private import Integer = tango.text.convert.Integer;
}

version (Escape)
{
	private import tango.stdc.env;
	private import tango.stdc.string;
}

/*******************************************************************************

        Models an OS-specific file-system. Included here are methods to
        manipulate the current working directory, and to convert a path
        to its absolute form.

*******************************************************************************/

struct FileSystem
{
        /***********************************************************************

                Convert the provided path to an absolute path, using the
                current working directory where prefix is not provided. 
                If the given path is already an absolute path, return it 
                intact.

                Returns the provided path, adjusted as necessary

                deprecated: see FilePath.absolute

        ***********************************************************************/

        deprecated static FilePath toAbsolute (FilePath target, char[] prefix=null)
        {
                if (! target.isAbsolute)
                   {
                   if (prefix is null)
                       prefix = getDirectory;

                   target.prepend (target.padded(prefix));
                   }
                return target;
        }

        /***********************************************************************

                Convert the provided path to an absolute path, using the
                current working directory where prefix is not provided. 
                If the given path is already an absolute path, return it 
                intact.

                Returns the provided path, adjusted as necessary

                deprecated: see FilePath.absolute

        ***********************************************************************/

        deprecated static char[] toAbsolute (char[] path, char[] prefix=null)
        {
                scope target = new FilePath (path);
                return toAbsolute (target, prefix).toString;
        }

        /***********************************************************************

                Compare to paths for absolute equality. The given prefix
                is prepended to the paths where they are not already in
                absolute format (start with a '/'). Where prefix is not
                provided, the current working directory will be used

                Returns true if the paths are equivalent, false otherwise

                deprecated: see FilePath.equals

        ***********************************************************************/

        deprecated static bool equals (char[] path1, char[] path2, char[] prefix=null)
        {
                scope p1 = new FilePath (path1);
                scope p2 = new FilePath (path2);
                return (toAbsolute(p1, prefix) == toAbsolute(p2, prefix)) is 0;
        }

        /***********************************************************************

        ***********************************************************************/

        private static void exception (char[] msg)
        {
                throw new IOException (msg);
        }

        /***********************************************************************
        
                Windows specifics

        ***********************************************************************/

        version (Windows)
        {
                /***************************************************************

                        private helpers

                ***************************************************************/

                version (Win32SansUnicode)
                {
                        private static void windowsPath(char[] path, ref char[] result)
                        {
                                result[0..path.length] = path;
                                result[path.length] = 0;
                        }
                }
                else
                {
                        private static void windowsPath(char[] path, ref wchar[] result)
                        {
                                assert (path.length < result.length);
                                auto i = MultiByteToWideChar (CP_UTF8, 0, 
                                                              cast(PCHAR)path.ptr, 
                                                              path.length, 
                                                              result.ptr, result.length);
                                result[i] = 0;
                        }
                }

                /***************************************************************

                        Set the current working directory

                        deprecated: see Environment.cwd()

                ***************************************************************/

                deprecated static void setDirectory (char[] path)
                {
                        version (Win32SansUnicode)
                                {
                                char[MAX_PATH+1] tmp = void;
                                tmp[0..path.length] = path;
                                tmp[path.length] = 0;

                                if (! SetCurrentDirectoryA (tmp.ptr))
                                      exception ("Failed to set current directory");
                                }
                             else
                                {
                                // convert into output buffer
                                wchar[MAX_PATH+1] tmp = void;
                                assert (path.length < tmp.length);
                                auto i = MultiByteToWideChar (CP_UTF8, 0, 
                                                              cast(PCHAR)path.ptr, path.length, 
                                                              tmp.ptr, tmp.length);
                                tmp[i] = 0;

                                if (! SetCurrentDirectoryW (tmp.ptr))
                                      exception ("Failed to set current directory");
                                }
                }

                /***************************************************************

                        Return the current working directory

                        deprecated: see Environment.cwd()

                ***************************************************************/

                deprecated static char[] getDirectory ()
                {
                        char[] path;

                        version (Win32SansUnicode)
                                {
                                int len = GetCurrentDirectoryA (0, null);
                                auto dir = new char [len];
                                GetCurrentDirectoryA (len, dir.ptr);
                                if (len)
                                   {
                                   dir[len-1] = '/';                                   
                                   path = standard (dir);
                                   }
                                else
                                   exception ("Failed to get current directory");
                                }
                             else
                                {
                                wchar[MAX_PATH+2] tmp = void;

                                auto len = GetCurrentDirectoryW (0, null);
                                assert (len < tmp.length);
                                auto dir = new char [len * 3];
                                GetCurrentDirectoryW (len, tmp.ptr); 
                                auto i = WideCharToMultiByte (CP_UTF8, 0, tmp.ptr, len, 
                                                              cast(PCHAR)dir.ptr, dir.length, null, null);
                                if (len && i)
                                   {
                                   path = standard (dir[0..i]);
                                   path[$-1] = '/';
                                   }
                                else
                                   exception ("Failed to get current directory");
                                }

                        return path;
                }

                /***************************************************************
                        
                        List the set of root devices (C:, D: etc)

                ***************************************************************/

                static char[][] roots ()
                {
                        int             len;
                        char[]          str;
                        char[][]        roots;

                        // acquire drive strings
                        len = GetLogicalDriveStringsA (0, null);
                        if (len)
                           {
                           str = new char [len];
                           GetLogicalDriveStringsA (len, cast(PCHAR)str.ptr);

                           // split roots into seperate strings
                           roots = Text.delimit (str [0 .. $-1], "\0");
                           }
                        return roots;
                }

                private enum {
                    volumePathBufferLen = MAX_PATH + 6
                }
                
                private static TCHAR[] getVolumePath(char[] folder, TCHAR[] volPath_,
                                                     bool trailingBackslash)
                in {
                    assert (volPath_.length > 5);
                } body {
                    version (Win32SansUnicode) {
                        alias GetVolumePathNameA GetVolumePathName;
                        alias fromStringz fromStringzT;
                    }
                    else {
                        alias GetVolumePathNameW GetVolumePathName;
                        alias fromString16z fromStringzT;
                    }

                    // convert to (w)stringz
                    TCHAR[MAX_PATH+2] tmp_ = void;
                    TCHAR[] tmp = tmp_;
                    windowsPath(folder, tmp);

                    // we'd like to open a volume
                    volPath_[0..4] = `\\.\`;

                    if (!GetVolumePathName(tmp.ptr, volPath_.ptr+4, volPath_.length-4)) 
                        exception ("GetVolumePathName failed");
                    
                    TCHAR[] volPath;

                    // the path could have the volume/network prefix already
                    if (volPath_[4..6] != `\\`) {
                        volPath = fromStringzT(volPath_.ptr);
                    } else {
                        volPath = fromStringzT(volPath_[4..$].ptr);
                    }

                    // GetVolumePathName returns a path with a trailing backslash
                    // some winapi functions want that backslash, some don't
                    if ('\\' == volPath[$-1] && !trailingBackslash) {
                        volPath[$-1] = '\0';
                    }

                    return volPath;
                }
 
                /***************************************************************
 
                        Request how much free space in bytes is available on the 
                        disk/mountpoint where folder resides.

                        If a quota limit exists for this area, that will be taken 
                        into account unless superuser is set to true.

                        If a user has exceeded the quota, a negative number can 
                        be returned.

                        Note that the difference between total available space
                        and free space will not equal the combined size of the 
                        contents on the file system, since the numbers for the
                        functions here are calculated from the used blocks,
                        including those spent on metadata and file nodes.

                        If actual used space is wanted one should use the
                        statistics functionality of tango.io.vfs.

                        See also: totalSpace()

                        Since: 0.99.9

                ***************************************************************/

                static long freeSpace(char[] folder, bool superuser = false)
                {
                    scope fp = new FilePath(folder);

                    const bool wantTrailingBackslash = true;                    
                    TCHAR[volumePathBufferLen] volPathBuf;
                    auto volPath = getVolumePath(fp.native.toString, volPathBuf, wantTrailingBackslash);

                    version (Win32SansUnicode) {
                        alias GetDiskFreeSpaceExA GetDiskFreeSpaceEx;
                    } else {
                        alias GetDiskFreeSpaceExW GetDiskFreeSpaceEx;
                    }

                    ULARGE_INTEGER free, totalFree;
                    GetDiskFreeSpaceEx(volPath.ptr, &free, null, &totalFree);
                    return cast(long) (superuser ? totalFree : free).QuadPart;
                }

                /***************************************************************

                        Request how large in bytes the
                        disk/mountpoint where folder resides is.

                        If a quota limit exists for this area, then
                        that quota can be what will be returned unless superuser
                        is set to true. On Posix systems this distinction is not
                        made though.

                        NOTE Access to this information when _superuser is
                        set to true may only be available if the program is
                        run in superuser mode.

                        See also: freeSpace()

                        Since: 0.99.9

                ***************************************************************/

                static ulong totalSpace(char[] folder, bool superuser = false)
                {
                    version (Win32SansUnicode) {
                        alias GetDiskFreeSpaceExA GetDiskFreeSpaceEx;
                        alias CreateFileA CreateFile;
                    } else {
                        alias GetDiskFreeSpaceExW GetDiskFreeSpaceEx;
                        alias CreateFileW CreateFile;
                    }
                    
                    scope fp = new FilePath(folder);

                    bool wantTrailingBackslash = (false == superuser);                    
                    TCHAR[volumePathBufferLen] volPathBuf;
                    auto volPath = getVolumePath(fp.native.toString, volPathBuf, wantTrailingBackslash);

                    if (superuser) {
                        struct GET_LENGTH_INFORMATION {
                            LARGE_INTEGER Length;
                        }
                        GET_LENGTH_INFORMATION lenInfo;
                        DWORD numBytes;
                        OVERLAPPED overlap;
                        
                        HANDLE h = CreateFile(
                                volPath.ptr, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                null, OPEN_EXISTING, 0, null
                        );
                        
                        if (h == INVALID_HANDLE_VALUE) {
                            exception ("Failed to open volume for reading");
                        }
                                               
                        if (0 == DeviceIoControl(
                                h, IOCTL_DISK_GET_LENGTH_INFO, null , 0,
                                cast(void*)&lenInfo, lenInfo.sizeof, &numBytes, &overlap
                            )) {
                            exception ("IOCTL_DISK_GET_LENGTH_INFO failed:" ~ SysError.lastMsg);
                        }

                        return cast(ulong)lenInfo.Length.QuadPart;
                    }
                    else {
                        ULARGE_INTEGER total;
                        GetDiskFreeSpaceEx(volPath.ptr, null, &total, null);
                        return cast(ulong)total.QuadPart;
                    }
                }
        }

        /***********************************************************************

        ***********************************************************************/

        version (Posix)
        {
                /***************************************************************

                        Set the current working directory

                        deprecated: see Environment.cwd()

                ***************************************************************/

                deprecated static void setDirectory (char[] path)
                {
                        char[512] tmp = void;
                        tmp [path.length] = 0;
                        tmp[0..path.length] = path;

                        if (tango.stdc.posix.unistd.chdir (tmp.ptr))
                            exception ("Failed to set current directory");
                }

                /***************************************************************

                        Return the current working directory

                        deprecated: see Environment.cwd()

                ***************************************************************/

                deprecated static char[] getDirectory ()
                {
                        char[512] tmp = void;

                        char *s = tango.stdc.posix.unistd.getcwd (tmp.ptr, tmp.length);
                        if (s is null)
                            exception ("Failed to get current directory");

                        auto path = s[0 .. strlen(s)+1].dup;
                        path[$-1] = '/';
                        return path;
                }

                /***************************************************************

                        List the set of root devices.

                 ***************************************************************/

                static char[][] roots ()
                {
                        version(darwin)
                        {
                            assert(0);
                        }
                        else
                        {
                            char[] path = "";
                            char[][] list;
                            int spaces;

                            auto fc = new File("/etc/mtab");
                            scope (exit)
                                   fc.close;
                            
                            auto content = new char[cast(int) fc.length];
                            fc.input.read (content);
                            
                            for(int i = 0; i < content.length; i++)
                            {
                                if(content[i] == ' ') spaces++;
                                else if(content[i] == '\n')
                                {
                                    spaces = 0;
                                    list ~= path;
                                    path = "";
                                }
                                else if(spaces == 1)
                                {
                                    if(content[i] == '\\')
                                    {
                                        path ~= Integer.parse(content[++i..i+3], 8u);
                                        i += 2;
                                    }
                                    else path ~= content[i];
                                }
                            }
                            
                            return list;
                        }
                }

                /***************************************************************
 
                        Request how much free space in bytes is available on the 
                        disk/mountpoint where folder resides.

                        If a quota limit exists for this area, that will be taken 
                        into account unless superuser is set to true.

                        If a user has exceeded the quota, a negative number can 
                        be returned.

                        Note that the difference between total available space
                        and free space will not equal the combined size of the 
                        contents on the file system, since the numbers for the
                        functions here are calculated from the used blocks,
                        including those spent on metadata and file nodes.

                        If actual used space is wanted one should use the
                        statistics functionality of tango.io.vfs.

                        See also: totalSpace()

                        Since: 0.99.9

                ***************************************************************/

                static long freeSpace(char[] folder, bool superuser = false)
                {
                    scope fp = new FilePath(folder);
                    statvfs_t info;
                    int res = statvfs(fp.native.cString.ptr, &info);
                    if (res == -1)
                        exception ("freeSpace->statvfs failed:"
                                   ~ SysError.lastMsg);

                    if (superuser)
                        return cast(long)info.f_bfree *  cast(long)info.f_bsize;
                    else
                        return cast(long)info.f_bavail * cast(long)info.f_bsize;
                }

                /***************************************************************

                        Request how large in bytes the
                        disk/mountpoint where folder resides is.

                        If a quota limit exists for this area, then
                        that quota can be what will be returned unless superuser
                        is set to true. On Posix systems this distinction is not
                        made though.

                        NOTE Access to this information when _superuser is
                        set to true may only be available if the program is
                        run in superuser mode.

                        See also: freeSpace()

                        Since: 0.99.9

                ***************************************************************/

                static long totalSpace(char[] folder, bool superuser = false)
                {
                    scope fp = new FilePath(folder);
                    statvfs_t info;
                    int res = statvfs(fp.native.cString.ptr, &info);
                    if (res == -1)
                        exception ("totalSpace->statvfs failed:"
                                   ~ SysError.lastMsg);

                    return cast(long)info.f_blocks *  cast(long)info.f_frsize;
                }
        }

        /***********************************************************************

        ***********************************************************************/

        version (Escape)
        {
                /***************************************************************

                        Set the current working directory

                        deprecated: see Environment.cwd()

                ***************************************************************/

                deprecated static void setDirectory (char[] path)
                {
                        char[512] tmp = void;
                        tmp [path.length] = 0;
                        tmp[0..path.length] = path;

                        if (.setEnv("CWD",tmp.ptr) < 0)
                            exception ("Failed to set current directory");
                }

                /***************************************************************

                        Return the current working directory

                        deprecated: see Environment.cwd()

                ***************************************************************/

                deprecated static char[] getDirectory ()
                {
                        char[512] tmp = void;

                        if (.getEnv(tmp.ptr,tmp.length,"CWD") < 0)
                            exception ("Failed to get current directory");

                        auto path = tmp[0 .. strlen(tmp.ptr)+1].dup;
                        path[$-1] = '/';
                        return path;
                }

                /***************************************************************

                        List the set of root devices.

                 ***************************************************************/

                static char[][] roots ()
                {
                        assert(0);
                }

                /***************************************************************
 
                        Request how much free space in bytes is available on the 
                        disk/mountpoint where folder resides.

                        If a quota limit exists for this area, that will be taken 
                        into account unless superuser is set to true.

                        If a user has exceeded the quota, a negative number can 
                        be returned.

                        Note that the difference between total available space
                        and free space will not equal the combined size of the 
                        contents on the file system, since the numbers for the
                        functions here are calculated from the used blocks,
                        including those spent on metadata and file nodes.

                        If actual used space is wanted one should use the
                        statistics functionality of tango.io.vfs.

                        See also: totalSpace()

                        Since: 0.99.9

                ***************************************************************/

                static long freeSpace(char[] folder, bool superuser = false)
                {
                	// TODO
                	assert(0);
                	return 0;
                }

                /***************************************************************

                        Request how large in bytes the
                        disk/mountpoint where folder resides is.

                        If a quota limit exists for this area, then
                        that quota can be what will be returned unless superuser
                        is set to true. On Posix systems this distinction is not
                        made though.

                        NOTE Access to this information when _superuser is
                        set to true may only be available if the program is
                        run in superuser mode.

                        See also: freeSpace()

                        Since: 0.99.9

                ***************************************************************/

                static long totalSpace(char[] folder, bool superuser = false)
                {
                	// TODO
                	assert(0);
                	return 0;
                }
        }
}


/******************************************************************************

******************************************************************************/

debug (FileSystem)
{
        import tango.io.Stdout;

        static void foo (FilePath path)
        {
        Stdout("all: ") (path).newline;
        Stdout("path: ") (path.path).newline;
        Stdout("file: ") (path.file).newline;
        Stdout("folder: ") (path.folder).newline;
        Stdout("name: ") (path.name).newline;
        Stdout("ext: ") (path.ext).newline;
        Stdout("suffix: ") (path.suffix).newline.newline;
        }

        void main() 
        {
        Stdout.formatln ("dir: {}", FileSystem.getDirectory);

        auto path = new FilePath (".");
        foo (path);

        path.set ("..");
        foo (path); 

        path.set ("...");
        foo (path); 

        path.set (r"/x/y/.file");
        foo (path); 

        path.suffix = ".foo";
        foo (path);

        path.set ("file.bar");
        path.absolute("c:/prefix");
        foo(path);

        path.set (r"arf/test");
        foo(path);
        path.absolute("c:/prefix");
        foo(path);

        path.name = "foo";
        foo(path);

        path.suffix = ".d";
        path.name = path.suffix;
        foo(path);

        }
}
