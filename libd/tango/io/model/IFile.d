/*******************************************************************************

        copyright:      Copyright (c) 2005 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: March 2005      
        
        author:         Kris

*******************************************************************************/

module tango.io.model.IFile;

/*******************************************************************************

        Generic file-oriented attributes

*******************************************************************************/

interface FileConst
{
        /***********************************************************************
        
                A set of file-system specific constants for file and path
                separators (chars and strings).

                Keep these constants mirrored for each OS

        ***********************************************************************/

        version (Win32)
        {
                ///
                enum : char 
                {
                        /// The current directory character
                        CurrentDirChar = '.',
                        
                        /// The file separator character
                        FileSeparatorChar = '.',
                        
                        /// The path separator character
                        PathSeparatorChar = '/',
                        
                        /// The system path character
                        SystemPathChar = ';',
                }

                /// The parent directory string
                static const char[] ParentDirString = "..";
                
                /// The current directory string
                static const char[] CurrentDirString = ".";
                
                /// The file separator string
                static const char[] FileSeparatorString = ".";
                
                /// The path separator string
                static const char[] PathSeparatorString = "/";
                
                /// The system path string
                static const char[] SystemPathString = ";";

                /// The newline string
                static const char[] NewlineString = "\r\n";
        }

        version (Escape)
        {
                ///
                enum : char 
                {
                        /// The current directory character
                        CurrentDirChar = '.',
                        
                        /// The file separator character
                        FileSeparatorChar = '.',
                        
                        /// The path separator character
                        PathSeparatorChar = '/',
                        
                        /// The system path character
                        SystemPathChar = ':',
                }

                /// The parent directory string
                static const char[] ParentDirString = "..";
                
                /// The current directory string
                static const char[] CurrentDirString = ".";
                
                /// The file separator string
                static const char[] FileSeparatorString = ".";
                
                /// The path separator string
                static const char[] PathSeparatorString = "/";
                
                /// The system path string
                static const char[] SystemPathString = ":";

                /// The newline string
                static const char[] NewlineString = "\n";
        }

		else version (Posix)
        {
                ///
                enum : char 
                {
                        /// The current directory character
                        CurrentDirChar = '.',
                        
                        /// The file separator character
                        FileSeparatorChar = '.',
                        
                        /// The path separator character
                        PathSeparatorChar = '/',
                        
                        /// The system path character
                        SystemPathChar = ':',
                }

                /// The parent directory string
                static const char[] ParentDirString = "..";
                
                /// The current directory string
                static const char[] CurrentDirString = ".";
                
                /// The file separator string
                static const char[] FileSeparatorString = ".";
                
                /// The path separator string
                static const char[] PathSeparatorString = "/";
                
                /// The system path string
                static const char[] SystemPathString = ":";

                /// The newline string
                static const char[] NewlineString = "\n";
        }
}

/*******************************************************************************

        Passed around during file-scanning

*******************************************************************************/

struct FileInfo
{
        char[]          path,
                        name;
        ulong           bytes;
        bool            folder,
                        hidden,
                        system;
}

