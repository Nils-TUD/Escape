/*******************************************************************************

        This module provides a general-purpose formatting system to convert
        values to strings suitable for display. There is support for alignment
        and justification, common _format specifiers for numbers and date/time
        data, and culture-specific utput. Custom formats are also supported.

        The _format notation is influenced by that used in the .NET framework
        rather than the C-style printf or D-style writef.

        ---
        // format with the user's current culture (for example, en-GB)
        Formatter ("General: {0} Hexadecimal: 0x{0:x4} Numeric: {0:N}", 1000);
        ---
        
        >> General: 1000 Hexadecimal: 0x03e8 Numeric: 1,000.00


        The formatter supports I18N via a Culture instance, configured as
        a ctor argument. For example, configure a french formatter like so:

        ---
        // utf8 format, using a french culture
        auto format = new Format!(char) (Culture.getCulture("fr-FR"));
        
        format ("{0:D}", DateTime.today);
        ---
        
        >> vendredi 3 mars 2006

******************************************************************************/

module tango.text.convert.Format;

private import tango.core.Exception;

private import tango.text.convert.Utf;

private import tango.text.convert.model.IFormatService;


/*******************************************************************************

        enable floating point support?
        
*******************************************************************************/

// TODO we need some functions for that; at least ecvt
//version = mlfp;
version (mlfp)
{
        private extern (C) private char* ecvt(double d, int digits, out int decpt, out bool sign);
}


/*******************************************************************************

        Platform issues ...
        
*******************************************************************************/

version (DigitalMars)
         alias void* Args;
   else 
      alias char* Args;


/*******************************************************************************

        Contains methods for replacing format items in a string with string
        equivalents of each argument.
        
*******************************************************************************/

public class Format(T)
{
        alias uint delegate (T[])               Sink;
        alias convert                           opCall;
        alias tango.text.convert.Utf            Unicode;

        private NumericAttributes               numAttr;
        private NumericTextAttributes!(T)       textAttr;
        private IFormatService                  formatService;
        

        /***********************************************************************

        ***********************************************************************/

        // Must match NumberFormat.decimalPositivePattern
        private const   T[] positiveNumberFormat = "#";

        // Must match NumberFormat.decimalNegativePattern
        private const   T[][] negativeNumberFormats =
                        [
                        "(#)", "-#", "- #", "#-", "# -"
                        ];

        // Must match NumberFormat.currencyPositivePattern
        private const   T[][] positiveCurrencyFormats =
                        [
                        "$#", "#$", "$ #", "# $"
                        ];

        // Must match NumberFormat.currencyNegativePattern
        private const   T[][] negativeCurrencyFormats =
                        [
                        "($#)", "-$#", "$-#", "$#-", "(#$)",
                        "-#$", "#-$", "#$-", "-# $", "-$ #",
                        "# $-", "$ #-", "$ -#", "#- $", "($ #)", "(# $)"
                        ];


        /**********************************************************************
        
        **********************************************************************/

        this ()
        {
                numAttr = new NumberFormat;
                textAttr = getText (numAttr);
        }
        
        /**********************************************************************
        
        **********************************************************************/

        this (IFormatService formatService)
        {
                if ((this.formatService = formatService) !is null)
                   {
                   numAttr = cast(NumericAttributes) formatService.getFormat (typeid(NumericAttributes));
                   if (numAttr)
                      {
                      textAttr = getText (numAttr);
                      return;
                      }
                   }
                error ("invalid formatservice argument");
        }
        
        /**********************************************************************
        
        **********************************************************************/

