/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Initial release: May 2004
        
        author:         Kris

*******************************************************************************/

module tango.util.log.model.IHierarchy;

/*******************************************************************************

        The Logger hierarchy Interface. We use this to break the
        interdependency between a couple of modules        

*******************************************************************************/

interface IHierarchy
{
        /**********************************************************************

                Return the name of this Hierarchy

        **********************************************************************/

        char[] getName ();

        /**********************************************************************

                Return the address of this Hierarchy. This is typically
                attached when sending events to remote monitors.

        **********************************************************************/

        char[] getAddress ();
}
