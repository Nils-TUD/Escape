/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Oct 2004: Initial version
        version:        Nov 2006: Australian version

        author:         Kris

*******************************************************************************/

module tango.io.FilePath;

private import tango.io.FileConst;

/*******************************************************************************

        Models a file path. These are expected to be used as the constructor
        argument to various file classes. The intention is that they easily
        convert to other representations such as absolute, canonical, or Url.

        File paths containing non-ansi characters should be UTF-8 encoded.
        Supporting Unicode in this manner was deemed to be more suitable
        than providing a wchar version of FilePath, and is both consistent
        & compatible with the approach taken with the Uri class.

        A FilePath is intended to be immutable, thus each mutating method
        returns a distinct entity. This approach reflects experience with
        typical FilePath instances as read-mostly entities, and suggests
        FilePath instances should be reasonably thread-safe. However, the
        getXYZ methods return slices of the internal content rather than
        provoking heap activity -- protecting said content from mutation
        is considered to be beyond the realm of this module.

*******************************************************************************/

class FilePath
{
        private char[]  fp;                     // filepath with trailing 0

        private bool    dir;                    // this represents a dir?

        private int     end,                    // before the trailing 0
                        ext,                    // after rightmost '.'
                        path,                   // path before name
                        name,                   // file/dir name
                        suffix;                 // inclusive of leftmost '.'


        /***********************************************************************

                Create a FilePath from a copy of the provided string.

                FilePath assumes both path & name are present, and therefore
                may split what is otherwise a logically valid path. That is,
                the 'name' of a file is typically the path segment following
                a rightmost path-separator. The intent is to treat files and
                directories in the same manner; as a name with an optional
                ancestral structure. It is possible to bias the interpretation
                by adding a trailing path-separator to the argument. Doing so
                will result in an empty name attribute.

                To ascertain if a FilePath exists on a system, or to access
                various other physical attributes, use methods exposed via
                tango.io.FileProxy and tango.io.File

                With regard to the filepath copy, we found the common case to
                be an explicit .dup, whereas aliasing appeared to be rare by
                comparison. We also noted a large proportion interacting with
                C-oriented OS calls, implying the postfix of a null terminator.
                Thus, FilePath combines both as a single operation.

        ***********************************************************************/

        this (char[] filepath)
        {
                this ((filepath ~ '\0').ptr, filepath.length + 1);
        }

        /***********************************************************************

                As above, but with C-style interface to bypass heap activity.
                The provided path and length *must* include a null terminator

        ***********************************************************************/

        package this (char* filepath, int len, bool dir=false)
        {
                path = 0;
                end  = len-1;
                name = suffix = ext = -1;
                fp   = filepath [0 .. len];

                // check for terminating null
                assert (*(filepath+end) is 0);

                for (int i=end; --i >= 0;)
                     switch (filepath[i])
                            {
                            case FileConst.FileSeparatorChar:
                                 if (name < 0)
                                    {
                                    suffix = i;
                                    if (ext < 0)
                                        ext = i + 1;
                                    }
                                 break;

                            case FileConst.PathSeparatorChar:
                                 if (name < 0)
                                     name = i + 1;
                                 break;

                            version (Win32)
                                    {
                                    case FileConst.RootSeparatorChar:
                                         path = i + 1;
                                         break;
                                    }
                            default:
                                 break;
                            }

                if (name < 0)
                    name = path;

                if (suffix < 0)
                    suffix = end;

                if (ext < 0)
                    ext = end;

                this.dir = dir;
        }

        /***********************************************************************

                Return the complete text of this filepath

        ***********************************************************************/

        final override char[] toUtf8 ()
        {
                return fp [0 .. end];
        }

        /***********************************************************************

                Return the root of this path. Roots are constructs such as
                "c:"

        ***********************************************************************/

        final char[] getRoot ()
        {
                return fp[0 .. path];
        }

        /***********************************************************************

                Return the file path. Paths may start and end with a "/".
                The root path is "/" and an unspecified path is returned as
                an empty string. Directory paths may be split such that the
                directory name is placed into the 'name' member; directory
                paths are treated no differently than file paths

        ***********************************************************************/

        final char[] getPath ()
        {
                return fp[path .. name];
        }

        /***********************************************************************

                return the root + path combination

        ***********************************************************************/

        final char[] getFullPath ()
        {
                return fp[0 .. name];
        }

