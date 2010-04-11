/*******************************************************************************

        copyright:      Copyright (c) 2005 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
        
        version:        Initial release: May 2005
        
        author:         Kris

*******************************************************************************/

module tango.core.Epoch;

private import tango.sys.Common;

private import tango.core.Exception;

version (Posix)
{
        extern (C) tm *gmtime (int *);
        extern (C) int timegm (tm *);
}

version (Escape)
{
	private import tango.stdc.time;
	extern (C) tm *gmtime(int *);
	extern (C) int timegm(tm *);
}

/******************************************************************************

        Represents UTC time relative to Jan 1st 1970, and converts between
        the native time format (a long integer) and a set of fields. The
        native time is expected to be accurate to the millisecond level,
        though this is OS dependent. Note that Epoch is independent of a
        local time, unless you happen to live at GMT + 0.

        Epoch is wrapped in a struct purely to maintain a namespace; there
        is no need to instantiate it since all methods are static. The inner
        struct Fields is a traditional struct, and should be instantiated in
        the normal fashion. To set all fields to the current local time, for
        example:
        
        ---
        Epoch.Fields fields;

        fields.asLocalTime (Epoch.utcMilli);
        ---

        Attributes exposed by the Fields struct include year, month, day,
        hour, minute, second, millisecond, and day of week. Epoch itself
        exposes methods for retriving time in milliseconds & microseconds,
        and for retrieving the local GMT offset in minutes.

        Note that Epoch does not provide conversion to and from textual
        representations, since that requires support for both I18N and
        wide characters. 

        See TimeStamp.d for an example of a simple derivation, and see 
        http://en.wikipedia.org/wiki/Unix_time for details on UTC time.

******************************************************************************/

class Epoch
{
        private static ulong beginTime;

        const ulong InvalidEpoch = -1;

        /***********************************************************************
                
                        Utc time this executable started 

        ***********************************************************************/

        final static ulong startTime ()
        {
                return beginTime;
        }

        /***********************************************************************
                     
                A set of fields representing date/time components. This is 
                used to abstract differences between Win32 & Posix platforms.
                                           
        ***********************************************************************/

        struct Fields
        {
                int     year,           // fully defined year ~ e.g. 2005
                        month,          // 1 through 12
                        day,            // 1 through 31
                        hour,           // 0 through 23
                        min,            // 0 through 59
                        sec,            // 0 through 59
                        ms,             // 0 through 999
                        dow;            // 0 through 6; sunday == 0


                // list of english day names
                static char[][] Days = 
                [
                        "Sun",
                        "Mon",
                        "Tue",
                        "Wed",
                        "Thu",
                        "Fri",
                        "Sat",
                ];
                
                // list of english month names
                static char[][] Months = 
                [
                        "Jan",
                        "Feb",
                        "Mar",
                        "Apr",
                        "May",
                        "Jun",
                        "Jul",
                        "Aug",
                        "Sep",
                        "Oct",
                        "Nov",
                        "Dec",
                ];

                /**************************************************************

                        Get the english short month name
                        
                **************************************************************/

                char[] toShortMonth ()
                {
                        return Months [month-1];
                }
                
                /**************************************************************

                        Get the english short day name
                        
                **************************************************************/

                char[] toShortDay ()
                {
                        return Days [dow];
                }
                
                /**************************************************************

                        Set the date-related values

                        year  : fully defined year ~ e.g. 2005
                        month : 1 through 12
                        day   : 1 through 31
                        dow   : 0 through 6; sunday=0 (typically set by O/S)

                **************************************************************/

                void setDate (int year, int month, int day, int dow = 0)
                {
                        this.year = year;
                        this.month = month;
                        this.day = day;
                        this.dow = dow;
                }

                /**************************************************************

                        Set the time-related values 

                        hour : 0 through 23
                        min  : 0 through 59
                        sec  : 0 through 59
                        ms   : 0 through 999

                **************************************************************/

                void setTime (int hour, int min, int sec, int ms = 0)
                {
                        this.hour = hour;
                        this.min = min;
                        this.sec = sec;
                        this.ms = ms;
                }

                /***************************************************************

                        Win32 implementation

                ***************************************************************/

