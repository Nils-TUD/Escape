/*******************************************************************************

        copyright:      Copyright (c) 2007 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: Nov 2007

        author:         Kris

*******************************************************************************/

module tango.io.stream.DataFile;

private import tango.io.device.File;

private import tango.io.stream.Data;

/*******************************************************************************

        Composes a seekable file with buffered binary input. A seek causes
        the input buffer to be cleared

*******************************************************************************/

class DataFileInput : DataInput
{
        private File conduit;

        /***********************************************************************

                Compose a FileStream              

        ***********************************************************************/

        this (char[] path, File.Style style = File.ReadExisting)
        {
                this (new File (path, style));
        }

        /***********************************************************************

                Wrap a File instance

        ***********************************************************************/

        this (File file)
        {
                super (conduit = file);
        }

        /***********************************************************************

                Return the underlying conduit

        ***********************************************************************/

        final File file ()
        {       
                return conduit;
        }
}


/*******************************************************************************
       
        Composes a seekable file with buffered binary output. A seek causes
        the output buffer to be flushed first

*******************************************************************************/

class DataFileOutput : DataOutput
{
        private File conduit;

        /***********************************************************************

                Compose a FileStream              

        ***********************************************************************/

        this (char[] path, File.Style style = File.WriteCreate)
        {
                this (new File (path, style));
        }

        /***********************************************************************

                Wrap a FileConduit instance

        ***********************************************************************/

        this (File file)
        {
                super (conduit = file);
        }

        /***********************************************************************

                Return the underlying conduit

        ***********************************************************************/

        final File file ()
        {       
                return conduit;
        }
}

debug (DataFile)
{
        import tango.io.Stdout;

        void main()
        {
                auto myFile = new DataFileOutput("Hello.txt");
                myFile.write("some text");
                myFile.flush;
                Stdout.formatln ("{}:{}", myFile.file.position, myFile.seek(myFile.Anchor.Current));
        }
}
