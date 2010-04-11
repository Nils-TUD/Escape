/*******************************************************************************
  copyright:   Copyright (c) 2006 Juan Jose Comellas. All rights reserved
  license:     BSD style: $(LICENSE)
  author:      Juan Jose Comellas <juanjo@comellas.com.ar>
*******************************************************************************/

module tango.sys.TimeConverter;


public import tango.core.Interval;
public import tango.stdc.time;
public import tango.sys.Common;

version (Posix)
    public import tango.stdc.posix.time;


/**
 * Converter functions from/to the different existing time units to/from an
 * Interval.
 */

version (Windows)
{
    public struct timeval
    {
        int tv_sec;     // seconds
        int tv_usec;    // microseconds
    }


    // Static constant to remove time skew between the Windows FILETIME
    // and POSIX time. POSIX and Win32 use different epochs
    // (Jan. 1, 1970 v.s. Jan. 1, 1601). The following constant defines
    // the difference in 100ns ticks.
    // private static const ulong FiletimeToUsecSkew = 0x19db1ded53e8000;
    private static const ulong FiletimeToUnixEpochSkew;

    static this()
    {
        SYSTEMTIME stime;
        FILETIME   ftime;

        // First second of 1970 (Unix epoch)
        stime.wYear = 1970;
        stime.wMonth = 1;
        stime.wDayOfWeek = 0;
        stime.wDay = 1;
        stime.wHour = 0;
        stime.wMinute = 0;
        stime.wSecond = 0;
        stime.wMilliseconds = 0;
        SystemTimeToFileTime(&stime, &ftime);

        FiletimeToUnixEpochSkew = (cast(ulong) ftime.dwHighDateTime) << 32 |
                                  (cast(ulong) ftime.dwLowDateTime);
    }
}

version (Escape)
{
    public struct timeval
    {
        int tv_sec;     // seconds
        int tv_usec;    // microseconds
    }
    
    struct timespec
    {
        time_t  tv_sec;
        long    tv_nsec;
    };
}

/**
 * Convert a timeval to an Interval.
 */
public Interval toInterval(inout timeval tv)
{
    return (cast(Interval) tv.tv_sec * Interval.second) + cast(Interval) tv.tv_usec;
}

/**
 * Cast the Interval to a C timeval struct.
 */
public timeval* toTimeval(Interval interval)
{
    return toTimeval(new timeval, interval);
}

/**
 * Cast the time duration to a C timeval struct.
 */
public timeval* toTimeval(timeval* tv, Interval interval)
in
{
    assert(tv !is null);
}
body
{
    tv.tv_sec = cast(typeof(tv.tv_sec)) (interval / Interval.second);
    tv.tv_usec = cast(typeof(tv.tv_usec)) (interval % Interval.second);

    return tv;
}

version (Posix)
{
    /**
     * Convert the timespec to an Interval.
     */
    public Interval toInterval(inout timespec ts)
    {
        return (cast(Interval) (ts.tv_sec * Interval.second) +
                cast(Interval) (ts.tv_nsec / 1000));
    }

    /**
     * Cast the time duration to a C timespec struct.
     */
    public timespec* toTimespec(Interval interval)
    {
        return toTimespec(new timespec, interval);
    }

    /**
     * Cast the time duration to a C timespec struct.
     */
    public timespec* toTimespec(timespec* ts, Interval interval)
    in
    {
        assert(ts !is null);
    }
    body
    {
        ts.tv_sec = cast(typeof(ts.tv_sec)) (interval / Interval.second);
        ts.tv_nsec = cast(typeof(ts.tv_nsec)) ((interval % Interval.second) * 1000);

        return ts;
    }
}

version (Escape)
{
    /**
     * Convert the timespec to an Interval.
     */
    public Interval toInterval(inout timespec ts)
    {
        return (cast(Interval) (ts.tv_sec * Interval.second) +
                cast(Interval) (ts.tv_nsec / 1000));
    }

    /**
     * Cast the time duration to a C timespec struct.
     */
    public timespec* toTimespec(Interval interval)
    {
        return toTimespec(new timespec, interval);
    }

    /**
     * Cast the time duration to a C timespec struct.
     */
    public timespec* toTimespec(timespec* ts, Interval interval)
    in
    {
        assert(ts !is null);
    }
    body
    {
        ts.tv_sec = cast(typeof(ts.tv_sec)) (interval / Interval.second);
        ts.tv_nsec = cast(typeof(ts.tv_nsec)) ((interval % Interval.second) * 1000);

        return ts;
    }
}

version (Windows)
{
    /**
     * Convert a Windows FILETIME struct to an Interval.
     */
    public Interval toInterval(inout FILETIME ftime)
    {
        return cast(Interval) (((cast(ulong) ftime.dwHighDateTime << 32 |
                                 cast(ulong) ftime.dwLowDateTime) -
                                FiletimeToUnixEpochSkew) / 10);
    }

    /**
     * Cast the time interval to a Windows FILETIME struct.
     */
    public FILETIME* toFiletime(Interval interval)
    {
        return toFiletime(new FILETIME, interval);
    }

    /**
     * Cast the time interval to a Windows FILETIME struct.
     */
    public FILETIME* toFiletime(FILETIME* ftime, Interval interval)
    {
        // Convert interval to ticks and add the offset between the
        // Unix epoch and the Windows FILETIME base date (Jan. 1, 1970
        // vs. Jan. 1, 1601).
        ulong value = FiletimeToUnixEpochSkew + (interval * 10);

        ftime.dwHighDateTime = cast(DWORD) (value >> 32);
        ftime.dwLowDateTime = cast(DWORD) (value & 0xffffffff);

        return ftime;
    }
}

/**
 * Return the current time as an Interval (i.e. to the number of microseconds
 * since Jan 1, 1970 at 00:00:00).
 *
 * Remarks:
 * On platforms that do not provide the current system time with
 * microsecond precision, the value will only have a 1-second precision.
 */
public Interval currentTime()
{
    version (Posix)
    {
        timeval tv;

        gettimeofday(&tv, null);

        return (cast(Interval) tv.tv_sec * Interval.second +
                cast(Interval) tv.tv_usec);
    }
    else version (Escape)
    {
    	// TODO no milliseconds yet
        timeval tv;
        tv.tv_sec = .time(null);
        tv.tv_usec = 0;
        return (cast(Interval) tv.tv_sec * Interval.second +
                cast(Interval) tv.tv_usec);
    }
    else version (Windows)
    {
        FILETIME ft;

        GetSystemTimeAsFileTime(&ft);

        return cast(Interval) ((((cast(Interval) ft.dwHighDateTime) << 32 |
                               (cast(Interval) ft.dwLowDateTime)) - FiletimeToUnixEpochSkew) / 10);
    }
    else
    {
        // Low precision compatibility method
        return (cast(Interval) time(null) * Interval.second);
    }
}

/**
 * Converts the interval from the a relative time to an absolute time
 * (i.e. to the number of microseconds since Jan 1, 1970 at 00:00:00
 * plus the given interval).
 *
 * Remarks:
 * On platforms that do not provide the current system time with
 * microsecond precision, the value will only have a 1-second precision.
 */
public Interval toAbsoluteTime(Interval interval)
{
    return currentTime() + interval;
}
