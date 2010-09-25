/*******************************************************************************

        copyright:      Copyright (c) 2007 Tango. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Feb 2007: Initial release

        author:         Deewiant, Maxter, Gregor, Kris

*******************************************************************************/

module tango.sys.Environment;

private import  tango.sys.Common;

private import  tango.io.Path,
                tango.io.FilePath;

private import  tango.core.Exception;

private import  tango.io.model.IFile;

private import  Text = tango.text.Util;

/*******************************************************************************

        Platform decls

*******************************************************************************/

version (Windows)
{
        private import tango.text.convert.Utf;

        pragma (lib, "kernel32.lib");

        extern (Windows)
        {
                private void* GetEnvironmentStringsW();
                private bool FreeEnvironmentStringsW(wchar**);
        }
        extern (Windows)
        {
                private int SetEnvironmentVariableW(wchar*, wchar*);
                private uint GetEnvironmentVariableW(wchar*, wchar*, uint);
                private const int ERROR_ENVVAR_NOT_FOUND = 203;
        }
}
else version (Escape)
{
	private import tango.stdc.env;
	private import tango.stdc.stat;
	private import tango.stdc.string;
}
else
{
    version (darwin)
    {
        extern (C) char*** _NSGetEnviron();
        private char** environ;
        
        static this ()
        {
            environ = *_NSGetEnviron();
        }
    }
    
    else
        private extern (C) extern char** environ;

    import tango.stdc.posix.stdlib;
    import tango.stdc.string;
}


/*******************************************************************************

        Exposes the system Environment settings, along with some handy
        utilities

*******************************************************************************/

struct Environment
{
        public alias cwd directory;
                    
        /***********************************************************************

                Throw an exception

        ***********************************************************************/

        private static void exception (char[] msg)
        {
                throw new PlatformException (msg);
        }
        
        /***********************************************************************

            Returns an absolute version of the provided path, where cwd is used
            as the prefix.

            The provided path is returned as is if already absolute.

        ***********************************************************************/

        static char[] toAbsolute(char[] path)
        {
            scope fp = new FilePath(path);
            if (fp.isAbsolute)
                return path;

            fp.absolute(cwd);
            return fp.toString;
        }

        /***********************************************************************

                Returns the full path location of the provided executable
                file, rifling through the PATH as necessary.

                Returns null if the provided filename was not found

        ***********************************************************************/

        static FilePath exePath (char[] file)
        {
                auto bin = new FilePath (file);

                // on Windows, this is a .exe
                version (Windows)
                         if (bin.ext.length is 0)
                             bin.suffix = "exe";

                // is this a directory? Potentially make it absolute
                if (bin.isChild && !bin.isAbsolute)
                    return bin.absolute (cwd);

                // is it in cwd?
                version (Windows)
                         if (bin.path(cwd).exists)
                             return bin;

                // rifle through the path (after converting to standard format)
                foreach (pe; Text.patterns (standard(get("PATH")), FileConst.SystemPathString))
                         if (bin.path(pe).exists)
                             version (Windows)
                                      return bin;
                                  else
                                     {
                                     stat_t stats;
                                     stat(bin.cString.ptr, &stats);
                                     if (stats.st_mode & 0100)
                                         return bin;
                                     }
                return null;
        }

        /***********************************************************************

                Windows implementation

        ***********************************************************************/

