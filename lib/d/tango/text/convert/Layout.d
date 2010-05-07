/*******************************************************************************

        copyright:      Copyright (c) 2005 Kris. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: 2005

        author:         Kris, Keinfarbton

        This module provides a general-purpose formatting system to
        convert values to text suitable for display. There is support
        for alignment, justification, and common format specifiers for
        numbers.

        Layout can be customized via configuring various handlers and
        associated meta-data. This is utilized to plug in text.locale
        for handling custom formats, date/time and culture-specific
        conversions.

        The format notation is influenced by that used by the .NET
        and ICU frameworks, rather than C-style printf or D-style
        writef notation.

******************************************************************************/

module tango.text.convert.Layout;

private import  tango.core.Exception;

private import  Utf = tango.text.convert.Utf;

private import  Float = tango.text.convert.Float,
                Integer = tango.text.convert.Integer;

private import  tango.io.model.IConduit : OutputStream;

version(WithVariant)
        private import tango.core.Variant;

version(WithExtensions)
        private import tango.text.convert.Extensions;
else
   {
   private import tango.time.Time;
   private import tango.text.convert.DateTime;
   }


/*******************************************************************************

        Platform issues ...

*******************************************************************************/

version (GNU)
        {
        private import tango.core.Vararg;
        alias void* Arg;
        alias va_list ArgList;
        }
else version(LDC)
        {
        private import tango.core.Vararg;
        alias void* Arg;
        alias va_list ArgList;
        }
     else
        {
        alias void* Arg;
        alias void* ArgList;
        }

/*******************************************************************************

        Contains methods for replacing format items in a string with string
        equivalents of each argument.

*******************************************************************************/

class Layout(T)
{
        public alias convert opCall;
        public alias uint delegate (T[]) Sink;
       
        version (WithExtensions) {} else
                 private DateTimeLocale* dateTime = &DateTimeDefault;

        /**********************************************************************

                Return shared instance
                
                Note that this is not threadsafe, and that static-ctor
                usage doesn't get invoked appropriately (compiler bug)

        **********************************************************************/

        static Layout instance ()
        {
                static Layout common;

                if (common is null)
                    common = new Layout!(T);
                return common;
        }

        /**********************************************************************

        **********************************************************************/

        public final T[] sprint (T[] result, T[] formatStr, ...)
        {
                return vprint (result, formatStr, _arguments, _argptr);
        }

        /**********************************************************************

        **********************************************************************/

        public final T[] vprint (T[] result, T[] formatStr, TypeInfo[] arguments, ArgList args)
        {
                T*  p = result.ptr;
                int available = result.length;

                uint sink (T[] s)
                {
                        int len = s.length;
                        if (len > available)
                            len = available;

                        available -= len;
                        p [0..len] = s[0..len];
                        p += len;
                        return len;
                }

                convert (&sink, arguments, args, formatStr);
                return result [0 .. p-result.ptr];
        }

        /**********************************************************************

                Replaces the _format item in a string with the string
                equivalent of each argument.

                Params:
                  formatStr  = A string containing _format items.
                  args       = A list of arguments.

                Returns: A copy of formatStr in which the items have been
                replaced by the string equivalent of the arguments.

                Remarks: The formatStr parameter is embedded with _format
                items of the form: $(BR)$(BR)
                  {index[,alignment][:_format-string]}$(BR)$(BR)
                  $(UL $(LI index $(BR)
                    An integer indicating the element in a list to _format.)
                  $(LI alignment $(BR)
                    An optional integer indicating the minimum width. The
                    result is padded with spaces if the length of the value
                    is less than alignment.)
                  $(LI _format-string $(BR)
                    An optional string of formatting codes.)
                )$(BR)

                The leading and trailing braces are required. To include a
                literal brace character, use two leading or trailing brace
                characters.$(BR)$(BR)
                If formatStr is "{0} bottles of beer on the wall" and the
                argument is an int with the value of 99, the return value
                will be:$(BR) "99 bottles of beer on the wall".

        **********************************************************************/

        public final T[] convert (T[] formatStr, ...)
        {
                return convert (_arguments, _argptr, formatStr);
        }

        /**********************************************************************

        **********************************************************************/

        public final uint convert (Sink sink, T[] formatStr, ...)
        {
                return convert (sink, _arguments, _argptr, formatStr);
        }

        /**********************************************************************

            Tentative convert using an OutputStream as sink - may still be
            removed.

            Since: 0.99.7

        **********************************************************************/

        public final uint convert (OutputStream output, T[] formatStr, ...)
        {
                uint sink (T[] s)
                {
                        return output.write(s);
                }

                return convert (&sink, _arguments, _argptr, formatStr);
        }

