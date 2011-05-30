/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
        
        version:        Initial release: May 2005      
      
        author:         Kris

        Converts between native and text representations of HTTP time
        values. Internally, time is represented as UTC with an epoch 
        fixed at Jan 1st 1970. The text representation is formatted in
        accordance with RFC 1123, and the parser will accept one of 
        RFC 1123, RFC 850, or asctime formats.

        See http://www.w3.org/Protocols/rfc2616/rfc2616-sec3.html for
        further detail.

        Applying the D "import alias" mechanism to this module is highly
        recommended, in order to limit namespace pollution:
        ---
        import TimeStamp = tango.text.convert.TimeStamp;

        auto t = TimeStamp.parse ("Sun, 06 Nov 1994 08:49:37 GMT");
        ---
        
*******************************************************************************/

module tango.text.convert.TimeStamp;

private import tango.time.Time;

private import tango.core.Exception;

private import Util = tango.text.Util;

private import tango.time.chrono.Gregorian;

private import Integer = tango.text.convert.Integer;

/******************************************************************************

        Parse provided input and return a UTC epoch time. An exception
        is raised where the provided string is not fully parsed.

******************************************************************************/

ulong toTime(T) (T[] src)
{
        uint len;

        auto x = parse (src, &len);
        if (len < src.length)
            throw new IllegalArgumentException ("unknown time format: "~src);
        return x;
}

/******************************************************************************

        Template wrapper to make life simpler. Returns a text version
        of the provided value.

        See format() for details

******************************************************************************/

char[] toString (Time time)
{
        char[32] tmp = void;
        
        return format (tmp, time).dup;
}
               
/******************************************************************************

        Template wrapper to make life simpler. Returns a text version
        of the provided value.

        See format() for details

******************************************************************************/

wchar[] toString16 (Time time)
{
        wchar[32] tmp = void;
        
        return format (tmp, time).dup;
}
               
/******************************************************************************

        Template wrapper to make life simpler. Returns a text version
        of the provided value.

        See format() for details

******************************************************************************/

dchar[] toString32 (Time time)
{
        dchar[32] tmp = void;
        
        return format (tmp, time).dup;
}
               
/******************************************************************************

        RFC1123 formatted time

        Converts to the format "Sun, 06 Nov 1994 08:49:37 GMT", and
        returns a populated slice of the provided buffer. Note that
        RFC1123 format is always in absolute GMT time, and a thirty-
        element buffer is sufficient for the produced output

        Throws an exception where the supplied time is invalid

******************************************************************************/

T[] format(T, U=Time) (T[] output, U t)
{return format!(T)(output, cast(Time) t);}

T[] format(T) (T[] output, Time t)
{
        static T[][] Months = ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
        static T[][] Days   = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];

        T[] convert (T[] tmp, long i)
        {
                return Integer.formatter!(T) (tmp, i, 'u', 0, 8);
        }

        assert (output.length >= 29);
        if (t is t.max)
            throw new IllegalArgumentException ("TimeStamp.format :: invalid Time argument");

        // convert time to field values
        auto time = t.time;
        auto date = Gregorian.generic.toDate (t);

        // use the featherweight formatter ...
        T[14] tmp = void;
        return Util.layout (output, cast(T[])"%0, %1 %2 %3 %4:%5:%6 GMT", 
                            Days[date.dow],
                            convert (tmp[0..2], date.day),
                            Months[date.month-1],
                            convert (tmp[2..6], date.year),
                            convert (tmp[6..8], time.hours),
                            convert (tmp[8..10], time.minutes),
                            convert (tmp[10..12], time.seconds)
                           );
}


/******************************************************************************

        ISO-8601 format :: "2006-01-31T14:49:30Z"

        Throws an exception where the supplied time is invalid

******************************************************************************/

T[] format8601(T, U=Time) (T[] output, U t)
{return format!(T)(output, cast(Time) t);}

T[] format8601(T) (T[] output, Time t)
{
        T[] convert (T[] tmp, long i)
        {
                return Integer.formatter!(T) (tmp, i, 'u', 0, 8);
        }


        assert (output.length >= 29);
        if (t is t.max)
            throw new IllegalArgumentException ("TimeStamp.format :: invalid Time argument");

        // convert time to field values
        auto time = t.time;
        auto date = Gregorian.generic.toDate (t);

        // use the featherweight formatter ...
        T[20] tmp = void;
        return Util.layout (output, cast(T[]) "%0-%1-%2T%3%:%4:%5Z", 
                            convert (tmp[0..4], date.year),
                            convert (tmp[4..6], date.month),
                            convert (tmp[6..8], date.day),
                            convert (tmp[8..10], time.hours),
                            convert (tmp[10..12], time.minutes),
                            convert (tmp[12..14], time.seconds)
                           );
}

