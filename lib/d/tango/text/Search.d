/*******************************************************************************

        copyright:      Copyright (c) 2009 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        May 2009: Initial release

        since:          0.99.9

        author:         Kris

*******************************************************************************/

module tango.text.Search;

private import Util = tango.text.Util;

/******************************************************************************

        Returns a lightweight pattern matcher, good for short patterns 
        and/or short to medium length content. Brute-force approach with
        fast multi-byte comparisons

******************************************************************************/

FindFruct!(T) find(T) (T[] what)
{
        return FindFruct!(T) (what);
}

/******************************************************************************

        Returns a welterweight pattern matcher, good for long patterns 
        and/or extensive content. Based on the QS algorithm which is a
        Boyer-Moore variant. Does not allocate memory for the alphabet.

        Generally becomes faster as the match-length grows

******************************************************************************/

SearchFruct!(T) search(T) (T[] what)
{
        return SearchFruct!(T) (what);
}

/******************************************************************************

        Convenient bundle of lightweight find utilities, without the
        hassle of IFTI problems. Create one of these using the find() 
        function:
        ---
        auto match = find ("foo");
        auto content = "wumpus foo bar"

        // search in the forward direction
        auto index = match.forward (content);
        assert (index is 7);

        // search again - returns length when no match found
        assert (match.forward(content, index+1) is content.length);
        ---

        Searching operates both forward and backward, with an optional
        start offset (can be more convenient than slicing the content).
        There are methods to replace matches within given content, and 
        others which return foreach() iterators for traversing content.

        SearchFruct is a more sophisticated variant, which operates more
        efficiently on longer matches and/or more extensive content.

******************************************************************************/

private struct FindFruct(T)
{       
        private T[] what;

        /***********************************************************************

                Search forward in the given content, starting at the 
                optional index.

                Returns the index of a match, or content.length where
                no match was located.

        ***********************************************************************/

        size_t forward (T[] content, size_t ofs = 0)
        {
                return Util.index (content, what, ofs);
        }

        /***********************************************************************

                Search backward in the given content, starting at the 
                optional index.

                Returns the index of a match, or content.length where
                no match was located.

        ***********************************************************************/

        size_t reverse (T[] content, size_t ofs = size_t.max)
        {       
                return Util.rindex (content, what, ofs);
        }

        /***********************************************************************

                Return the match text

        ***********************************************************************/

        T[] match ()
        {
                return what;
        }

        /***********************************************************************

                Reset the text to match

        ***********************************************************************/

        void match (T[] what)
        {
                this.what = what;
        }

        /***********************************************************************

                Returns true if there is a match within the given content

        ***********************************************************************/

        bool within (T[] content)
        {       
                return forward(content) != content.length;
        }

        /***********************************************************************
                
                Returns number of matches within the given content

        ***********************************************************************/

        size_t count (T[] content)
        {       
                size_t mark, count;

                while ((mark = Util.index (content, what, mark)) != content.length)
                        ++count, ++mark;
                return count;
        }

        /***********************************************************************

                Replace all matches with the given character. Use method
                tokens() instead to avoid heap activity.

                Returns a copy of the content with replacements made

        ***********************************************************************/

        T[] replace (T[] content, T chr)
        {     
                return replace (content, (&chr)[0..1]);  
        }

        /***********************************************************************

                Replace all matches with the given substitution. Use 
                method tokens() instead to avoid heap activity.

                Returns a copy of the content with replacements made

        ***********************************************************************/

        T[] replace (T[] content, T[] sub = null)
        {  
                T[] output;

                foreach (s; tokens (content, sub))
                         output ~= s;
                return output;
        }

        /***********************************************************************

                Returns a foreach() iterator which exposes text segments
                between all matches within the given content. Substitution
                text is also injected in place of each match, and null can
                be used to indicate removal instead:
                ---
                char[] result;

                auto match = find ("foo");
                foreach (token; match.tokens ("$foo&&foo*", "bar"))
                         result ~= token;
                assert (result == "$bar&&bar*");
                ---
                
                This mechanism avoids internal heap activity.                

        ***********************************************************************/

        Util.PatternFruct!(T) tokens (T[] content, T[] sub = null)
        {
                return Util.patterns (content, what, sub);
        }
        
        /***********************************************************************

                Returns a foreach() iterator which exposes the indices of
                all matches within the given content:
                ---
                int count;

                auto f = find ("foo");
                foreach (index; f.indices("$foo&&foo*"))
                         ++count;
                assert (count is 2);
                ---

        ***********************************************************************/

        Indices indices (T[] content)
        {
                return Indices (what, content);
        }
 
        /***********************************************************************

                Simple foreach() iterator

        ***********************************************************************/

        private struct Indices
        {
                T[]     what,
                        content;

