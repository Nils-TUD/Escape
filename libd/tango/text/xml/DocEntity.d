/*******************************************************************************

        Copyright: Copyright (C) 2008 Kris Bell.  All rights reserved.

        License:   BSD style: $(LICENSE)

        version:   Aug 2008: Initial release

        Authors:   Kris

*******************************************************************************/

module tango.text.xml.DocEntity;

private import Util = tango.text.Util;

/******************************************************************************

        Convert XML entity patterns to normal characters
        ---
        &amp; => ;
        &quot => "
        etc
        ---
        
******************************************************************************/

T[] fromEntity (T) (T[] src, T[] dst = null)
{
        int delta;
        auto s = src.ptr;
        auto len = src.length;

        // take a peek first to see if there's anything
        if ((delta = Util.indexOf (s, '&', len)) < len)
           {
           // make some room if not enough provided
           if (dst.length < src.length)
               dst.length = src.length;
           auto d = dst.ptr;

           // copy segments over, a chunk at a time
           do {
              d [0 .. delta] = s [0 .. delta];
              len -= delta;
              s += delta;
              d += delta;

              // translate entity
              auto token = 0;

              switch (s[1])
                     {
                      case 'a':
                           if (len > 4 && s[1..5] == "amp;")
                               *d++ = '&', token = 5;
                           else
                           if (len > 5 && s[1..6] == "apos;")
                               *d++ = '\'', token = 6;
                           break;
                           
                      case 'g':
                           if (len > 3 && s[1..4] == "gt;")
                               *d++ = '>', token = 4;
                           break;
                           
                      case 'l':
                           if (len > 3 && s[1..4] == "lt;")
                               *d++ = '<', token = 4;
                           break;
                           
                      case 'q':
                           if (len > 5 && s[1..6] == "quot;")
                               *d++ = '"', token = 6;
                           break;

                      default:
                           break;
                     }

              if (token is 0)
                  *d++ = '&', token = 1;

              s += token, len -= token;
              } while ((delta = Util.indexOf (s, '&', len)) < len);

           // copy tail too
           d [0 .. len] = s [0 .. len];
           return dst [0 .. (d + len) - dst.ptr];
           }
        return src;
}


/******************************************************************************

        Convert XML entity patterns to normal characters
        ---
        &amp; => ;
        &quot => "
        etc
        ---
        
        This variant does not require an interim workspace, and instead
        emits directly via the provided delegate
              
******************************************************************************/

void fromEntity (T) (T[] src, void delegate(T[]) emit)
{
        int delta;
        auto s = src.ptr;
        auto len = src.length;

        // take a peek first to see if there's anything
        if ((delta = Util.indexOf (s, '&', len)) < len)
           {
           // copy segments over, a chunk at a time
           do {
              emit (s [0 .. delta]);
              len -= delta;
              s += delta;

              // translate entity
              auto token = 0;

              switch (s[1])
                     {
                      case 'a':
                           if (len > 4 && s[1..5] == "amp;")
                               emit("&"), token = 5;
                           else
                           if (len > 5 && s[1..6] == "apos;")
                               emit("'"), token = 6;
                           break;
                           
                      case 'g':
                           if (len > 3 && s[1..4] == "gt;")
                               emit(">"), token = 4;
                           break;
                           
                      case 'l':
                           if (len > 3 && s[1..4] == "lt;")
                               emit("<"), token = 4;
                           break;
                           
                      case 'q':
                           if (len > 5 && s[1..6] == "quot;")
                               emit("\""), token = 6;
                           break;

                      default:
                           break;
                     }

              if (token is 0)
                  emit ("&"), token = 1;

              s += token, len -= token;
              } while ((delta = Util.indexOf (s, '&', len)) < len);

           // copy tail too
           emit (s [0 .. len]);
           }
        else
           emit (src);
}


/******************************************************************************

        Convert reserved chars to entities. For example: " => &quot; 

        Either a slice of the provided output buffer is returned, or the 
        original content, depending on whether there were reserved chars
        present or not. The output buffer should be sufficiently large to  
        accomodate the converted output, or it will be allocated from the 
        heap instead 
        
******************************************************************************/