        private NumericTextAttributes!(T) getText (NumericAttributes attr)
        {
                static if (is (T == char))
                           return attr.utf8();
                static if (is (T == wchar))
                           return attr.utf16();
                static if (is (T == dchar))
                           return attr.utf32();
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

        public final T[] convert (TypeInfo[] arguments, Args args, T[] formatStr)
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

        /**********************************************************************

        **********************************************************************/

        public final uint convert (Sink sink, T[] formatStr, ...)
        {
                return convert (sink, _arguments, _argptr, formatStr);
        }

        /**********************************************************************

        **********************************************************************/

        public final uint convert (Sink sink, TypeInfo[] arguments, Args args, T[] formatStr)
        {
                assert (formatStr, "null format specifier");
                auto it = ArgumentIterator (arguments, args);
                return parse (formatStr, it, sink);
        }

        /**********************************************************************

        **********************************************************************/

        public final T[] convert (inout Result result, TypeInfo ti, Args arg)
        {
                auto argument = Argument (ti, arg);

                return convertArgument (result, null, argument);
        }

        /**********************************************************************
        
        **********************************************************************/

        public final T[] sprint (T[] result, T[] formatStr, ...)
        {
                return sprint (result, formatStr, _arguments, _argptr);
        }
        
        /**********************************************************************
        
        **********************************************************************/

        public final T[] sprint (T[] result, T[] formatStr, TypeInfo[] arguments, Args args)
        {
                T* p = result.ptr;

                uint sink (T[] s)
                {
                        int len = s.length;
                        if (len < (result.ptr + result.length) - p)
                           {
                           p [0..len] = s;
                           p += len;
                           }
                        else
                           error ("Format.sprint :: output buffer is full");
                        return len;
                }

                convert (&sink, arguments, args, formatStr);
                return result [0 .. p-result.ptr];
        }

        /**********************************************************************

        **********************************************************************/

        private final uint spaces (Sink sink, int count)
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

                Parse the format-string, emitting formatted args and text
                fragments as we go.

        **********************************************************************/

        private final uint parse (T[] layout, inout ArgumentIterator it, Sink sink)
        {
                T[256] output = void;
                T[256] convert = void;
                auto result = Result (output, convert);
                
                int length;

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

                      // extract index
                      int index;
                      while (*s >= '0' && *s <= '9')
                             index = index * 10 + *s++ -'0';

                      // skip spaces
                      while (s < end && *s is ' ')
                             ++s;

                      int  width;
                      bool leftAlign;

                      // has width?
                      if (*s is ',')
                         {
                         while (++s < end && *s is ' ') {}

                         if (*s is '-')
                            {
                            leftAlign = true;
                            ++s;
                            }

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
                          error ("missing or misplaced '}' in format specifier");

                      // next char is start of following fragment
                      fragment = ++s;

                      // convert argument to a string
                      Argument arg = it [index];
                      T[] str = convertArgument (result, format, arg);
                      int padding = width - str.length;

                      // if not left aligned, pad out with spaces
                      if (! leftAlign && padding > 0)
                            length += spaces (sink, padding);

                      // emit formatted argument
                      length += sink (str);

                      // finally, pad out on right
                      if (leftAlign && padding > 0)
                          length += spaces (sink, padding);
                      }

                return length;
        }

        /**********************************************************************

        **********************************************************************/

        public static void error (char[] msg)
        {
                throw new IllegalArgumentException (msg);
        }

        /***********************************************************************

        ***********************************************************************/

        private T[] convertArgument (inout Result result, T[] format, inout Argument arg)
        {
                void* p = arg.getValue;
                
                switch (arg.getTypeCode)
                       {
                            // Currently we only format char[] arrays.
                       case TypeCode.ARRAY:
                            auto type = arg.getType();
                               
                            if (type is typeid(char[]))
                                return fromUtf8 (*cast(char[]*) p, result);

                            if (type is typeid(wchar[]))
                                return fromUtf16 (*cast(wchar[]*) p, result);

                            if (type is typeid(dchar[]))
                                return fromUtf32 (*cast(dchar[]*) p, result);

                            return fromUtf8 (type.toUtf8, result);

                       case TypeCode.BOOL:
                            static T[] t = "true";
                            static T[] f = "false";
                            return (*cast(bool*) p) ? t : f;

                       case TypeCode.BYTE:
                            return formatInteger (result, *cast(byte*) p, format);

                            case TypeCode.UBYTE:
                            return formatInteger (result, *cast(ubyte*) p, format);

                       case TypeCode.SHORT:
                            return formatInteger (result, *cast(short*) p, format);

                            case TypeCode.USHORT:
                            return formatInteger (result, *cast(ushort*) p, format);

                       case TypeCode.INT:
                            return formatInteger (result, *cast(int*) p, format);

                            case TypeCode.UINT:
                            return formatInteger (result, *cast(uint*) p, format);

                       case TypeCode.LONG:
                            return formatInteger (result, *cast(long*) p, format);
                            
                       case TypeCode.ULONG:
                            return formatInteger (result, *cast(long*) p, format);

                       version (mlfp)
                               {
                               case TypeCode.FLOAT:
                                    return formatDouble (result, *cast(float*) p, format);

                               case TypeCode.DOUBLE:
                                    return formatDouble (result, *cast(double*) p, format);

                               case TypeCode.REAL:
                                    return formatDouble (result, *cast(real*) p, format);
                               }

                       case TypeCode.CHAR:
                            return fromUtf8 ((cast(char*) p)[0..1], result);

                       case TypeCode.WCHAR:
                            return fromUtf16 ((cast(wchar*) p)[0..1], result);

                       case TypeCode.DCHAR:
                            return fromUtf32 ((cast(dchar*) p)[0..1], result);

                       case TypeCode.CLASS:
                            return fromUtf8 ((*cast(Object*) p).toUtf8, result);

                       case TypeCode.POINTER:
                            return fromUtf8 (arg.getType().toUtf8, result);

                       case TypeCode.TYPEDEF:
                            arg.setType ((cast(TypeInfo_Typedef) arg.getType).base);
                            return convertArgument (result, format, arg);

                       default:
                            break;
                       }

                // For everything else, return an error string.
                return "<unknown argument type> " ~ fromUtf8 (arg.getType.toUtf8, result);
        }


        /***********************************************************************

        ***********************************************************************/

        private T[] longToString (T[] buffer, long value, int digits, T[] negativeSign)
        {
                if (digits < 1)
                    digits = 1;

                int n = buffer.length;
                ulong uv = (value >= 0) ? value : cast(ulong) -value;

                if (uv > uint.max)
                   {
                   while (--digits >= 0 || uv != 0)
                         {
                         buffer[--n] = uv % 10 + '0';
                         uv /= 10;
                         }
                   }
                else
                   {
                   uint v = cast(uint) uv;
                   while (--digits >= 0 || v != 0)
                         {
                         buffer[--n] = v % 10 + '0';
                         v /= 10;
                         }
                   }


                if (value < 0)
                   {
                   for (int i = negativeSign.length - 1; i >= 0; i--)
                        buffer[--n] = negativeSign[i];
                   }

                return buffer[n .. $];
        }

        /***********************************************************************

        ***********************************************************************/

        private T[] longToHexString (T[] buffer, ulong value, int digits, T format)
        {
                if (digits < 1)
                    digits = 1;

                int n = buffer.length;
                while (--digits >= 0 || value != 0)
                      {
                      ulong v = value & 0xF;
                      buffer[--n] = (v < 10) ? v + '0' : v + format - ('X' - 'A' + 10);
                      value >>= 4;
                      }

                return buffer[n .. $];
        }

        /***********************************************************************

        ***********************************************************************/

        private T parseFormatSpecifier (T[] format, out int length)
        {
                int     i = -1;
                T       specifier;
                
                if (format.length)
                   {
                   auto s = format[0];
                   
                   if (s >= 'A' && s <= 'Z' || s >= 'a' && s <= 'z')
                      {
                      specifier = s;

                      foreach (c; format [1..$])
                               if (c >= '0' && c <= '9')
                                  {
                                  c -= '0';
                                  if (i < 0)
                                     i = c;
                                  else
                                     i = i * 10 + c;
                                  }
                               else
                                  break;
                      }
                   }
                else
                   specifier = 'G';

                length = i;
                return specifier;
        }

        /***********************************************************************

        ***********************************************************************/

        private T[] formatInteger (inout Result result, long value, T[] format)
        {
                int     length;
                auto    specifier = parseFormatSpecifier (format, length);

                switch (specifier)
                       {
                       case 'g':
                       case 'G':
                            if (length > 0)
                                break;
                            // Fall through.

                       case 'd':
                       case 'D':
                            return longToString (result.scratch, value, length, textAttr.negativeSign);

                       case 'x':
                       case 'X':
                            return longToHexString (result.scratch, cast(ulong)value, length, specifier);

                       default:
                            break;
                       }

                Number number = Number (value);
                if (specifier != T.init)
                    return toUtf8 (number, result, specifier, length);

                return number.toUtf8Format (result, format, this);
        }

        /***********************************************************************

        ***********************************************************************/

        // Only if floating-point support is enabled.
        version (mlfp)
        {
                private enum {
                             NAN_FLAG = 0x80000000,
                             INFINITY_FLAG = 0x7fffffff,
                             EXP = 0x7ff
                             }

                private T[] formatDouble (inout Result result, double value, T[] format)
                {
                        int length;
                        T specifier = parseFormatSpecifier (format, length);
                        int precision = 15;

                        switch (specifier)
                               {
                               case 'r':
                               case 'R':
                                    Number number = Number (value, 15);

                                    if (number.scale == NAN_FLAG)
                                        return textAttr.nanSymbol;

                                    if (number.scale == INFINITY_FLAG)
                                        return number.sign ? textAttr.negativeInfinitySymbol
                                                           : textAttr.positiveInfinitySymbol;

                                    double d;
                                    number.toDouble(d);
                                    if (d == value)
                                        return toUtf8 (number, result, 'G', 15);

                                    number = Number(value, 17);
                                    return toUtf8 (number, result, 'G', 17);

                               case 'g':
                               case 'G':
                                    if (length > 15)
                                        precision = 17;
                                    // Fall through.

                               default:
                                    break;
                        }

                        Number number = Number(value, precision);

                        if (number.scale == NAN_FLAG)
                            return textAttr.nanSymbol;

                        if (number.scale == INFINITY_FLAG)
                            return number.sign ? textAttr.negativeInfinitySymbol
                                               : textAttr.positiveInfinitySymbol;

                        if (specifier != T.init)
                            return toUtf8 (number, result, specifier, length);

                        return number.toUtf8Format (result, format, this);
                }

        } // version (mlfp)


        /***********************************************************************

        ***********************************************************************/

        private static T[] fromUtf8 (char[] s, inout Result result)
        {
                static if (is (T == char))
                           return s;
                
                static if (is (T == wchar))
                           return Unicode.toUtf16 (s, result.convert);
                
                static if (is (T == dchar))
                           return Unicode.toUtf32 (s, result.convert);
        }
        
        /***********************************************************************

        ***********************************************************************/

        private static T[] fromUtf16 (wchar[] s, inout Result result)
        {
                static if (is (T == wchar))
                           return s;
                
                static if (is (T == char))
                           return Unicode.toUtf8 (s, result.convert);
                
                static if (is (T == dchar))
                           return Unicode.toUtf32 (s, result.convert);
        }
        
        /***********************************************************************

        ***********************************************************************/

        private static T[] fromUtf32 (dchar[] s, inout Result result)
        {
                static if (is (T == dchar))
                           return s;
                
                static if (is (T == char))
                           return Unicode.toUtf8 (s, result.convert);
                
                static if (is (T == wchar))
                           return Unicode.toUtf16 (s, result.convert);
        }
        
        /***********************************************************************

        ***********************************************************************/

        private void formatGeneral (inout Number number, inout Result target, int length, T format)
        {
                int pos = number.scale;

                T* p = number.digits.ptr;
                if (pos > 0)
                   {
                   while (pos > 0)
                         {
                         target ~= (*p != '\0') ? *p++ : '0';
                         pos--;
                         }
                   }
                else
                   target ~= '0';

                if (*p != '\0')
                   {
                   target ~= textAttr.numberDecimalSeparator;
                   while (pos < 0)
                         {
                         target ~= '0';
                         pos++;
                         }

                   while (*p != '\0')
                          target ~= *p++;
                   }
        }

        /***********************************************************************

        ***********************************************************************/

        private void formatNumber (inout Number number, inout Result target, int length)
        {
                T[] format = number.sign ? negativeNumberFormats[numAttr.numberNegativePattern]
                                         : positiveNumberFormat;

                // Parse the format.
                foreach (c; format)
                        {
                        switch (c)
                               {
                               case '#':
                                    formatFixed (number, target, length, numAttr.numberGroupSizes,
                                                 textAttr.numberDecimalSeparator, textAttr.numberGroupSeparator);
                                    break;

                               case '-':
                                    target ~= textAttr.negativeSign;
                                    break;

                               default:
                                    target ~= c;
                                    break;
                               }
                        }
        }

        /***********************************************************************

        ***********************************************************************/

        private void formatCurrency (inout Number number, inout Result target, int length)
        {
                T[] format = number.sign ? negativeCurrencyFormats[numAttr.currencyNegativePattern]
                                         : positiveCurrencyFormats[numAttr.currencyPositivePattern];

                // Parse the format.
                foreach (c; format)
                        {
                        switch (c)
                               {
                               case '#':
                                    formatFixed (number, target, length, numAttr.currencyGroupSizes,
                                                 textAttr.currencyDecimalSeparator, textAttr.currencyGroupSeparator);
                                    break;

                               case '-':
                                    target ~= textAttr.negativeSign;
                                    break;

                               case '$':
                                    target ~= textAttr.currencySymbol;
                                    break;

                               default:
                                    target ~= c;
                                    break;
                               }
                        }
        }

        /***********************************************************************

        ***********************************************************************/

        private void formatFixed (inout Number number, inout Result target, int length,
                                  int[] groupSizes, T[] decimalSeparator, T[] groupSeparator)
        {
                int pos = number.scale;
                T* p = number.digits.ptr;

                if (pos > 0)
                   {
                   if (groupSizes.length != 0)
                      {
                      // Calculate whether we have enough digits to format.
                      int count = groupSizes[0];
                      int index, size;

                      while (pos > count)
                            {
                            size = groupSizes[index];
                            if (size == 0)
                                break;

                            if (index < groupSizes.length - 1)
                               index++;

                            count += groupSizes[index];
                            }

                      size = (count == 0) ? 0 : groupSizes[0];

                      // Insert the separator according to groupSizes.
                      int end = charTerm(p);
                      int start = (pos < end) ? pos : end;


                      T[] separator = groupSeparator;
                      index = 0;

                      // questionable: use the back end of the output buffer to
                      // format the separators, and then copy back to start
                      T[] temp = target.scratch;
                      uint ii = temp.length;

                      for (int c, i = pos - 1; i >= 0; i--)
                          {
                          temp[--ii] = (i < start) ? number.digits[i] : '0';
                          if (size > 0)
                             {
                             c++;
                             if (c == size && i != 0)
                                {
                                uint iii = ii - separator.length;
                                temp[iii .. ii] = separator;
                                ii = iii;

                                if (index < groupSizes.length - 1)
                                    size = groupSizes[++index];

                                c = 0;
                                }
                             }
                          }
                      target ~= temp[ii..$];
                      p += start;
                      }
                   else
                      {
                      while (pos > 0)
                            {
                            target ~= (*p != '\0') ? *p++ : '0';
                            pos--;
                            }
                      }
                   }
                else
                   // Negative scale.
                   target ~= '0';

                if (length > 0)
                   {
                   target ~= decimalSeparator;
                   while (pos < 0 && length > 0)
                         {
                         target ~= '0';
                         pos++;
                         length--;
                         }

                   while (length > 0)
                         {
                         target ~= (*p != '\0') ? *p++ : '0';
                         length--;
                         }
                   }
        }

        /**********************************************************************

        **********************************************************************/

        private T[] toUtf8 (inout Number number, inout Result result, T format, int length)
        {
                switch (format)
                       {
                       case 'c':
                       case 'C':
                             // Currency
                             if (length < 0)
                                 length = numAttr.currencyDecimalDigits;

                             number.round(number.scale + length);
                             formatCurrency (number, result, length);
                             break;

                       case 'f':
                       case 'F':
                             // Fixed
                             if (length < 0)
                                 length = numAttr.numberDecimalDigits;

                             number.round(number.scale + length);
                             if (number.sign)
                                 result ~= textAttr.negativeSign;

                             formatFixed (number, result, length, null, textAttr.numberDecimalSeparator, null);
                             break;

                       case 'n':
                       case 'N':
                             // Number
                                if (length < 0)
                                    length = numAttr.numberDecimalDigits;

                             number.round(number.scale + length);
                             formatNumber (number, result, length);
                             break;

                       case 'g':
                       case 'G':
                             // General
                             if (length < 1)
                                 length = number.precision;

                             number.round(length);
                             if (number.sign)
                                 result ~= textAttr.negativeSign;

                             formatGeneral (number, result, length, (format == 'g') ? 'e' : 'E');
                             break;

                       default:
                             error ("Invalid format specifier.");
                       }
                return result.get;
        }


        /***********************************************************************

        ***********************************************************************/

        private struct Number
        {
                int scale;
                bool sign;
                int precision;
                T[32] digits = void;

                /**************************************************************

                **************************************************************/

                static Number opCall (long value)
                {
                        Number number;
                        number.precision = 20;

                        if (value < 0)
                           {
                           number.sign = true;
                           value = -value;
                           }

                        T[20] buffer = void;
                        int n = buffer.length;

                        while (value != 0)
                              {
                              buffer[--n] = value % 10 + '0';
                              value /= 10;
                              }

                        int end = number.scale = -(n - buffer.length);
                        number.digits[0 .. end] = buffer[n .. n + end];
                        number.digits[end] = '\0';

                        return number;
                }


                /**************************************************************

                **************************************************************/

                // Only if floating-point support is enabled.
                version (mlfp)
                static Number opCall (double value, int precision)
                {
                        Number number;
                        number.precision = precision;

                        T* p = number.digits.ptr;
                        long bits = *cast(long*) & value;
                        long mant = bits & 0x000FFFFFFFFFFFFFL;
                        int exp = cast(int)((bits >> 52) & EXP);

                        if (exp == EXP)
                           {
                           number.scale = (mant != 0) ? NAN_FLAG : INFINITY_FLAG;
                           if (((bits >> 63) & 1) != 0)
                                 number.sign = true;
                           }
                        else
                           {
                           // Get the digits, decimal point and sign.
                           char* chars = ecvt(value, number.precision, number.scale, number.sign);
                           if (*chars != '\0')
                              {
                              while (*chars != '\0')
                                     *p++ = *chars++;
                              }
                           }

                        *p = '\0';
                        return number;
                }

                /**************************************************************

                **************************************************************/

                version (mlfp)
                private bool toDouble(out double value)
                {
                        const   ulong[] pow10 =
                                [
                                0xa000000000000000UL,
                                0xc800000000000000UL,
                                0xfa00000000000000UL,
                                0x9c40000000000000UL,
                                0xc350000000000000UL,
                                0xf424000000000000UL,
                                0x9896800000000000UL,
                                0xbebc200000000000UL,
                                0xee6b280000000000UL,
                                0x9502f90000000000UL,
                                0xba43b74000000000UL,
                                0xe8d4a51000000000UL,
                                0x9184e72a00000000UL,
                                0xb5e620f480000000UL,
                                0xe35fa931a0000000UL,
                                0xcccccccccccccccdUL,
                                0xa3d70a3d70a3d70bUL,
                                0x83126e978d4fdf3cUL,
                                0xd1b71758e219652eUL,
                                0xa7c5ac471b478425UL,
                                0x8637bd05af6c69b7UL,
                                0xd6bf94d5e57a42beUL,
                                0xabcc77118461ceffUL,
                                0x89705f4136b4a599UL,
                                0xdbe6fecebdedd5c2UL,
                                0xafebff0bcb24ab02UL,
                                0x8cbccc096f5088cfUL,
                                0xe12e13424bb40e18UL,
                                0xb424dc35095cd813UL,
                                0x901d7cf73ab0acdcUL,
                                0x8e1bc9bf04000000UL,
                                0x9dc5ada82b70b59eUL,
                                0xaf298d050e4395d6UL,
                                0xc2781f49ffcfa6d4UL,
                                0xd7e77a8f87daf7faUL,
                                0xefb3ab16c59b14a0UL,
                                0x850fadc09923329cUL,
                                0x93ba47c980e98cdeUL,
                                0xa402b9c5a8d3a6e6UL,
                                0xb616a12b7fe617a8UL,
                                0xca28a291859bbf90UL,
                                0xe070f78d39275566UL,
                                0xf92e0c3537826140UL,
                                0x8a5296ffe33cc92cUL,
                                0x9991a6f3d6bf1762UL,
                                0xaa7eebfb9df9de8aUL,
                                0xbd49d14aa79dbc7eUL,
                                0xd226fc195c6a2f88UL,
                                0xe950df20247c83f8UL,
                                0x81842f29f2cce373UL,
                                0x8fcac257558ee4e2UL,
                                ];

                        const   uint[] pow10Exp =
                                [
                                4, 7, 10, 14, 17, 20, 24, 27, 30, 34,
                                37, 40, 44, 47, 50, 54, 107, 160, 213, 266,
                                319, 373, 426, 479, 532, 585, 638, 691, 745, 798,
                                851, 904, 957, 1010, 1064, 1117
                                ];

                        uint getDigits(T* p, int len)
                        {
                                T* end = p + len;
                                uint r = *p - '0';
                                p++;
                                while (p < end)
                                      {
                                      r = 10 * r + *p - '0';
                                      p++;
                                      }
                                return r;
                        }

                        ulong mult64(uint val1, uint val2)
                        {
                                return cast(ulong)val1 * cast(ulong)val2;
                        }

                        ulong mult64L(ulong val1, ulong val2)
                        {
                                ulong v = mult64(cast(uint)(val1 >> 32), cast(uint)(val2 >> 32));
                                v += mult64(cast(uint)(val1 >> 32), cast(uint)val2) >> 32;
                                v += mult64(cast(uint)val1, cast(uint)(val2 >> 32)) >> 32;
                                return v;
                        }

                        T* p = digits.ptr;
                        int count = charTerm(p);
                        int left = count;

                        while (*p == '0')
                              {
                              left--;
                              p++;
                              }

                        // If the digits consist of nothing but zeros...
                        if (left == 0)
                           {
                           value = 0.0;
                           return true;
                           }

                        // Get digits, 9 at a time.
                        int n = (left > 9) ? 9 : left;
                        left -= n;
                        ulong bits = getDigits(p, n);
                        if (left > 0)
                           {
                           n = (left > 9) ? 9 : left;
                           left -= n;
                           bits = mult64(cast(uint)bits, cast(uint)(pow10[n - 1] >>> (64 - pow10Exp[n - 1])));
                           bits += getDigits(p + 9, n);
                           }

                        int scale = this.scale - (count - left);
                        int s = (scale < 0) ? -scale : scale;

                        if (s >= 352)
                           {
                           *cast(long*)&value = (scale > 0) ? 0x7FF0000000000000 : 0;
                           return false;
                           }

                        // Normalise mantissa and bits.
                        int bexp = 64;
                        int nzero;
                        if ((bits >> 32) != 0)
                             nzero = 32;

                        if ((bits >> (16 + nzero)) != 0)
                             nzero += 16;

                        if ((bits >> (8 + nzero)) != 0)
                             nzero += 8;

                        if ((bits >> (4 + nzero)) != 0)
                             nzero += 4;

                        if ((bits >> (2 + nzero)) != 0)
                             nzero += 2;

                        if ((bits >> (1 + nzero)) != 0)
                             nzero++;

                        if ((bits >> nzero) != 0)
                             nzero++;

                        bits <<= 64 - nzero;
                        bexp -= 64 - nzero;

                        // Get decimal exponent.
                        if ((s & 15) != 0)
                           {
                           int expMult = pow10Exp[(s & 15) - 1];
                           bexp += (scale < 0) ? ( -expMult + 1) : expMult;
                           bits = mult64L(bits, pow10[(s & 15) + ((scale < 0) ? 15 : 0) - 1]);
                           if ((bits & 0x8000000000000000L) == 0)
                              {
                              bits <<= 1;
                              bexp--;
                              }
                           }

                        if ((s >> 4) != 0)
                           {
                           int expMult = pow10Exp[15 + ((s >> 4) - 1)];
                           bexp += (scale < 0) ? ( -expMult + 1) : expMult;
                           bits = mult64L(bits, pow10[30 + ((s >> 4) + ((scale < 0) ? 21 : 0) - 1)]);
                           if ((bits & 0x8000000000000000L) == 0)
                              {
                              bits <<= 1;
                              bexp--;
                              }
                           }

                        // Round and scale.
                        if (cast(uint)bits & (1 << 10) != 0)
                           {
                           bits += (1 << 10) - 1 + (bits >>> 11) & 1;
                           bits >>= 11;
                           if (bits == 0)
                               bexp++;
                           }
                        else
                           bits >>= 11;

                        bexp += 1022;
                        if (bexp <= 0)
                           {
                           if (bexp < -53)
                               bits = 0;
                           else
                              bits >>= ( -bexp + 1);
                           }
                        bits = (cast(ulong)bexp << 52) + (bits & 0x000FFFFFFFFFFFFFL);

                        if (sign)
                            bits |= 0x8000000000000000L;

                        value = *cast(double*) & bits;
                        return true;
                }



                /**************************************************************

                **************************************************************/

                private T[] toUtf8Format (inout Result result, T[] format, Format fmt)
                {
                        bool hasGroups;
                        int groupCount;
                        int groupPos = -1, pointPos = -1;
                        int first = int.max, last, count;
                        bool scientific;
                        int n;
                        T c;

                        while (n < format.length)
                              {
                              c = format[n++];
                              switch (c)
                                     {
                                     case '#':
                                          count++;
                                          break;

                                     case '0':
                                          if (first == int.max)
                                              first = count;
                                          count++;
                                          last = count;
                                          break;

                                     case '.':
                                          if (pointPos < 0)
                                              pointPos = count;
                                          break;

                                     case ',':
                                          if (count > 0 && pointPos < 0)
                                             {
                                             if (groupPos >= 0)
                                                {
                                                if (groupPos == count)
                                                   {
                                                   groupCount++;
                                                   break;
                                                   }
                                                hasGroups = true;
                                                }
                                             groupPos = count;
                                             groupCount = 1;
                                             }
                                          break;

                                     case '\'':
                                     case '\"':
                                           while (n < format.length && format[n++] != c)
                                                 {}
                                           break;

                                     case '\\':
                                          if (n < format.length)
                                              n++;
                                          break;

                                     default:
                                          break;
                                     }
                              }

                        if (pointPos < 0)
                            pointPos = count;

                        int adjust;
                        if (groupPos >= 0)
                           {
                           if (groupPos == pointPos)
                               adjust -= groupCount * 3;
                           else
                              hasGroups = true;
                           }

                        if (digits[0] != '\0')
                           {
                           scale += adjust;
                           round(scientific ? count : scale + count - pointPos);
                           }

                        first = (first < pointPos) ? pointPos - first : 0;
                        last = (last > pointPos) ? pointPos - last : 0;

                        int pos = pointPos;
                        int extra;
                        if (!scientific)
                           {
                           pos = (scale > pointPos) ? scale : pointPos;
                           extra = scale - pointPos;
                           }

                        T[] groupSeparator = fmt.textAttr.numberGroupSeparator;
                        T[] decimalSeparator = fmt.textAttr.numberDecimalSeparator;

                        // Work out the positions of the group separator.
                        int[] groupPositions;
                        int groupIndex = -1;
                        if (hasGroups)
                           {
                           if (fmt.numAttr.numberGroupSizes.length == 0)
                               hasGroups = false;
                           else
                              {
                              int groupSizesTotal = fmt.numAttr.numberGroupSizes[0];
                              int groupSize = groupSizesTotal;
                              int digitsTotal = pos + ((extra < 0) ? extra : 0);
                              int digitCount = (first > digitsTotal) ? first : digitsTotal;

                              int sizeIndex;
                              while (digitCount > groupSizesTotal)
                                    {
                                    if (groupSize == 0)
                                        break;

                                    groupPositions ~= groupSizesTotal;
                                    groupIndex++;

                                    if (sizeIndex < fmt.numAttr.numberGroupSizes.length - 1)
                                        groupSize = fmt.numAttr.numberGroupSizes[++sizeIndex];

                                    groupSizesTotal += groupSize;
                                    }
                              }
                        }

                        //char[] result;
                        if (sign)
                            result ~= fmt.textAttr.negativeSign;

                        T* p = digits.ptr;
                        n = 0;
                        bool pointWritten;

                        while (n < format.length)
                              {
                              c = format[n++];
                              if (extra > 0 && (c == '#' || c == '0' || c == '.'))
                                 {
                                 while (extra > 0)
                                       {
                                       result ~= (*p != '\0') ? *p++ : '0';

                                       if (hasGroups && pos > 1 && groupIndex >= 0)
                                          {
                                          if (pos == groupPositions[groupIndex] + 1)
                                             {
                                             result ~= groupSeparator;
                                             groupIndex--;
                                             }
                                          }
                                       pos--;
                                       extra--;
                                       }
                                 }

                              switch (c)
                                     {
                                     case '#':
                                     case '0':
                                          if (extra < 0)
                                             {
                                             extra++;
                                             c = (pos <= first) ? '0' : T.init;
                                             }
                                          else
                                             c = (*p != '\0') ? *p++ : pos > last ? '0' : T.init;

                                          if (c != T.init)
                                             {
                                             result ~= c;

                                             if (hasGroups && pos > 1 && groupIndex >= 0)
                                                {
                                                if (pos == groupPositions[groupIndex] + 1)
                                                   {
                                                   result ~= groupSeparator;
                                                   groupIndex--;
                                                   }
                                                }
                                             }
                                          pos--;
                                          break;

                                     case '.':
                                          if (pos != 0 || pointWritten)
                                              break;
                                          if (last < 0 || (pointPos < count && *p != '\0'))
                                             {
                                             result ~= decimalSeparator;
                                             pointWritten = true;
                                             }
                                          break;

                                     case ',':
                                          break;

                                     case '\'':
                                     case '\"':
                                          if (n < format.length)
                                              n++;
                                          break;

                                     case '\\':
                                          if (n < format.length)
                                              result ~= format[n++];
                                          break;

                                     default:
                                          result ~= c;
                                          break;
                                     }
                              }
                        return result.get;
                }


                /**************************************************************

                **************************************************************/

                private void round (int pos)
                {
                        int index;
                        while (index < pos && digits[index] != '\0')
                               index++;

                        if (index == pos && digits[index] >= '5')
                           {
                           while (index > 0 && digits[index - 1] == '9')
                                  index--;

                           if (index > 0)
                               digits[index - 1]++;
                           else
                              {
                              scale++;
                              digits[0] = '1';
                              index = 1;
                              }
                           }
                        else
                           while (index > 0 && digits[index - 1] == '0')
                                  index--;

                        if (index == 0)
                           {
                           scale = 0;
                           sign = false;
                           }

                        digits[index] = '\0';
                }

        }


        /**********************************************************************

        **********************************************************************/

        struct Result
        {
                private uint    index;
                private T[]     target_;
                private T[]     convert;

                /**************************************************************

                **************************************************************/

                static Result opCall (T[] target, T[] convert)
                {
                        Result result;

                        result.target_ = target;
                        result.convert = convert;
                        return result;
                }

                /**************************************************************

                **************************************************************/

                void opCatAssign (T[] rhs)
                {
                        uint end = index + rhs.length;

                        target_[index .. end] = rhs;
                        index = end;
                }

                /**************************************************************

                **************************************************************/

                void opCatAssign (T rhs)
                {
                        target_[index++] = rhs;
                }

                /**************************************************************

                **************************************************************/

                T[] get ()
                {
                        return target_[0..index];
                }

                /**************************************************************

                **************************************************************/

                T[] scratch ()
                {
                        return target_;
                }
        }

}                               


/*******************************************************************************

*******************************************************************************/

private class TextAttributes(T) : NumericTextAttributes!(T)
{
        /**********************************************************************

        **********************************************************************/

