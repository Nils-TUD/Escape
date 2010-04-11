/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Initial release: May 2004
        
        author:         Kris

*******************************************************************************/

module tango.util.log.LayoutChainsaw;

private import  tango.core.Thread;

private import  tango.util.log.Log;

/*******************************************************************************

        A layout with XML output conforming to Log4J specs.
       
*******************************************************************************/

public class LayoutChainsaw : Appender.Layout
{
        /***********************************************************************
                
                Subclasses should implement this method to perform the
                formatting of the actual message content.

        ***********************************************************************/

        void format (LogEvent event, size_t delegate(void[]) dg)
        {
                char[20] tmp;
                char[]   threadName;
                
                threadName = Thread.getThis.name;
                if (threadName.length is 0)
                    threadName = "{unknown}";

                dg ("<log4j:event logger=\"");
                dg (event.name);
                dg ("\" timestamp=\"");
                dg (event.toMilli (tmp, event.time.span));
                dg ("\" level=\"");
                dg (event.levelName);
                dg ("\" thread=\"");
                dg (threadName);
                dg ("\">\r\n<log4j:message><![CDATA[");

                dg (event.toString);

                dg ("]]></log4j:message>\r\n<log4j:properties><log4j:data name=\"application\" value=\"");
                dg (event.host.name);
                dg ("\"/><log4j:data name=\"hostname\" value=\"");
                dg (event.host.address);
                dg ("\"/></log4j:properties></log4j:event>\r\n");
        }
}