                version (Win32)
                {
                        /*******************************************************

                                Convert fields to UTC time, and return 
                                milliseconds since epoch

                        *******************************************************/

                        ulong toUtcTime ()
                        {
                                SYSTEMTIME sTime = void;
                                FILETIME   fTime = void;

                                sTime.wYear = cast(ushort) year;
                                sTime.wMonth = cast(ushort) month;
                                sTime.wDayOfWeek = 0;
                                sTime.wDay = cast(ushort) day;
                                sTime.wHour = cast(ushort) hour;
                                sTime.wMinute = cast(ushort) min;
                                sTime.wSecond = cast(ushort) sec;
                                sTime.wMilliseconds = cast(ushort) ms;

                                SystemTimeToFileTime (&sTime, &fTime);

                                return fromFileTime (&fTime);
                        }

                        /*******************************************************

                                Set fields to represent the provided epoch 
                                time
                                
                        *******************************************************/

                        void asUtcTime (ulong time)
                        {
                                SYSTEMTIME sTime = void;
                                FILETIME   fTime = void;

                                toFileTime (&fTime, time);
                                FileTimeToSystemTime (&fTime, &sTime);

                                year = sTime.wYear;
                                month = sTime.wMonth;
                                day = sTime.wDay;
                                hour = sTime.wHour;
                                min = sTime.wMinute;
                                sec = sTime.wSecond;
                                ms = sTime.wMilliseconds;
                                dow = sTime.wDayOfWeek;
                        }

                        /*******************************************************

                                Set fields to represent a localized version
                                of the provided epoch time

                        *******************************************************/

                        void asLocalTime (ulong time)
                        {
                                FILETIME fTime = void;
                                FILETIME local = void;

                                toFileTime (&fTime, time);
                                FileTimeToLocalFileTime (&fTime, &local);
                                asUtcTime (fromFileTime (&local));
                        }
                }


                /***************************************************************

                        Posix implementation

                ***************************************************************/

                version (Posix)
                {
                        /*******************************************************

                                Convert fields to UTC time, and return 
                                milliseconds since epoch

                        *******************************************************/

                        ulong toUtcTime ()
                        {
                                tm t;

                                t.tm_year = year - 1900;
                                t.tm_mon = month - 1;
                                t.tm_mday = day;
                                t.tm_hour = hour;
                                t.tm_min = min;
                                t.tm_sec = sec;
                                return 1000L * cast(ulong) timegm(&t) + ms;
                        }

                        /*******************************************************

                                Set fields to represent the provided epoch 
                                time


                        *******************************************************/

                        void asUtcTime (ulong time)
                        {
                                ms = time % 1000;                                
                                int utc = time / 1000;

                                synchronized (Epoch.classinfo)
                                             {
                                             tm* t = gmtime (&utc);
                                             assert (t, "gmtime failed");

                                             year = t.tm_year + 1900;
                                             month = t.tm_mon + 1;
                                             day = t.tm_mday;
                                             hour = t.tm_hour;
                                             min = t.tm_min;
                                             sec = t.tm_sec;
                                             dow = t.tm_wday;                        
                                             }
                        }

                        /*******************************************************

                                Set fields to represent a localized version
                                of the provided epoch time 

                        *******************************************************/

                        void asLocalTime (ulong time)
                        {
                                ms = time % 1000;                                
                                int utc = time / 1000;

                                synchronized (Epoch.classinfo)
                                             {
                                             tm* t = localtime (&utc);
                                             year = t.tm_year + 1900;
                                             month = t.tm_mon + 1;
                                             day = t.tm_mday;
                                             hour = t.tm_hour;
                                             min = t.tm_min;
                                             sec = t.tm_sec;
                                             dow = t.tm_wday;                        
                                             }
                        }
                }


                /***************************************************************

                        Escape implementation

                ***************************************************************/

                version (Escape)
                {
                        /*******************************************************

                                Convert fields to UTC time, and return 
                                milliseconds since epoch

                        *******************************************************/

                        ulong toUtcTime ()
                        {
                                tm t;

                                t.tm_year = year - 1900;
                                t.tm_mon = month - 1;
                                t.tm_mday = day;
                                t.tm_hour = hour;
                                t.tm_min = min;
                                t.tm_sec = sec;
                                return 1000L * cast(ulong) mktime(&t) + ms;
                        }

                        /*******************************************************

                                Set fields to represent the provided epoch 
                                time


                        *******************************************************/

                        void asUtcTime (ulong time)
                        {
                                ms = time % 1000;                                
                                int utc = time / 1000;

                                synchronized (Epoch.classinfo)
                                             {
                                             tm* t = gmtime (&utc);
                                             assert (t, "gmtime failed");

                                             year = t.tm_year + 1900;
                                             month = t.tm_mon + 1;
                                             day = t.tm_mday;
                                             hour = t.tm_hour;
                                             min = t.tm_min;
                                             sec = t.tm_sec;
                                             dow = t.tm_wday;                        
                                             }
                        }

                        /*******************************************************

                                Set fields to represent a localized version
                                of the provided epoch time 

                        *******************************************************/

                        void asLocalTime (ulong time)
                        {
                                ms = time % 1000;                                
                                int utc = time / 1000;

                                synchronized (Epoch.classinfo)
                                             {
                                             tm* t = localtime (&utc);
                                             year = t.tm_year + 1900;
                                             month = t.tm_mon + 1;
                                             day = t.tm_mday;
                                             hour = t.tm_hour;
                                             min = t.tm_min;
                                             sec = t.tm_sec;
                                             dow = t.tm_wday;                        
                                             }
                        }
                }
        }


