/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
        
        version:        Nov 2005: Initial release
                        Jan 2010: added internal ecvt() 

        author:         Kris

        A set of functions for converting between string and floating-
        point values.

        Applying the D "import alias" mechanism to this module is highly
        recommended, in order to limit namespace pollution:
        ---
        import Float = tango.text.convert.Float;

        auto f = Float.parse ("3.14159");
        ---
        
*******************************************************************************/

module tango.text.convert.Float;

private import tango.core.Exception;

/******************************************************************************

        select an internal version
                
******************************************************************************/

version = float_internal;

private alias real NumType;

/******************************************************************************

        optional math functions
                
******************************************************************************/

private extern (C)
{
		version (Escape)
		{
			private import tango.stdc.math;
			
			/**
			 * Since all usages of log10 cast the result to an integer, we can use a very
			 * simple version here.
			 */
			real log10l(real x)
			{
				real res = 0;
				while(x >= 10)
				{
					x /= 10;
					res += 1;
				}
				return res;
			}
			
			real ceill(real num)
			{
				long inum = cast(long)num;
				if(inum == num || inum < 0)
					return inum;
				return inum + 1;
			}
			
			real modfl(real num,real *i)
			{
				if(num is real.nan)
				{
					*i = real.nan;
					return real.nan;
				}
				if(num is real.infinity)
				{
					*i = 0;
					return real.infinity;
				}
				if(num is -real.infinity)
				{
					*i = -0;
					return -real.infinity;
				}
				long inum = cast(long)num;
				*i = inum;
				real res = num - inum;
				return res;
			}
			
			/**
			 * In all usages exp is an integer (positive or negative). So we can provide a
			 * simply version here
			 */
			real powl(real base,real exp)
			{
				real res = 1;
				if(exp >= 0)
				{
					while(exp-- > 0)
						res *= base;
				}
				else
				{
					while(exp++ < 0)
						res /= base;
				}
				return res;
			}
			
			unittest
			{
				assert(log10l(4) == 0);
				assert(log10l(10) == 1);
				assert(log10l(100) == 2);
				assert(log10l(123) == 2);
				assert(log10l(5812) == 3);
				
				assert(ceill(0.5) == 1);
				assert(ceill(0.2) == 1);
				assert(ceill(41.2) == 42);
				assert(ceill(-13.9) == -13);
				assert(ceill(-11) == -11);
				assert(ceill(123) == 123);
				assert(ceill(0) == 0);

				assert(powl(10.,3.) == 1000.);
				assert(powl(10.,1.) == 10.);
				assert(powl(10.,0.) == 1.);
				assert(powl(10.,-1.) == 0.1);
				assert(powl(10.,-3.) == 0.001);
			}
		}
		else
		{
	        real log10l (real x);
	        real ceill (real num);
	        real modfl (real num, real *i);
	        real powl  (real base, real exp);
		}

        int printf (char*, ...);
        version (Windows)
	    {
		    alias ecvt econvert;
		    alias ecvt fconvert;
	    }
}

/******************************************************************************

        Constants
                
******************************************************************************/

private enum 
{
        Pad = 0,                // default trailing decimal zero
        Dec = 2,                // default decimal places
        Exp = 10,               // default switch to scientific notation
}

/******************************************************************************

        Convert a formatted string of digits to a floating-point
        number. Throws an exception where the input text is not
        parsable in its entirety.
        
******************************************************************************/

NumType toFloat(T) (T[] src)
{
        uint len;

        auto x = parse (src, &len);
        if (len < src.length || len == 0)
            throw new IllegalArgumentException ("Float.toFloat :: invalid number");
        return x;
}

/******************************************************************************

        Template wrapper to make life simpler. Returns a text version
        of the provided value.

        See format() for details

******************************************************************************/

char[] toString (NumType d, uint decimals=Dec, int e=Exp)
{
        char[64] tmp = void;
        
        return format (tmp, d, decimals, e).dup;
}
               
/******************************************************************************

        Template wrapper to make life simpler. Returns a text version
        of the provided value.

        See format() for details

******************************************************************************/

wchar[] toString16 (NumType d, uint decimals=Dec, int e=Exp)
{
        wchar[64] tmp = void;
        
        return format (tmp, d, decimals, e).dup;
}
               
/******************************************************************************

        Template wrapper to make life simpler. Returns a text version
        of the provided value.

        See format() for details

******************************************************************************/

