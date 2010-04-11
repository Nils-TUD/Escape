/*******************************************************************************

        copyright:      Copyright (c) 2009 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        rewritten: Nov 2009

        Various low-level console oriented utilities

*******************************************************************************/

module rt.compiler.util.console;

private import rt.compiler.util.string;

/*******************************************************************************
        
        External functions

*******************************************************************************/

version (Windows) 
        {
        enum {UTF8 = 65001};
        private extern (Windows) int GetStdHandle (int);
        private extern (Windows) int WriteFile (int, char*, int, int*, void*);
        private extern (Windows) bool GetConsoleMode (int, int*);
        private extern (Windows) bool WriteConsoleW (int, wchar*, int, int*, void*);
        private extern (Windows) int MultiByteToWideChar (int, int, char*, int, wchar*, int);
        } 
else 
version (Posix)
         extern(C) ptrdiff_t write (int, in void*, size_t);
else version (Escape)
         extern(C) ptrdiff_t write (int, in void*, size_t);


/*******************************************************************************
        
        Emit an integer to the console

*******************************************************************************/

extern(C) void consoleInteger (ulong i)
{
        char[25] tmp = void;

        consoleString (ulongToUtf8 (tmp, i));
}

/*******************************************************************************

        Emit a utf8 string to the console. Codepages are not supported

*******************************************************************************/

extern(C) void consoleString (char[] s)
{
        version (Windows)
                {
                int  mode, count;
                auto handle = GetStdHandle (0xfffffff5);

                if (handle != -1 && GetConsoleMode (handle, &mode))
                   {
                   wchar[512] utf;
                   while (s.length)
                         {
                         // crop to last complete utf8 sequence
                         auto t = crop (s[0 .. (s.length > utf.length) ? utf.length : s.length]);

                         // convert into output buffer and emit
                         auto i = MultiByteToWideChar (UTF8, 0, s.ptr, t.length, utf.ptr, utf.length);
                         WriteConsoleW (handle, utf.ptr, i, &count, null);

                         // process next chunk
                         s = s [t.length .. $];
                         }
                   }
                else
                   // output is probably redirected (we assume utf8 output)
                   WriteFile (handle, s.ptr, s.length, &count, null);
                }

        version (Posix)
                 write (2, s.ptr, s.length);
        version (Escape)
                 write (2, s.ptr, s.length);
}

/*******************************************************************************

        Support for chained console (pseudo formatting) output

*******************************************************************************/

struct Console
{
        alias newline opCall;
        alias emit    opCall;
    
        /// emit a utf8 string to the console
        Console emit (char[] s)
        {
                consoleString (s);
                return *this;
        }

        /// emit an unsigned integer to the console
        Console emit (ulong i)
        {
                consoleInteger (i);
                return *this;
        }

        /// emit a newline to the console
        Console newline ()
        {
                version (Windows)
                         const eol = "\r\n";
                version (Posix)
                         const eol = "\n";
                version (Escape)
                         const eol = "\n";

                return emit (eol);
        }
}

public Console console;


version (Windows)
{
/*******************************************************************************

        Adjust the content such that no partial encodings exist on the 
        right side of the provided text.

        Returns a slice of the input

*******************************************************************************/

private char[] crop (char[] s)
{
        if (s.length)
           {
           auto i = s.length - 1;
           while (i && (s[i] & 0x80))
                  if ((s[i] & 0xc0) is 0xc0)
                     {
                     // located the first byte of a sequence
                     ubyte b = s[i];
                     int d = s.length - i;

                     // is it a 3 byte sequence?
                     if (b & 0x20)
                         --d;
   
                     // or a four byte sequence?
                     if (b & 0x10)
                         --d;

                     // is the sequence complete?
                     if (d is 2)
                         i = s.length;
                     return s [0..i];
                     }
                  else 
                     --i;
           }
        return s;
}
}


/*******************************************************************************
        
*******************************************************************************/

debug (console)
{
        void main()
        {
                console ("hello world \u263a")();
        }
}
