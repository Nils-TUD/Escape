/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Apr 2004: Initial release
                        Dec 2006: South Seas version

        author:         Kris


        Placeholder for a variety of wee functions. These functions are all
        templated with the intent of being used for arrays of char, wchar,
        and dchar. However, they operate correctly with other array types
        also.

        Several of these functions return an index value, representing where
        some criteria was identified. When said criteria is not matched, the
        functions return a value representing the array length provided to
        them. That is, for those scenarios where C functions might typically
        return -1 these functions return length instead. This operate nicely
        with D slices:
        ---
        auto text = "happy:faces";
        
        assert (text[0 .. locate (text, ':')] == "happy");
        
        assert (text[0 .. locate (text, '!')] == "happy:faces");
        ---

        The contains() function is more convenient for trivial lookup
        cases:
        ---
        if (contains ("fubar", '!'))
            ...
        ---

        Note that where some functions expect a uint as an argument, the
        D template-matching algorithm will fail where an int is provided
        instead. This is the typically the cause of "template not found"
        errors. Also note that name overloading is not supported cleanly
        by IFTI at this time, so is not applied here.


        Applying the D "import alias" mechanism to this module is highly
        recommended, in order to limit namespace pollution:
        ---
        import Util = tango.text.Util;

        auto s = Util.trim ("  foo ");
        ---
                

        Function templates:
        ---
        trim (source)                               // trim whitespace
        strip (source, match)                       // trim elements
        delineate (source);                         // split on lines
        delimit (source, delimiters)                // split on delims
        demarcate (source, pattern)                 // split on pattern
        replace (source, match, replacement)        // replace chars
        contains (source, match)                    // has char?
        containsPattern (source, match)             // has pattern?
        locate (source, match, start)               // find char
        locatePrior (source, match, start)          // find prior char
        locatePattern (source, match, start);       // find pattern
        locatePatternPrior (source, match, start);  // find prior pattern
        indexOf (s*, match, length)                 // low-level lookup
        mismatch (s1*, s2*, length)                 // low-level compare
        matching (s1*, s2*, length)                 // low-level compare
        isSpace (match)                             // is whitespace?
        layout (destination, format ...)            // featherweight printf
        lines (str)                                 // foreach lines
        quotes (str, set)                           // foreach quotes
        delims (str, set)                           // foreach delimiters
        patterns (str, pattern)                     // foreach patterns
        ---

*******************************************************************************/

module tango.text.Util;

/******************************************************************************

        Trim the provided string by stripping whitespace from both
        ends. Returns a slice of the original content

******************************************************************************/

T[] trim(T) (T[] source)
{
        T*   head = source.ptr,
             tail = head + source.length;

        while (head < tail && isSpace(*head))
               ++head;

        while (tail > head && isSpace(*(tail-1)))
               --tail;

        return head [0 .. tail - head];
}

/******************************************************************************

        Trim the given string by stripping the provided match from
        both ends. Returns a slice of the original content

******************************************************************************/

T[] strip(T) (T[] source, T match)
{
        T*   head = source.ptr,
             tail = head + source.length;

        while (head < tail && *head is match)
               ++head;

        while (tail > head && *(tail-1) is match)
               --tail;

        return head [0 .. tail - head];
}

/******************************************************************************

        Replace all instances of one char with another (in place)

******************************************************************************/

T[] replace(T) (T[] source, T match, T replacement)
{
        foreach (inout c; source)
                 if (c is match)
                     c = replacement;
        return source;
}

/******************************************************************************

        Returns whether or not the provided array contains an instance
        of the given match
        
******************************************************************************/

bool contains(T) (T[] source, T match)
{
        return indexOf (source.ptr, match, source.length) != source.length;
}

/******************************************************************************

        Returns whether or not the provided array contains an instance
        of the given match
        
******************************************************************************/

bool containsPattern(T) (T[] source, T[] match)
{
        return locatePattern (source, match) != source.length;
}

