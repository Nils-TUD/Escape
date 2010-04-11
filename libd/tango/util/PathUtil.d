/*******************************************************************************

        copyright:      Copyright (c) 2006-2009 Max Samukha, Thomas Kühne,
                                            Grzegorz Adam Hankiewicz

        license:        BSD style: $(LICENSE)

        version:        Dec 2006: Initial release
                        Jan 2009: Replaced normalize

        author:         Max Samukha, Thomas Kühne,
                        Grzegorz Adam Hankiewicz

*******************************************************************************/

module tango.util.PathUtil;

private import  tango.core.Exception;

private extern (C) void memmove (void* dst, void* src, uint bytes);

version (Quiet){} else
         pragma (msg, "revision: util.PathUtil has been merged into io.Path - please update your import statements");


/*******************************************************************************

    Normalizes a path component.

    . segments are removed
    <segment>/.. are removed

    On Windows, \ will be converted to / prior to normalization.

    Multiple consecutive forward slashes are replaced with a single forward slash.

    Note that any number of .. segments at the front is ignored,
    unless it is an absolute path, in which case they are removed.

    The input path is copied into either the provided buffer, or a heap
    allocated array if no buffer was provided. Normalization modifies
    this copy before returning the relevant slice.

    Examples:
    -----
     normalize("/home/foo/./bar/../../john/doe"); // => "/home/john/doe"
    -----

*******************************************************************************/

private enum {NodeStackLength = 64}

deprecated char[] normalize(char[] path, char[] buf = null)
{
    // Whether the path is absolute
    bool isAbsolute;
    // Current position
    size_t idx;
    // Position to move
    size_t moveTo;

    // Starting positions of regular path segments are pushed on this stack
    // to avoid backward scanning when .. segments are encountered.
    size_t[NodeStackLength] nodeStack;
    size_t nodeStackTop;

    // Moves the path tail starting at the current position to moveTo.
    // Then sets the current position to moveTo.
    void move()
    {
        auto len = path.length - idx;
        memmove(path.ptr + moveTo, path.ptr + idx, len);
        path = path[0..moveTo + len];
        idx = moveTo;
    }

    // Checks if the character at the current position is a separator.
    // If true, normalizes the separator to '/' on Windows and advances the 
    // current position to the next character.
    bool isSep(ref size_t i)
    {
        char c = path[i];
        version (Windows)
        {
            if (c == '\\')
                path[i] = '/';
            else if (c != '/')
                return false;
        }
        else
        {
            if (c != '/')
                return false;
        }
        i++;
        return true;
    }

    if (buf is null)
        path = path.dup;
    else
        path = buf[0..path.length] = path;

    version (Windows)
    {
        // Skip Windows drive specifiers
        if (path.length >= 2 && path[1] == ':')
        {
            auto c = path[0];

            if (c >= 'a' && c <= 'z')
            {
                path[0] = c - 32;
                idx = 2;
            }
            else if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z')
                idx = 2;
        }
    }

    if (idx == path.length)
        return path;

    moveTo = idx;
    if (isSep(idx))
    {
        moveTo++; // preserve root separator.
        isAbsolute = true;
    }

    while (idx < path.length)
    {
        // Skip duplicate separators
        if (isSep(idx))
            continue;

        if (path[idx] == '.')
        {
            // leave the current position at the start of the segment
            auto i = idx + 1;
            if (i < path.length && path[i] == '.')
            {
                i++;
                if (i == path.length || isSep(i))
                {
                    // It is a '..' segment. If the stack is not empty, set 
                    // moveTo and the current position
                    // to the start position of the last found regular segment.
                    if (nodeStackTop > 0)
                        moveTo = nodeStack[--nodeStackTop];
                    // If no regular segment start positions on the stack, drop the 
                    // .. segment if it is absolute path
                    // or, otherwise, advance moveTo and the current position to 
                    // the character after the '..' segment
                    else if (!isAbsolute)
                    {
                        if (moveTo != idx)
                        {
                            i -= idx - moveTo;
                            move();
                        }
                        moveTo = i;
                    }

                    idx = i;
                    continue;
                }
            }

            // If it is '.' segment, skip it.
            if (i == path.length || isSep(i))
            {
                idx = i;
                continue;
            }
        }

        // Remove excessive '/', '.' and/or '..' preceeding the segment.
        if (moveTo != idx)
            move();

        // Push the start position of the regular segment on the stack
        assert(nodeStackTop < NodeStackLength);
        nodeStack[nodeStackTop++] = idx;

        // Skip the regular segment and set moveTo to the position after the segment
        // (including the trailing '/' if present)
        for(; idx < path.length && !isSep(idx); idx++) {}
        moveTo = idx;
    }

    if (moveTo != idx)
        move();

    return path;
}

