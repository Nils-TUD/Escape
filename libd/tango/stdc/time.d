/**
 * D header file for C99.
 *
 * Copyright: Public Domain
 * License:   Public Domain
 * Authors:   Sean Kelly
 * Standards: ISO/IEC 9899:1999 (E)
 */
module tango.stdc.time;

private import tango.stdc.config;
private import tango.stdc.stddef;

extern (C):

version( Win32 )
{
    struct tm
    {
        int     tm_sec;     // seconds after the minute - [0, 60]
        int     tm_min;     // minutes after the hour - [0, 59]
        int     tm_hour;    // hours since midnight - [0, 23]
        int     tm_mday;    // day of the month - [1, 31]
        int     tm_mon;     // months since January - [0, 11]
        int     tm_year;    // years since 1900
        int     tm_wday;    // days since Sunday - [0, 6]
        int     tm_yday;    // days since January 1 - [0, 365]
        int     tm_isdst;   // Daylight Saving Time flag
    }
}
else version(Escape)
{
	struct tm
	{
		int tm_sec;
		int tm_min;
		int tm_hour;
		int tm_mday;
		int tm_mon;
		int tm_year;
		int tm_wday;
		int tm_yday;
		int tm_isdst;
	}
	
    struct timeval
    {
    	time_t	tv_sec;
    	time_t	tv_usec;
    }
}
else
{
    struct tm
    {
        int     tm_sec;     // seconds after the minute [0-60]
        int     tm_min;     // minutes after the hour [0-59]
        int     tm_hour;    // hours since midnight [0-23]
        int     tm_mday;    // day of the month [1-31]
        int     tm_mon;     // months since January [0-11]
        int     tm_year;    // years since 1900
        int     tm_wday;    // days since Sunday [0-6]
        int     tm_yday;    // days since January 1 [0-365]
        int     tm_isdst;   // Daylight Savings Time flag
        c_long  tm_gmtoff;  // offset from CUT in seconds
        char*   tm_zone;    // timezone abbreviation
    }
}

alias c_long time_t;
alias c_long clock_t;

version( Win32 )
{
    enum:clock_t {CLOCKS_PER_SEC = 1000}
}
else version( darwin )
{
    enum:clock_t {CLOCKS_PER_SEC = 100}
}
else version( freebsd )
{
    enum:clock_t {CLOCKS_PER_SEC = 128}
}
else version( solaris )
{
    enum:clock_t {CLOCKS_PER_SEC = 1000000}
}
else
{
    enum:clock_t {CLOCKS_PER_SEC = 1000000}
}

// TODO clock_t clock();
double  difftime(time_t time1, time_t time0);
time_t  mktime(tm* timeptr);
time_t  time(time_t* timer);
char*   asctime(in tm* timeptr);
char*   ctime(in time_t* timer);
tm*     gmtime(in time_t* timer);
tm*     localtime(in time_t* timer);
size_t  strftime(char* s, size_t maxsize, in char* format, in tm* timeptr);
// TODO size_t  wcsftime(wchar_t* s, size_t maxsize, in wchar_t* format, in tm* timeptr);

version( Win32 )
{
    void  tzset();
    void  _tzset();
    char* _strdate(char* s);
    char* _strtime(char* s);

    wchar_t* _wasctime(tm*);
    wchar_t* _wctime(time_t*);
    wchar_t* _wstrdate(wchar_t*);
    wchar_t* _wstrtime(wchar_t*);
}
else version( darwin )
{
    void tzset();
}
else version( linux )
{
    void tzset();
}
else version( freebsd )
{
    void tzset();
}
else version( solaris )
{
    void tzset();
}