        /**********************************************************************

        **********************************************************************/

        public final T[] convert (TypeInfo[] arguments, ArgList args, T[] formatStr)
        {
                T[] output;

                uint sink (T[] s)
                {
                        output ~= s;
                        return s.length;
                }

                convert (&sink, arguments, args, formatStr);
                return output;
        }

version (old)
{
        /**********************************************************************

        **********************************************************************/

        public final T[] convertOne (T[] result, TypeInfo ti, Arg arg)
        {
                return dispatch (result, null, ti, arg);
        }
}
        /**********************************************************************

        **********************************************************************/

        public final uint convert (Sink sink, TypeInfo[] arguments, ArgList args, T[] formatStr)
        {
                assert (formatStr, "null format specifier");
                assert (arguments.length < 64, "too many args in Layout.convert");

                version (GNU)
                        {
                        union ArgU {int i; byte b; long l; short s; void[] a;
                                    real r; float f; double d;
                                    cfloat cf; cdouble cd; creal cr;}

                        Arg[64] arglist = void;
                        ArgU[64] storedArgs = void;
        
                        foreach (i, arg; arguments)
                                {
                                static if (is(typeof(args.ptr)))
                                    arglist[i] = args.ptr;
                                else
                                   arglist[i] = args;

                                /* Since floating point types don't live on
                                 * the stack, they must be accessed by the
                                 * correct type. */
                                bool converted = false;
                                switch (arg.classinfo.name[9])
                                       {
                                       case TypeCode.FLOAT, TypeCode.IFLOAT:
                                            storedArgs[i].f = va_arg!(float)(args);
                                            arglist[i] = &(storedArgs[i].f);
                                            converted = true;
                                            break;
                                        
                                       case TypeCode.CFLOAT:
                                            storedArgs[i].cf = va_arg!(cfloat)(args);
                                            arglist[i] = &(storedArgs[i].cf);
                                            converted = true;
                                            break;
        
                                       case TypeCode.DOUBLE, TypeCode.IDOUBLE:
                                            storedArgs[i].d = va_arg!(double)(args);
                                            arglist[i] = &(storedArgs[i].d);
                                            converted = true;
                                            break;
                                        
                                       case TypeCode.CDOUBLE:
                                            storedArgs[i].cd = va_arg!(cdouble)(args);
                                            arglist[i] = &(storedArgs[i].cd);
                                            converted = true;
                                            break;
        
                                       case TypeCode.REAL, TypeCode.IREAL:
                                            storedArgs[i].r = va_arg!(real)(args);
                                            arglist[i] = &(storedArgs[i].r);
                                            converted = true;
                                            break;

                                       case TypeCode.CREAL:
                                            storedArgs[i].cr = va_arg!(creal)(args);
                                            arglist[i] = &(storedArgs[i].cr);
                                            converted = true;
                                            break;
        
                                       default:
                                            break;
                                        }
                                if (! converted)
                                   {
                                   switch (arg.tsize)
                                          {
                                          case 1:
                                               storedArgs[i].b = va_arg!(byte)(args);
                                               arglist[i] = &(storedArgs[i].b);
                                               break;
                                          case 2:
                                               storedArgs[i].s = va_arg!(short)(args);
                                               arglist[i] = &(storedArgs[i].s);
                                               break;
                                          case 4:
                                               storedArgs[i].i = va_arg!(int)(args);
                                               arglist[i] = &(storedArgs[i].i);
                                               break;
                                          case 8:
                                               storedArgs[i].l = va_arg!(long)(args);
                                               arglist[i] = &(storedArgs[i].l);
                                               break;
                                          case 16:
                                               assert((void[]).sizeof==16,"Structure size not supported");
                                               storedArgs[i].a = va_arg!(void[])(args);
                                               arglist[i] = &(storedArgs[i].a);
                                               break;
                                          default:
                                               assert (false, "Unknown size: " ~ Integer.toString (arg.tsize));
                                          }
                                   }
                                }
                        }
                     else
                        {
                        Arg[64] arglist = void;
                        foreach (i, arg; arguments)
                                {
                                arglist[i] = args;
                                args += (arg.tsize + size_t.sizeof - 1) & ~ (size_t.sizeof - 1);
                                }
                        }
                return parse (formatStr, arguments, arglist, sink);
        }

        /**********************************************************************

                Parse the format-string, emitting formatted args and text
                fragments as we go

        **********************************************************************/