/******************************************************************************

        Return the index of the next instance of 'match' starting at
        position 'start', or source.length where there is no match.

        Parameter 'start' defaults to 0

******************************************************************************/

uint locate(T, U=uint) (T[] source, T match, U start=0)
{
        if (start > source.length)
            start = source.length;
        
        return indexOf (source.ptr+start, match, source.length - start) + start;
}

/******************************************************************************

        Return the index of the prior instance of 'match' starting
        just before 'start', or source.length where there is no match.

        Parameter 'start' defaults to source.length

******************************************************************************/

uint locatePrior(T, U=uint) (T[] source, T match, U start=uint.max)
{
        if (start > source.length)
            start = source.length;

        while (start > 0)
               if (source[--start] is match)
                   return start;
        return source.length;
}

/******************************************************************************

        Return the index of the next instance of 'match' starting at
        position 'start', or source.length where there is no match. 

        Parameter 'start' defaults to 0

******************************************************************************/

uint locatePattern(T, U=uint) (T[] source, T[] match, U start=0)
{
        uint    idx;
        T*      p = source.ptr + start;
        uint    extent = source.length - start - match.length + 1;

        if (match.length && extent < source.length)
            while (extent)
                   if ((idx = indexOf (p, match[0], extent)) is extent)
                        break;
                   else
                      if (matching (p+=idx, match.ptr, match.length))
                          return p - source.ptr;
                      else
                         {
                         extent -= (idx+1);
                         ++p;
                         }

        return source.length;
}
   
/******************************************************************************

        Return the index of the prior instance of 'match' starting
        just before 'start', or source.length where there is no match.

        Parameter 'start' defaults to source.length

******************************************************************************/

uint locatePatternPrior(T, U=uint) (T[] source, T[] match, U start=uint.max)
{
        auto len = source.length;
        
        if (start > len)
            start = len;

        if (match.length && match.length <= len)
            while (start)
                  {
                  start = locatePrior (source, match[0], start);
                  if (start is len)
                      break;
                  else
                     if ((start + match.length) <= len)
                          if (matching (source.ptr+start, match.ptr, match.length))
                              return start;
                  }

        return len;
}

/******************************************************************************

        Split the provided array wherever a delimiter-set instance is
        found, and return the resultant segments. The delimiters are
        excluded from each of the segments. Note that delimiters are
        matched as a set of alternates rather than as a pattern.

        Splitting on a single delimiter is considerably faster than
        splitting upon a set of alternatives

******************************************************************************/

T[][] delimit(T) (T[] src, T[] set)
{
        T[][] result;

        foreach (segment; delims (src, set))
                 result ~= segment;
        return result;
}

/******************************************************************************

        Split the provided array wherever a pattern instance is
        found, and return the resultant segments. The pattern is
        excluded from each of the segments.
        
******************************************************************************/

T[][] demarcate(T) (T[] src, T[] pattern)
{
        T[][] result;

        foreach (segment; patterns (src, pattern))
                 result ~= segment;
        return result;
}

/******************************************************************************

        Convert text into a set of lines, where each line is identified
        by a \n or \r\n combination. The line terminator is stripped from
        each resultant array

******************************************************************************/

T[][] delineate(T) (T[] src)
{
        int count;
        
        foreach (line; lines (src))
                 ++count;
        
        T[][] result = new T[][count];

        count = 0;
        foreach (line; lines (src))
                 result [count++] = line;

        return result;
}

/******************************************************************************

        Is the argument a whitespace character?

******************************************************************************/

bool isSpace(T) (T c)
{
        static if (T.sizeof is 1)
                   return (c <= 32 && (c is ' ' | c is '\t' | c is '\r' | c is '\n' | c is '\f'));
        else
           return (c <= 32 && (c is ' ' | c is '\t' | c is '\r' | c is '\n' | c is '\f')) || (c is '\u2028' | c is '\u2029');
}

/******************************************************************************

        Return whether or not the two arrays have matching content
        
******************************************************************************/

bool matching(T, U=uint) (T* s1, T* s2, U length)
{
        return mismatch(s1, s2, length) is length;
}

