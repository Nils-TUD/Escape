/**
 *  Compile-time conversions between numerics and strings.
 *
 * License:   BSD style: $(LICENSE)
 * Authors:   Don Clugston
 * Copyright: Copyright (C) 2005-2006 Don Clugston
 */
module tango.util.meta.Convert;

private {

/* Convert a single-digit integer to a decimal or hexadecimal char.
*/
template decimaldigit(int n) { const char [] decimaldigit = "0123456789"[n..n+1]; }
template hexdigit(int n, bool upperCase=true) {
    static if (upperCase) const char [] hexdigit = "0123456789ABCDEF"[n..n+1];
    else                  const char [] hexdigit = "0123456789abcdef"[n..n+1];
}

/* Convert a single hex character to integer. Can be lower or upper case.
 */
template hexCharToInteger(char c)
{
    static if (c>='a' && c<='f') const int hexCharToInteger = 10+ c -'a';
    else static if (c>='A' && c<='F') const int hexCharToInteger = 10+ c -'A';
    else static if (c>='0' && c<='9') const int hexCharToInteger = c -'0';
    else static assert(0, "Invalid hexadecimal character");
}

}

/** uint hexToUint!(char [] str)
 * Convert a hexadecimal string (eg "7B2a") into an unsigned number.
 */
template hexToUint(char [] str)
{
    static if (str.length==1) const uint hexToUint = hexCharToInteger!((str[0]));
    else const uint hexToUint = hexCharToInteger!((str[0]))*16 + hexToUint!(str[1..$]);
}

/** uint toUint!(char [] str)
 * Convert a decimal string to an unsigned number
 * Aborts with a static assert if the number > uint.max
 */
template toUint(char [] str, uint result = 0)
{
    static assert(str[0]>='0' && str[0]<='9');
    static if (result <= (uint.max-str[0]+'0')/10u) {
        static if(str.length==1) const uint toUint = result * 10 + (str[0]-'0');
        else const uint toUint = toUint!(str[1..$], result * 10 + (str[0]-'0') );
    } else static assert(0, "toUint: Number is > uint.max");
}

debug(UnitTest) {
static assert(toUint!("23")==23);
static assert(toUint!("4294967295")==uint.max);
}

/**
 *  char [] uintToString!(ulong n);
 *  Convert a unsigned number to a decimal string.
 */
template uintToString(ulong n) {
    static if (n<10L)
        const char [] uintToString = decimaldigit!(n);
    else
        const char [] uintToString = uintToString!(n/10L) ~ decimaldigit!(n%10L);
}

/**
 *  char [] uintToHexString!(ulong n);
 *  Convert a unsigned number to a hexadecimal string.
 */
template uintToHexString(ulong n)
{
    static if (n<16L)
        const char [] uintToHexString = hexdigit!(n);
    else
        const char [] uintToHexString = uintToHexString!(n >> 4) ~ hexdigit!(n&0xF);
}

package {

/** Display the floating point number in %a format (eg 0x1.ABCp-35)
 *
 *  As per IEEE quadreal format, rawExponent must be biased by 0x3FFF.
 *  Mantissa must not include an implicit bit.
 */
template rawFloatToHexString(ushort rawExponent, ulong rawMantissa)
{
    static if ((rawExponent & 0x8000)!=0) {
        // it's negative
        const char [] rawFloatToHexString = "-" ~ rawFloatToHexString!(rawExponent&0x7FFF, rawMantissa);
    } else static if (rawExponent == 0) { // denormal
        static if (rawMantissa==0) const char [] rawFloatToHexString = "0";
        else const char [] rawFloatToHexString = "0x0."
        ~ toZeroPaddedHexString!(rawMantissa, 16) ~ "p" ~ signeditoa!(rawExponent - 0x3FFF);
    } else static if (rawExponent == 0x7FFF) { // inf or NaN.
        static if (rawMantissa==0) const char [] rawFloatToHexString = "inf";
        else const char [] rawFloatToHexString = "NaN:" ~ toZeroPaddedHexString!(rawMantissa/2, 16);
    } else {
        const char [] rawFloatToHexString = "0x1."
        ~ toZeroPaddedHexString!(rawMantissa, 16) ~ "p" ~ signeditoa!(rawExponent - 0x3FFF);
    }
}

template toZeroPaddedHexString(ulong n, int len)
{
    const char[] toZeroPaddedHexString = "0000000000000000"[0..len-uintToHexString!(n).length] ~ uintToHexString!(n);
}

template signeditoa(long n)
{
    static if (n>=0) const char [] signeditoa = "+" ~ uintToString!(n);
    else const char [] signeditoa = "-" ~ uintToString!(-n);
}

  /*
   * Return true if the string begins with a decimal digit
   *
   * beginsWithDigit!(s) is equivalent to isdigit!((s[0]));
   * it allows us to avoid the ugly double parentheses.
   */
template beginsWithDigit(char [] s)
{
  static if (s.length>0 && s[0]>='0' && s[0]<='9')
    const bool beginsWithDigit = true;
  else const bool beginsWithDigit = false;
}

// Count the number of decimal digits at the start of the string (eg, return 3 for "674A")
template countLeadingDigits(char [] str)
{
    static if (str.length>0 && beginsWithDigit!( str))
        const int countLeadingDigits = 1 + countLeadingDigits!( str[1..$]);
    else const int countLeadingDigits = 0;
}

// Count the number of decimal digits at the start of the string (eg, return 3 for "674A")
template countLeadingHexDigits(char [] str)
{
    static if (str.length>0 && ((str[0]>='0' && str[0]<='9') || (str[0]>='A' && str[0]<='F')))
        const int countLeadingHexDigits = 1 + countLeadingHexDigits!( str[1..$]);
    else const int countLeadingHexDigits = 0;
}
}