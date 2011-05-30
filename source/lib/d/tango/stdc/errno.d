/**
 * D header file for C99.
 *
 * Copyright: Public Domain
 * License:   Public Domain
 * Authors:   Sean Kelly
 * Standards: ISO/IEC 9899:1999 (E)
 */
module tango.stdc.errno;

// TODO
//public import tango.sys.consts.errno;

private
{
    extern (C) int getErrno();
    extern (C) int setErrno(int);
}

int errno()          { return getErrno();      }
int errno( int val ) { return setErrno( val ); }