/******************************************************************************

        Returns the index of the first match in str, failing once
        length is reached. Note that we return 'length' for failure
        and a 0-based index on success

******************************************************************************/

uint indexOf(T, U=uint) (T* str, T match, U length)
{
        version (D_InlineAsm_X86)
        {       
                static if (T.sizeof == 1)
                {
                        asm {
                            mov   EDI, str;
                            mov   ECX, length;
                            movzx EAX, match;
                            mov   ESI, ECX;
                            and   ESI, ESI;            
                            jz    end;        

                            cld;
                            repnz;
                            scasb;
                            jnz   end;
                            sub   ESI, ECX;
                            dec   ESI;
                        end:;
                            mov   EAX, ESI;
                            }
                }
                else static if (T.sizeof == 2)
                {
                        asm {
                            mov   EDI, str;
                            mov   ECX, length;
                            movzx EAX, match;
                            mov   ESI, ECX;
                            and   ESI, ESI;            
                            jz    end;        

                            cld;
                            repnz;
                            scasw;
                            jnz   end;
                            sub   ESI, ECX;
                            dec   ESI;
                        end:;
                            mov   EAX, ESI;
                            }
                }
                else static if (T.sizeof == 4)
                {
                        asm {
                            mov   EDI, str;
                            mov   ECX, length;
                            mov   EAX, match;
                            mov   ESI, ECX;
                            and   ESI, ESI;            
                            jz    end;        

                            cld;
                            repnz;
                            scasd;
                            jnz   end;
                            sub   ESI, ECX;
                            dec   ESI;
                        end:;
                            mov   EAX, ESI;
                            }
                }
                else
                {
                        auto len = length;
                        for (auto p=str-1; len--;)
                             if (*++p is match)
                                 return p - str;
                        return length;
                }
        }
        else
        {
                auto len = length;
                for (auto p=str-1; len--;)
                     if (*++p is match)
                         return p - str;
                return length;
        }
}

/******************************************************************************

        Returns the index of a mismatch between s1 & s2, failing when
        length is reached. Note that we return 'length' upon failure
        (array content matches) and a 0-based index upon success.

        Use this as a faster opEquals (the assembler version). Also
        provides the basis for a much faster opCmp, since the index
        of the first mismatched character can be used to determine
        the (greater or less than zero) return value

******************************************************************************/

uint mismatch(T, U=uint) (T* s1, T* s2, U length)
{
        version (D_InlineAsm_X86)
        {
                static if (T.sizeof == 1)
                {
                        asm {
                            mov   EDI, s1;
                            mov   ESI, s2;
                            mov   ECX, length;
                            mov   EAX, ECX;
                            and   EAX, EAX;
                            jz    end;

                            cld;
                            repz;
                            cmpsb;
                            jz    end;
                            sub   EAX, ECX;
                            dec   EAX;
                        end:;
                            }
                }
                else static if (T.sizeof == 2)
                {
                        asm {
                            mov   EDI, s1;
                            mov   ESI, s2;
                            mov   ECX, length;
                            mov   EAX, ECX;
                            and   EAX, EAX;
                            jz    end;

                            cld;
                            repz;
                            cmpsw;
                            jz    end;
                            sub   EAX, ECX;
                            dec   EAX;
                        end:;
                            }
                }
                else static if (T.sizeof == 4)
                {
                        asm {
                            mov   EDI, s1;
                            mov   ESI, s2;
                            mov   ECX, length;
                            mov   EAX, ECX;
                            and   EAX, EAX;
                            jz    end;

                            cld;
                            repz;
                            cmpsd;
                            jz    end;
                            sub   EAX, ECX;
                            dec   EAX;
                        end:;
                            }
                }
                else
                {
                        auto len = length;
                        for (auto p=s1-1; len--;)
                             if (*++p != *s2++)
                                 return p - s1;
                        return length;
                }
        }
        else
        {
                auto len = length;
                for (auto p=s1-1; len--;)
                     if (*++p != *s2++)
                         return p - s1;
                return length;
        }
}