dchar[] toString32 (NumType d, uint decimals=Dec, int e=Exp)
{
        dchar[64] tmp = void;
        
        return format (tmp, d, decimals, e).dup;
}
               
/******************************************************************************

        Truncate trailing '0' and '.' from a string, such that 200.000 
        becomes 200, and 20.10 becomes 20.1

        Returns a potentially shorter slice of what you give it.

******************************************************************************/

T[] truncate(T) (T[] s)
{
        auto tmp = s;
        int i = tmp.length;
        foreach (int idx, T c; tmp)
                 if (c is '.')
                     while (--i >= idx)
                            if (tmp[i] != '0')
                               {  
                               if (tmp[i] is '.')
                                   --i;
                               s = tmp [0 .. i+1];
                               while (--i >= idx)
                                      if (tmp[i] is 'e')
                                          return tmp;
                               break;
                               }
        return s;
}

/******************************************************************************

        Extract a sign-bit

******************************************************************************/

private bool negative (NumType x)
{
        static if (NumType.sizeof is 4) 
                   return ((*cast(uint *)&x) & 0x8000_0000) != 0;
        else
           static if (NumType.sizeof is 8) 
                      return ((*cast(ulong *)&x) & 0x8000_0000_0000_0000) != 0;
                else
                   {
                   auto pe = cast(ubyte *)&x;
                   return (pe[9] & 0x80) != 0;
                   }
}


/******************************************************************************

        Convert a floating-point number to a string. 

        The e parameter controls the number of exponent places emitted, 
        and can thus control where the output switches to the scientific 
        notation. For example, setting e=2 for 0.01 or 10.0 would result
        in normal output. Whereas setting e=1 would result in both those
        values being rendered in scientific notation instead. Setting e
        to 0 forces that notation on for everything. Parameter pad will
        append trailing '0' decimals when set ~ otherwise trailing '0's 
        will be elided

******************************************************************************/

T[] format(T, D=NumType, U=int) (T[] dst, D x, U decimals=Dec, U e=Exp, bool pad=Pad)
{return format!(T)(dst, x, decimals, e, pad);}

T[] format(T) (T[] dst, NumType x, int decimals=Dec, int e=Exp, bool pad=Pad)
{
        char*           end,
                        str;
        int             exp,
                        sign,
                        mode=5;
        char[32]        buf = void;

        // test exponent to determine mode
        exp = (x is 0) ? 1 : cast(int) log10l (x < 0 ? -x : x);
        if (exp <= -e || exp >= e)
            mode = 2, ++decimals;

version (float_internal)
         str = convertl (buf.ptr, x, decimals, &exp, &sign, mode is 5);
version (float_dtoa)
         str = dtoa (x, mode, decimals, &exp, &sign, &end);
version (float_lib)
        {
        if (mode is 5)
            str = fconvert (x, decimals, &exp, &sign);
        else
           str = econvert (x, decimals, &exp, &sign);
        }

        auto p = dst.ptr;
        if (sign)
            *p++ = '-';

        if (exp is 9999)
            while (*str) 
                   *p++ = *str++;
        else
           {
           if (mode is 2)
              {
              --exp;
              *p++ = *str++;
              if (*str || pad)
                 {
                 auto d = p;
                 *p++ = '.';
                 while (*str)
                        *p++ = *str++;
                 if (pad)
                     while (p-d < decimals)
                            *p++ = '0';
                 }
              *p++ = 'e';
              if (exp < 0)
                  *p++ = '-', exp = -exp;
              else
                 *p++ = '+';
              if (exp >= 1000)
                 {
                 *p++ = cast(T)((exp/1000) + '0');
                 exp %= 1000;
                 }
              if (exp >= 100)
                 {
                 *p++ = exp / 100 + '0';
                 exp %= 100;
                 }
              *p++ = exp / 10 + '0';
              *p++ = exp % 10 + '0';
              }
           else
              {
              if (exp <= 0)
                  *p++ = '0';
              else
                 for (; exp > 0; --exp)
                        *p++ = (*str) ? *str++ : '0';
              if (*str || pad)
                 {
                 *p++ = '.';
                 auto d = p;
                 for (; exp < 0; ++exp)
                        *p++ = '0';
                 while (*str)
                        *p++ = *str++;
                 if (pad)
                     while (p-d < decimals)
                            *p++ = '0';
                 }
              } 
           }

        // stuff a C terminator in there too ...
        *p = 0;
        return dst[0..(p - dst.ptr)];
}