        version (Windows)
        {
                /**************************************************************

                        Returns the provided 'def' value if the variable 
                        does not exist

                **************************************************************/

                static char[] get (char[] variable, char[] def = null)
                {
                        wchar[] var = toString16(variable) ~ "\0";

                        uint size = GetEnvironmentVariableW(var.ptr, cast(wchar*)null, 0);
                        if (size is 0)
                           {
                           if (SysError.lastCode is ERROR_ENVVAR_NOT_FOUND)
                               return def;
                           else
                              exception (SysError.lastMsg);
                           }

                        auto buffer = new wchar[size];
                        size = GetEnvironmentVariableW(var.ptr, buffer.ptr, size);
                        if (size is 0)
                            exception (SysError.lastMsg);

                        return toString (buffer[0 .. size]);
                }

                /**************************************************************

                        clears the variable if value is null or empty

                **************************************************************/

                static void set (char[] variable, char[] value = null)
                {
                        wchar * var, val;

                        var = (toString16 (variable) ~ "\0").ptr;

                        if (value.length > 0)
                            val = (toString16 (value) ~ "\0").ptr;

                        if (! SetEnvironmentVariableW(var, val))
                              exception (SysError.lastMsg);
                }

                /**************************************************************

                        Get all set environment variables as an associative
                        array.

                **************************************************************/

                static char[][char[]] get ()
                {
                        char[][char[]] arr;

                        wchar[] key = new wchar[20],
                                value = new wchar[40];

                        wchar** env = cast(wchar**) GetEnvironmentStringsW();
                        scope (exit)
                               FreeEnvironmentStringsW (env);

                        for (wchar* str = cast(wchar*) env; *str; ++str)
                            {
                            size_t k = 0, v = 0;

                            while (*str != '=')
                                  {
                                  key[k++] = *str++;

                                  if (k is key.length)
                                      key.length = 2 * key.length;
                                  }

                            ++str;

                            while (*str)
                                  {
                                  value [v++] = *str++;

                                  if (v is value.length)
                                      value.length = 2 * value.length;
                                  }

                            arr [toString(key[0 .. k])] = toString(value[0 .. v]);
                            }

                        return arr;
                }

                /**************************************************************

                        Set the current working directory

                **************************************************************/

                static void cwd (char[] path)
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

                /**************************************************************

                        Get the current working directory

                **************************************************************/

                static char[] cwd ()
                {
                        char[] path;

                        version (Win32SansUnicode)
                                {
                                int len = GetCurrentDirectoryA (0, null);
                                auto dir = new char [len];
                                GetCurrentDirectoryA (len, dir.ptr);
                                if (len)
                                   {
                                   if (dir[len-2] is '/')
                                       dir.length = len-1;
                                   else
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
                                   if (path[$-2] is '/')
                                       path.length = path.length-1;
                                   else
                                       path[$-1] = '/';
                                   }
                                else
                                   exception ("Failed to get current directory");
                                }

                        return path;
                }

        }

        /***********************************************************************

                Escape implementation

        ***********************************************************************/

        version (Escape)
        {
        		private const uint MAX_NAME_LEN = 64;
        	
                /**************************************************************

                        Returns the provided 'def' value if the variable 
                        does not exist

                **************************************************************/

                static char[] get (char[] variable, char[] def = null)
                {
                		char tmp[MAX_NAME_LEN];
                        if (.getenvto(tmp.ptr,MAX_NAME_LEN,(variable ~ '\0').ptr) < 0)
                            return def;

                        return tmp[0 .. strlen(tmp.ptr)].dup;
                }

                /**************************************************************

                        clears the variable, if value is null or empty
        
                **************************************************************/

                static void set (char[] variable, char[] value = null)
                {
                        int result;

                        // TODO not supported yet
                        //if (value.length is 0)
                        //    unsetenv ((variable ~ '\0').ptr);
                        //else
                           result = .setenv((variable ~ '\0').ptr, (value ~ '\0').ptr);

                        if (result < 0)
                            exception (SysError.lastMsg);
                }

                /**************************************************************

                        Get all set environment variables as an associative
                        array.

                **************************************************************/

                static char[][char[]] get ()
                {
                        char[][char[]] arr;
                        char tmp[MAX_NAME_LEN];
                        for(uint i = 0; .getenvito(tmp.ptr,MAX_NAME_LEN,i) >= 0; i++)
                        {
                        	char[] key = tmp[0..strlen(tmp.ptr)].dup;
                        	arr[key] = get(key);
                        }
                        return arr;
                }

                /**************************************************************

                        Set the current working directory

                **************************************************************/

                static void cwd (char[] path)
                {
                        set("CWD",path);
                }

                /**************************************************************

                        Get the current working directory

                **************************************************************/

                static char[] cwd ()
                {
                		auto path = get("CWD");
                        if (path[$-2] is '/') // root path has the slash
                            path.length = path.length-1;
                        else
                            path[$-1] = '/';
                        return path;
                }
        }