/******************************************************************************

      Parse provided input and return a UTC epoch time. A return value 
      of Time.max (or false, respectively) indicated a parse-failure.

      An option is provided to return the count of characters parsed - 
      an unchanged value here also indicates invalid input.

******************************************************************************/

Time parse(T) (T[] src, uint* ate = null)
{
        size_t len;
        Time   value;

        if ((len = rfc1123 (src, value)) > 0 || 
            (len = rfc850  (src, value)) > 0 || 
            (len = iso8601  (src, value)) > 0 || 
            (len = dostime  (src, value)) > 0 || 
            (len = asctime (src, value)) > 0)
           {
           if (ate)
               *ate = len;
           return value;
           }
        return Time.max;
}


/******************************************************************************

      Parse provided input and return a UTC epoch time. A return value 
      of Time.max (or false, respectively) indicated a parse-failure.

      An option is provided to return the count of characters parsed - 
      an unchanged value here also indicates invalid input.

******************************************************************************/

bool parse(T) (T[] src, ref TimeOfDay tod, ref Date date, uint* ate = null)
{
        size_t len;
    
        if ((len = rfc1123 (src, tod, date)) > 0 || 
           (len = rfc850   (src, tod, date)) > 0 || 
           (len = iso8601  (src, tod, date)) > 0 || 
           (len = dostime  (src, tod, date)) > 0 || 
           (len = asctime (src, tod, date)) > 0)
           {
           if (ate)
               *ate = len;
           return true;
           }
        return false;
}

/******************************************************************************

        RFC 822, updated by RFC 1123 :: "Sun, 06 Nov 1994 08:49:37 GMT"

        Returns the number of elements consumed by the parse; zero if
        the parse failed

******************************************************************************/

size_t rfc1123(T) (T[] src, ref Time value)
{
        TimeOfDay tod;
        Date      date;

        auto r = rfc1123!(T)(src, tod, date);
        if (r)   
            value = Gregorian.generic.toTime(date, tod);
        return r;
}


/******************************************************************************

        RFC 822, updated by RFC 1123 :: "Sun, 06 Nov 1994 08:49:37 GMT"

        Returns the number of elements consumed by the parse; zero if
        the parse failed

******************************************************************************/

size_t rfc1123(T) (T[] src, ref TimeOfDay tod, ref Date date)
{
        T* p = src.ptr;
        T* e = p + src.length;

        bool dt (ref T* p)
        {
                return ((date.day = parseInt(p, e)) > 0  &&
                         *p++ == ' '                     &&
                        (date.month = parseMonth(p)) > 0 &&
                         *p++ == ' '                     &&
                        (date.year = parseInt(p, e)) > 0);
        }

        if (parseShortDay(p) >= 0 &&
            *p++ == ','           &&
            *p++ == ' '           &&
            dt (p)                &&
            *p++ == ' '           &&
            time (tod, p, e)      &&
            *p++ == ' '           &&
            p[0..3] == "GMT")
            {
            return (p+3) - src.ptr;
            }
        return 0;
}


/******************************************************************************

        RFC 850, obsoleted by RFC 1036 :: "Sunday, 06-Nov-94 08:49:37 GMT"

        Returns the number of elements consumed by the parse; zero if
        the parse failed

******************************************************************************/

size_t rfc850(T) (T[] src, ref Time value)
{
        TimeOfDay tod;
        Date      date;

        auto r = rfc850!(T)(src, tod, date);
        if (r)
            value = Gregorian.generic.toTime (date, tod);
        return r;
}

/******************************************************************************

        RFC 850, obsoleted by RFC 1036 :: "Sunday, 06-Nov-94 08:49:37 GMT"

        Returns the number of elements consumed by the parse; zero if
        the parse failed

******************************************************************************/

size_t rfc850(T) (T[] src, ref TimeOfDay tod, ref Date date)
{
        T* p = src.ptr;
        T* e = p + src.length;

        bool dt (ref T* p)
        {
                return ((date.day = parseInt(p, e)) > 0  &&
                         *p++ == '-'                     &&
                        (date.month = parseMonth(p)) > 0 &&
                         *p++ == '-'                     &&
                        (date.year = parseInt(p, e)) > 0);
        }

        if (parseFullDay(p) >= 0 &&
            *p++ == ','          &&
            *p++ == ' '          &&
            dt (p)               &&
            *p++ == ' '          &&
            time (tod, p, e)     &&
            *p++ == ' '          &&
            p[0..3] == "GMT")
            {
            if (date.year < 70)
                date.year += 2000;
            else
               if (date.year < 100)
                   date.year += 1900;

            return (p+3) - src.ptr;
            }
        return 0;
}


