/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Oct 2004: Initial version
        version:        Nov 2006: Australian version
        version:        Feb 2007: Mutating version
        version:        Mar 2007: Folded FileProxy in
        version:        Nov 2007: VFS dictates '/' always be used
        version:        Feb 2008: Split file-system calls into a struct

        author:         Kris

        FilePath provides a means to efficiently edit path components and 
        of accessing the underlying file system.

        Use module Path.d instead when you need pedestrian access to the
        file-system, and are not mutating the path components themselves

*******************************************************************************/

module tango.io.FilePath;

private import  tango.io.Path;

private import  tango.io.model.IFile : FileConst, FileInfo;

/*******************************************************************************

*******************************************************************************/

private extern (C) void memmove (void* dst, void* src, uint bytes);

/*******************************************************************************

        Models a file path. These are expected to be used as the constructor
        argument to various file classes. The intention is that they easily
        convert to other representations such as absolute, canonical, or Url.

        File paths containing non-ansi characters should be UTF-8 encoded.
        Supporting Unicode in this manner was deemed to be more suitable
        than providing a wchar version of FilePath, and is both consistent
        & compatible with the approach taken with the Uri class.

        FilePath is designed to be transformed, thus each mutating method
        modifies the internal content. See module Path.d for a lightweight
        immutable variation.

        Note that patterns of adjacent '.' separators are treated specially
        in that they will be assigned to the name where there is no distinct
        suffix. In addition, a '.' at the start of a name signifies it does 
        not belong to the suffix i.e. ".file" is a name rather than a suffix.
        Patterns of intermediate '.' characters will otherwise be assigned
        to the suffix, such that "file....suffix" includes the dots within
        the suffix itself. See method ext() for a suffix without dots.

        Note that Win32 '\' characters are converted to '/' by default via
        the FilePath constructor.

*******************************************************************************/

class FilePath : PathView
{
        private PathParser      p;              // the parsed path
        private bool            dir_;           // this represents a dir?

        public alias    append  opCatAssign;    // path ~= x;


        /***********************************************************************

                Filter used for screening paths via toList()

        ***********************************************************************/

        public alias bool delegate (FilePath, bool) Filter;

        /***********************************************************************

                Call-site shortcut to create a FilePath instance. This
                enables the same syntax as struct usage, so may expose
                a migration path

        ***********************************************************************/

        static FilePath opCall (char[] filepath = null)
        {
                return new FilePath (filepath);
        }

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

                With regard to the filepath copy, we found the common case to
                be an explicit .dup, whereas aliasing appeared to be rare by
                comparison. We also noted a large proportion interacting with
                C-oriented OS calls, implying the postfix of a null terminator.
                Thus, FilePath combines both as a single operation.

                Note that Win32 '\' characters are normalized to '/' instead.

        ***********************************************************************/

        this (char[] filepath = null)
        {
                set (filepath, true);
        }
        
        /***********************************************************************

                Return the complete text of this filepath

        ***********************************************************************/

        final char[] toString ()
        {
                return  p.toString;
        }

        /***********************************************************************

                Duplicate this path

        ***********************************************************************/

        final FilePath dup ()
        {
                return FilePath (toString);
        }

        /***********************************************************************

                Return the complete text of this filepath as a null
                terminated string for use with a C api. Use toString
                instead for any D api.

                Note that the nul is always embedded within the string
                maintained by FilePath, so there's no heap overhead when
                making a C call

        ***********************************************************************/

        final char[] cString ()
        {
                return p.fp [0 .. p.end_+1];
        }

        /***********************************************************************

                Return the root of this path. Roots are constructs such as
                "c:"

        ***********************************************************************/

        final char[] root ()
        {
                return p.root;
        }

        /***********************************************************************

                Return the file path. Paths may start and end with a "/".
                The root path is "/" and an unspecified path is returned as
                an empty string. Directory paths may be split such that the
                directory name is placed into the 'name' member; directory
                paths are treated no differently than file paths

        ***********************************************************************/