        final T[] negativeSign ()
        {
                return "-";
        }

        /**********************************************************************

        **********************************************************************/

        final T[] positiveSign ()
        {
                return "+";
        }

        /**********************************************************************

        **********************************************************************/

        final T[] numberDecimalSeparator ()
        {
                return ".";
        }

        /**********************************************************************

        **********************************************************************/

        final T[] numberGroupSeparator ()
        {
                return ",";
        }

        /**********************************************************************

        **********************************************************************/

        final T[] currencyGroupSeparator ()
        {
                return ",";
        }

        /**********************************************************************

        **********************************************************************/

        final T[] currencyDecimalSeparator ()
        {
                return ".";
        }

        /**********************************************************************

        **********************************************************************/

        final T[] currencySymbol ()
        {
                return "\u00A4";
        }

        /**********************************************************************

        **********************************************************************/

        final T[] negativeInfinitySymbol ()
        {
                return "-infinity";
        }

        /**********************************************************************

        **********************************************************************/

        final T[] positiveInfinitySymbol ()
        {
                return "+infinity";
        }

        /**********************************************************************

        **********************************************************************/

        final T[] nanSymbol ()
        {
                return "NaN";
        }
}




/*******************************************************************************

*******************************************************************************/

class NumberFormat : NumericAttributes
{
        private NumericTextAttributes!(char)  cAttr;
        private NumericTextAttributes!(wchar) wAttr;
        private NumericTextAttributes!(dchar) dAttr;

