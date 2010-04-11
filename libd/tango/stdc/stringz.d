/*******************************************************************************

        copyright:      Copyright (c) 2006 Keinfarbton. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: October 2006

        author:         Keinfarbton

*******************************************************************************/

module tango.stdc.stringz;

private import tango.stdc.string;

debug private import tango.stdc.stdio;

/*********************************
 * Convert array of chars s[] to a C-style 0 terminated string.
 */

char* toUtf8z (char[] s)
{
        return s.ptr ? (s ~ '\0').ptr : null;
}

/*********************************
 * Convert a C-style 0 terminated string to an array of char
 */

char[] fromUtf8z (char* s)
{
        return s ? s[0..strlen(s)] : null;
}

unittest
{
    debug(string) printf("stdc.stringz.unittest\n");

    char* p = toUtf8z("foo");
    assert(strlen(p) == 3);
    char foo[] = "abbzxyzzy";
    p = toUtf8z(foo[3..5]);
    assert(strlen(p) == 2);

    char[] test = "";
    p = toUtf8z(test);
    assert(*p == 0);
}