        final char[] folder ()
        {
                return p.folder;
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

                See pop() also, which is generally more useful when working
                with FilePath instances

        ***********************************************************************/

        final char[] parent ()
        {
                return p.parent;
        }

        /***********************************************************************

                Return the name of this file, or directory.

        ***********************************************************************/

        final char[] name ()
        {
                return p.name;
        }

        /***********************************************************************

                Ext is the tail of the filename, rightward of the rightmost
                '.' separator e.g. path "foo.bar" has ext "bar". Note that
                patterns of adjacent separators are treated specially; for
                example, ".." will wind up with no ext at all

        ***********************************************************************/

        final char[] ext ()
        {
                return p.ext;
        }

        /***********************************************************************

                Suffix is like ext, but includes the separator e.g. path
                "foo.bar" has suffix ".bar"

        ***********************************************************************/

        final char[] suffix ()
        {
                return p.suffix;
        }

        /***********************************************************************

                return the root + folder combination

        ***********************************************************************/

        final char[] path ()
        {
                return p.path;
        }

        /***********************************************************************

                return the name + suffix combination

        ***********************************************************************/

        final char[] file ()
        {
                return p.file;
        }

        /***********************************************************************

                Returns true if all fields are equal.

        ***********************************************************************/

        final override int opEquals (Object o)
        {
                return (this is o) || (o && toString == o.toString);
        }

        /***********************************************************************

                Does this FilePath equate to the given text?

        ***********************************************************************/

        final int opEquals (char[] s)
        {
                return p.opEquals(s);
        }

        /***********************************************************************

                Returns true if this FilePath is *not* relative to the
                current working directory

        ***********************************************************************/

        final bool isAbsolute ()
        {
                return p.isAbsolute;
        }

        /***********************************************************************

                Returns true if this FilePath is empty

        ***********************************************************************/

        final bool isEmpty ()
        {
                return p.isEmpty;
        }

        /***********************************************************************

                Returns true if this FilePath has a parent. Note that a
                parent is defined by the presence of a path-separator in
                the path. This means 'foo' within "\foo" is considered a
                child of the root

        ***********************************************************************/

        final bool isChild ()
        {
                return p.isChild;
        }

        /***********************************************************************

                Replace all 'from' instances with 'to'

        ***********************************************************************/

        final FilePath replace (char from, char to)
        {
                .replace (path, from, to);
                return this;
        }

        /***********************************************************************

                Convert path separators to a standard format, using '/' as
                the path separator. This is compatible with URI and all of 
                the contemporary O/S which Tango supports. Known exceptions
                include the Windows command-line processor, which considers
                '/' characters to be switches instead. Use the native()
                method to support that.

                Note: mutates the current path.

        ***********************************************************************/

        final FilePath standard ()
        {
                .standard (path);
                return this;
        }

        /***********************************************************************

                Convert to native O/S path separators where that is required,
                such as when dealing with the Windows command-line. 
                
                Note: mutates the current path. Use this pattern to obtain a 
                copy instead: path.dup.native

        ***********************************************************************/

        final FilePath native ()
        {
                .native (path);
                return this;
        }

        /***********************************************************************

                Concatenate text to this path; no separators are added.
                See join() also

        ***********************************************************************/

        final FilePath cat (char[][] others...)
        {
                foreach (other; others)
                        {
                        auto len = p.end_ + other.length;
                        expand (len);
                        p.fp [p.end_ .. len] = other;
                        p.fp [len] = 0;
                        p.end_ = len;
                        }
                return parse;
        }

        /***********************************************************************

                Append a folder to this path. A leading separator is added
                as required

        ***********************************************************************/

        final FilePath append (char[] path)
        {
                if (file.length)
                    path = prefixed (path);
                return cat (path);
        }

        /***********************************************************************

                Prepend a folder to this path. A trailing separator is added
                if needed

        ***********************************************************************/

        final FilePath prepend (char[] path)
        {
                adjust (0, p.folder_, p.folder_, padded (path));
                return parse;
        }

        /***********************************************************************

                Reset the content of this path to that of another and
                reparse

        ***********************************************************************/

        FilePath set (FilePath path)
        {
                return set (path.toString, false);
        }

        /***********************************************************************

                Reset the content of this path, and reparse. There's an
                optional boolean flag to convert the path into standard
                form, before parsing (converting '\' into '/')

        ***********************************************************************/

        final FilePath set (char[] path, bool convert = false)
        {
                p.end_ = path.length;

                expand (p.end_);
                if (p.end_)
                   {
                   p.fp[0 .. p.end_] = path;
                   if (convert)
                       .standard (p.fp [0 .. p.end_]);
                   }

                p.fp[p.end_] = '\0';
                return parse;
        }

        /***********************************************************************

                Sidestep the normal lookup for paths that are known to
                be folders. Where folder is true, file-system lookups
                will be skipped.

        ***********************************************************************/

        final FilePath isFolder (bool folder)
        {
                dir_ = folder;
                return this;
        }

        /***********************************************************************

                Replace the root portion of this path

        ***********************************************************************/

        final FilePath root (char[] other)
        {
                auto x = adjust (0, p.folder_, p.folder_, padded (other, ':'));
                p.folder_ += x;
                p.suffix_ += x;
                p.name_ += x;
                return this;
        }

        /***********************************************************************

                Replace the folder portion of this path. The folder will be
                padded with a path-separator as required

        ***********************************************************************/

        final FilePath folder (char[] other)
        {
                auto x = adjust (p.folder_, p.name_, p.name_ - p.folder_, padded (other));
                p.suffix_ += x;
                p.name_ += x;
                return this;
        }

        /***********************************************************************

                Replace the name portion of this path

        ***********************************************************************/

        final FilePath name (char[] other)
        {
                auto x = adjust (p.name_, p.suffix_, p.suffix_ - p.name_, other);
                p.suffix_ += x;
                return this;
        }

        /***********************************************************************

                Replace the suffix portion of this path. The suffix will be
                prefixed with a file-separator as required

        ***********************************************************************/

        final FilePath suffix (char[] other)
        {
                adjust (p.suffix_, p.end_, p.end_ - p.suffix_, prefixed (other, '.'));
                return this;
        }

        /***********************************************************************

                Replace the root and folder portions of this path and
                reparse. The replacement will be padded with a path
                separator as required

        ***********************************************************************/

        final FilePath path (char[] other)
        {
                adjust (0, p.name_, p.name_, padded (other));
                return parse;
        }

        /***********************************************************************

                Replace the file and suffix portions of this path and
                reparse. The replacement will be prefixed with a suffix
                separator as required

        ***********************************************************************/

        final FilePath file (char[] other)
        {
                adjust (p.name_, p.end_, p.end_ - p.name_, other);
                return parse;
        }

        /***********************************************************************

                Pop to the parent of the current filepath (in situ - mutates
                this FilePath)

        ***********************************************************************/

        final FilePath pop ()
        {
                auto path = parent();
                p.end_ = path.length;
                p.fp[p.end_] = '\0';
                return parse;
        }

        /***********************************************************************

                Join a set of path specs together. A path separator is
                potentially inserted between each of the segments.

        ***********************************************************************/

        static char[] join (char[][] paths...)
        {
                return FS.join (paths);
        }

        /***********************************************************************

                Convert this FilePath to absolute format, using the given
                prefix as necessary. If this FilePath is already absolute,
                return it intact.

                Returns this FilePath, adjusted as necessary

        ***********************************************************************/

        final FilePath absolute (char[] prefix)
        {
                if (! isAbsolute)
                      prepend (padded(prefix));
                return this;
        }

        /***********************************************************************

                Return an adjusted path such that non-empty instances do not
                have a trailing separator

        ***********************************************************************/

        static char[] stripped (char[] path, char c = FileConst.PathSeparatorChar)
        {
                return FS.stripped (path, c);
        }

        /***********************************************************************

                Return an adjusted path such that non-empty instances always
                have a trailing separator

        ***********************************************************************/

        static char[] padded (char[] path, char c = FileConst.PathSeparatorChar)
        {
                return FS.padded (path, c);
        }

        /***********************************************************************

                Return an adjusted path such that non-empty instances always
                have a prefixed separator

        ***********************************************************************/

        static char[] prefixed (char[] s, char c = FileConst.PathSeparatorChar)
        {
                if (s.length && s[0] != c)
                    s = c ~ s;
                return s;
        }

        /***********************************************************************

                Parse the path spec, and mutate '\' into '/' as necessary

        ***********************************************************************/

        private final FilePath parse ()
        {
                p.parse (p.fp, p.end_);
                return this;
        }

        /***********************************************************************

                Potentially make room for more content

        ***********************************************************************/

        private final void expand (uint size)
        {
                ++size;
                if (p.fp.length < size)
                    p.fp.length = (size + 127) & ~127;
        }

        /***********************************************************************

                Insert/delete internal content

        ***********************************************************************/

        private final int adjust (int head, int tail, int len, char[] sub)
        {
                len = sub.length - len;

                // don't destroy self-references!
                if (len && sub.ptr >= p.fp.ptr+head+len && sub.ptr < p.fp.ptr+p.fp.length)
                   {
                   char[512] tmp = void;
                   assert (sub.length < tmp.length);
                   sub = tmp[0..sub.length] = sub;
                   }

                // make some room if necessary
                expand (len + p.end_);

                // slide tail around to insert or remove space
                memmove (p.fp.ptr+tail+len, p.fp.ptr+tail, p.end_ +1 - tail);

                // copy replacement
                memmove (p.fp.ptr + head, sub.ptr, sub.length);

                // adjust length
                p.end_ += len;
                return len;
        }


        /**********************************************************************/
        /********************** file-system methods ***************************/
        /**********************************************************************/


        /***********************************************************************

                Create an entire path consisting of this folder along with
                all parent folders. The path must not contain '.' or '..'
                segments. Related methods include PathUtil.normalize() and
                absolute()

                Note that each segment is created as a folder, including the
                trailing segment.

                Returns: a chaining reference (this)

                Throws: IOException upon systen errors

                Throws: IllegalArgumentException if a segment exists but as 
                a file instead of a folder

        ***********************************************************************/

        final FilePath create ()
        {       
                createPath (this.toString);
                return this;
        }

        /***********************************************************************

                List the set of filenames within this folder, using
                the provided filter to control the list:
                ---
                bool delegate (FilePath path, bool isFolder) Filter
                ---

                Returning true from the filter includes the given path,
                whilst returning false excludes it. Parameter 'isFolder'
                indicates whether the path is a file or folder.

                Note that paths composed of '.' characters are ignored.

        ***********************************************************************/

        final FilePath[] toList (Filter filter = null)
        {
                FilePath[] paths;

                foreach (info; this)
                        {
                        auto p = from (info);

                        // test this entry for inclusion
                        if (filter is null || filter (p, info.folder))
                            paths ~= p;
                        else
                           delete p;
                        }

                return paths;
        }

        /***********************************************************************

                Construct a FilePath from the given FileInfo

        ***********************************************************************/

        static FilePath from (ref FileInfo info)
        {
                char[512] tmp = void;

                auto len = info.path.length + info.name.length;
                assert (len < tmp.length);

                // construct full pathname
                tmp [0 .. info.path.length] = info.path;
                tmp [info.path.length .. len] = info.name;

                return FilePath(tmp[0 .. len]).isFolder(info.folder);
        }

        /***********************************************************************

                Does this path currently exist?

        ***********************************************************************/

        final bool exists ()
        {
                return FS.exists (cString);
        }

        /***********************************************************************

                Returns the time of the last modification. Accurate
                to whatever the OS supports, and in a format dictated
                by the file-system. For example NTFS keeps UTC time, 
                while FAT timestamps are based on the local time. 

        ***********************************************************************/

        final Time modified ()
        {
                return timeStamps.modified;
        }

        /***********************************************************************

                Returns the time of the last access. Accurate to
                whatever the OS supports, and in a format dictated
                by the file-system. For example NTFS keeps UTC time, 
                while FAT timestamps are based on the local time.

        ***********************************************************************/

        final Time accessed ()
        {
                return timeStamps.accessed;
        }

        /***********************************************************************

                Returns the time of file creation. Accurate to
                whatever the OS supports, and in a format dictated
                by the file-system. For example NTFS keeps UTC time,  
                while FAT timestamps are based on the local time.

        ***********************************************************************/

        final Time created ()
        {
                return timeStamps.created;
        }

        /***********************************************************************

                change the name or location of a file/directory, and
                adopt the provided Path

        ***********************************************************************/

        final FilePath rename (FilePath dst)
        {
                FS.rename (cString, dst.cString);
                return this.set (dst);
        }

        /***********************************************************************

                Transfer the content of another file to this one. Returns a
                reference to this class on success, or throws an IOException
                upon failure.

        ***********************************************************************/

        final FilePath copy (char[] source)
        {
                FS.copy (source~'\0', cString);
                return this;
        }

        /***********************************************************************

                Return the file length (in bytes)

        ***********************************************************************/

        final ulong fileSize ()
        {
                return FS.fileSize (cString);
        }

        /***********************************************************************

                Is this file writable?

        ***********************************************************************/

        final bool isWritable ()
        {
                return FS.isWritable (cString);
        }

        /***********************************************************************

                Is this file actually a folder/directory?

        ***********************************************************************/

        final bool isFolder ()
        {
                if (dir_)
                    return true;

                return FS.isFolder (cString);
        }

        /***********************************************************************

                Is this a regular file?

        ***********************************************************************/

        final bool isFile ()
        {
                if (dir_)
                    return false;

                return FS.isFile (cString);
        }

        /***********************************************************************

                Return timestamp information

                Timstamps are returns in a format dictated by the 
                file-system. For example NTFS keeps UTC time, 
                while FAT timestamps are based on the local time

        ***********************************************************************/

        final Stamps timeStamps ()
        {
                return FS.timeStamps (cString);
        }

        /***********************************************************************

                Transfer the content of another file to this one. Returns a
                reference to this class on success, or throws an IOException
                upon failure.

        ***********************************************************************/

        final FilePath copy (FilePath src)
        {
                FS.copy (src.cString, cString);
                return this;
        }

        /***********************************************************************

                Remove the file/directory from the file-system

        ***********************************************************************/

        final FilePath remove ()
        {      
                FS.remove (cString);
                return this;
        }

        /***********************************************************************

               change the name or location of a file/directory, and
               adopt the provided Path

        ***********************************************************************/

        final FilePath rename (char[] dst)
        {
                FS.rename (cString, dst~'\0');
                return this.set (dst, true);
        }

        /***********************************************************************

                Create a new file

        ***********************************************************************/

        final FilePath createFile ()
        {
                FS.createFile (cString);
                return this;
        }

        /***********************************************************************

                Create a new directory

        ***********************************************************************/

        final FilePath createFolder ()
        {
                FS.createFolder (cString);
                return this;
        }

        /***********************************************************************

                List the set of filenames within this folder.

                Each path and filename is passed to the provided
                delegate, along with the path prefix and whether
                the entry is a folder or not.

                Returns the number of files scanned.

        ***********************************************************************/

        final int opApply (int delegate(ref FileInfo) dg)
        {
                return FS.list (cString, dg);
        }
}



/*******************************************************************************

*******************************************************************************/

interface PathView
{
        alias FS.Stamps         Stamps;
        //alias FS.FileInfo       FileInfo;