/******************************************************************************

        Freachable iterator to isolate lines.

        Converts text into a set of lines, where each line is identified
        by a \n or \r\n combination. The line terminator is stripped from
        each resultant array.

        ---
        foreach (line; lines ("one\ntwo\nthree"))
                 ...
        ---
        
******************************************************************************/

LineFreach!(T) lines(T) (T[] src)
{
        LineFreach!(T) lines;
        lines.src = src;
        return lines;
}

/******************************************************************************

        Freachable iterator to isolate text elements.

        Splits the provided array wherever a delimiter-set instance is
        found, and return the resultant segments. The delimiters are
        excluded from each of the segments. Note that delimiters are
        matched as a set of alternates rather than as a pattern.

        Splitting on a single delimiter is considerably faster than
        splitting upon a set of alternatives.

        ---
        foreach (segment; delims ("one,two;three", ",;"))
                 ...
        ---
        
******************************************************************************/

DelimFreach!(T) delims(T) (T[] src, T[] set)
{
        DelimFreach!(T) elements;
        elements.set = set;
        elements.src = src;
        return elements;
}

/******************************************************************************

        Freachable iterator to isolate text elements.

        Split the provided array wherever a pattern instance is
        found, and return the resultant segments. The pattern is
        excluded from each of the segments.
        
        ---
        foreach (segment; patterns ("one, two, three", ", "))
                 ...
        ---
        
******************************************************************************/

PatternFreach!(T) patterns(T) (T[] src, T[] pattern, T[] sub=null)
{
        PatternFreach!(T) elements;
        elements.pattern = pattern;
        elements.sub = sub;
        elements.src = src;
        return elements;
}

/******************************************************************************

        Freachable iterator to isolate optionally quoted text elements.

        As per elements(), but with the extension of being quote-aware;
        the set of delimiters is ignored inside a pair of quotes. Note
        that an unterminated quote will consume remaining content.
        
        ---
        foreach (quote; quotes ("one two 'three four' five", " "))
                 ...
        ---
        
******************************************************************************/

QuoteFreach!(T) quotes(T) (T[] src, T[] set)
{
        QuoteFreach!(T) quotes;
        quotes.set = set;
        quotes.src = src;
        return quotes;
}

/*******************************************************************************

        Arranges text strings in order, using indices to specify where
        each particular argument should be positioned within the text. 
        This is handy for collating I18N components, or as a simplistic
        and lightweight formatter. Indices range from zero through nine. 
        
        ---
        // write ordered text to the console
        char[64] tmp;

        Cout (layout (tmp, "%1 is after %0", "zero", "one")).newline;
        ---

*******************************************************************************/

T[] layout(T) (T[] output, T[][] layout ...)
{
        static void error (char[] msg) {throw new Exception (msg);}

        static char[] badarg   = "Util.layout :: index out of range";
        static char[] toosmall = "Util.layout :: output buffer too small";
        
        int     pos,
                args;
        bool    state;

        args = layout.length - 1;
        foreach (c; layout[0])
                {
                if (state)
                   {
                   state = false;
                   if (c >= '0' && c <= '9')
                      {
                      uint index = c - '0';
                      if (index < args)
                         {
                         T[] x = layout[index+1];

                         int limit = pos + x.length;
                         if (limit < output.length)
                            {
                            output [pos .. limit] = x;
                            pos = limit;
                            continue;
                            } 
                         else
                            error (toosmall);
                         }
                      else
                         error (badarg);
                      }
                   }
                else
                   if (c is '%')
                      {
                      state = true;
                      continue;
                      }

                if (pos < output.length)
                   {
                   output[pos] = c;
                   ++pos;
                   }
                else     
                   error (toosmall);
                }

        return output [0..pos];
}