        private uint parse (T[] layout, TypeInfo[] ti, Arg[] args, Sink sink)
        {
                T[512] result = void;
                int length, nextIndex;


                T* s = layout.ptr;
                T* fragment = s;
                T* end = s + layout.length;

                while (true)
                      {
                      while (s < end && *s != '{')
                             ++s;

                      // emit fragment
                      length += sink (fragment [0 .. s - fragment]);

                      // all done?
                      if (s is end)
                          break;

                      // check for "{{" and skip if so
                      if (*++s is '{')
                         {
                         fragment = s++;
                         continue;
                         }

                      int index = 0;
                      bool indexed = false;

                      // extract index
                      while (*s >= '0' && *s <= '9')
                            {
                            index = index * 10 + *s++ -'0';
                            indexed = true;
                            }

                      // skip spaces
                      while (s < end && *s is ' ')
                             ++s;

                      bool crop;
                      bool left;
                      bool right;
                      int  width;

                      // has minimum or maximum width?
                      if (*s is ',' || *s is '.')
                         {
                         if (*s is '.')
                             crop = true;

                         while (++s < end && *s is ' ') {}
                         if (*s is '-')
                            {
                            left = true;
                            ++s;
                            }
                         else
                            right = true;

                         // get width
                         while (*s >= '0' && *s <= '9')
                                width = width * 10 + *s++ -'0';

                         // skip spaces
                         while (s < end && *s is ' ')
                                ++s;
                         }

                      T[] format;

                      // has a format string?
                      if (*s is ':' && s < end)
                         {
                         T* fs = ++s;

                         // eat everything up to closing brace
                         while (s < end && *s != '}')
                                ++s;
                         format = fs [0 .. s - fs];
                         }

                      // insist on a closing brace
                      if (*s != '}')
                         {
                         length += sink ("{malformed format}");
                         continue;
                         }

                      // check for default index & set next default counter
                      if (! indexed)
                            index = nextIndex;
                      nextIndex = index + 1;

                      // next char is start of following fragment
                      fragment = ++s;

                      // handle alignment
                      void emit (T[] str)
                      {
                                int padding = width - str.length;

                                if (crop)
                                   {
                                   if (padding < 0)
                                      {
                                      if (left)
                                         {
                                         length += sink ("...");
                                         length += sink (Utf.cropLeft (str[-padding..$]));
                                         }
                                      else
                                         {
                                         length += sink (Utf.cropRight (str[0..width]));
                                         length += sink ("...");
                                         }
                                      }
                                   else
                                       length += sink (str);
                                   }
                                else
                                   {
                                   // if right aligned, pad out with spaces
                                   if (right && padding > 0)
                                       length += spaces (sink, padding);

                                   // emit formatted argument
                                   length += sink (str);

                                   // finally, pad out on right
                                   if (left && padding > 0)
                                       length += spaces (sink, padding);
                                   }
                      }

                      // an astonishing number of typehacks needed to handle arrays :(
                      void process (TypeInfo _ti, Arg _arg)
                      {
                                // Because Variants can contain AAs (and maybe
                                // even static arrays someday), we need to
                                // process them here.
version (WithVariant)
{
                                if (_ti is typeid(Variant))
                                   {
                                   // Unpack the variant and forward
                                   auto vptr = cast(Variant*)_arg;
                                   auto innerTi = vptr.type;
                                   auto innerArg = vptr.ptr;
                                   process (innerTi, innerArg);
                                   }
}
                                if (_ti.classinfo.name.length is 20 && _ti.classinfo.name[9..$] == "StaticArray" )
                                   {
                                   auto tiStat = cast(TypeInfo_StaticArray)_ti;
                                   auto p = _arg;
                                   length += sink ("[");
                                   for (int i = 0; i < tiStat.len; i++)
                                       {
                                       if (p !is _arg )
                                           length += sink (", ");
                                       process (tiStat.value, p);
                                       p += tiStat.tsize/tiStat.len;
                                       }
                                   length += sink ("]");
                                   }
                                else 
                                if (_ti.classinfo.name.length is 25 && _ti.classinfo.name[9..$] == "AssociativeArray")
                                   {
                                   auto tiAsso = cast(TypeInfo_AssociativeArray)_ti;
                                   auto tiKey = tiAsso.key;
                                   auto tiVal = tiAsso.next();
                                   // the knowledge of the internal k/v storage is used
                                   // so this might break if, that internal storage changes
                                   alias ubyte AV; // any type for key, value might be ok, the sizes are corrected later
                                   alias ubyte AK;
                                   auto aa = *cast(AV[AK]*) _arg;

                                   length += sink ("{");
                                   bool first = true;
                                  
                                   int roundUp (int sz)
                                   {
                                        return (sz + (void*).sizeof -1) & ~((void*).sizeof - 1);
                                   }

                                   foreach (ref v; aa)
                                           {
                                           // the key is befor the value, so substrace with fixed key size from above
                                           auto pk = cast(Arg)( &v - roundUp(AK.sizeof));
                                           // now the real value pos is plus the real key size
                                           auto pv = cast(Arg)(pk + roundUp(tiKey.tsize()));

                                           if (!first)
                                                length += sink (", ");
                                           process (tiKey, pk);
                                           length += sink (" => ");
                                           process (tiVal, pv);
                                           first = false;
                                           }
                                   length += sink ("}");
                                   }
                                else 
                                if (_ti.classinfo.name[9] is TypeCode.ARRAY)
                                   {
                                   if (_ti is typeid(char[]))
                                       emit (Utf.fromString8 (*cast(char[]*) _arg, result));
                                   else
                                   if (_ti is typeid(wchar[]))        
                                       emit (Utf.fromString16 (*cast(wchar[]*) _arg, result));
                                   else
                                   if (_ti is typeid(dchar[]))
                                       emit (Utf.fromString32 (*cast(dchar[]*) _arg, result));
                                   else
                                      {
                                      // for all non string array types (including char[][])
                                      auto arr = *cast(void[]*)_arg;
                                      auto len = arr.length;
                                      auto ptr = cast(Arg) arr.ptr;
                                      auto elTi = _ti.next();
                                      auto size = elTi.tsize();
                                      length += sink ("[");
                                      while (len > 0)
                                            {
                                            if (ptr !is arr.ptr)
                                                length += sink (", ");
                                            process (elTi, ptr);
                                            len -= 1;
                                            ptr += size;
                                            }
                                      length += sink ("]");
                                      }
                                   }
                                else
                                   // the standard processing
                                   emit (dispatch (result, format, _ti, _arg));
                      }

                      
                      // process this argument
                      if (index >= ti.length)
                          emit ("{invalid index}");
                      else
                         process (ti[index], args[index]);
                      }
                return length;
        }