        /***********************************************************************

                Return the name of this file, or directory.

        ***********************************************************************/

        final char[] getName ()
        {
                return fp[name .. suffix];
        }

        /***********************************************************************

                Suffix is like an extension, except it may include multiple
                '.' sequences and the dot-prefix is included in the suffix.
                For example, "wumpus.foo.bar" has suffix ".foo.bar"

        ***********************************************************************/

        final char[] getSuffix ()
        {
                return fp [suffix .. end];
        }

        /***********************************************************************

                Ext is the tail of the filename, rightward of the rightmost
                '.' separator. For example, "wumpus.foo.bar" has ext "bar"

        ***********************************************************************/

        final char[] getExt ()
        {
                return fp [ext .. end];
        }

        /***********************************************************************

                return the name + suffix combination

        ***********************************************************************/

        final char[] getFullName ()
        {
                return fp[name .. end];
        }

        /***********************************************************************

                Return a null-terminated version of this file path

        ***********************************************************************/

        final char[] cString ()
        {
                return fp;
        }

        /***********************************************************************

                Returns true if all fields are equal.

        ***********************************************************************/

        final override int opEquals (Object o)
        {
                return (this is o) || (o !is null && toUtf8 == o.toUtf8);
        }

        /***********************************************************************

                Returns true if this FilePath is *not* relative to the
                current working directory.

        ***********************************************************************/

        final bool isAbsolute ()
        {
                return (path > 0) ||
                       (path < end && fp[path] is FileConst.PathSeparatorChar);
        }

        /***********************************************************************

                Returns true if this FilePath is empty

        ***********************************************************************/

        final bool isEmpty ()
        {
                return end is 0;
        }

        /***********************************************************************

                Returns true if this FilePath has a parent

        ***********************************************************************/

        final bool isChild ()
        {
                auto path = getPath();

                for (int i=path.length; --i > 0;)
                     if (path[i] is FileConst.PathSeparatorChar)
                         return true;
                return false;
        }

        /***********************************************************************

                Returns true if this FilePath has been marked as a
                directory, via the constructor

        ***********************************************************************/

        final bool isDir ()
        {
                return dir;
        }

        /***********************************************************************

                Join this FilePath with the provided prefix.

                Assumes prefix is a directory name. If this is an absolute
                path it will be returned intact, ignoring prefix.

        ***********************************************************************/

        final FilePath join (char[] prefix)
        {
                if (isAbsolute)
                    return this;

                return new FilePath (asPadded(prefix) ~ toUtf8);
        }

        /***********************************************************************

                Join this FilePath with the provided prefix.

                Assumes prefix is a directory name. If this is an absolute
                path it will be returned intact, ignoring prefix.

        ***********************************************************************/

        final FilePath join (FilePath prefix)
        {
                return join (prefix.toUtf8);
        }

        /***********************************************************************

                Returns a path representing the parent of this one.

                Note that this returns a path suitable for splitting into
                path and name components (there's no trailing separator).

        ***********************************************************************/

        final char[] asParent ()
        {
                return asStripped (getFullPath);
        }

        /***********************************************************************

                Return a path with a different name and suffix.

        ***********************************************************************/

        final char[] asSibling (char[] s)
        {
                return getFullPath() ~ s;
        }

        /***********************************************************************

                Change the name of this FilePath. The prior suffix is
                retained

                Note that "." and ".." patterns are expected to be represented
                via the suffix rather than via the name.

        ***********************************************************************/

        final char[] asName (char[] s)
        {
                return getFullPath() ~ s ~ getSuffix();
        }

        /***********************************************************************

                Change the path of this FilePath. Non-empty paths are
                potentially adjusted to have a trailing separator

        ***********************************************************************/

        final char[] asPath (char[] s)
        {
                return getRoot() ~ asPadded(s) ~ getFullName();
        }

        /***********************************************************************

                Change the root of this FilePath. The root should have an
                appropriate trailing seperator

        ***********************************************************************/

        final char[] asRoot (char[] s)
        in {
           version( Win32 ){
               if (s.length)
                   assert (s[$-1] is FileConst.RootSeparatorChar, "root must end with a separator");
               }
           }
        body
        {
                return s ~ fp[path .. end];
        }

        /***********************************************************************

                Change the suffix of this FilePath. A non-empty suffix should
                start with a suffix separator (e.g. a '.' character).

                Note that "." and ".." patterns are expected to be represented
                via the suffix rather than via the name.

        ***********************************************************************/