        /**********************************************************************

        **********************************************************************/

        this ()
        {
                cAttr = new TextAttributes!(char);
                wAttr = new TextAttributes!(wchar);
                dAttr = new TextAttributes!(dchar);
        }
        
        /**********************************************************************

        **********************************************************************/

        final int numberDecimalDigits ()
        {
                return 2;
        }

        /**********************************************************************

        **********************************************************************/

        final int[] numberGroupSizes ()
        {
                static int[] groups = [3, 3, 3];
                return groups;
        }

        /**********************************************************************

        **********************************************************************/

        final int numberNegativePattern ()
        {
                return 1;
        }

        /**********************************************************************

        **********************************************************************/

        final int currencyNegativePattern ()
        {
                return 1;
        }

        /**********************************************************************

        **********************************************************************/

        final int currencyPositivePattern ()
        {
                return 0;
        }

        /**********************************************************************

        **********************************************************************/

        final int[] currencyGroupSizes ()
        {
                static int[] groups = [3, 3, 3];
                return groups;
        }

        /**********************************************************************

        **********************************************************************/

        final int currencyDecimalDigits ()
        {
                return 2;
        }

        /**********************************************************************

        **********************************************************************/

        final NumericTextAttributes!(char) utf8()
        {
                return cAttr;
        }

