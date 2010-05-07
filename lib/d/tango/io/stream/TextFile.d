/*******************************************************************************

        copyright:      Copyright (c) 2007 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: Nov 2007

        author:         Kris

*******************************************************************************/

module tango.io.stream.TextFile;

public  import tango.io.device.File;

private import tango.io.stream.Text;

/*******************************************************************************

        Composes a file with line-oriented input. The input is buffered

*******************************************************************************/

class TextFileInput : TextInput
{
        /***********************************************************************

                compose a FileStream              

        ***********************************************************************/

        this (char[] path, File.Style style = File.ReadExisting)
        {
                this (new File (path, style));
        }

        /***********************************************************************

                Wrap a FileConduit instance

        ***********************************************************************/

        this (File file)
        {
                super (file);
        }
}


/*******************************************************************************
       
        Composes a file with formatted text output. Output is buffered

*******************************************************************************/

class TextFileOutput : TextOutput
{
        /***********************************************************************

                compose a FileStream              

        ***********************************************************************/

        this (char[] path, File.Style style = File.WriteCreate)
        {
                this (new File (path, style));
        }

        /***********************************************************************

                Wrap a File instance

        ***********************************************************************/

        this (File file)
        {
                super (file);
        }
 }


/*******************************************************************************

*******************************************************************************/

debug (TextFile)
{
        import tango.io.Console;

        void main()
        {
                auto t = new TextFileInput ("TextFile.d");
                foreach (line; t)
                         Cout(line).newline;                  
        }
}