                int opApply (int delegate (ref size_t index) dg)
                {
                        int    ret;
                        size_t mark;

                        while ((mark = Util.index(content, what, mark)) != content.length)                        
                                if ((ret = dg(mark)) is 0)                                
                                     ++mark;       
                                else
                                   break;                        
                        return ret;   
                }     
        } 
}


/******************************************************************************

        Convenient bundle of welterweight search utilities, without the
        hassle of IFTI problems. Create one of these using the search() 
        function:
        ---
        auto match = search ("foo");
        auto content = "wumpus foo bar"

        // search in the forward direction
        auto index = match.forward (content);
        assert (index is 7);

        // search again - returns length when no match found
        assert (match.forward(content, index+1) is content.length);
        ---

        Searching operates both forward and backward, with an optional
        start offset (can be more convenient than slicing the content).
        There are methods to replace matches within given content, and 
        others which return foreach() iterators for traversing content.

        FindFruct is a simpler variant, which can operate efficiently on 
        short matches and/or short content (employs brute-force strategy)

******************************************************************************/

private struct SearchFruct(T)
{
        private T[]             what;
        private bool            fore;
        private int[256]        offsets = void;

        /***********************************************************************

                Construct the fruct

        ***********************************************************************/

        static SearchFruct opCall (T[] what) 
        {
                SearchFruct find = void;
                find.match = what;
                return find;
        }
        
        /***********************************************************************

                Return the match text

        ***********************************************************************/

        T[] match ()
        {
                return what;
        }

        /***********************************************************************

                Reset the text to match

        ***********************************************************************/

        void match (T[] what)
        {
                offsets[] = what.length + 1;
                this.fore = true;
                this.what = what;
                reset;
        }

        /***********************************************************************

                Search forward in the given content, starting at the 
                optional index.

                Returns the index of a match, or content.length where
                no match was located.

        ***********************************************************************/

        size_t forward (T[] content, size_t ofs = 0) 
        {
                if (! fore)
                      flip;

                if (ofs > content.length)
                    ofs = content.length;

                return find (cast(char*) what.ptr, what.length * T.sizeof, 
                             cast(char*) content.ptr, content.length * T.sizeof, 
                             ofs * T.sizeof) / T.sizeof;
        }

        /***********************************************************************

                Search backward in the given content, starting at the 
                optional index.

                Returns the index of a match, or content.length where
                no match was located.

        ***********************************************************************/

        size_t reverse (T[] content, size_t ofs = size_t.max) 
        {
                if (fore)
                    flip;

                if (ofs > content.length)
                    ofs = content.length;

                return rfind (cast(char*) what.ptr, what.length * T.sizeof, 
                              cast(char*) content.ptr, content.length * T.sizeof, 
                              ofs * T.sizeof) / T.sizeof;
        }

        /***********************************************************************

                Returns true if there is a match within the given content

        ***********************************************************************/

        bool within (T[] content)
        {       
                return forward(content) != content.length;
        }

        /***********************************************************************
                
                Returns number of matches within the given content

        ***********************************************************************/

        size_t count (T[] content)
        {       
                size_t mark, count;

                while ((mark = forward (content, mark)) != content.length)
                        ++count, ++mark;
                return count;
        }

        /***********************************************************************

                Replace all matches with the given character. Use method
                tokens() instead to avoid heap activity.

                Returns a copy of the content with replacements made

        ***********************************************************************/

        T[] replace (T[] content, T chr)
        {     
                return replace (content, (&chr)[0..1]);  
        }

        /***********************************************************************

                Replace all matches with the given substitution. Use 
                method tokens() instead to avoid heap activity.

                Returns a copy of the content with replacements made

        ***********************************************************************/

        T[] replace (T[] content, T[] sub = null)
        {  
                T[] output;

                foreach (s; tokens (content, sub))
                         output ~= s;
                return output;
        }

        /***********************************************************************

                Returns a foreach() iterator which exposes text segments
                between all matches within the given content. Substitution
                text is also injected in place of each match, and null can
                be used to indicate removal instead:
                ---
                char[] result;

                auto match = search ("foo");
                foreach (token; match.tokens("$foo&&foo*", "bar"))
                         result ~= token;
                assert (result == "$bar&&bar*");
                ---
                
                This mechanism avoids internal heap activity             

        ***********************************************************************/

        Substitute tokens (T[] content, T[] sub = null)
        {
                return Substitute (sub, what, content, &forward);
        }
        
        /***********************************************************************

                Returns a foreach() iterator which exposes the indices of
                all matches within the given content:
                ---
                int count;

                auto match = search ("foo");
                foreach (index; match.indices("$foo&&foo*"))
                         ++count;
                assert (count is 2);
                ---

        ***********************************************************************/