T[] toEntity(T) (T[] src, T[] dst = null)
{
        T[]  entity;
        auto s = src.ptr;
        auto t = s;
        auto e = s + src.length;
        auto index = 0;

        while (s < e)
               switch (*s)
                      {
                      case '"':
                           entity = "&quot;";
                           goto common;

                      case '>':
                           entity = "&gt;";
                           goto common;

                      case '<':
                           entity = "&lt;";
                           goto common;

                      case '&':
                           entity = "&amp;";
                           goto common;

                      case '\'':
                           entity = "&apos;";
                           goto common;

                      common:
                           auto len = s - t;
                           if (dst.length <= index + len + entity.length)
                               dst.length = (dst.length + len + entity.length) + dst.length / 2;

                           dst [index .. index + len] = t [0 .. len];
                           index += len;

                           dst [index .. index + entity.length] = entity;
                           index += entity.length;
                           t = ++s;
                           break;

                      default:
                           ++s;
                           break;
                      }


        // did we change anything?
        if (index)
           {
           // copy tail too
           auto len = e - t;
           if (dst.length <= index + len)
               dst.length = index + len;

           dst [index .. index + len] = t [0 .. len];
           return dst [0 .. index + len];
           }

        return src;
}


/******************************************************************************

        Convert reserved chars to entities. For example: " => &quot; 

        This variant does not require an interim workspace, and instead
        emits directly via the provided delegate
        
******************************************************************************/

void toEntity(T) (T[] src, void delegate(T[]) emit)
{
        T[]  entity;
        auto s = src.ptr;
        auto t = s;
        auto e = s + src.length;

        while (s < e)
               switch (*s)
                      {
                      case '"':
                           entity = "&quot;";
                           goto common;

                      case '>':
                           entity = "&gt;";
                           goto common;

                      case '<':
                           entity = "&lt;";
                           goto common;

                      case '&':
                           entity = "&amp;";
                           goto common;

                      case '\'':
                           entity = "&apos;";
                           goto common;

                      common:
                           if (s - t > 0)
                               emit (t [0 .. s - t]);
                           emit (entity);
                           t = ++s;
                           break;

                      default:
                           ++s;
                           break;
                      }

        // did we change anything? Copy tail also
        if (entity.length)
            emit (t [0 .. e - t]);
        else
           emit (src);
}



/*******************************************************************************

*******************************************************************************/

debug (DocEntity)
{
        import tango.io.Console;

        void main()
        {
                auto s = fromEntity ("&amp;");
                assert (s == "&");
                s = fromEntity ("&quot;");
                assert (s == "\"");
                s = fromEntity ("&apos;");
                assert (s == "'");
                s = fromEntity ("&gt;");
                assert (s == ">");
                s = fromEntity ("&lt;");
                assert (s == "<");
                s = fromEntity ("&lt;&amp;&apos;");
                assert (s == "<&'");
                s = fromEntity ("*&lt;&amp;&apos;*");
                assert (s == "*<&'*");

                assert (fromEntity ("abc") == "abc");
                assert (fromEntity ("abc&") == "abc&");
                assert (fromEntity ("abc&lt;") == "abc<");
                assert (fromEntity ("abc&gt;goo") == "abc>goo");
                assert (fromEntity ("&amp;") == "&");
                assert (fromEntity ("&quot;&apos;") == "\"'");
                assert (fromEntity ("&q&s") == "&q&s");

                auto d = toEntity (">");
                assert (d == "&gt;");
                d = toEntity ("<");
                assert (d == "&lt;");
                d = toEntity ("&");
                assert (d == "&amp;");
                d = toEntity ("'");
                assert (d == "&apos;");
                d = toEntity ("\"");
                assert (d == "&quot;");
                d = toEntity ("^^>*>*");
                assert (d == "^^&gt;*&gt;*");
        }
}