/******************************************************************************

        ecvt() and fcvt() for 80bit FP, which DMD does not include. Based
        upon the following:

        Copyright (c) 2009 Ian Piumarta
        
        All rights reserved.

        Permission is hereby granted, free of charge, to any person 
        obtaining a copy of this software and associated documentation 
        files (the 'Software'), to deal in the Software without restriction, 
        including without limitation the rights to use, copy, modify, merge, 
        publish, distribute, and/or sell copies of the Software, and to permit 
        persons to whom the Software is furnished to do so, provided that the 
        above copyright notice(s) and this permission notice appear in all 
        copies of the Software.  

******************************************************************************/

version (float_internal)
{
private char *convertl (char* buf, real value, int ndigit, int *decpt, int *sign, int fflag)
{
        if ((*sign = negative(value)) != 0)
             value = -value;

        *decpt = 9999;
        if (value !<>= value)
            return "nan\0";

        if (value is value.infinity)
            return "inf\0";

        int exp10 = (value is 0) ? !fflag : cast(int) ceill(log10l(value));
        if (exp10 < -4931) 
            exp10 = -4931;	
        value *= powl (10.0, -exp10);
        if (value) 
           {
           while (value <  0.1) { value *= 10;  --exp10; }
           while (value >= 1.0) { value /= 10;  ++exp10; }
           }
        assert(value is 0 || (0.1 <= value && value < 1.0));
        //auto zero = pad ? int.max : 1;
        auto zero = 1;
        if (fflag) 
           {
           // if (! pad)
                 zero = exp10;
           if (ndigit + exp10 < 0) 
              {
              *decpt= -ndigit;
              return "\0";
              }
           ndigit += exp10;
           }
        *decpt = exp10;
        int ptr = 1;

        if (ndigit > real.dig) 
            ndigit = real.dig;
        //printf ("< flag %d, digits %d, exp10 %d, decpt %d\n", fflag, ndigit, exp10, *decpt);
        while (ptr <= ndigit) 
              {
              real i = void;
              value = modfl (value * 10, &i);
              buf [ptr++]= '0' + cast(int) i;
              }

        if (value >= 0.5)
            while (--ptr && ++buf[ptr] > '9')
                   buf[ptr] = (ptr > zero) ? 0 : '0';
        else
           for (auto i=ptr; i && --i > zero && buf[i] is '0';)
                buf[i] = 0;

        if (ptr) 
           {
           buf [ndigit + 1]= 0;
           return buf + 1;
           }
        if (fflag) 
           {
           ++ndigit;
           ++*decpt;
           }
        buf[0]= '1';
        buf[ndigit]= 0;
        return buf;
}
}


/******************************************************************************

        David Gay's extended conversions between string and floating-point
        numeric representations. Use these where you need extended accuracy
        for convertions. 

        Note that this class requires the attendent file dtoa.c be compiled 
        and linked to the application

******************************************************************************/