        final char[] asSuffix (char[] s)
        in {
           if (s.length)
               assert (s[0] is FileConst.FileSeparatorChar, "suffix must begin with a separator");
           }
        body
        {
                return fp[0 .. suffix] ~ s;
        }

        /***********************************************************************

                Change the extension of this FilePath. The extension provided
                should exclude a leading separator -- use asSuffix() instead
                where a separator is embedded

                Note that providing an empty extension will strip a trailing
                separator from the returned path

        ***********************************************************************/

        final char[] asExt (char[] s)
        {
                if (s.length)
                    return asPadded (fp[0 .. ext], FileConst.FileSeparatorChar) ~ s;

                return asStripped (fp[0 .. ext], FileConst.FileSeparatorChar);
        }

        /***********************************************************************

                Convert path separators to the correct format according to
                the current platform. This mutates the provided 'path' content,
                so .dup it as necessary.

        ***********************************************************************/

        static char[] asNormal (char[] path)
        {
                version (Win32)
                         return replace (path, '/', '\\');
                     else
                         return replace (path, '\\', '/');
        }

        /***********************************************************************

                Return an adjusted path such that non-empty instances always
                have a trailing separator

        ***********************************************************************/

        static char[] asPadded (char[] path, char c = FileConst.PathSeparatorChar)
        {
                if (path.length && path[$-1] != c)
                    path ~= c;
                return path;
        }

        /***********************************************************************

                Return an adjusted path such that non-empty instances do not
                have a trailing separator

        ***********************************************************************/

        static char[] asStripped (char[] path, char c = FileConst.PathSeparatorChar)
        {
                if (path.length && path[$-1] is c)
                    path = path [0 .. $-1];
                return path;
        }

        /***********************************************************************

                Replace all 'from' instances in the provided path with 'to'.
                This mutates the provided 'path', so apply .dup as necessary

        ***********************************************************************/

        static char[] replace (char[] path, char from, char to)
        {
                foreach (inout char c; path)
                         if (c is from)
                             c = to;
                return path;
        }
}



/*******************************************************************************

*******************************************************************************/