        /***********************************************************************

        ***********************************************************************/

        private T[] dispatch (T[] result, T[] format, TypeInfo type, Arg p)
        {
                switch (type.classinfo.name[9])
                       {
                       case TypeCode.BOOL:
                            static T[] t = "true";
                            static T[] f = "false";
                            return (*cast(bool*) p) ? t : f;

                       case TypeCode.BYTE:
                            return integer (result, *cast(byte*) p, format, ubyte.max);

                       case TypeCode.VOID:
                       case TypeCode.UBYTE:
                            return integer (result, *cast(ubyte*) p, format, ubyte.max, "u");

                       case TypeCode.SHORT:
                            return integer (result, *cast(short*) p, format, ushort.max);

                       case TypeCode.USHORT:
                            return integer (result, *cast(ushort*) p, format, ushort.max, "u");

                       case TypeCode.INT:
                            return integer (result, *cast(int*) p, format, uint.max);

                       case TypeCode.UINT:
                            return integer (result, *cast(uint*) p, format, uint.max, "u");

                       case TypeCode.ULONG:
                            return integer (result, *cast(long*) p, format, ulong.max, "u");

                       case TypeCode.LONG:
                            return integer (result, *cast(long*) p, format, ulong.max);

                       case TypeCode.FLOAT:
                            return floater (result, *cast(float*) p, format);

                       case TypeCode.IFLOAT:
                            return imaginary (result, *cast(ifloat*) p, format);

                       case TypeCode.IDOUBLE:
                            return imaginary (result, *cast(idouble*) p, format);

                       case TypeCode.IREAL:
                           return imaginary (result, *cast(ireal*) p, format);

                       case TypeCode.CFLOAT:
                            return complex (result, *cast(cfloat*) p, format);

                       case TypeCode.CDOUBLE:
                            return complex (result, *cast(cdouble*) p, format);

                       case TypeCode.CREAL:
                            return complex (result, *cast(creal*) p, format);

                       case TypeCode.DOUBLE:
                            return floater (result, *cast(double*) p, format);

                       case TypeCode.REAL:
                            return floater (result, *cast(real*) p, format);

                       case TypeCode.CHAR:
                            return Utf.fromString8 ((cast(char*) p)[0..1], result);

                       case TypeCode.WCHAR:
                            return Utf.fromString16 ((cast(wchar*) p)[0..1], result);

                       case TypeCode.DCHAR:
                            return Utf.fromString32 ((cast(dchar*) p)[0..1], result);

                       case TypeCode.POINTER:
                            return integer (result, *cast(size_t*) p, format, size_t.max, "x");

                       case TypeCode.CLASS:
                            auto c = *cast(Object*) p;
                            if (c)
                                return Utf.fromString8 (c.toString, result);
                            break;

                       case TypeCode.STRUCT:
                            auto s = cast(TypeInfo_Struct) type;
                            if (s.xtoString) 
                               {
                               char[] delegate() toString;
                               toString.ptr = p;
                               toString.funcptr = cast(char[] function())s.xtoString;
                               return Utf.fromString8 (toString(), result);
                               }
                            goto default;

                       case TypeCode.INTERFACE:
                            auto x = *cast(void**) p;
                            if (x)
                               {
                               auto pi = **cast(Interface ***) x;
                               auto o = cast(Object)(*cast(void**)p - pi.offset);
                               return Utf.fromString8 (o.toString, result);
                               }
                            break;

                       case TypeCode.ENUM:
                            return dispatch (result, format, (cast(TypeInfo_Enum) type).base, p);

                       case TypeCode.TYPEDEF:
                            return dispatch (result, format, (cast(TypeInfo_Typedef) type).base, p);

                       default:
                            return unknown (result, format, type, p);
                       }

                return cast(T[]) "{null}";
        }