version (float_dtoa)
{
        private extern(C)
        {
        // these should be linked in via dtoa.c
        double strtod (char* s00, char** se);
        char*  dtoa (double d, int mode, int ndigits, int* decpt, int* sign, char** rve);
        }

        /**********************************************************************

                Convert a formatted string of digits to a floating-
                point number. 

        **********************************************************************/

        NumType parse (char[] src, uint* ate=null)
        {
                char* end;

                auto value = strtod (src.ptr, &end);
                assert (end <= src.ptr + src.length);
                if (ate)
                    *ate = end - src.ptr;
                return value;
        }

        /**********************************************************************

                Convert a formatted string of digits to a floating-
                point number.

        **********************************************************************/

        NumType parse (wchar[] src, uint* ate=null)
        {
                // cheesy hack to avoid pre-parsing :: max digits == 100
                char[100] tmp = void;
                auto p = tmp.ptr;
                auto e = p + tmp.length;
                foreach (c; src)
                         if (p < e && (c & 0x80) is 0)
                             *p++ = c;                        
                         else
                            break;

                return parse (tmp[0..p-tmp.ptr], ate);
        }

        /**********************************************************************

                Convert a formatted string of digits to a floating-
                point number. 

        **********************************************************************/

        NumType parse (dchar[] src, uint* ate=null)
        {
                // cheesy hack to avoid pre-parsing :: max digits == 100
                char[100] tmp = void;
                auto p = tmp.ptr;
                auto e = p + tmp.length;
                foreach (c; src)
                         if (p < e && (c & 0x80) is 0)
                             *p++ = c;
                         else
                            break;
                return parse (tmp[0..p-tmp.ptr], ate);
        }
}
else
{
private import Integer = tango.text.convert.Integer;

/******************************************************************************

        Convert a formatted string of digits to a floating-point number.
        Good for general use, but use David Gay's dtoa package if serious
        rounding adjustments should be applied.

******************************************************************************/

NumType parse(T) (T[] src, uint* ate=null)
{
        T               c;
        T*              p;
        int             exp;
        bool            sign;
        uint            radix;
        NumType         value = 0.0;

        static bool match (T* aa, T[] bb)
        {
                foreach (b; bb)
                        {
                        auto a = *aa++;
                        if (a >= 'A' && a <= 'Z')
                            a += 'a' - 'A';
                        if (a != b)
                            return false;
                        }
                return true;
        }

        // remove leading space, and sign
        p = src.ptr + Integer.trim (src, sign, radix);

        // bail out if the string is empty
        if (src.length is 0 || p > &src[$-1])
            return NumType.nan;
        c = *p;

        // handle non-decimal representations
        if (radix != 10)
           {
           long v = Integer.parse (src, radix, ate); 
           return cast(NumType) v;
           }

        // set begin and end checks
        auto begin = p;
        auto end = src.ptr + src.length;

        // read leading digits; note that leading
        // zeros are simply multiplied away
        while (c >= '0' && c <= '9' && p < end)
              {
              value = value * 10 + (c - '0');
              c = *++p;
              }

        // gobble up the point
        if (c is '.' && p < end)
            c = *++p;

        // read fractional digits; note that we accumulate
        // all digits ... very long numbers impact accuracy
        // to a degree, but perhaps not as much as one might
        // expect. A prior version limited the digit count,
        // but did not show marked improvement. For maximum
        // accuracy when reading and writing, use David Gay's
        // dtoa package instead
        while (c >= '0' && c <= '9' && p < end)
              {
              value = value * 10 + (c - '0');
              c = *++p;
              --exp;
              } 

        // did we get something?
        if (p > begin)
           {
           // parse base10 exponent?
           if ((c is 'e' || c is 'E') && p < end )
              {
              uint eaten;
              exp += Integer.parse (src[(++p-src.ptr) .. $], 0, &eaten);
              p += eaten;
              }

           // adjust mantissa; note that the exponent has
           // already been adjusted for fractional digits
           if (exp < 0)
               value /= pow10 (-exp);
           else
              value *= pow10 (exp);
           }
        else
           if (end - p >= 3)
               switch (*p)
                      {
                      case 'I': case 'i':
                           if (match (p+1, "nf"))
                              {
                              value = value.infinity;
                              p += 3;
                              if (end - p >= 5 && match (p, "inity"))
                                  p += 5;
                              }
                           break;

                      case 'N': case 'n':
                           if (match (p+1, "an"))
                              {
                              value = value.nan;
                              p += 3;
                              }
                           break;
                      default:
                           break;
                      }

        // set parse length, and return value
        if (ate)
            *ate = p - src.ptr;

        if (sign)
            value = -value;
        return value;
}

/******************************************************************************

        Internal function to convert an exponent specifier to a floating
        point value.

******************************************************************************/

private NumType pow10 (uint exp)
{
        static  NumType[] Powers = 
                [
                1.0e1L,
                1.0e2L,
                1.0e4L,
                1.0e8L,
                1.0e16L,
                1.0e32L,
                1.0e64L,
                1.0e128L,
                1.0e256L,
                1.0e512L,
                1.0e1024L,
                1.0e2048L,
                1.0e4096L,
                1.0e8192L,
                ];

        if (exp >= 16384)
            throw new IllegalArgumentException ("Float.pow10 :: exponent too large");

        NumType mult = 1.0;
        foreach (NumType power; Powers)
                {
                if (exp & 1)
                    mult *= power;
                if ((exp >>= 1) is 0)
                     break;
                }
        return mult;
}
}

