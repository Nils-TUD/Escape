/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Initial release: May 2004
        
        author:         Kris

*******************************************************************************/

module tango.util.log.Layout;

private import tango.util.log.Event;

/*******************************************************************************

        Base class for all Layout instances

*******************************************************************************/

public class Layout
{
        /***********************************************************************
                
                Subclasses should implement this method to perform the
                formatting of each message header

        ***********************************************************************/

        abstract char[]  header (Event event);

        /***********************************************************************
                
                Subclasses should implement this method to perform the
                formatting of each message footer

        ***********************************************************************/

        char[] footer (Event event)
        {
                return "";
        }

        /***********************************************************************
                
                Subclasses should implement this method to perform the
                formatting of the actual message content.

        ***********************************************************************/

        char[] content (Event event)
        {
                return event.toUtf8;
        }

        /***********************************************************************
                
                Convert a time value (in milliseconds) to ascii

        ***********************************************************************/

        final char[] ultoa (char[] s, ulong l)
        in {
           assert (s.length > 0);
           }
        body 
        {
                int len = s.length;
                do {
                   s[--len] = l % 10 + '0';
                   l /= 10;
                   } while (l && len);
                return s[len..s.length];                
        }
}


/*******************************************************************************

        A bare layout comprised of tag and message

*******************************************************************************/

public class SpartanLayout : Layout
{
        /***********************************************************************
                
                Format outgoing message

        ***********************************************************************/

        char[] header (Event event)
        {
                event.append(event.getName).append(" - ");
                return event.getContent;
        }
}


/*******************************************************************************

        A simple layout comprised only of level, name, and message

*******************************************************************************/

public class SimpleLayout : Layout
{
        /***********************************************************************
                
                Format outgoing message

        ***********************************************************************/

        char[] header (Event event)
        {
                event.append (event.getLevelName)
                     .append(event.getName)
                     .append(" - ");
                return event.getContent;
        }
}


/*******************************************************************************

        A simple layout comprised only of time(ms), level, name, and message

*******************************************************************************/

public class SimpleTimerLayout : Layout
{
        /***********************************************************************
                
                Format outgoing message

        ***********************************************************************/

        char[] header (Event event)
        {
                char[20] tmp;

                event.append (ultoa (tmp, event.getTime))
                     .append (" ")
                     .append (event.getLevelName)
                     .append (event.getName)
                     .append (" - ");
                return event.getContent;
        }
}