        /**********************************************************************

                handle "unknown-type" errors

        **********************************************************************/

        protected T[] unknown (T[] result, T[] format, TypeInfo type, Arg p)
        {
        version (WithExtensions)
                {
                result = Extensions!(T).run (type, result, p, format);
                return (result) ? result : 
                       "{unhandled argument type: " ~ Utf.fromString8 (type.toString, result) ~ "}";
                }
             else
                {
                if (type is typeid(Time))
                   {
                   static if (is (T == char))
                              return dateTime.format(result, *cast(Time*) p, format);
                          else
                             {
                             // TODO: this needs to be cleaned up
                             char[128] tmp0 = void;
                             char[128] tmp1 = void;
                             return Utf.fromString8(dateTime.format(tmp0, *cast(Time*) p, Utf.toString(format, tmp1)), result);
                             }
                   }

                return "{unhandled argument type: " ~ Utf.fromString8 (type.toString, result) ~ "}";
                }
        }

        /**********************************************************************

                Format an integer value

        **********************************************************************/

        protected T[] integer (T[] output, long v, T[] format, ulong mask = ulong.max, T[] def="d")
        {
                if (format.length is 0)
                    format = def;
                if (format[0] != 'd')
                    v &= mask;

                return Integer.format (output, v, format);
        }

        /**********************************************************************

                format a floating-point value. Defaults to 2 decimal places

        **********************************************************************/

        protected T[] floater (T[] output, real v, T[] format)
        {
                uint dec = 2,
                     exp = 10;
                bool pad = true;

                for (auto p=format.ptr, e=p+format.length; p < e; ++p)
                     switch (*p)
                            {
                            case '.':
                                 pad = false;
                                 break;
                            case 'e':
                            case 'E':
                                 exp = 0;
                                 break;
                            case 'x':
                            case 'X':
                                 double d = v;
                                 return integer (output, *cast(long*) &d, "x#");
                            default:
                                 auto c = *p;
                                 if (c >= '0' && c <= '9')
                                    {
                                    dec = c - '0', c = p[1];
                                    if (c >= '0' && c <= '9' && ++p < e)
                                        dec = dec * 10 + c - '0';
                                    }
                                 break;
                            }
                
                return Float.format (output, v, dec, exp, pad);
        }

        /**********************************************************************

        **********************************************************************/

        private void error (char[] msg)
        {
                throw new IllegalArgumentException (msg);
        }

        /**********************************************************************

        **********************************************************************/

        private uint spaces (Sink sink, int count)
        {
                uint ret;

                static const T[32] Spaces = ' ';
                while (count > Spaces.length)
                      {
                      ret += sink (Spaces);
                      count -= Spaces.length;
                      }
                return ret + sink (Spaces[0..count]);
        }

        /**********************************************************************

                format an imaginary value

        **********************************************************************/

        private T[] imaginary (T[] result, ireal val, T[] format)
        {
                return floatingTail (result, val.im, format, "*1i");
        }
        
        /**********************************************************************

                format a complex value

        **********************************************************************/

        private T[] complex (T[] result, creal val, T[] format)
        {
                static bool signed (real x)
                {
                        static if (real.sizeof is 4) 
                                   return ((*cast(uint *)&x) & 0x8000_0000) != 0;
                        else
                        static if (real.sizeof is 8) 
                                   return ((*cast(ulong *)&x) & 0x8000_0000_0000_0000) != 0;
                               else
                                  {
                                  auto pe = cast(ubyte *)&x;
                                  return (pe[9] & 0x80) != 0;
                                  }
                }
                static T[] plus = "+";

                auto len = floatingTail (result, val.re, format, signed(val.im) ? null : plus).length;
                return result [0 .. len + floatingTail (result[len..$], val.im, format, "*1i").length];
        }

        /**********************************************************************

                formats a floating-point value, and appends a tail to it

        **********************************************************************/