/******************************************************************************

        ANSI C's asctime() format :: "Sun Nov 6 08:49:37 1994"

        Returns the number of elements consumed by the parse; zero if
        the parse failed

******************************************************************************/

size_t asctime(T) (T[] src, ref Time value)
{
        TimeOfDay tod;
        Date      date;
    
        auto r = asctime!(T)(src, tod, date);
        if (r)
            value = Gregorian.generic.toTime (date, tod);
        return r;
}

/******************************************************************************

        ANSI C's asctime() format :: "Sun Nov 6 08:49:37 1994"

        Returns the number of elements consumed by the parse; zero if
        the parse failed

******************************************************************************/

size_t asctime(T) (T[] src, ref TimeOfDay tod, ref Date date)
{
        T* p = src.ptr;
        T* e = p + src.length;

        bool dt (ref T* p)
        {
                return ((date.month = parseMonth(p)) > 0  &&
                         *p++ == ' '                      &&
                        ((date.day = parseInt(p, e)) > 0  ||
                        (*p++ == ' '                      &&
                        (date.day = parseInt(p, e)) > 0)));
        }

        if (parseShortDay(p) >= 0 &&
            *p++ == ' '           &&
            dt (p)                &&
            *p++ == ' '           &&
            time (tod, p, e)      &&
            *p++ == ' '           &&
            (date.year = parseInt (p, e)) > 0)
            {
            return p - src.ptr;
            }
        return 0;
}

/******************************************************************************

        DOS time format :: "12-31-06 08:49AM"

        Returns the number of elements consumed by the parse; zero if
        the parse failed

******************************************************************************/

size_t dostime(T) (T[] src, ref Time value)
{
        TimeOfDay tod;
        Date      date;
    
        auto r = dostime!(T)(src, tod, date);
        if (r)
            value = Gregorian.generic.toTime(date, tod);
        return r;
}


/******************************************************************************

        DOS time format :: "12-31-06 08:49AM"

        Returns the number of elements consumed by the parse; zero if
        the parse failed

******************************************************************************/

size_t dostime(T) (T[] src, ref TimeOfDay tod, ref Date date)
{
        T* p = src.ptr;
        T* e = p + src.length;

        bool dt (ref T* p)
        {
                return ((date.month = parseInt(p, e)) > 0 &&
                         *p++ == '-'                      &&
                        ((date.day = parseInt(p, e)) > 0  &&
                        (*p++ == '-'                      &&
                        (date.year = parseInt(p, e)) > 0)));
        }

        if (dt(p) >= 0                         &&
            *p++ == ' '                        &&
            (tod.hours = parseInt(p, e)) > 0   &&
            *p++ == ':'                        &&
            (tod.minutes = parseInt(p, e)) > 0 &&
            (*p == 'A' || *p == 'P'))
            {
            if (*p is 'P')
                tod.hours += 12;
            
            if (date.year < 70)
                date.year += 2000;
            else
               if (date.year < 100)
                   date.year += 1900;
            
            return (p+2) - src.ptr;
            }
        return 0;
}

/******************************************************************************

        ISO-8601 format :: "2006-01-31 14:49:30,001"

        Returns the number of elements consumed by the parse; zero if
        the parse failed

        Quote from http://en.wikipedia.org/wiki/ISO_8601 (2009-09-01):
        "Decimal fractions may also be added to any of the three time elements.
        A decimal point, either a comma or a dot (without any preference as
        stated most recently in resolution 10 of the 22nd General Conference
        CGPM in 2003), is used as a separator between the time element and
        its fraction."

******************************************************************************/

size_t iso8601(T) (T[] src, ref Time value)
{
        TimeOfDay tod;
        Date      date;

        int r = iso8601!(T)(src, tod, date);
        if (r)   
            value = Gregorian.generic.toTime(date, tod);
        return r;
}

/******************************************************************************

        ISO-8601 format :: "2006-01-31 14:49:30,001"

        Returns the number of elements consumed by the parse; zero if
        the parse failed

        Quote from http://en.wikipedia.org/wiki/ISO_8601 (2009-09-01):
        "Decimal fractions may also be added to any of the three time elements.
        A decimal point, either a comma or a dot (without any preference as
        stated most recently in resolution 10 of the 22nd General Conference
        CGPM in 2003), is used as a separator between the time element and
        its fraction."

******************************************************************************/