/******************************************************************************

        jhash() -- hash a variable-length key into a 32-bit value

          k     : the key (the unaligned variable-length array of bytes)
          len   : the length of the key, counting by bytes
          level : can be any 4-byte value

        Returns a 32-bit value.  Every bit of the key affects every bit of
        the return value.  Every 1-bit and 2-bit delta achieves avalanche.

        About 4.3*len + 80 X86 instructions, with excellent pipelining

        The best hash table sizes are powers of 2.  There is no need to do
        mod a prime (mod is sooo slow!).  If you need less than 32 bits,
        use a bitmask.  For example, if you need only 10 bits, do

                    h = (h & hashmask(10));

        In which case, the hash table should have hashsize(10) elements.
        If you are hashing n strings (ub1 **)k, do it like this:

                    for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);

        By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may use 
        this code any way you wish, private, educational, or commercial.  
        It's free.

        See http://burlteburtle.net/bob/hash/evahash.html
        Use for hash table lookup, or anything where one collision in 2^32 
        is acceptable. Do NOT use for cryptographic purposes.

******************************************************************************/

uint jhash (ubyte* k, uint len, uint c = 0)
{
        uint a = 0x9e3779b9,
             b = 0x9e3779b9,
             i = len;

        // handle most of the key 
        while (i >= 12) 
              {
              a += *cast(uint *)(k+0);
              b += *cast(uint *)(k+4);
              c += *cast(uint *)(k+8);

              a -= b; a -= c; a ^= (c>>13); 
              b -= c; b -= a; b ^= (a<<8); 
              c -= a; c -= b; c ^= (b>>13); 
              a -= b; a -= c; a ^= (c>>12);  
              b -= c; b -= a; b ^= (a<<16); 
              c -= a; c -= b; c ^= (b>>5); 
              a -= b; a -= c; a ^= (c>>3);  
              b -= c; b -= a; b ^= (a<<10); 
              c -= a; c -= b; c ^= (b>>15); 
              k += 12; i -= 12;
              }

        // handle the last 11 bytes 
        c += len;
        switch (i)
               {
               case 11: c+=(cast(uint)k[10]<<24);
               case 10: c+=(cast(uint)k[9]<<16);
               case 9 : c+=(cast(uint)k[8]<<8);
               case 8 : b+=(cast(uint)k[7]<<24);
               case 7 : b+=(cast(uint)k[6]<<16);
               case 6 : b+=(cast(uint)k[5]<<8);
               case 5 : b+=(cast(uint)k[4]);
               case 4 : a+=(cast(uint)k[3]<<24);
               case 3 : a+=(cast(uint)k[2]<<16);
               case 2 : a+=(cast(uint)k[1]<<8);
               case 1 : a+=(cast(uint)k[0]);
               default:
               }

        a -= b; a -= c; a ^= (c>>13); 
        b -= c; b -= a; b ^= (a<<8); 
        c -= a; c -= b; c ^= (b>>13); 
        a -= b; a -= c; a ^= (c>>12);  
        b -= c; b -= a; b ^= (a<<16); 
        c -= a; c -= b; c ^= (b>>5); 
        a -= b; a -= c; a ^= (c>>3);  
        b -= c; b -= a; b ^= (a<<10); 
        c -= a; c -= b; c ^= (b>>15); 

        return c;
}

/// ditto
uint jhash (void[] x, uint c = 0)
{
        return jhash (cast(ubyte*) x.ptr, x.length, c);
}


/******************************************************************************

        Helper struct for iterator lines()
         
******************************************************************************/

private struct LineFreach(T)
{
        private T[] src;

        int opApply (int delegate (inout T[] line) dg)
        {
                uint    ret,
                        pos,
                        mark;
                T[]     line;

                const T nl = '\n';
                const T cr = '\r';

                while ((pos = locate (src, nl, mark)) < src.length)
                      {
                      auto end = pos;
                      if (end && src[end-1] is cr)
                          --end;

                      line = src [mark .. end];
                      if ((ret = dg (line)) != 0)
                           return ret;
                      mark = pos + 1;
                      }

                line = src [mark .. $];
                if (mark < src.length)
                    ret = dg (line);

                return ret;
        }
}