debug (UnitTest)
{

    unittest
    {
        assert (normalize ("") == "");
        assert (normalize ("/home/../john/../.tango/.htaccess") == "/.tango/.htaccess");
        assert (normalize ("/home/../john/../.tango/foo.conf") == "/.tango/foo.conf");
        assert (normalize ("/home/john/.tango/foo.conf") == "/home/john/.tango/foo.conf");
        assert (normalize ("/foo/bar/.htaccess") == "/foo/bar/.htaccess");
        assert (normalize ("foo/bar/././.") == "foo/bar/");
        assert (normalize ("././foo/././././bar") == "foo/bar");
        assert (normalize ("/foo/../john") == "/john");
        assert (normalize ("foo/../john") == "john");
        assert (normalize ("foo/bar/..") == "foo/");
        assert (normalize ("foo/bar/../john") == "foo/john");
        assert (normalize ("foo/bar/doe/../../john") == "foo/john");
        assert (normalize ("foo/bar/doe/../../john/../bar") == "foo/bar");
        assert (normalize ("./foo/bar/doe") == "foo/bar/doe");
        assert (normalize ("./foo/bar/doe/../../john/../bar") == "foo/bar");
        assert (normalize ("./foo/bar/../../john/../bar") == "bar");
        assert (normalize ("foo/bar/./doe/../../john") == "foo/john");
        assert (normalize ("../../foo/bar") == "../../foo/bar");
        assert (normalize ("../../../foo/bar") == "../../../foo/bar");
        assert (normalize ("d/") == "d/");
        assert (normalize ("/home/john/./foo/bar.txt") == "/home/john/foo/bar.txt");
        assert (normalize ("/home//john") == "/home/john");

        assert (normalize("/../../bar/") == "/bar/");
        assert (normalize("/../../bar/../baz/./") == "/baz/");
        assert (normalize("/../../bar/boo/../baz/.bar/.") == "/bar/baz/.bar/");
        assert (normalize("../..///.///bar/..//..//baz/.//boo/..") == "../../../baz/");
        assert (normalize("./bar/./..boo/./..bar././/") == "bar/..boo/..bar./");
        assert (normalize("/bar/..") == "/");
        assert (normalize("bar/") == "bar/");
        assert (normalize(".../") == ".../");
        assert (normalize("///../foo") == "/foo");
        assert (normalize("./foo") == "foo");
        auto buf = new char[100];
        auto ret = normalize("foo/bar/./baz", buf);
        assert (ret.ptr == buf.ptr);
        assert (ret == "foo/bar/baz");

version (Windows) {
        assert (normalize ("\\foo\\..\\john") == "/john");
        assert (normalize ("foo\\..\\john") == "john");
        assert (normalize ("foo\\bar\\..") == "foo/");
        assert (normalize ("foo\\bar\\..\\john") == "foo/john");
        assert (normalize ("foo\\bar\\doe\\..\\..\\john") == "foo/john");
        assert (normalize ("foo\\bar\\doe\\..\\..\\john\\..\\bar") == "foo/bar");
        assert (normalize (".\\foo\\bar\\doe") == "foo/bar/doe");
        assert (normalize (".\\foo\\bar\\doe\\..\\..\\john\\..\\bar") == "foo/bar");
        assert (normalize (".\\foo\\bar\\..\\..\\john\\..\\bar") == "bar");
        assert (normalize ("foo\\bar\\.\\doe\\..\\..\\john") == "foo/john");
        assert (normalize ("..\\..\\foo\\bar") == "../../foo/bar");
        assert (normalize ("..\\..\\..\\foo\\bar") == "../../../foo/bar");
        assert (normalize(r"C:") == "C:");
        assert (normalize(r"C") == "C");
        assert (normalize(r"c:\") == "C:/");
        assert (normalize(r"C:\..\.\..\..\") == "C:/");
        assert (normalize(r"c:..\.\boo\") == "C:../boo/");
        assert (normalize(r"C:..\..\boo\foo\..\.\..\..\bar") == "C:../../../bar");
        assert (normalize(r"C:boo\..") == "C:");
}
    }
}

/******************************************************************************

    Matches a pattern against a filename.

    Some characters of pattern have special a meaning (they are
    <i>meta-characters</i>) and <b>can't</b> be escaped. These are:
    <p><table>
    <tr><td><b>*</b></td>
        <td>Matches 0 or more instances of any character.</td></tr>
    <tr><td><b>?</b></td>
        <td>Matches exactly one instances of any character.</td></tr>
    <tr><td><b>[</b><i>chars</i><b>]</b></td>
        <td>Matches one instance of any character that appears
        between the brackets.</td></tr>
    <tr><td><b>[!</b><i>chars</i><b>]</b></td>
        <td>Matches one instance of any character that does not appear
        between the brackets after the exclamation mark.</td></tr>
    </table><p>
    Internally individual character comparisons are done calling
    charMatch(), so its rules apply here too. Note that path
    separators and dots don't stop a meta-character from matching
    further portions of the filename.

    Returns: true if pattern matches filename, false otherwise.

    See_Also: charMatch().

    Throws: Nothing.

    Examples:
    -----
    version(Win32)
    {
        patternMatch("foo.bar", "*") // => true
        patternMatch(r"foo/foo\bar", "f*b*r") // => true
        patternMatch("foo.bar", "f?bar") // => false
        patternMatch("Goo.bar", "[fg]???bar") // => true
        patternMatch(r"d:\foo\bar", "d*foo?bar") // => true
    }
    version(Posix)
    {
        patternMatch("Go*.bar", "[fg]???bar") // => false
        patternMatch("/foo*home/bar", "?foo*bar") // => true
        patternMatch("foobar", "foo?bar") // => true
    }
    -----
    
******************************************************************************/

deprecated bool patternMatch(char[] filename, char[] pattern)
in
{
    // Verify that pattern[] is valid
    int i;
    int inbracket = false;

    for (i = 0; i < pattern.length; i++)
    {
        switch (pattern[i])
        {
        case '[':
            assert(!inbracket);
            inbracket = true;
            break;

        case ']':
            assert(inbracket);
            inbracket = false;
            break;

        default:
            break;
        }
    }
}
body
{
    int pi;
    int ni;
    char pc;
    char nc;
    int j;
    int not;
    int anymatch;

    ni = 0;
    for (pi = 0; pi < pattern.length; pi++)
    {
        pc = pattern[pi];
        switch (pc)
        {
        case '*':
            if (pi + 1 == pattern.length)
                goto match;
            for (j = ni; j < filename.length; j++)
            {
                if (patternMatch(filename[j .. filename.length],
                            pattern[pi + 1 .. pattern.length]))
                    goto match;
            }
            goto nomatch;

        case '?':
            if (ni == filename.length)
            goto nomatch;
            ni++;
            break;

        case '[':
            if (ni == filename.length)
                goto nomatch;
            nc = filename[ni];
            ni++;
            not = 0;
            pi++;
            if (pattern[pi] == '!')
            {
                not = 1;
                pi++;
            }
            anymatch = 0;
            while (1)
            {
                pc = pattern[pi];
                if (pc == ']')
                    break;
                if (!anymatch && charMatch(nc, pc))
                    anymatch = 1;
                pi++;
            }
            if (!(anymatch ^ not))
                goto nomatch;
            break;

        default:
            if (ni == filename.length)
                goto nomatch;
            nc = filename[ni];
            if (!charMatch(pc, nc))
                goto nomatch;
            ni++;
            break;
        }
    }
    if (ni < filename.length)
        goto nomatch;

    match:
    return true;

    nomatch:
    return false;
}


debug (UnitTest)
{
    unittest
    {
    version (Win32)
        assert(patternMatch("foo", "Foo"));
    version (Posix)
        assert(!patternMatch("foo", "Foo"));
    
    assert(patternMatch("foo", "*"));
    assert(patternMatch("foo.bar", "*"));
    assert(patternMatch("foo.bar", "*.*"));
    assert(patternMatch("foo.bar", "foo*"));
    assert(patternMatch("foo.bar", "f*bar"));
    assert(patternMatch("foo.bar", "f*b*r"));
    assert(patternMatch("foo.bar", "f???bar"));
    assert(patternMatch("foo.bar", "[fg]???bar"));
    assert(patternMatch("foo.bar", "[!gh]*bar"));

    assert(!patternMatch("foo", "bar"));
    assert(!patternMatch("foo", "*.*"));
    assert(!patternMatch("foo.bar", "f*baz"));
    assert(!patternMatch("foo.bar", "f*b*x"));
    assert(!patternMatch("foo.bar", "[gh]???bar"));
    assert(!patternMatch("foo.bar", "[!fg]*bar"));
    assert(!patternMatch("foo.bar", "[fg]???baz"));

    }
}


/******************************************************************************

     Matches filename characters.

     Under Windows, the comparison is done ignoring case. Under Linux
     an exact match is performed.

     Returns: true if c1 matches c2, false otherwise.

     Throws: Nothing.

     Examples:
     -----
     version(Win32)
     {
         charMatch('a', 'b') // => false
         charMatch('A', 'a') // => true
     }
     version(Posix)
     {
         charMatch('a', 'b') // => false
         charMatch('A', 'a') // => false
     }
     -----
******************************************************************************/

private bool charMatch(char c1, char c2)
{
    version (Win32)
    {
        
        if (c1 != c2)
        {
            return ((c1 >= 'a' && c1 <= 'z') ? c1 - ('a' - 'A') : c1) ==
                   ((c2 >= 'a' && c2 <= 'z') ? c2 - ('a' - 'A') : c2);
        }
        return true;
    }
    version (Posix)
    {
        return c1 == c2;
    }
    version (Escape)
    {
        return c1 == c2;
    }
}