        /***********************************************************************

                Return the complete text of this filepath

        ***********************************************************************/

        abstract char[] toString ();

        /***********************************************************************

                Return the complete text of this filepath

        ***********************************************************************/

        abstract char[] cString ();

        /***********************************************************************

                Return the root of this path. Roots are constructs such as
                "c:"

        ***********************************************************************/

        abstract char[] root ();

        /***********************************************************************

                Return the file path. Paths may start and end with a "/".
                The root path is "/" and an unspecified path is returned as
                an empty string. Directory paths may be split such that the
                directory name is placed into the 'name' member; directory
                paths are treated no differently than file paths

        ***********************************************************************/

        abstract char[] folder ();

        /***********************************************************************

                Return the name of this file, or directory, excluding a
                suffix.

        ***********************************************************************/

        abstract char[] name ();

        /***********************************************************************

                Ext is the tail of the filename, rightward of the rightmost
                '.' separator e.g. path "foo.bar" has ext "bar". Note that
                patterns of adjacent separators are treated specially; for
                example, ".." will wind up with no ext at all

        ***********************************************************************/

        abstract char[] ext ();

        /***********************************************************************

                Suffix is like ext, but includes the separator e.g. path
                "foo.bar" has suffix ".bar"

        ***********************************************************************/

        abstract char[] suffix ();