        private T[] floatingTail (T[] result, real val, T[] format, T[] tail)
        {
                assert (result.length > tail.length);

                auto res = floater (result[0..$-tail.length], val, format);
                auto len=res.length;
                if (res.ptr!is result.ptr)
                    result[0..len]=res;
                result [len .. len + tail.length] = tail;
                return result [0 .. len + tail.length];
        }
}


/*******************************************************************************

*******************************************************************************/

private enum TypeCode
{
        EMPTY = 0,
        VOID = 'v',
        BOOL = 'b',
        UBYTE = 'h',
        BYTE = 'g',
        USHORT = 't',
        SHORT = 's',
        UINT = 'k',
        INT = 'i',
        ULONG = 'm',
        LONG = 'l',
        REAL = 'e',
        FLOAT = 'f',
        DOUBLE = 'd',
        CHAR = 'a',
        WCHAR = 'u',
        DCHAR = 'w',
        ARRAY = 'A',
        CLASS = 'C',
        STRUCT = 'S',
        ENUM = 'E',
        CONST = 'x',
        INVARIANT = 'y',
        DELEGATE = 'D',
        FUNCTION = 'F',
        POINTER = 'P',
        TYPEDEF = 'T',
        INTERFACE = 'I',
        CFLOAT = 'q',
        CDOUBLE = 'r',
        CREAL = 'c',
        IFLOAT = 'o',
        IDOUBLE = 'p',
        IREAL = 'j'  
}



/*******************************************************************************

*******************************************************************************/