size_t iso8601(T) (T[] src, ref TimeOfDay tod, ref Date date)
{
        T* p = src.ptr;
        T* e = p + src.length;

        bool dt (ref T* p)
        {
                return ((date.year = parseInt(p, e)) > 0   &&
                         *p++ == '-'                       &&
                        ((date.month = parseInt(p, e)) > 0 &&
                        (*p++ == '-'                       &&
                        (date.day = parseInt(p, e)) > 0)));
        }

        if (dt(p) >= 0       &&
            *p++ == ' '      &&
            time (tod, p, e))
            {
            // Are there chars left? If yes, parse millis. If no, millis = 0.
            if (p - src.ptr) {
                // check fraction separator
                T frac_sep = *p++;
                if (frac_sep is ',' || frac_sep is '.')
                    // separator is ok: parse millis
                    tod.millis = parseInt (p, e);
                else
                    // wrong separator: error 
                    return 0;
            } else
                tod.millis = 0;
            
            return p - src.ptr;
            }
        return 0;
}


/******************************************************************************

        Parse a time field

******************************************************************************/

private bool time(T) (ref TimeOfDay time, ref T* p, T* e)
{
        return ((time.hours = parseInt(p, e)) >= 0   &&
                 *p++ == ':'                         &&
                (time.minutes = parseInt(p, e)) >= 0 &&
                 *p++ == ':'                         &&
                (time.seconds = parseInt(p, e)) >= 0);
}


/******************************************************************************

        Match a month from the input

******************************************************************************/

private int parseMonth(T) (ref T* p)
{
        int month;

        switch (p[0..3])
               {
               case "Jan":
                    month = 1;
                    break; 
               case "Feb":
                    month = 2;
                    break; 
               case "Mar":
                    month = 3;
                    break; 
               case "Apr":
                    month = 4;
                    break; 
               case "May":
                    month = 5;
                    break; 
               case "Jun":
                    month = 6;
                    break; 
               case "Jul":
                    month = 7;
                    break; 
               case "Aug":
                    month = 8;
                    break; 
               case "Sep":
                    month = 9;
                    break; 
               case "Oct":
                    month = 10;
                    break; 
               case "Nov":
                    month = 11;
                    break; 
               case "Dec":
                    month = 12;
                    break; 
               default:
                    return month;
               }
        p += 3;
        return month;
}


/******************************************************************************

        Match a day from the input

******************************************************************************/

private int parseShortDay(T) (ref T* p)
{
        int day;

        switch (p[0..3])
               {
               case "Sun":
                    day = 0;
                    break;
               case "Mon":
                    day = 1;
                    break; 
               case "Tue":
                    day = 2;
                    break; 
               case "Wed":
                    day = 3;
                    break; 
               case "Thu":
                    day = 4;
                    break; 
               case "Fri":
                    day = 5;
                    break; 
               case "Sat":
                    day = 6;
                    break; 
               default:
                    return -1;
               }
        p += 3;
        return day;
}


/******************************************************************************

        Match a day from the input. Sunday is 0

******************************************************************************/

private int parseFullDay(T) (ref T* p)
{
        static  T[][] days =
                [
                "Sunday", 
                "Monday", 
                "Tuesday", 
                "Wednesday", 
                "Thursday", 
                "Friday", 
                "Saturday", 
                ];

        foreach (i, day; days)
                 if (day == p[0..day.length])
                    {
                    p += day.length;
                    return i;
                    }
        return -1;
}


/******************************************************************************

        Extract an integer from the input

******************************************************************************/

private static int parseInt(T) (ref T* p, T* e)
{
        int value;

        while (p < e && (*p >= '0' && *p <= '9'))
               value = value * 10 + *p++ - '0';
        return value;
}


/******************************************************************************

******************************************************************************/

debug (UnitTest)
{
        unittest
        {
        wchar[30] tmp;
        wchar[] test = "Sun, 06 Nov 1994 08:49:37 GMT";
                
        auto time = parse (test);
        auto text = format (tmp, time);
        assert (text == test);

        char[] garbageTest = "Wed Jun 11 17:22:07 20088";
        garbageTest = garbageTest[0..$-1];
        char[128] tmp2;

        time = parse(garbageTest);
        auto text2 = format(tmp2, time);
        assert (text2 == "Wed, 11 Jun 2008 17:22:07 GMT");
        }
}

/******************************************************************************

******************************************************************************/

debug (TimeStamp)
{
        void main()
        {
                Time t;

                auto dos = "12-31-06 08:49AM";
                auto iso = "2006-01-31 14:49:30,001";
                assert (dostime(dos, t) == dos.length);
                assert (iso8601(iso, t) == iso.length);

                wchar[30] tmp;
                wchar[] test = "Sun, 06 Nov 1994 08:49:37 GMT";
                
                auto time = parse (test);
                auto text = format (tmp, time);
                assert (text == test);              
        }
}
