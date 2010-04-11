/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Jun 2004: Initial release
        version:        Dec 2006: Pacific release

        author:         Kris

*******************************************************************************/

module tango.io.FileScan;

public import   tango.io.File,
                tango.io.FilePath,
                tango.io.FileProxy;

/*******************************************************************************

        Recursively scan files and directories, adding filtered files to
        an output structure as we go. This can be used to produce a list
        of subdirectories and the files contained therein. The following
        example lists all files with suffix ".d" located via the current
        directory, along with the folders containing them:

        ---
        auto scan = new FileScan;

        scan (new FilePath ("."), ".d");

        Stdout.formatln ("{0} Folders", scan.folders.length);
        foreach (file; scan.folders)
                 Stdout.formatln ("{0}", file);

        Stdout.formatln ("\n{0} Files", scan.files.length);
        foreach (file; scan.files)
                 Stdout.formatln ("{0}", file);
        ---

        This is unlikely the most efficient method to scan a vast number of
        files, but operates in a convenient manner
        
*******************************************************************************/

class FileScan
{       
        alias sweep opCall;

        uint            total_;
        File[]          files_,
                        folders_;
        
        /***********************************************************************

            alias for Filter delegate. Takes a FileProxy and a bool as arguments
            , and returns a bool.

            The FileProxy argument represents a file found by the scan, and the
            bool whether the FileProxy represents a diretory.

            The filter should return true, if matched by the filter. Note that
            returning false if the FileProxy is a directory, means that the
            directory isn't matched, and thus not recursed into. To always
            recurse, do something in the vein of

            ---
            return (isDir || match(fp.getPath));
            ---

        ***********************************************************************/

        alias bool delegate (FileProxy, bool) Filter;

        /***********************************************************************

                Return the number of files found in the last scan

        ***********************************************************************/

        public uint inspected ()
        {
                return total_;
        }

        /***********************************************************************

                Return all the files found in the last scan

        ***********************************************************************/

        public File[] files ()
        {
                return files_;
        }

        /***********************************************************************
        
                Return all directories found in the last scan

        ***********************************************************************/

        public FileProxy[] folders ()
        {
                return folders_;
        }

        /***********************************************************************

                Sweep a set of files and directories from the given parent
                path, where the files are filtered by the provided delegate

        ***********************************************************************/
        
        FileScan sweep (FilePath path, Filter filter)
        {
                total_ = 0;
                files_ = folders_ = null;
                scan (new File(path), filter);
                return this;
        }

        /***********************************************************************

                Sweep a set of files and directories from the given parent
                path, where the files are filtered by the given suffix
        
        ***********************************************************************/
        
        FileScan sweep (FilePath path, char[] match)
        {
                return sweep (path, (FileProxy fp, bool isDir)
                             {return isDir || fp.getPath.getSuffix == match;});
        }

        /***********************************************************************

                Internal routine to locate files and sub-directories. We
                skip folders composed only of '.' chars. 

                Heap activity is avoided for everything the filter discards.
                        
        ***********************************************************************/

        private void scan (File folder, Filter filter) 
        {
                File[] files;

                void add (char[] prefix, char[] name, bool isDir)
                { 
                        ++total_;

                        char[512] tmp;
                        
                        int len = prefix.length + name.length;
                        assert (len < tmp.length);

                        // construct full pathname
                        tmp[0..prefix.length] = prefix;
                        tmp[prefix.length..len] = name;

                        // temporaries, allocated on stack 
                        scope x = new FilePath (tmp.ptr, len);
                        scope p = new FileProxy (x);

                        // test this entry for inclusion
                        if (filter (p, isDir))
                           {
                           // skip dirs composed only of '.'
                           char[] suffix = x.getSuffix;
                           if (x.getName.length is 0 && suffix.length <= 3 &&
                               suffix == "..."[0..suffix.length]) {}
                           else
                              // create persistent instance for post processing
                              files ~= new File (new FilePath (tmp[0..len].dup.ptr, len, isDir));
                           }
                }

                folder.toList (&add);
                auto count = files_.length;
                
                foreach (file; files)
                         if (file.getPath.isDir)
                             scan (file, filter);
                         else
                            files_ ~= file;
                
                // add packages only if there's something in them
                if (files_.length > count)
                    folders_ ~= folder;
        }
}