debug (UnitTest)
{
        unittest
        {
        auto Formatter = Layout!(char).instance;

        // basic layout tests
        assert( Formatter( "abc" ) == "abc" );
        assert( Formatter( "{0}", 1 ) == "1" );
        assert( Formatter( "{0}", -1 ) == "-1" );

        assert( Formatter( "{}", 1 ) == "1" );
        assert( Formatter( "{} {}", 1, 2) == "1 2" );
        assert( Formatter( "{} {0} {}", 1, 3) == "1 1 3" );
        assert( Formatter( "{} {0} {} {}", 1, 3) == "1 1 3 {invalid index}" );
        assert( Formatter( "{} {0} {} {:x}", 1, 3) == "1 1 3 {invalid index}" );

        assert( Formatter( "{0}", true ) == "true" , Formatter( "{0}", true ));
        assert( Formatter( "{0}", false ) == "false" );

        assert( Formatter( "{0}", cast(byte)-128 ) == "-128" );
        assert( Formatter( "{0}", cast(byte)127 ) == "127" );
        assert( Formatter( "{0}", cast(ubyte)255 ) == "255" );

        assert( Formatter( "{0}", cast(short)-32768  ) == "-32768" );
        assert( Formatter( "{0}", cast(short)32767 ) == "32767" );
        assert( Formatter( "{0}", cast(ushort)65535 ) == "65535" );
        assert( Formatter( "{0:x4}", cast(ushort)0xafe ) == "0afe" );
        assert( Formatter( "{0:X4}", cast(ushort)0xafe ) == "0AFE" );

        assert( Formatter( "{0}", -2147483648 ) == "-2147483648" );
        assert( Formatter( "{0}", 2147483647 ) == "2147483647" );
        assert( Formatter( "{0}", 4294967295 ) == "4294967295" );

        // large integers
        assert( Formatter( "{0}", -9223372036854775807L) == "-9223372036854775807" );
        assert( Formatter( "{0}", 0x8000_0000_0000_0000L) == "9223372036854775808" );
        assert( Formatter( "{0}", 9223372036854775807L ) == "9223372036854775807" );
        assert( Formatter( "{0:X}", 0xFFFF_FFFF_FFFF_FFFF) == "FFFFFFFFFFFFFFFF" );
        assert( Formatter( "{0:x}", 0xFFFF_FFFF_FFFF_FFFF) == "ffffffffffffffff" );
        assert( Formatter( "{0:x}", 0xFFFF_1234_FFFF_FFFF) == "ffff1234ffffffff" );
        assert( Formatter( "{0:x19}", 0x1234_FFFF_FFFF) == "00000001234ffffffff" );
        assert( Formatter( "{0}", 18446744073709551615UL ) == "18446744073709551615" );
        assert( Formatter( "{0}", 18446744073709551615UL ) == "18446744073709551615" );

        // fragments before and after
        assert( Formatter( "d{0}d", "s" ) == "dsd" );
        assert( Formatter( "d{0}d", "1234567890" ) == "d1234567890d" );

        // brace escaping
        assert( Formatter( "d{0}d", "<string>" ) == "d<string>d");
        assert( Formatter( "d{{0}d", "<string>" ) == "d{0}d");
        assert( Formatter( "d{{{0}d", "<string>" ) == "d{<string>d");
        assert( Formatter( "d{0}}d", "<string>" ) == "d<string>}d");

        // hex conversions, where width indicates leading zeroes
        assert( Formatter( "{0:x}", 0xafe0000 ) == "afe0000" );
        assert( Formatter( "{0:x7}", 0xafe0000 ) == "afe0000" );
        assert( Formatter( "{0:x8}", 0xafe0000 ) == "0afe0000" );
        assert( Formatter( "{0:X8}", 0xafe0000 ) == "0AFE0000" );
        assert( Formatter( "{0:X9}", 0xafe0000 ) == "00AFE0000" );
        assert( Formatter( "{0:X13}", 0xafe0000 ) == "000000AFE0000" );
        assert( Formatter( "{0:x13}", 0xafe0000 ) == "000000afe0000" );

        // decimal width
        assert( Formatter( "{0:d6}", 123 ) == "000123" );
        assert( Formatter( "{0,7:d6}", 123 ) == " 000123" );
        assert( Formatter( "{0,-7:d6}", 123 ) == "000123 " );

        // width & sign combinations
        assert( Formatter( "{0:d7}", -123 ) == "-0000123" );
        assert( Formatter( "{0,7:d6}", 123 ) == " 000123" );
        assert( Formatter( "{0,7:d7}", -123 ) == "-0000123" );
        assert( Formatter( "{0,8:d7}", -123 ) == "-0000123" );
        assert( Formatter( "{0,5:d7}", -123 ) == "-0000123" );

        // Negative numbers in various bases
        assert( Formatter( "{:b}", cast(byte) -1 ) == "11111111" );
        assert( Formatter( "{:b}", cast(short) -1 ) == "1111111111111111" );
        assert( Formatter( "{:b}", cast(int) -1 )
                == "11111111111111111111111111111111" );
        assert( Formatter( "{:b}", cast(long) -1 )
                == "1111111111111111111111111111111111111111111111111111111111111111" );

        assert( Formatter( "{:o}", cast(byte) -1 ) == "377" );
        assert( Formatter( "{:o}", cast(short) -1 ) == "177777" );
        assert( Formatter( "{:o}", cast(int) -1 ) == "37777777777" );
        assert( Formatter( "{:o}", cast(long) -1 ) == "1777777777777777777777" );

        assert( Formatter( "{:d}", cast(byte) -1 ) == "-1" );
        assert( Formatter( "{:d}", cast(short) -1 ) == "-1" );
        assert( Formatter( "{:d}", cast(int) -1 ) == "-1" );
        assert( Formatter( "{:d}", cast(long) -1 ) == "-1" );

        assert( Formatter( "{:x}", cast(byte) -1 ) == "ff" );
        assert( Formatter( "{:x}", cast(short) -1 ) == "ffff" );
        assert( Formatter( "{:x}", cast(int) -1 ) == "ffffffff" );
        assert( Formatter( "{:x}", cast(long) -1 ) == "ffffffffffffffff" );

        // argument index
        assert( Formatter( "a{0}b{1}c{2}", "x", "y", "z" ) == "axbycz" );
        assert( Formatter( "a{2}b{1}c{0}", "x", "y", "z" ) == "azbycx" );
        assert( Formatter( "a{1}b{1}c{1}", "x", "y", "z" ) == "aybycy" );

        // alignment does not restrict the length
        assert( Formatter( "{0,5}", "hellohello" ) == "hellohello" );

        // alignment fills with spaces
        assert( Formatter( "->{0,-10}<-", "hello" ) == "->hello     <-" );
        assert( Formatter( "->{0,10}<-", "hello" ) == "->     hello<-" );
        assert( Formatter( "->{0,-10}<-", 12345 ) == "->12345     <-" );
        assert( Formatter( "->{0,10}<-", 12345 ) == "->     12345<-" );

        // chop at maximum specified length; insert ellipses when chopped
        assert( Formatter( "->{.5}<-", "hello" ) == "->hello<-" );
        assert( Formatter( "->{.4}<-", "hello" ) == "->hell...<-" );
        assert( Formatter( "->{.-3}<-", "hello" ) == "->...llo<-" );

        // width specifier indicates number of decimal places
        assert( Formatter( "{0:f}", 1.23f ) == "1.23" );
        assert( Formatter( "{0:f4}", 1.23456789L ) == "1.2346" );
        assert( Formatter( "{0:e4}", 0.0001) == "1.0000e-04");

        assert( Formatter( "{0:f}", 1.23f*1i ) == "1.23*1i");
        assert( Formatter( "{0:f4}", 1.23456789L*1i ) == "1.2346*1i" );
        assert( Formatter( "{0:e4}", 0.0001*1i) == "1.0000e-04*1i");

        assert( Formatter( "{0:f}", 1.23f+1i ) == "1.23+1.00*1i" );
        assert( Formatter( "{0:f4}", 1.23456789L+1i ) == "1.2346+1.0000*1i" );
        assert( Formatter( "{0:e4}", 0.0001+1i) == "1.0000e-04+1.0000e+00*1i");
        assert( Formatter( "{0:f}", 1.23f-1i ) == "1.23-1.00*1i" );
        assert( Formatter( "{0:f4}", 1.23456789L-1i ) == "1.2346-1.0000*1i" );
        assert( Formatter( "{0:e4}", 0.0001-1i) == "1.0000e-04-1.0000e+00*1i");

        // 'f.' & 'e.' format truncates zeroes from floating decimals
        assert( Formatter( "{:f4.}", 1.230 ) == "1.23" );
        assert( Formatter( "{:f6.}", 1.230 ) == "1.23" );
        assert( Formatter( "{:f1.}", 1.230 ) == "1.2" );
        assert( Formatter( "{:f.}", 1.233 ) == "1.23" );
        assert( Formatter( "{:f.}", 1.237 ) == "1.24" );
        assert( Formatter( "{:f.}", 1.000 ) == "1" );
        assert( Formatter( "{:f2.}", 200.001 ) == "200");
        
        // array output
        int[] a = [ 51, 52, 53, 54, 55 ];
        assert( Formatter( "{}", a ) == "[51, 52, 53, 54, 55]" );
        assert( Formatter( "{:x}", a ) == "[33, 34, 35, 36, 37]" );
        assert( Formatter( "{,-4}", a ) == "[51  , 52  , 53  , 54  , 55  ]" );
        assert( Formatter( "{,4}", a ) == "[  51,   52,   53,   54,   55]" );
        int[][] b = [ [ 51, 52 ], [ 53, 54, 55 ] ];
        assert( Formatter( "{}", b ) == "[[51, 52], [53, 54, 55]]" );

        ushort[3] c = [ cast(ushort)51, 52, 53 ];
        assert( Formatter( "{}", c ) == "[51, 52, 53]" );

        // integer AA 
        ushort[long] d;
        d[234] = 2;
        d[345] = 3;
        assert( Formatter( "{}", d ) == "{234 => 2, 345 => 3}" ||
                Formatter( "{}", d ) == "{345 => 3, 234 => 2}");
        
        // bool/string AA 
        bool[char[]] e;
        e[ "key".dup ] = true;
        e[ "value".dup ] = false;
        assert( Formatter( "{}", e ) == "{key => true, value => false}" ||
                Formatter( "{}", e ) == "{value => false, key => true}");

        // string/double AA 
        char[][ double ] f;
        f[ 1.0 ] = "one".dup;
        f[ 3.14 ] = "PI".dup;
        assert( Formatter( "{}", f ) == "{1.00 => one, 3.14 => PI}" ||
                Formatter( "{}", f ) == "{3.14 => PI, 1.00 => one}");
        }
}