version (float_old)
{
/******************************************************************************

        Convert a float to a string. This produces pretty good results
        for the most part, though one should use David Gay's dtoa package
        for best accuracy.

        Note that the approach first normalizes a base10 mantissa, then
        pulls digits from the left side whilst emitting them (rightward)
        to the output.

        The e parameter controls the number of exponent places emitted, 
        and can thus control where the output switches to the scientific 
        notation. For example, setting e=2 for 0.01 or 10.0 would result
        in normal output. Whereas setting e=1 would result in both those
        values being rendered in scientific notation instead. Setting e
        to 0 forces that notation on for everything.

        TODO: this should be replaced, as it is not sufficiently accurate 

******************************************************************************/

T[] format(T, D=double, U=uint) (T[] dst, D x, U decimals=Dec, int e=Exp, bool pad=Pad)
{return format!(T)(dst, x, decimals, e, pad);}

T[] format(T) (T[] dst, NumType x, uint decimals=Dec, int e=Exp, bool pad=Pad)
{
        static T[] inf = "-inf";
        static T[] nan = "-nan";

        // strip digits from the left of a normalized base-10 number
        static int toDigit (ref NumType v, ref int count)
        {
                int digit;

                // Don't exceed max digits storable in a real
                // (-1 because the last digit is not always storable)
                if (--count <= 0)
                    digit = 0;
                else
                   {
                   // remove leading digit, and bump
                   digit = cast(int) v;
                   v = (v - digit) * 10.0;
                   }
                return digit + '0';
        }

        // extract the sign
        bool sign = negative (x);
        if (sign)
            x = -x;

        if (x !<>= x)
            return sign ? nan : nan[1..$];

        if (x is x.infinity)
            return sign ? inf : inf[1..$];

        // assume no exponent
        int exp = 0;
        int abs = 0;

        // don't scale if zero
        if (x > 0.0)
           {
           // extract base10 exponent
           exp = cast(int) log10l (x);

           // round up a bit
           auto d = decimals;
           if (exp < 0)
               d -= exp;
           x += 0.5 / pow10 (d);

           // normalize base10 mantissa (0 < m < 10)
           abs = exp = cast(int) log10l (x);
           if (exp > 0)
               x /= pow10 (exp);
           else
              abs = -exp;

           // switch to exponent display as necessary
           if (abs >= e)
               e = 0; 
           }

        T* p = dst.ptr;
        int count = NumType.dig;

        // emit sign
        if (sign)
            *p++ = '-';
        
        // are we doing +/-exp format?
        if (e is 0)
           {
           assert (dst.length > decimals + 7);

           if (exp < 0)
               x *= pow10 (abs+1);

           // emit first digit, and decimal point
           *p++ = cast(T) toDigit (x, count);
           if (decimals)
              {
              *p++ = '.';

              // emit rest of mantissa
              while (decimals-- > 0)
                     *p++ = cast(T) toDigit (x, count);
              
              if (pad is false)
                 {
                 while (*(p-1) is '0')
                        --p;
                 if (*(p-1) is '.')
                     --p;
                 }
              }

           // emit exponent, if non zero
           if (abs)
              {
              *p++ = 'e';
              *p++ = (exp < 0) ? '-' : '+';
              if (abs >= 1000)
                 {
                 *p++ = cast(T)((abs/1000) + '0');
                 abs %= 1000;
                 *p++ = cast(T)((abs/100) + '0');
                 abs %= 100;
                 }
              else
                 if (abs >= 100)
                    {
                    *p++ = cast(T)((abs/100) + '0');
                    abs %= 100;
                    }
              *p++ = cast(T)((abs/10) + '0');
              *p++ = cast(T)((abs%10) + '0');
              }
           }
        else
           {
           assert (dst.length >= (((exp < 0) ? 0 : exp) + decimals + 1));

           // if fraction only, emit a leading zero
           if (exp < 0)
              {
              x *= pow10 (abs);
              *p++ = '0';
              }
           else
              // emit all digits to the left of point
              for (; exp >= 0; --exp)
                     *p++ = cast(T )toDigit (x, count);

           // emit point
           if (decimals)
              {
              *p++ = '.';

              // emit leading fractional zeros?
              for (++exp; exp < 0 && decimals > 0; --decimals, ++exp)
                   *p++ = '0';

              // output remaining digits, if any. Trailing
              // zeros are also returned from toDigit()
              while (decimals-- > 0)
                     *p++ = cast(T) toDigit (x, count);

              if (pad is false)
                 {
                 while (*(p-1) is '0')
                        --p;
                 if (*(p-1) is '.')
                     --p;
                 }
              }
           }

        return dst [0..(p - dst.ptr)];
}
}

