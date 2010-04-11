/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Initial release: May 2004
        
        author:         Kris

*******************************************************************************/

module tango.util.log.DateLayout;

private import  tango.util.log.Event,
                tango.util.log.Layout;

private import  tango.text.Util;

private import  tango.core.Epoch;

private import  tango.text.convert.Integer;

/*******************************************************************************

        A layout with ISO-8601 date information prefixed to each message
       
*******************************************************************************/

public class DateLayout : Layout
{
        private static char[6] spaces = ' ';

        /***********************************************************************
                
                Format message attributes into an output buffer and return
                the populated portion.

        ***********************************************************************/

        char[] header (Event event)
        {
                Epoch.Fields fields;
                char[20] tmp = void;
                char[] level = event.getLevelName;
                
                // convert time to field values
                fields.asUtcTime (event.getEpochTime);
                                
                // format fields according to ISO-8601 (lightweight formatter)
                return layout (event.scratch.content, "%0-%1-%2 %3:%4:%5,%6 %7%8 %9 - ", 
                               convert (tmp[0..4],   fields.year),
                               convert (tmp[4..6],   fields.month),
                               convert (tmp[6..8],   fields.day),
                               convert (tmp[8..10],  fields.hour),
                               convert (tmp[10..12], fields.min),
                               convert (tmp[12..14], fields.sec),
                               convert (tmp[14..17], fields.ms),
                               spaces [0 .. $-level.length],
                               level,
                               event.getName
                              );
        }
        
        /**********************************************************************

                Convert an integer to a zero prefixed text representation

        **********************************************************************/

        private char[] convert (char[] tmp, int i)
        {
                return format (tmp, cast(long) i, Format.Unsigned, Flags.Zero);
        }
}