        /***********************************************************************

                return the root + folder combination

        ***********************************************************************/

        abstract char[] path ();

        /***********************************************************************

                return the name + suffix combination

        ***********************************************************************/

        abstract char[] file ();

        /***********************************************************************

                Returns true if this FilePath is *not* relative to the
                current working directory.

        ***********************************************************************/

        abstract bool isAbsolute ();

        /***********************************************************************

                Returns true if this FilePath is empty

        ***********************************************************************/

        abstract bool isEmpty ();

        /***********************************************************************

                Returns true if this FilePath has a parent

        ***********************************************************************/

        abstract bool isChild ();

        /***********************************************************************

                Does this path currently exist?

        ***********************************************************************/

        abstract bool exists ();

        /***********************************************************************

                Returns the time of the last modification. Accurate
                to whatever the OS supports

        ***********************************************************************/

        abstract Time modified ();

        /***********************************************************************

                Returns the time of the last access. Accurate to
                whatever the OS supports

        ***********************************************************************/

        abstract Time accessed ();

        /***********************************************************************

                Returns the time of file creation. Accurate to
                whatever the OS supports

        ***********************************************************************/

        abstract Time created ();

        /***********************************************************************

                Return the file length (in bytes)

        ***********************************************************************/

        abstract ulong fileSize ();