/******************************************************************************

******************************************************************************/

debug (UnitTest)
{
        import tango.io.Console;
      
        unittest
        {
                char[164] tmp;

                auto f = parse ("nan");
                assert (format(tmp, f) == "nan");
                f = parse ("inf");
                assert (format(tmp, f) == "inf");
                f = parse ("-nan");
                assert (format(tmp, f) == "-nan");
                f = parse (" -inf");
                assert (format(tmp, f) == "-inf");

                assert (format (tmp, 3.14159, 6) == "3.14159");
                assert (format (tmp, 3.14159, 4) == "3.1416");
                assert (parse ("3.5") == 3.5);
                assert (format(tmp, parse ("3.14159"), 6) == "3.14159");
        }
}


debug (Float)
{
        import tango.io.Console;

        void main() 
        {
                char[500] tmp;
/+
                Cout (format(tmp, NumType.max)).newline;
                Cout (format(tmp, -NumType.nan)).newline;
                Cout (format(tmp, -NumType.infinity)).newline;
                Cout (format(tmp, toFloat("nan"w))).newline;
                Cout (format(tmp, toFloat("-nan"d))).newline;
                Cout (format(tmp, toFloat("inf"))).newline;
                Cout (format(tmp, toFloat("-inf"))).newline;
+/
                Cout (format(tmp, toFloat ("0.000000e+00"))).newline;
                Cout (format(tmp, toFloat("0x8000000000000000"))).newline;
                Cout (format(tmp, 1)).newline;
                Cout (format(tmp, -0)).newline;
                Cout (format(tmp, 0.000001)).newline.newline;

                Cout (format(tmp, 3.14159, 6, 0)).newline;
                Cout (format(tmp, 3.0e10, 6, 3)).newline;
                Cout (format(tmp, 314159, 6)).newline;
                Cout (format(tmp, 314159123213, 6, 15)).newline;
                Cout (format(tmp, 3.14159, 6, 2)).newline;
                Cout (format(tmp, 3.14159, 3, 2)).newline;
                Cout (format(tmp, 0.00003333, 6, 2)).newline;
                Cout (format(tmp, 0.00333333, 6, 3)).newline;
                Cout (format(tmp, 0.03333333, 6, 2)).newline;
                Cout.newline;

                Cout (format(tmp, -3.14159, 6, 0)).newline;
                Cout (format(tmp, -3e100, 6, 3)).newline;
                Cout (format(tmp, -314159, 6)).newline;
                Cout (format(tmp, -314159123213, 6, 15)).newline;
                Cout (format(tmp, -3.14159, 6, 2)).newline;
                Cout (format(tmp, -3.14159, 2, 2)).newline;
                Cout (format(tmp, -0.00003333, 6, 2)).newline;
                Cout (format(tmp, -0.00333333, 6, 3)).newline;
                Cout (format(tmp, -0.03333333, 6, 2)).newline;
                Cout.newline;

                Cout (format(tmp, -0.9999999, 7, 3)).newline;
                Cout (format(tmp, -3.0e100, 6, 3)).newline;
                Cout ((format(tmp, 1.0, 6))).newline;
                Cout ((format(tmp, 30, 6))).newline;
                Cout ((format(tmp, 3.14159, 6, 0))).newline;
                Cout ((format(tmp, 3e100, 6, 3))).newline;
                Cout ((format(tmp, 314159, 6))).newline;
                Cout ((format(tmp, 314159123213.0, 3, 15))).newline;
                Cout ((format(tmp, 3.14159, 6, 2))).newline;
                Cout ((format(tmp, 3.14159, 4, 2))).newline;
                Cout ((format(tmp, 0.00003333, 6, 2))).newline;
                Cout ((format(tmp, 0.00333333, 6, 3))).newline;
                Cout ((format(tmp, 0.03333333, 6, 2))).newline;
                Cout (format(tmp, NumType.min, 6)).newline;
                Cout (format(tmp, -1)).newline;
                Cout (format(tmp, toFloat(format(tmp, -1)))).newline;
                Cout.newline;
        }
}