debug (Layout)
{
        import tango.io.Console;
        import tango.time.WallClock;

        void main ()
        {
                auto layout = Layout!(char).instance;

                layout.convert (Cout.stream, "hi {}", "there\n");

                Cout (layout.sprint (new char[1], "hi")).newline;
                Cout (layout.sprint (new char[10], "{.4}", "hello")).newline;
                Cout (layout.sprint (new char[10], "{.-4}", "hello")).newline;

                Cout (layout ("{:f1}", 3.0)).newline;
                Cout (layout ("{:g}", 3.00)).newline;
                Cout (layout ("{:f1}", -0.0)).newline;
                Cout (layout ("{:g1}", -0.0)).newline;
                Cout (layout ("{:d2}", 56)).newline;
                Cout (layout ("{:d4}", cast(byte) -56)).newline;
                Cout (layout ("{:f4}", 1.0e+12)).newline;
                Cout (layout ("{:f4}", 1.23e-2)).newline;
                Cout (layout ("{:f8}", 3.14159)).newline;
                Cout (layout ("{:e20}", 1.23e-3)).newline;
                Cout (layout ("{:e4.}", 1.23e-07)).newline;
                Cout (layout ("{:.}", 1.2)).newline;
                Cout (layout ("ptr:{}", &layout)).newline;
                Cout (layout ("ulong.max {}", ulong.max)).newline;

                struct S
                {
                   char[] toString () {return "foo";}      
                }

                S s;
                Cout (layout ("struct: {}", s)).newline;
                Cout (layout ("time: {}", WallClock.now)).newline;
        }
}