debug (UnitTest)
{
        unittest
        {
        version (Win32)
                {
                auto fp = new FilePath(r"C:\home\foo\bar\john\");
                assert (fp.isAbsolute);
                assert (fp.getName == "");
                assert (fp.getPath == r"\home\foo\bar\john\");
                assert (fp.toUtf8 == r"C:\home\foo\bar\john\");
                assert (fp.getFullPath == r"C:\home\foo\bar\john\");
                assert (fp.getFullName == r"");
                assert (fp.getSuffix == r"");
                assert (fp.getRoot == r"C:");
                assert (fp.getExt == "", fp.getExt);
                assert (fp.isChild);

                fp = new FilePath(r"C:\home\foo\bar\john");
                assert (fp.isAbsolute);
                assert (fp.getName == "john");
                assert (fp.getPath == r"\home\foo\bar\");
                assert (fp.toUtf8 == r"C:\home\foo\bar\john");
                assert (fp.getFullPath == r"C:\home\foo\bar\");
                assert (fp.getFullName == r"john");
                assert (fp.getSuffix == r"");
                assert (fp.getExt == "");
                assert (fp.isChild);

                fp = new FilePath(fp.asParent);
                assert (fp.isAbsolute);
                assert (fp.getName == "bar");
                assert (fp.getPath == r"\home\foo\");
                assert (fp.toUtf8 == r"C:\home\foo\bar");
                assert (fp.getFullPath == r"C:\home\foo\");
                assert (fp.getFullName == r"bar");
                assert (fp.getSuffix == r"");
                assert (fp.getExt == "");
                assert (fp.isChild);

                fp = new FilePath(fp.asParent);
                assert (fp.isAbsolute);
                assert (fp.getName == "foo");
                assert (fp.getPath == r"\home\");
                assert (fp.toUtf8 == r"C:\home\foo");
                assert (fp.getFullPath == r"C:\home\");
                assert (fp.getFullName == r"foo");
                assert (fp.getSuffix == r"");
                assert (fp.getExt == "");
                assert (fp.isChild);

                fp = new FilePath(fp.asParent);
                assert (fp.isAbsolute);
                assert (fp.getName == "home");
                assert (fp.getPath == r"\");
                assert (fp.toUtf8 == r"C:\home");
                assert (fp.getFullPath == r"C:\");
                assert (fp.getFullName == r"home");
                assert (fp.getSuffix == r"");
                assert (fp.getExt == "");
                assert (!fp.isChild);

                fp = new FilePath(r"foo\bar\john.doe");
                assert (!fp.isAbsolute);
                assert (fp.getName == "john");
                assert (fp.getPath == r"foo\bar\");
                assert (fp.getSuffix == r".doe");
                assert (fp.getFullName == r"john.doe");
                assert (fp.toUtf8 == r"foo\bar\john.doe");
                assert (fp.getExt == "doe");
                assert (fp.isChild);

                fp = new FilePath(r"c:doe");
                assert (fp.isAbsolute);
                assert (fp.getSuffix == r"");
                assert (fp.toUtf8 == r"c:doe");
                assert (fp.getPath == r"");
                assert (fp.getName == "doe");
                assert (fp.getFullName == r"doe");
                assert (fp.getExt == "");
                assert (!fp.isChild);

                fp = new FilePath(r"\doe");
                assert (fp.isAbsolute);
                assert (fp.getSuffix == r"");
                assert (fp.toUtf8 == r"\doe");
                assert (fp.getName == "doe");
                assert (fp.getPath == r"\");
                assert (fp.getFullName == r"doe");
                assert (fp.getExt == "");
                assert (!fp.isChild);

                fp = new FilePath(r"john.doe.foo");
                assert (!fp.isAbsolute);
                assert (fp.getName == "john");
                assert (fp.getPath == r"");
                assert (fp.getSuffix == r".doe.foo");
                assert (fp.toUtf8 == r"john.doe.foo");
                assert (fp.getFullName == r"john.doe.foo");
                assert (fp.getExt == "foo");
                assert (!fp.isChild);

                fp = new FilePath(r".doe");
                assert (!fp.isAbsolute);
                assert (fp.getSuffix == r".doe");
                assert (fp.toUtf8 == r".doe");
                assert (fp.getName == "");
                assert (fp.getPath == r"");
                assert (fp.getFullName == r".doe");
                assert (fp.getExt == "doe");
                assert (!fp.isChild);

                fp = new FilePath(r"doe");
                assert (!fp.isAbsolute);
                assert (fp.getSuffix == r"");
                assert (fp.toUtf8 == r"doe");
                assert (fp.getName == "doe");
                assert (fp.getPath == r"");
                assert (fp.getFullName == r"doe");
                assert (fp.getExt == "");
                assert (!fp.isChild);

                fp = new FilePath(r".");
                assert (!fp.isAbsolute);
                assert (fp.getSuffix == r".");
                assert (fp.toUtf8 == r".");
                assert (fp.getName == "");
                assert (fp.getPath == r"");
                assert (fp.getFullName == r".");
                assert (fp.getExt == "");
                assert (!fp.isChild);

                fp = new FilePath(r"..");
                assert (!fp.isAbsolute);
                assert (fp.getSuffix == r"..");
                assert (fp.toUtf8 == r"..");
                assert (fp.getName == "");
                assert (fp.getPath == r"");
                assert (fp.getFullName == r"..");
                assert (fp.getExt == "");
                assert (!fp.isChild);

                fp = new FilePath(r"C:\foo\bar\test.bar");
                fp = new FilePath(fp.asPath ("foo"));
                assert (fp.getName == r"test");
                assert (fp.getPath == r"foo\");
                assert (fp.getFullPath == r"C:foo\");
                assert (fp.getExt == "bar");

                fp = new FilePath(fp.asPath (""));
                assert (fp.getName == r"test");
                assert (fp.getPath == r"");
                assert (fp.getFullPath == r"C:");
                assert (fp.getExt == "bar");

                fp = new FilePath("");
                assert (fp.isEmpty);
                assert (!fp.isChild);
                assert (!fp.isAbsolute);
                assert (fp.getSuffix == r"");
                assert (fp.toUtf8 == r"");
                assert (fp.getName == "");
                assert (fp.getPath == r"");
                assert (fp.getFullName == r"");
                assert (fp.getExt == "");

                fp = new FilePath(r"foo\bar\");
                assert(fp.join(r"c:\joe\bar").toUtf8 == r"c:\joe\bar\foo\bar\");
                assert(fp.join(new FilePath(r"c:\joe\bar")).toUtf8 == r"c:\joe\bar\foo\bar\");
                fp = new FilePath(r"c:\joe\bar");
                assert(fp.join(r"foo\bar\").toUtf8 == r"c:\joe\bar");
                assert(fp.join(new FilePath(r"foo\bar")).toUtf8 == r"c:\joe\bar");

                fp = new FilePath(r"c:\bar");
                assert(fp.join(r"foo").toUtf8 == r"c:\bar");
                assert(fp.join(new FilePath(r"foo")).toUtf8 == r"c:\bar");

                fp = new FilePath(r"bar\");
                assert(fp.join(r"c:\foo").toUtf8 == r"c:\foo\bar\");
                assert(fp.join(new FilePath(r"c:\foo")).toUtf8 == r"c:\foo\bar\");

                fp = new FilePath(r"C:\foo\bar\test.bar");
                assert (fp.asExt(null) == r"C:\foo\bar\test");
                assert (fp.asExt("foo") == r"C:\foo\bar\test.foo");
                }


        version (Posix)
                {
                /+
                 + TODO: Fix this test so it is not dependent on paths actually
                 +       being present in the system.
                 +
                auto fp = new FilePath(r"C:/home/foo/bar/john/");
                assert (fp.isAbsolute);
                assert (fp.getName == "");
                assert (fp.getPath == r"/home/foo/bar/john/");
                assert (fp.toUtf8 == r"C:/home/foo/bar/john/");
                assert (fp.getFullPath == r"C:/home/foo/bar/john/");
                assert (fp.getFullName == r"");
                assert (fp.getSuffix == r"");
                assert (fp.getRoot == r"C:");
                assert (fp.getExt == "", fp.getExt);
                assert (fp.isChild);

                fp = new FilePath(r"C:/home/foo/bar/john");
                assert (fp.isAbsolute);
                assert (fp.getName == "john");
                assert (fp.getPath == r"/home/foo/bar/");
                assert (fp.toUtf8 == r"C:/home/foo/bar/john");
                assert (fp.getFullPath == r"C:/home/foo/bar/");
                assert (fp.getFullName == r"john");
                assert (fp.getSuffix == r"");
                assert (fp.getExt == "");
                assert (fp.isChild);

                fp = new FilePath(fp.asParent);
                assert (fp.isAbsolute);
                assert (fp.getName == "bar");
                assert (fp.getPath == r"/home/foo/");
                assert (fp.toUtf8 == r"C:/home/foo/bar");
                assert (fp.getFullPath == r"C:/home/foo/");
                assert (fp.getFullName == r"bar");
                assert (fp.getSuffix == r"");
                assert (fp.getExt == "");
                assert (fp.isChild);

                fp = new FilePath(fp.asParent);
                assert (fp.isAbsolute);
                assert (fp.getName == "foo");
                assert (fp.getPath == r"/home/");
                assert (fp.toUtf8 == r"C:/home/foo");
                assert (fp.getFullPath == r"C:/home/");
                assert (fp.getFullName == r"foo");
                assert (fp.getSuffix == r"");
                assert (fp.getExt == "");
                assert (fp.isChild);

                fp = new FilePath(fp.asParent);
                assert (fp.isAbsolute);
                assert (fp.getName == "home");
                assert (fp.getPath == r"/");
                assert (fp.toUtf8 == r"C:/home");
                assert (fp.getFullPath == r"C:/");
                assert (fp.getFullName == r"home");
                assert (fp.getSuffix == r"");
                assert (fp.getExt == "");
                assert (!fp.isChild);

                fp = new FilePath(r"foo/bar/john.doe");
                assert (!fp.isAbsolute);
                assert (fp.getName == "john");
                assert (fp.getPath == r"foo/bar/");
                assert (fp.getSuffix == r".doe");
                assert (fp.getFullName == r"john.doe");
                assert (fp.toUtf8 == r"foo/bar/john.doe");
                assert (fp.getExt == "doe");
                assert (fp.isChild);

                fp = new FilePath(r"c:doe");
                assert (fp.isAbsolute);
                assert (fp.getSuffix == r"");
                assert (fp.toUtf8 == r"c:doe");
                assert (fp.getPath == r"");
                assert (fp.getName == "doe");
                assert (fp.getFullName == r"doe");
                assert (fp.getExt == "");
                assert (!fp.isChild);

                fp = new FilePath(r"/doe");
                assert (fp.isAbsolute);
                assert (fp.getSuffix == r"");
                assert (fp.toUtf8 == r"/doe");
                assert (fp.getName == "doe");
                assert (fp.getPath == r"/");
                assert (fp.getFullName == r"doe");
                assert (fp.getExt == "");
                assert (!fp.isChild);

                fp = new FilePath(r"john.doe.foo");
                assert (!fp.isAbsolute);
                assert (fp.getName == "john");
                assert (fp.getPath == r"");
                assert (fp.getSuffix == r".doe.foo");
                assert (fp.toUtf8 == r"john.doe.foo");
                assert (fp.getFullName == r"john.doe.foo");
                assert (fp.getExt == "foo");
                assert (!fp.isChild);

                fp = new FilePath(r".doe");
                assert (!fp.isAbsolute);
                assert (fp.getSuffix == r".doe");
                assert (fp.toUtf8 == r".doe");
                assert (fp.getName == "");
                assert (fp.getPath == r"");
                assert (fp.getFullName == r".doe");
                assert (fp.getExt == "doe");
                assert (!fp.isChild);

                fp = new FilePath(r"doe");
                assert (!fp.isAbsolute);
                assert (fp.getSuffix == r"");
                assert (fp.toUtf8 == r"doe");
                assert (fp.getName == "doe");
                assert (fp.getPath == r"");
                assert (fp.getFullName == r"doe");
                assert (fp.getExt == "");
                assert (!fp.isChild);

                fp = new FilePath(r".");
                assert (!fp.isAbsolute);
                assert (fp.getSuffix == r".");
                assert (fp.toUtf8 == r".");
                assert (fp.getName == "");
                assert (fp.getPath == r"");
                assert (fp.getFullName == r".");
                assert (fp.getExt == "");
                assert (!fp.isChild);

                fp = new FilePath(r"..");
                assert (!fp.isAbsolute);
                assert (fp.getSuffix == r"..");
                assert (fp.toUtf8 == r"..");
                assert (fp.getName == "");
                assert (fp.getPath == r"");
                assert (fp.getFullName == r"..");
                assert (fp.getExt == "");
                assert (!fp.isChild);

                fp = new FilePath(r"C:/foo/bar/test.bar");
                fp = new FilePath(fp.asPath ("foo"));
                assert (fp.getName == r"test");
                assert (fp.getPath == r"foo/");
                assert (fp.getFullPath == r"C:foo/");
                assert (fp.getExt == "bar");

                fp = new FilePath(fp.asPath (""));
                assert (fp.getName == r"test");
                assert (fp.getPath == r"");
                assert (fp.getFullPath == r"C:");
                assert (fp.getExt == "bar");

                fp = new FilePath("");
                assert (fp.isEmpty);
                assert (!fp.isChild);
                assert (!fp.isAbsolute);
                assert (fp.getSuffix == r"");
                assert (fp.toUtf8 == r"");
                assert (fp.getName == "");
                assert (fp.getPath == r"");
                assert (fp.getFullName == r"");
                assert (fp.getExt == "");

                fp = new FilePath(r"foo/bar/");
                assert(fp.join(r"c:/joe/bar").toUtf8 == r"c:/joe/bar/foo/bar/");
                assert(fp.join(new FilePath(r"c:/joe/bar")).toUtf8 == r"c:/joe/bar/foo/bar/");
                fp = new FilePath(r"c:/joe/bar");
                assert(fp.join(r"foo/bar/").toUtf8 == r"c:/joe/bar");
                assert(fp.join(new FilePath(r"foo/bar")).toUtf8 == r"c:/joe/bar");

                fp = new FilePath(r"c:/bar");
                assert(fp.join(r"foo").toUtf8 == r"c:/bar");
                assert(fp.join(new FilePath(r"foo")).toUtf8 == r"c:/bar");

                fp = new FilePath(r"bar/");
                assert(fp.join(r"c:/foo").toUtf8 == r"c:/foo/bar/");
                assert(fp.join(new FilePath(r"c:/foo")).toUtf8 == r"c:/foo/bar/");

                fp = new FilePath(r"C:/foo/bar/test.bar");
                assert (fp.asExt(null) == r"C:/foo/bar/test");
                assert (fp.asExt("foo") == r"C:/foo/bar/test.foo");
                 +/
                }
        }
}
