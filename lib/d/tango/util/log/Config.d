/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Initial release: May 2004
        
        author:         Kris

*******************************************************************************/

module tango.util.log.Config;

public  import  tango.util.log.Log : Log;

private import  tango.util.log.LayoutDate,
                tango.util.log.AppendConsole;

/*******************************************************************************

        Utility for initializing the basic behaviour of the default
        logging hierarchy.

        Adds a default console appender with a generic layout to the 
        root node, and set the activity level to be everything enabled
                
*******************************************************************************/

static this ()
{
        Log.root.add (new AppendConsole (new LayoutDate));
}