        /**********************************************************************

        **********************************************************************/

        final NumericTextAttributes!(wchar) utf16()
        {
                return wAttr;
        }

        /**********************************************************************

        **********************************************************************/

        final NumericTextAttributes!(dchar) utf32()
        {
                return dAttr;
        }
}


/*******************************************************************************

*******************************************************************************/

template charTerm (T)
{

        /**********************************************************************

        **********************************************************************/

        private int charTerm(T* s)
        {
                int i;
                while (*s++ != '\0')
                        i++;
                return i;
        }

}

/*******************************************************************************

*******************************************************************************/

private enum TypeCode
        {
        EMPTY = 0,
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
        POINTER = 'P',
        TYPEDEF = 'T',
        }

/*******************************************************************************

*******************************************************************************/

// Wraps a variadic argument.
private struct Argument
{
        private TypeInfo        type_;
        private void*           value_;
        private TypeCode        typeCode_;

        /**********************************************************************

        **********************************************************************/

        public static Argument opCall (TypeInfo type, void* value)
        {
                Argument arg;

                arg.value_ = value;
                arg.setType (type);
                return arg;
        }

        /**********************************************************************

        **********************************************************************/

        public final TypeInfo setType (TypeInfo type)
        {
                typeCode_ = cast(TypeCode)type.classinfo.name[9];
                type_ = type;
                return type;
        }

