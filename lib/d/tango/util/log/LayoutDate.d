/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
      
        version:        Initial release: May 2004
        
        author:         Kris

*******************************************************************************/

module tango.util.log.LayoutDate;

private import  tango.text.Util;

private import  tango.time.Clock,
                tango.time.WallClock;

private import  tango.util.log.Log;

private import  Integer = tango.text.convert.Integer;

/*******************************************************************************

        A layout with ISO-8601 date information prefixed to each message
       
*******************************************************************************/

public class LayoutDate : Appender.Layout
{
        private bool localTime;

        private static char[6] spaces = ' ';

        /***********************************************************************
        
                Ctor with indicator for local vs UTC time. Default is 
                local time.
                        
        ***********************************************************************/

        this (bool localTime = true)
        {
                this.localTime = localTime;
        }

        /***********************************************************************
                
                Subclasses should implement this method to perform the
                formatting of the actual message content.

        ***********************************************************************/

        void format (LogEvent event, size_t delegate(void[]) dg)
        {
                char[] level = event.levelName;
                
                // convert time to field values
                auto tm = event.time;
                auto dt = (localTime) ? WallClock.toDate(tm) : Clock.toDate(tm);
                                
                // format date according to ISO-8601 (lightweight formatter)
                char[20] tmp = void;
                char[256] tmp2 = void;
                dg (layout (tmp2, "%0-%1-%2 %3:%4:%5,%6 %7 [%8] - ", 
                            convert (tmp[0..4],   dt.date.year),
                            convert (tmp[4..6],   dt.date.month),
                            convert (tmp[6..8],   dt.date.day),
                            convert (tmp[8..10],  dt.time.hours),
                            convert (tmp[10..12], dt.time.minutes),
                            convert (tmp[12..14], dt.time.seconds),
                            convert (tmp[14..17], dt.time.millis),
                            level,
                            event.name
                            ));
                dg (event.toString);
        }

        /**********************************************************************

                Convert an integer to a zero prefixed text representation

        **********************************************************************/

        private char[] convert (char[] tmp, long i)
        {
                return Integer.formatter (tmp, i, 'u', '?', 8);
        }
}
