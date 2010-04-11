/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: March 2005      

        author:         Kris

*******************************************************************************/

module tango.io.File;

private import  tango.io.FileProxy,
                tango.io.FileConduit;

private import  tango.core.Exception;

/*******************************************************************************

        A wrapper atop of FileConduit to expose a simpler API. This one
        returns the entire file content as a void[], and sets the content
        to reflect a given void[].

        Method read() returns the current content of the file, whilst write()
        sets the file content, and file length, to the provided array. Method
        append() adds content to the tail of the file.

        Methods to inspect the file system, check the status of a file or
        directory, and other facilities are made available via the FileProxy
        superclass.

*******************************************************************************/

class File : FileProxy
{
        /***********************************************************************
        
                Construct a File from a text string

        ***********************************************************************/

        this (char[] path)
        {
                super (path);
        }

        /***********************************************************************
        
                Construct a File from the provided FilePath

        ***********************************************************************/
                                  
        this (FilePath path)
        {
                super (path);
        }

        /***********************************************************************

                Return the content of the file.

        ***********************************************************************/

        void[] read ()
        {
                auto conduit = new FileConduit (this);  
                scope (exit)
                       conduit.close();

                auto content = new ubyte[cast(int) conduit.length];

                // read the entire file into memory and return it
                if (conduit.fill (content) != content.length)
                    throw new IOException ("eof whilst reading");

                return content;
        }

        /***********************************************************************

                Set the file content and length to reflect the given array.

        ***********************************************************************/

        File write (void[] content)
        {
                return write (content, FileConduit.ReadWriteCreate);  
        }

        /***********************************************************************

                Append content to the file.

        ***********************************************************************/

        File append (void[] content)
        {
                return write (content, FileConduit.WriteAppending);  
        }

        /***********************************************************************

                Set the file content and length to reflect the given array.

        ***********************************************************************/

        private File write (void[] content, FileConduit.Style style)
        {      
                auto conduit = new FileConduit (this, style);  
                scope (exit)
                       conduit.close();

                conduit.flush (content);
                return this;
        }
}