/******************************************************************************

        Helper struct for iterator delims()
        
******************************************************************************/

private struct DelimFreach(T)
{
        private T[] src;
        private T[] set;

        int opApply (int delegate (inout T[] token) dg)
        {
                uint    ret,
                        pos,
                        mark;
                T[]     token;

                // optimize for single delimiter case
                if (set.length is 1)
                    while ((pos = locate (src, set[0], mark)) < src.length)
                          {
                          token = src [mark .. pos];
                          if ((ret = dg (token)) != 0)
                               return ret;
                          mark = pos + 1;
                          }
                else
                   if (set.length > 1)
                       foreach (i, elem; src)
                                if (contains (set, elem))
                                   {
                                   token = src [mark .. i];
                                   if ((ret = dg (token)) != 0)
                                        return ret;
                                   mark = i + 1;
                                   }

                token = src [mark .. $];
                if (mark < src.length)
                    ret = dg (token);

                return ret;
        }
}

/******************************************************************************

        Helper struct for iterator patterns()
        
******************************************************************************/

private struct PatternFreach(T)
{
        private T[] src,
                    sub,
                    pattern;

        int opApply (int delegate (inout T[] token) dg)
        {
                uint    ret,
                        pos,
                        mark;
                T[]     token;

                // optimize for single-element pattern
                if (pattern.length is 1)
                    while ((pos = locate (src, pattern[0], mark)) < src.length)
                          {
                          token = src [mark .. pos];
                          dg (token);
                          if (sub.ptr)
                              dg (sub);
                          mark = pos + 1;
                          }
                else
                   if (pattern.length > 1)
                       while ((pos = locatePattern (src, pattern, mark)) < src.length)
                             {
                             token = src [mark .. pos];
                             dg (token);
                             if (sub.ptr)
                                 dg (sub);
                             mark = pos + pattern.length;
                             }

                token = src [mark .. $];
                if (mark < src.length)
                    ret = dg (token);

                return ret;
        }
}

/******************************************************************************

        Helper struct for iterator quotes()
        
******************************************************************************/

private struct QuoteFreach(T)
{
        private T[] src;
        private T[] set;
        
        int opApply (int delegate (inout T[] token) dg)
        {
                int ret,
                    mark;
                T[] token;

                if (set.length)
                    for (uint i=0; i < src.length; ++i)
                        {
                        T c = src[i];
                        if (c is '"' || c is '\'')
                            i = locate (src, c, i+1);
                        else
                           if (contains (set, c))
                              {
                              token = src [mark .. i];
                              if ((ret = dg (token)) != 0)
                                   return ret;
                              mark = i + 1;
                              }
                        }
                
                token = src [mark .. $];
                if (mark < src.length)
                    ret = dg (token);

                return ret;
        }
}

/******************************************************************************

******************************************************************************/

