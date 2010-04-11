/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
        
        version:        Initial release: Nov 2005

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

private import Integer = tango.text.convert.Integer;

private extern (C) double log10(double x);

/******************************************************************************

        Convert a formatted string of digits to a floating-point
        number. Throws an exception where the input text is not
        parsable in its entirety.
        
******************************************************************************/

double toDouble(T) (T[] src)
{
        uint len;

        auto x = parse (src, &len);
        if (len < src.length)
            throw new IllegalArgumentException ("invalid input:"~src);
        return x;
}

/******************************************************************************

        Template wrapper to make life simpler. Returns a text version
        of the provided value.

        See format() for details

******************************************************************************/

char[] toUtf8 (double d, uint decimals=6, bool scientific=false)
{
        char[64] tmp = void;
        
        return format (tmp, d, decimals, scientific).dup;
}
               
/******************************************************************************

        Template wrapper to make life simpler. Returns a text version
        of the provided value.

        See format() for details

******************************************************************************/

wchar[] toUtf16 (double d, uint decimals=6, bool scientific=false)
{
        wchar[64] tmp = void;
        
        return format (tmp, d, decimals, scientific).dup;
}
               
/******************************************************************************

        Template wrapper to make life simpler. Returns a text version
        of the provided value.

        See format() for details

******************************************************************************/

dchar[] toUtf32 (double d, uint decimals=6, bool scientific=false)
{
        dchar[64] tmp = void;
        
        return format (tmp, d, decimals, scientific).dup;
}
               
/******************************************************************************

        Convert a float to a string. This produces pretty good results
        for the most part, though one should use David Gay's dtoa package
        for best accuracy.

        Note that the approach first normalizes a base10 mantissa, then
        pulls digits from the left side whilst emitting them (rightward)
        to the output.

******************************************************************************/

T[] format(T, U=uint) (T[] dst, double x, U decimals = 6, bool scientific = false)
{
        assert (dst.length >= 32);

        // function to strip digits from the
        // left of a normalized base-10 number
        static int toDigit (inout double v, inout int count)
        {
                int digit;

                // double can reliably hold 17 digits only
                if (++count > 17)
                    digit = 0;
                else
                   {
                   // remove leading digit, and bump
                   digit = cast(int) v;
                   v = (v - digit) * 10.0;
                   }
                return digit + '0';
        }

        // test for nan/infinity
        if (((cast(ushort*) &x)[3] & 0x7ff0) == 0x7ff0)
              if (*(cast(ulong*) &x) & 0x000f_ffff_ffff_ffff)
                    return "nan";
              else
                 return "inf";

        int exp;
        bool sign;

        // extract the sign
        if (x < 0.0)
           {
           x = -x;
           sign = true;
           }

        // don't scale if zero
        if (x > 0.0)
           {
           // round up a bit (should do even/odd test?)
           x += 0.5 / pow10 (decimals);

           // extract base10 exponent
           exp = cast(int) log10 (x);

           // normalize base10 mantissa (0 < m < 10)
           int len = exp;
           if (exp < 0)
               x *= pow10 (len = -exp);
           else
              x /= pow10 (exp);

           // switch to short display if not enough space
           if (len + 32 > dst.length)
               scientific = true;
           }

        T* p = dst.ptr;
        int count = 0;

        // emit sign
        if (sign)
            *p++ = '-';

        // are we doing +/-exp format?
        if (scientific)
           {
           // emit first digit, and decimal point
           *p++ = toDigit (x, count);
           *p++ = '.';

           // emit rest of mantissa
           while (decimals-- > 0)
                  *p++ = toDigit (x, count);

           // emit exponent, if non zero
           if (exp)
              {
              *p++ = 'e';
              *p++ = (exp < 0) ? '-' : '+';
              if (exp < 0)
                  exp = -exp;

              if (exp >= 100)
                 {
                 *p++ = (exp/100) + '0';
                 exp %= 100;
                 }

              *p++ = (exp/10) + '0';
              *p++ = (exp%10) + '0';
              }
           }
        else
           {
           // if fraction only, emit a leading zero
           if (exp < 0)
               *p++ = '0';
           else
              // emit all digits to the left of point
              for (; exp >= 0; --exp)
                     *p++ = toDigit (x, count);

           // emit point
           *p++ = '.';

           // emit leading fractional zeros?
           for (++exp; exp < 0 && decimals > 0; --decimals, ++exp)
                *p++ = '0';

           // output remaining digits, if any. Trailing
           // zeros are also returned from toDigit()
           while (decimals-- > 0)
                  *p++ = toDigit (x, count);
           }

        return dst [0..(p - dst.ptr)];
}


/******************************************************************************

        Convert a formatted string of digits to a floating-point number.
        Good for general use, but use David Gay's dtoa package if serious
        rounding adjustments should be applied.

******************************************************************************/

double parse(T) (T[] src, uint* ate=null)
{
        T               c;
        T*              p,
                        end;
        int             exp;
        bool            sign;
        uint            radix = 10;
        double          value = 0.0;

        // remove leading space, and sign
        c = *(p = src.ptr + Integer.trim (src, sign, radix));

        // handle non-decimal representations
        if (radix != 10)
           {
           long v = Integer.parse (src, radix, ate); 
           return *cast(double*) &v;
           }

        // set end check
        end = src.ptr + src.length;

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
        if (value)
           {
           // parse base10 exponent?
           if ((c is 'e' || c is 'E') && p < end )
              {
              uint eaten;
              exp += Integer.parse (src[(p-src.ptr)+1 .. $], 10u, &eaten);
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
           // was it was nan instead?
           if (p is src.ptr)
               if (p[0..3] == "inf")
                   p += 3, value = double.infinity;
               else
                  if (p[0..3] == "nan")
                      p += 3, value = double.nan;

        // set parse length, and return value
        if (ate)
            *ate = p - src.ptr;
        return sign ? -value : value; 
}


/******************************************************************************

        Internal function to convert an exponent specifier to a floating
        point value.

******************************************************************************/

private double pow10 (uint exp)
{
        static  double[] Powers = 
                [
                1.0e1,
                1.0e2,
                1.0e4,
                1.0e8,
                1.0e16,
                1.0e32,
                1.0e64,
                1.0e128,
                1.0e256,
                ];

        assert (exp < 512);
        
        double mult = 1.0;
        foreach (double power; Powers)
                {
                if (exp & 1)
                    mult *= power;
                if ((exp >>= 1) is 0)
                     break;
                }
        return mult;
}


/******************************************************************************

******************************************************************************/

debug (UnitTest)
{
        unittest
        {
                wchar[64] tmp;
                assert (format (tmp, 3.14159) == "3.141590");
                assert (format (tmp, 3.14159, 4) == "3.1416");
                assert (parse ("3.14159") == 3.14159);
        }
}