        Indices indices (T[] content)
        {
                return Indices (content, &forward);
        }
        
        /***********************************************************************

        ***********************************************************************/

        private size_t find (char* what, size_t wlen, char* content, size_t len, size_t ofs) 
        {
                auto s = content;
                content += ofs;
                auto e = s + len - wlen;
                while (content <= e)
                       if (*what is *content && matches(what, content, wlen))
                           return content - s;
                       else
                          content += offsets [content[wlen]];
                return len;
        }

        /***********************************************************************

        ***********************************************************************/

        private size_t rfind (char* what, size_t wlen, char* content, size_t len, size_t ofs) 
        {
                auto s = content;
                auto e = s + ofs - wlen;
                while (e >= content)
                       if (*what is *e && matches(what, e, wlen))
                           return e - s;
                       else
                          e -= offsets [*(e-1)];
                return len;
        }

        /***********************************************************************

        ***********************************************************************/

        private static bool matches (char* a, char* b, size_t length)
        {
                while (length > size_t.sizeof)
                       if (*cast(size_t*) a is *cast(size_t*) b)
                            a += size_t.sizeof, b += size_t.sizeof, length -= size_t.sizeof;
                       else
                          return false;

                while (length--)
                       if (*a++ != *b++) 
                           return false;
                return true;
        }

        /***********************************************************************

                Construct lookup table. We force the alphabet to be char[]
                always, and consider wider characters to be longer patterns
                instead

        ***********************************************************************/

        private void reset ()
        {
                auto what = cast(char[]) this.what;
                if (fore)   
                    for (int i=0; i < what.length; ++i)
                         offsets[what[i]] = what.length - i;
                else
                   for (int i=what.length; i--;)
                        offsets[what[i]] = i+1;
        }

        /***********************************************************************

                Reverse lookup-table direction

        ***********************************************************************/

        private void flip ()
        {
                fore ^= true;
                reset;
        }

        /***********************************************************************

                Simple foreach() iterator

        ***********************************************************************/

        private struct Indices
        {
                T[]    content;
                size_t delegate(T[], size_t) call;

                int opApply (int delegate (ref size_t index) dg)
                {
                        int     ret;
                        size_t  mark;

                        while ((mark = call(content, mark)) != content.length)
                                if ((ret = dg(mark)) is 0)
                                     ++mark;
                                else
                                   break;
                        return ret;   
                }     
        } 

        /***********************************************************************

                Substitution foreach() iterator

        ***********************************************************************/

        private struct Substitute
        {
                private T[] sub, 
                            what,
                            content;
                size_t      delegate(T[], size_t) call;

                int opApply (int delegate (ref T[] token) dg)
                {
                        uint    ret,
                                pos,
                                mark;
                        T[]     token;

                        while ((pos = call (content, mark)) < content.length)
                              {
                              token = content [mark .. pos];
                              if ((ret = dg(token)) != 0)
                                   return ret;
                              if (sub.ptr && (ret = dg(sub)) != 0)
                                  return ret;
                              mark = pos + what.length;
                              }

                        token = content [mark .. $];
                        if (mark <= content.length)
                            ret = dg (token);
                        return ret;
                }
        }
}




/******************************************************************************

******************************************************************************/

debug (Search)
{
        import tango.io.Stdout;
        import tango.time.StopWatch;

        auto x = import("Search.d");
        
        void main()
        {
                StopWatch elapsed;
        
                auto match = search("foo");
                auto index = match.reverse ("foo foo");
                assert (index is 4);
                index = match.reverse ("foo foo", index);
                assert (index is 0);
                index = match.reverse ("foo foo", 1);
                assert (index is 7);

                foreach (index; find("delegate").indices(x))
                         Stdout.formatln ("< {}", index);

                foreach (index; search("delegate").indices(x))
                         Stdout.formatln ("> {}", index);

                elapsed.start;
                for (auto i=5000; i--;)
                     Util.mismatch (x.ptr, x.ptr, x.length);
                Stdout.formatln ("mismatch {}", elapsed.stop);

                elapsed.start;
                for (auto i=5000; i--;)
                     Util.indexOf (x.ptr, '@', cast(uint) x.length);
                Stdout.formatln ("indexOf {}", elapsed.stop);

                elapsed.start;
                for (auto i=5000; i--;)
                     Util.locatePattern (x, "indexOf {}");
                Stdout.formatln ("pattern {}", elapsed.stop);

                elapsed.start;
                auto f = find ("indexOf {}");
                for (auto i=5000; i--;)
                     f.forward(x);
                Stdout.formatln ("find {}", elapsed.stop);

                elapsed.start;
                auto s = search ("indexOf {}");
                for (auto i=5000; i--;)
                     s.forward(x);
                Stdout.formatln ("search {}", elapsed.stop);
        }
}

