/*
 * Placed into the Public Domain.
 * written by Walter Bright
 * www.digitalmars.com
 */

/*
 *  Modified by Sean Kelly <sean@f4.ca> for use with Tango.
 */

private
{
    import util.console;

    import tango.stdc.stddef;
    import tango.stdc.stdlib;
    import tango.stdc.string;
}

version( Win32 )
{
    extern (Windows) void*      LocalFree(void*);
    extern (Windows) wchar_t*   GetCommandLineW();
    extern (Windows) wchar_t**  CommandLineToArgvW(wchar_t*, int*);
    extern (Windows) export int WideCharToMultiByte(uint, uint, wchar_t*, int, char*, int, char*, int);
    pragma(lib, "shell32.lib");   // needed for CommandLineToArgvW
    pragma(lib, "usergdi32.lib"); // links Tango's Win32 library to reduce EXE size
}

extern (C) void _STI_monitor_staticctor();
extern (C) void _STD_monitor_staticdtor();
extern (C) void _STI_critical_init();
extern (C) void _STD_critical_term();
extern (C) void gc_init();
extern (C) void gc_term();
extern (C) void _minit();
extern (C) void _moduleCtor();
extern (C) void _moduleDtor();
extern (C) void _moduleUnitTests();

/***********************************
 * These functions must be defined for any D program linked
 * against this library.
 */
extern (C) void onAssertError( char[] file, size_t line );
extern (C) void onAssertErrorMsg( char[] file, size_t line, char[] msg );
extern (C) void onArrayBoundsError( char[] file, size_t line );
extern (C) void onSwitchError( char[] file, size_t line );

// this function is called from the utf module
//extern (C) void onUnicodeError( char[] msg, size_t idx );

/***********************************
 * These are internal callbacks for various language errors.
 */
extern (C) void _d_assert( char[] file, uint line )
{
    onAssertError( file, line );
}

extern (C) static void _d_assert_msg( char[] msg, char[] file, uint line )
{
    onAssertErrorMsg( file, line, msg );
}

extern (C) void _d_array_bounds( char[] file, uint line )
{
    onArrayBoundsError( file, line );
}

extern (C) void _d_switch_error( char[] file, uint line )
{
    onSwitchError( file, line );
}

bool isHalting = false;

extern (C) bool cr_isHalting()
{
    return isHalting;
}

/***********************************
 * The D main() function supplied by the user's program
 */
int main(char[][] args);

/***********************************
 * Substitutes for the C main() function.
 * It's purpose is to wrap the call to the D main()
 * function and catch any unhandled exceptions.
 */
extern (C) int main(int argc, char **argv)
{
    char[][] args;
    int result;

    version (Win32)
    {
        gc_init();
        _minit();
    }
    else version (linux)
    {
        _STI_monitor_staticctor();
        _STI_critical_init();
        gc_init();
    }
    else version (Escape)
    {
        _STI_monitor_staticctor();
        _STI_critical_init();
    	gc_init();
    }
    else
    {
        static assert( false );
    }

    version (Win32)
    {
        wchar_t*  wcbuf = GetCommandLineW();
        size_t    wclen = wcslen(wcbuf);
        int       wargc = 0;
        wchar_t** wargs = CommandLineToArgvW(GetCommandLineW(), &wargc);
        assert(wargc == argc);

        char*     cargp = null;
        size_t    cargl = WideCharToMultiByte(65001, 0, wcbuf, wclen, null, 0, null, 0);

        cargp = cast(char*) alloca(cargl);
        args  = ((cast(char[]*) alloca(wargc * (char[]).sizeof)))[0 .. wargc];

        for (size_t i = 0, p = 0; i < wargc; i++)
        {
            int wlen = wcslen( wargs[i] );
            int clen = WideCharToMultiByte(65001, 0, &wargs[i][0], wlen, null, 0, null, 0);
            args[i]  = cargp[p .. p+clen];
            p += clen; assert(p <= cargl);
            WideCharToMultiByte(65001, 0, &wargs[i][0], wlen, &args[i][0], clen, null, 0);
        }
        LocalFree(wargs);
        wargs = null;
        wargc = 0;
    }
    else version (linux)
    {
        char[]* am = cast(char[]*) malloc(argc * (char[]).sizeof);
        scope(exit) free(am);

        for (int i = 0; i < argc; i++)
        {
            int len = strlen(argv[i]);
            am[i] = argv[i][0 .. len];
        }
        args = am[0 .. argc];
    }
    else version (Escape)
    {
        char[]* am = cast(char[]*) malloc(argc * (char[]).sizeof);
        scope(exit) free(am);

        for (int i = 0; i < argc; i++)
        {
            int len = strlen(argv[i]);
            am[i] = argv[i][0 .. len];
        }
        args = am[0 .. argc];
    }

    try
    {
        _moduleCtor();
        _moduleUnitTests();
        result = main(args);
        isHalting = true;
        _moduleDtor();
        gc_term();
    }
    catch (Exception e)
    {
        while (e)
        {
            if (e.file)
            {
               // fprintf(stderr, "%.*s(%u): %.*s\n", e.file, e.line, e.msg);
               console (e.file)("(")(e.line)("): ")(e.msg)("\n");
            }
            else
            {
               // fprintf(stderr, "%.*s\n", e.toUtf8());
               console (e.toUtf8)("\n");
            }
            e = e.next;
        }
        exit(EXIT_FAILURE);
    }
    catch (Object o)
    {
        // fprintf(stderr, "%.*s\n", o.toUtf8());
        console (o.toUtf8)("\n");
        exit(EXIT_FAILURE);
    }

    version (linux)
    {
        _STD_critical_term();
        _STD_monitor_staticdtor();
    }
    else version (Escape)
    {
        _STD_critical_term();
        _STD_monitor_staticdtor();
    }
    return result;
}