        /**********************************************************************

        **********************************************************************/

        public final TypeInfo getType ()
        {
                return type_;
        }

        /**********************************************************************

        **********************************************************************/

        public final TypeCode getTypeCode ()
        {
                return typeCode_;
        }
        /**********************************************************************

        **********************************************************************/

        public final void* getValue ()
        {
                return value_;
        }
}


/*******************************************************************************

        Wraps variadic arguments

*******************************************************************************/

private struct ArgumentIterator
{
        private int             size_;
        private Argument[32]    args_ = void;

        /**********************************************************************

        **********************************************************************/

        public static ArgumentIterator opCall (TypeInfo[] types, void* argptr)
        {
                ArgumentIterator it;

                foreach (TypeInfo type; types)
                        {
                        it.args_[it.size_] = Argument(type, argptr);
                        argptr += (type.tsize() + int.sizeof - 1) & ~ (int.sizeof - 1);

                        if (it.size_++ >= args_.length)
                            throw new IllegalArgumentException ("too many arguments");
                        }
                return it;
        }

        /**********************************************************************

        **********************************************************************/

        public Argument opIndex (int index)
        {
                return args_[index];
        }

        /**********************************************************************

        **********************************************************************/

        public int count ()
        {
                return size_;
        }

        /**********************************************************************

        **********************************************************************/

