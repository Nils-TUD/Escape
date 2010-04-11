/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Initial release: May 2004
        
        author:         Kris

*******************************************************************************/

module tango.util.log.AppendConsole;

private import  tango.io.Console;

private import  tango.util.log.Log;

/*******************************************************************************

        Appender for sending formatted output to the console

*******************************************************************************/

public class AppendConsole : AppendStream
{
        /***********************************************************************
                
                Create with the given layout

        ***********************************************************************/

        this (Appender.Layout how = null)
        {
                super (Cerr.stream, true, how);
        }

        /***********************************************************************
                
                Return the name of this class

        ***********************************************************************/

        override char[] name ()
        {
                return this.classinfo.name;
        }
}