debug (UnitTest)
{
        //void main() {}
        
        unittest
        {
        char[64] tmp;

        assert (isSpace (' ') && !isSpace ('d'));

        assert (indexOf ("abc".ptr, 'a', 3) is 0);
        assert (indexOf ("abc".ptr, 'b', 3) is 1);
        assert (indexOf ("abc".ptr, 'c', 3) is 2);
        assert (indexOf ("abc".ptr, 'd', 3) is 3);

        assert (mismatch ("abc".ptr, "abc".ptr, 3) is 3);
        assert (mismatch ("abc".ptr, "abd".ptr, 3) is 2);
        assert (mismatch ("abc".ptr, "acc".ptr, 3) is 1);
        assert (mismatch ("abc".ptr, "ccc".ptr, 3) is 0);

        assert (matching ("abc".ptr, "abc".ptr, 3));
        assert (matching ("abc".ptr, "abb".ptr, 3) is false);
        
        assert (contains ("abc", 'a'));
        assert (contains ("abc", 'b'));
        assert (contains ("abc", 'c'));
        assert (contains ("abc", 'd') is false);

        assert (containsPattern ("abc", "ab"));
        assert (containsPattern ("abc", "bc"));
        assert (containsPattern ("abc", "abc"));
        assert (containsPattern ("abc", "zabc") is false);
        assert (containsPattern ("abc", "abcd") is false);
        assert (containsPattern ("abc", "za") is false);
        assert (containsPattern ("abc", "cd") is false);

        assert (trim ("") == "");
        assert (trim (" abc  ") == "abc");
        assert (trim ("   ") == "");

        assert (strip ("", '%') == "");
        assert (strip ("%abc%%%", '%') == "abc");
        assert (strip ("#####", '#') == "");

        assert (replace ("abc".dup, 'b', ':') == "a:c");

        assert (locate ("abc", 'c', 1) is 2);

        assert (locate ("abc", 'c') is 2);
        assert (locate ("abc", 'a') is 0);
        assert (locate ("abc", 'd') is 3);
        assert (locate ("", 'c') is 0);

        assert (locatePrior ("abce", 'c') is 2);
        assert (locatePrior ("abce", 'a') is 0);
        assert (locatePrior ("abce", 'd') is 4);
        assert (locatePrior ("abce", 'c', 3) is 2);
        assert (locatePrior ("abce", 'c', 2) is 4);
        assert (locatePrior ("", 'c') is 0);

        auto x = delimit ("::b", ":");
        assert (x.length is 3 && x[0] == "" && x[1] == "" && x[2] == "b");
        x = delimit ("a:bc:d", ":");
        assert (x.length is 3 && x[0] == "a" && x[1] == "bc" && x[2] == "d");
        x = delimit ("abcd", ":");
        assert (x.length is 1 && x[0] == "abcd");
        x = delimit ("abcd:", ":");
        assert (x.length is 1 && x[0] == "abcd");
        x = delimit ("a;b$c#d:e@f", ";:$#@");
        assert (x.length is 6 && x[0]=="a" && x[1]=="b" && x[2]=="c" &&
                                 x[3]=="d" && x[4]=="e" && x[5]=="f");

        assert (locatePattern ("abcdefg", "") is 7);
        assert (locatePattern ("abcdefg", "abcdefg") is 0);
        assert (locatePattern ("abcdefg", "abcdefgx") is 7);
        assert (locatePattern ("abcdefg", "cce") is 7);
        assert (locatePattern ("abcdefg", "cde") is 2);
        assert (locatePattern ("abcdefgcde", "cde", 3) is 7);

        assert (locatePatternPrior ("abcdefg", "") is 7);
        assert (locatePatternPrior ("abcdefg", "cce") is 7);
        assert (locatePatternPrior ("abcdefg", "cde") is 2);
        assert (locatePatternPrior ("abcdefgcde", "cde", 6) is 2);
        assert (locatePatternPrior ("abcdefgcde", "cde", 4) is 2);
        assert (locatePatternPrior ("abcdefg", "abcdefgx") is 7);

        x = delineate ("a\nb\n");
        assert (x.length is 2 && x[0] == "a" && x[1] == "b");
        x = delineate ("a\r\n");
        assert (x.length is 1 && x[0] == "a");
        x = delineate ("a");
        assert (x.length is 1 && x[0] == "a");
        x = delineate ("");
        assert (x.length is 0);

        char[][] q;
        foreach (element; quotes ("1 'avcc   cc ' 3", " "))
                 q ~= element;
        assert (q.length is 3 && q[0] == "1" && q[1] == "'avcc   cc '" && q[2] == "3");

        assert (layout (tmp, "%1,%%%c %0", "abc", "efg") == "efg,%c abc");

        x = demarcate ("one, two, three", ",");
        assert (x.length is 3 && x[0] == "one" && x[1] == " two" && x[2] == " three");
        x = demarcate ("one, two, three", ", ");
        assert (x.length is 3 && x[0] == "one" && x[1] == "two" && x[2] == "three");
        x = demarcate ("one, two, three", ",,");
        assert (x.length is 1 && x[0] == "one, two, three");
        }
}