        public int opApply (int delegate(inout Argument) del)
        {
                int r;
                foreach (Argument arg; args_)
                        {
                        if ((r = del(arg)) != 0)
                             break;
                        }
                return r;
        }
}




/*******************************************************************************

*******************************************************************************/

debug (UnitTest)
{
unittest{

    assert( Formatter( "abc" ) == "abc" );

    assert( Formatter( "{0}", 1 ) == "1" );
    assert( Formatter( "{0}", -1 ) == "-1" );

    assert( Formatter( "{0}", true ) == "true" , Formatter( "{0}", true ));
    assert( Formatter( "{0}", false ) == "false" );

    assert( Formatter( "{0}", cast(byte)-128 ) == "-128" );
    assert( Formatter( "{0}", cast(byte)127 ) == "127" );
    assert( Formatter( "{0}", cast(ubyte)255 ) == "255" );

    assert( Formatter( "{0}", cast(short)-32768  ) == "-32768" );
    assert( Formatter( "{0}", cast(short)32767 ) == "32767" );
    assert( Formatter( "{0}", cast(ushort)65535 ) == "65535" );
    // assert( Formatter( "{0:x4}", cast(ushort)0xafe ) == "0afe" );
    // assert( Formatter( "{0:X4}", cast(ushort)0xafe ) == "0AFE" );

    assert( Formatter( "{0}", -2147483648 ) == "-2147483648" );
    assert( Formatter( "{0}", 2147483647 ) == "2147483647" );
    assert( Formatter( "{0}", 4294967295 ) == "4294967295" );
    // compiler error
    //assert( Formatter( "{0}", -9223372036854775808L) == "-9223372036854775808" );
    assert( Formatter( "{0}", 0x8000_0000_0000_0000L) == "-9223372036854775808" );
    assert( Formatter( "{0}", 9223372036854775807L ) == "9223372036854775807" );
    // Error: prints -1
    // assert( Formatter( "{0}", 18446744073709551615UL ) == "18446744073709551615" );

    assert( Formatter( "{0}", "s" ) == "s" );
    // fragments before and after
    assert( Formatter( "d{0}d", "s" ) == "dsd" );
    assert( Formatter( "d{0}d", "1234567890" ) == "d1234567890d" );

    // brace escaping
    assert( Formatter( "d{0}d", "<string>" ) == "d<string>d");
    assert( Formatter( "d{{0}d", "<string>" ) == "d{0}d");
    assert( Formatter( "d{{{0}d", "<string>" ) == "d{<string>d");
    assert( Formatter( "d{0}}d", "<string>" ) == "d<string>}d");

    assert( Formatter( "{0:x}", 0xafe0000 ) == "afe0000" );
    // todo: is it correct to print 7 instead of 6 chars???
    assert( Formatter( "{0:x6}", 0xafe0000 ) == "afe0000" );
    assert( Formatter( "{0:x8}", 0xafe0000 ) == "0afe0000" );
    assert( Formatter( "{0:X8}", 0xafe0000 ) == "0AFE0000" );
    assert( Formatter( "{0:X9}", 0xafe0000 ) == "00AFE0000" );
    assert( Formatter( "{0:X13}", 0xafe0000 ) == "000000AFE0000" );
    assert( Formatter( "{0:x13}", 0xafe0000 ) == "000000afe0000" );
    // decimal width
    assert( Formatter( "{0:d6}", 123 ) == "000123" );
    assert( Formatter( "{0,7:d6}", 123 ) == " 000123" );
    assert( Formatter( "{0,-7:d6}", 123 ) == "000123 " );

    assert( Formatter( "{0:d6}", -123 ) == "-000123" );
    assert( Formatter( "{0,7:d6}", 123 ) == " 000123" );
    assert( Formatter( "{0,7:d6}", -123 ) == "-000123" );
    assert( Formatter( "{0,8:d6}", -123 ) == " -000123" );
    assert( Formatter( "{0,5:d6}", -123 ) == "-000123" );

    // compiler error
    //assert( Formatter( "{0}", -9223372036854775808L) == "-9223372036854775808" );
    assert( Formatter( "{0}", 0x8000_0000_0000_0000L) == "-9223372036854775808" );
    assert( Formatter( "{0}", 9223372036854775807L ) == "9223372036854775807" );
    assert( Formatter( "{0:X}", 0xFFFF_FFFF_FFFF_FFFF) == "FFFFFFFFFFFFFFFF" );
    assert( Formatter( "{0:x}", 0xFFFF_FFFF_FFFF_FFFF) == "ffffffffffffffff" );
    assert( Formatter( "{0:x}", 0xFFFF_1234_FFFF_FFFF) == "ffff1234ffffffff" );
    assert( Formatter( "{0:x19}", 0x1234_FFFF_FFFF) == "00000001234ffffffff" );
    // Error: prints -1
    // assert( Formatter( "{0}", 18446744073709551615UL ) == "18446744073709551615" );
    assert( Formatter( "{0}", "s" ) == "s" );
    // fragments before and after
    assert( Formatter( "d{0}d", "s" ) == "dsd" );

    // argument index
    assert( Formatter( "a{0}b{1}c{2}", "x", "y", "z" ) == "axbycz" );
    assert( Formatter( "a{2}b{1}c{0}", "x", "y", "z" ) == "azbycx" );
    assert( Formatter( "a{1}b{1}c{1}", "x", "y", "z" ) == "aybycy" );

    // alignment
    // align does not restrict the length
    assert( Formatter( "{0,5}", "hellohello" ) == "hellohello" );
    // align fills with spaces
    assert( Formatter( "->{0,-10}<-", "hello" ) == "->hello     <-" );
    assert( Formatter( "->{0,10}<-", "hello" ) == "->     hello<-" );
    assert( Formatter( "->{0,-10}<-", 12345 ) == "->12345     <-" );
    assert( Formatter( "->{0,10}<-", 12345 ) == "->     12345<-" );

    assert( Formatter("General: {0} Hexadecimal: 0x{0:x4} Numeric: {0:N}", 1000)
            == "General: 1000 Hexadecimal: 0x03e8 Numeric: 1,000.00" );
    /+ Not yet implemented +/ //assert( Formatter(Culture.getCulture("de-DE"), "{0:#,#}", 12345678)
    /+ Not yet implemented +/ //        == "12.345.678" );
    /+ Not yet implemented +/ //assert( Formatter(Culture.getCulture("es-ES"), "{0:C}", 59.99)
    /+ Not yet implemented +/ //        == "59,99 €" );
    /+ Not yet implemented +/ //assert( Formatter(Culture.getCulture("fr-FR"), "{0:D}", DateTime.today)
    /+ Not yet implemented +/ //        == "vendredi 3 mars 2006" );

    version( mlfp ){
        assert( Formatter( "{0:f}", 1.23f ) == "1.23" );
        assert( Formatter( "{0:f}", 1.23 ) == "1.23" );
    }
}
}


static Format!(char) Formatter;

static this()
{
        Formatter = new Format!(char);
}




version (Test) {
import tango.io.Console;


void main ()
{
        Cout (Formatter ("0x{0:x4} {1} bottles", 10, "green")) ();
}


} // version(Test)