        /***********************************************************************
                        
                Basic functions for epoch time

        ***********************************************************************/

        version (Win32)
        {
                private static ulong epochOffset;

                /***************************************************************
                
                        Construct an offset representing epoch time

                ***************************************************************/

                static this ()
                {
                        SYSTEMTIME sTime = void;
                        FILETIME   fTime = void;

                        // first second of 1970 ...
                        sTime.wYear = 1970;
                        sTime.wMonth = 1;
                        sTime.wDayOfWeek = 0;
                        sTime.wDay = 1;
                        sTime.wHour = 0;
                        sTime.wMinute = 0;
                        sTime.wSecond = 0;
                        sTime.wMilliseconds = 0;
                        SystemTimeToFileTime (&sTime, &fTime);

                        epochOffset = (cast(ulong) fTime.dwHighDateTime) << 32 | 
                                                   fTime.dwLowDateTime;
                        beginTime = utcMilli();
                }

                /***************************************************************
                
                        Return the current time as UTC milliseconds since 
                        the epoch

                ***************************************************************/

                static ulong utcMilli ()
                {
                        return utcMicro / 1000;
                }

                /***************************************************************
                
                        Return the current time as UTC microsecs since 
                        the epoch

                ***************************************************************/

                static ulong utcMicro ()
                {
                        FILETIME fTime = void;

                        GetSystemTimeAsFileTime (&fTime);
                        ulong tmp = (cast(ulong) fTime.dwHighDateTime) << 32 | 
                                                 fTime.dwLowDateTime;

                        // convert to nanoseconds
                        return (tmp - epochOffset) / 10;
                }

                /***************************************************************
                
                        Return the timezone minutes relative to GMT

                ***************************************************************/

                static int tzMinutes ()
                {
                        TIME_ZONE_INFORMATION tz = void;

                        int ret = GetTimeZoneInformation (&tz);
                        return -tz.Bias;
                }

                /***************************************************************
                
                        Convert filetime to epoch time
                         
                ***************************************************************/

                private static ulong fromFileTime (FILETIME* ft)
                {
                        ulong tmp = (cast(ulong) ft.dwHighDateTime) << 32 | 
                                                 ft.dwLowDateTime;

                        // convert to milliseconds
                        return (tmp - epochOffset) / 10_000;
                }

                /***************************************************************
                
                        convert epoch time to file time

                ***************************************************************/

                private static void toFileTime (FILETIME* ft, ulong ms)
                {
                        ms = ms * 10_000 + epochOffset;

                        ft.dwHighDateTime = cast(uint) (ms >> 32); 
                        ft.dwLowDateTime  = cast(uint) (ms & 0xFFFFFFFF);
                }
        }


        /***********************************************************************
                        
        ***********************************************************************/

        version (Posix)
        {
                // these are exposed via tango.stdc.time
                //extern (C) int timezone;
                //extern (C) int daylight;

                /***************************************************************
                
                        set start time

                ***************************************************************/

                static this()
                {
                        beginTime = utcMilli();
                }

                /***************************************************************
                
                        Return the current time as UTC milliseconds since 
                        the epoch. 

                ***************************************************************/

                static ulong utcMilli ()
                {
                        return utcMicro / 1000;
                }

                /***************************************************************
                
                        Return the current time as UTC microseconds since 
                        the epoch. 

                ***************************************************************/

                static ulong utcMicro ()
                {
                        timeval tv;

                        if (gettimeofday (&tv, null))
                            throw new PlatformException ("Epoch.utcMicro :: linux timer is not available");

                        return 1_000_000L * cast(ulong) tv.tv_sec + tv.tv_usec;
                }

                /***************************************************************
                
                        Return the timezone minutes relative to GMT

                ***************************************************************/

                static int tzMinutes ()
                {
                        return -timezone / 60;
                }
        }


        /***********************************************************************
                        
        ***********************************************************************/

        version (Escape)
        {
                /***************************************************************
                
                        set start time

                ***************************************************************/

                static this()
                {
                        beginTime = utcMilli();
                }

                /***************************************************************
                
                        Return the current time as UTC milliseconds since 
                        the epoch. 

                ***************************************************************/

                static ulong utcMilli ()
                {
                        return utcMicro / 1000;
                }

                /***************************************************************
                
                        Return the current time as UTC microseconds since 
                        the epoch. 

                ***************************************************************/

                static ulong utcMicro ()
                {
                		// TODO no microseconds atm
                        return 1_000_000L * cast(ulong)time(null);
                }

                /***************************************************************
                
                        Return the timezone minutes relative to GMT

                ***************************************************************/

                static int tzMinutes ()
                {
                		// TODO no timezone atm
                        return 0;
                }
        }
}