        /***********************************************************************

                Is this file writable?

        ***********************************************************************/

        abstract bool isWritable ();

        /***********************************************************************

                Is this file actually a folder/directory?

        ***********************************************************************/

        abstract bool isFolder ();

        /***********************************************************************

                Return timestamp information

        ***********************************************************************/

        abstract Stamps timeStamps ();
}





/*******************************************************************************

*******************************************************************************/

debug (UnitTest)
{
        unittest
        {
                version(Win32)
                {
                auto fp = new FilePath(r"C:/home/foo/bar");
                fp ~= "john";
                assert (fp == r"C:/home/foo/bar/john");
                fp.set (r"C:/");
                fp ~= "john";
                assert (fp == r"C:/john");
                fp.set("foo.bar");
                fp ~= "john";
                assert (fp == r"foo.bar/john");
                fp.set("");
                fp ~= "john";
                assert (fp == r"john");

                fp.set(r"C:/home/foo/bar/john/foo.d");
                assert (fp.pop == r"C:/home/foo/bar/john");
                assert (fp.pop == r"C:/home/foo/bar");
                assert (fp.pop == r"C:/home/foo");
                assert (fp.pop == r"C:/home");
                assert (fp.pop == r"C:");
                assert (fp.pop == r"C:");
        
                // special case for popping empty names
                fp.set (r"C:/home/foo/bar/john/");
                assert (fp.pop == r"C:/home/foo/bar");

                fp = new FilePath;
                fp.set (r"C:/home/foo/bar/john/");
                assert (fp.isAbsolute);
                assert (fp.name == "");
                assert (fp.folder == r"/home/foo/bar/john/");
                assert (fp == r"C:/home/foo/bar/john/");
                assert (fp.path == r"C:/home/foo/bar/john/");
                assert (fp.file == r"");
                assert (fp.suffix == r"");
                assert (fp.root == r"C:");
                assert (fp.ext == "");
                assert (fp.isChild);

                fp = new FilePath(r"C:/home/foo/bar/john");
                assert (fp.isAbsolute);
                assert (fp.name == "john");
                assert (fp.folder == r"/home/foo/bar/");
                assert (fp == r"C:/home/foo/bar/john");
                assert (fp.path == r"C:/home/foo/bar/");
                assert (fp.file == r"john");
                assert (fp.suffix == r"");
                assert (fp.ext == "");
                assert (fp.isChild);

                fp.pop;
                assert (fp.isAbsolute);
                assert (fp.name == "bar");
                assert (fp.folder == r"/home/foo/");
                assert (fp == r"C:/home/foo/bar");
                assert (fp.path == r"C:/home/foo/");
                assert (fp.file == r"bar");
                assert (fp.suffix == r"");
                assert (fp.ext == "");
                assert (fp.isChild);

                fp.pop;
                assert (fp.isAbsolute);
                assert (fp.name == "foo");
                assert (fp.folder == r"/home/");
                assert (fp == r"C:/home/foo");
                assert (fp.path == r"C:/home/");
                assert (fp.file == r"foo");
                assert (fp.suffix == r"");
                assert (fp.ext == "");
                assert (fp.isChild);

                fp.pop;
                assert (fp.isAbsolute);
                assert (fp.name == "home");
                assert (fp.folder == r"/");
                assert (fp == r"C:/home");
                assert (fp.path == r"C:/");
                assert (fp.file == r"home");
                assert (fp.suffix == r"");
                assert (fp.ext == "");
                assert (fp.isChild);

                fp = new FilePath(r"foo/bar/john.doe");
                assert (!fp.isAbsolute);
                assert (fp.name == "john");
                assert (fp.folder == r"foo/bar/");
                assert (fp.suffix == r".doe");
                assert (fp.file == r"john.doe");
                assert (fp == r"foo/bar/john.doe");
                assert (fp.ext == "doe");
                assert (fp.isChild);

                fp = new FilePath(r"c:doe");
                assert (fp.isAbsolute);
                assert (fp.suffix == r"");
                assert (fp == r"c:doe");
                assert (fp.folder == r"");
                assert (fp.name == "doe");
                assert (fp.file == r"doe");
                assert (fp.ext == "");
                assert (!fp.isChild);

                fp = new FilePath(r"/doe");
                assert (fp.isAbsolute);
                assert (fp.suffix == r"");
                assert (fp == r"/doe");
                assert (fp.name == "doe");
                assert (fp.folder == r"/");
                assert (fp.file == r"doe");
                assert (fp.ext == "");
                assert (fp.isChild);

                fp = new FilePath(r"john.doe.foo");
                assert (!fp.isAbsolute);
                assert (fp.name == "john.doe");
                assert (fp.folder == r"");
                assert (fp.suffix == r".foo");
                assert (fp == r"john.doe.foo");
                assert (fp.file == r"john.doe.foo");
                assert (fp.ext == "foo");
                assert (!fp.isChild);

                fp = new FilePath(r".doe");
                assert (!fp.isAbsolute);
                assert (fp.suffix == r"");
                assert (fp == r".doe");
                assert (fp.name == ".doe");
                assert (fp.folder == r"");
                assert (fp.file == r".doe");
                assert (fp.ext == "");
                assert (!fp.isChild);

                fp = new FilePath(r"doe");
                assert (!fp.isAbsolute);
                assert (fp.suffix == r"");
                assert (fp == r"doe");
                assert (fp.name == "doe");
                assert (fp.folder == r"");
                assert (fp.file == r"doe");
                assert (fp.ext == "");
                assert (!fp.isChild);

                fp = new FilePath(r".");
                assert (!fp.isAbsolute);
                assert (fp.suffix == r"");
                assert (fp == r".");
                assert (fp.name == ".");
                assert (fp.folder == r"");
                assert (fp.file == r".");
                assert (fp.ext == "");
                assert (!fp.isChild);

                fp = new FilePath(r"..");
                assert (!fp.isAbsolute);
                assert (fp.suffix == r"");
                assert (fp == r"..");
                assert (fp.name == "..");
                assert (fp.folder == r"");
                assert (fp.file == r"..");
                assert (fp.ext == "");
                assert (!fp.isChild);

                fp = new FilePath(r"c:/a/b/c/d/e/foo.bar");
                assert (fp.isAbsolute);
                fp.folder (r"/a/b/c/");
                assert (fp.suffix == r".bar");
                assert (fp == r"c:/a/b/c/foo.bar");
                assert (fp.name == "foo");
                assert (fp.folder == r"/a/b/c/");
                assert (fp.file == r"foo.bar");
                assert (fp.ext == "bar");
                assert (fp.isChild);

                fp = new FilePath(r"c:/a/b/c/d/e/foo.bar");
                assert (fp.isAbsolute);
                fp.folder (r"/a/b/c/d/e/f/g/");
                assert (fp.suffix == r".bar");
                assert (fp == r"c:/a/b/c/d/e/f/g/foo.bar");
                assert (fp.name == "foo");
                assert (fp.folder == r"/a/b/c/d/e/f/g/");
                assert (fp.file == r"foo.bar");
                assert (fp.ext == "bar");
                assert (fp.isChild);

                fp = new FilePath(r"C:/foo/bar/test.bar");
                assert (fp.path == "C:/foo/bar/");
                fp = new FilePath(r"C:\foo\bar\test.bar");
                assert (fp.path == r"C:/foo/bar/");

                fp = new FilePath("");
                assert (fp.isEmpty);
                assert (!fp.isChild);
                assert (!fp.isAbsolute);
                assert (fp.suffix == r"");
                assert (fp == r"");
                assert (fp.name == "");
                assert (fp.folder == r"");
                assert (fp.file == r"");
                assert (fp.ext == "");
/+
                fp = new FilePath(r"C:/foo/bar/test.bar");
                fp = new FilePath(fp.asPath ("foo"));
                assert (fp.name == r"test");
                assert (fp.folder == r"foo/");
                assert (fp.path == r"C:foo/");
                assert (fp.ext == ".bar");

                fp = new FilePath(fp.asPath (""));
                assert (fp.name == r"test");
                assert (fp.folder == r"");
                assert (fp.path == r"C:");
                assert (fp.ext == ".bar");

                fp = new FilePath(r"c:/joe/bar");
                assert(fp.cat(r"foo/bar/") == r"c:/joe/bar/foo/bar/");
                assert(fp.cat(new FilePath(r"foo/bar")).toString == r"c:/joe/bar/foo/bar");

                assert (FilePath.join (r"a/b/c/d", r"e/f/" r"g") == r"a/b/c/d/e/f/g");

                fp = new FilePath(r"C:/foo/bar/test.bar");
                assert (fp.asExt(null) == r"C:/foo/bar/test");
                assert (fp.asExt("foo") == r"C:/foo/bar/test.foo");
+/      
                }
        }
}


debug (FilePath)
{       
        import tango.io.Console;

        void main() 
        {
                assert (FilePath("/foo/").create.exists);
                Cout (FilePath("c:/temp/").file("foo.bar")).newline;
        }

}