        /***********************************************************************

                Posix implementation

        ***********************************************************************/

		else version (Posix)
        {
                /**************************************************************

                        Returns the provided 'def' value if the variable 
                        does not exist

                **************************************************************/

                static char[] get (char[] variable, char[] def = null)
                {
                        char* ptr = getenv ((variable ~ '\0').ptr);

                        if (ptr is null)
                            return def;

                        return ptr[0 .. strlen(ptr)].dup;
                }

                /**************************************************************

                        clears the variable, if value is null or empty
        
                **************************************************************/

                static void set (char[] variable, char[] value = null)
                {
                        int result;

                        if (value.length is 0)
                            unsetenv ((variable ~ '\0').ptr);
                        else
                           result = setenv ((variable ~ '\0').ptr, (value ~ '\0').ptr, 1);

                        if (result != 0)
                            exception (SysError.lastMsg);
                }

                /**************************************************************

                        Get all set environment variables as an associative
                        array.

                **************************************************************/

                static char[][char[]] get ()
                {
                        char[][char[]] arr;

                        for (char** p = environ; *p; ++p)
                            {
                            size_t k = 0;
                            char* str = *p;

                            while (*str++ != '=')
                                   ++k;
                            char[] key = (*p)[0..k];

                            k = 0;
                            char* val = str;
                            while (*str++)
                                   ++k;
                            arr[key] = val[0 .. k];
                            }

                        return arr;
                }

                /**************************************************************

                        Set the current working directory

                **************************************************************/

                static void cwd (char[] path)
                {
                        char[512] tmp = void;
                        tmp [path.length] = 0;
                        tmp[0..path.length] = path;

                        if (tango.stdc.posix.unistd.chdir (tmp.ptr))
                            exception ("Failed to set current directory");
                }

                /**************************************************************

                        Get the current working directory

                **************************************************************/

                static char[] cwd ()
                {
                        char[512] tmp = void;

                        char *s = tango.stdc.posix.unistd.getcwd (tmp.ptr, tmp.length);
                        if (s is null)
                            exception ("Failed to get current directory");

                        auto path = s[0 .. strlen(s)+1].dup;
                        if (path[$-2] is '/') // root path has the slash
                            path.length = path.length-1;
                        else
                            path[$-1] = '/';
                        return path;
                }
        }
}

                
/*******************************************************************************


*******************************************************************************/

debug (Environment)
{
        import tango.io.Console;


        void main(char[][] args)
        {
        const char[] VAR = "TESTENVVAR";
        const char[] VAL1 = "VAL1";
        const char[] VAL2 = "VAL2";

        assert(Environment.get(VAR) is null);

        Environment.set(VAR, VAL1);
        assert(Environment.get(VAR) == VAL1);

        Environment.set(VAR, VAL2);
        assert(Environment.get(VAR) == VAL2);

        Environment.set(VAR, null);
        assert(Environment.get(VAR) is null);

        Environment.set(VAR, VAL1);
        Environment.set(VAR, "");

        assert(Environment.get(VAR) is null);

        foreach (key, value; Environment.get)
                 Cout (key) ("=") (value).newline;

        if (args.length > 0)
           {
           auto p = Environment.exePath (args[0]);
           Cout (p).newline;
           }

        if (args.length > 1)
           {
           if (auto p = Environment.exePath (args[1]))
               Cout (p).newline;
           }
        }
}

