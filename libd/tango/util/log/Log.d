/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Initial release: May 2004
        version:        Hierarchy moved due to circular dependencies; Oct 2004
        
        author:         Kris

*******************************************************************************/

module tango.util.log.Log;

public  import  tango.util.log.Logger;

private import  tango.util.log.Event,
                tango.util.log.Hierarchy;

/*******************************************************************************

        Manager for routing Logger calls to the default hierarchy. Note 
        that you may have multiple hierarchies per application, but must
        access the hierarchy directly for getRootLogger() and getLogger()
        methods within each additional instance.

*******************************************************************************/

class Log 
{
        static private Hierarchy base;

        /***********************************************************************
        
                This is a singleton, so hide the constructor.

        ***********************************************************************/

        private this ()
        {
        }

        /***********************************************************************
        
                Initialize the base hierarchy.                
              
        ***********************************************************************/

        static this ()
        {
                base = new Hierarchy ("tango");
                Event.initialize ();
        }

        /***********************************************************************

                Return the root Logger instance. This is the ancestor of
                all loggers and, as such, can be used to manipulate the 
                entire hierarchy. For instance, setting the root 'level' 
                attribute will affect all other loggers in the tree.

        ***********************************************************************/

        static Logger getRootLogger ()
        {
                return base.getRootLogger ();
        }

        /***********************************************************************
        
                Return an instance of the named logger. Names should be
                hierarchical in nature, using dot notation (with '.') to 
                seperate each name section. For example, a typical name 
                might be something like "tango.io.Buffer".

                If the logger does not currently exist, it is created and
                inserted into the hierarchy. A parent will be attached to
                it, which will be either the root logger or the closest
                ancestor in terms of the hierarchical name space.

        ***********************************************************************/

        static Logger getLogger (char[] name)
        {
                return base.getLogger (name);
        }

        /***********************************************************************
        
                Return the singleton hierarchy.

        ***********************************************************************/

        static Hierarchy getHierarchy ()
        {
                return base;
        }
}
