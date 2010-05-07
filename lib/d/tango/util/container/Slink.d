/*******************************************************************************

        copyright:      Copyright (c) 2008 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Apr 2008: Initial release

        authors:        Kris

        Since:          0.99.7

        Based upon Doug Lea's Java collection package

*******************************************************************************/

module tango.util.container.Slink;

private import tango.util.container.model.IContainer;

/*******************************************************************************

        Slink instances provide standard linked list next-fields, and
        support standard operations upon them. Slink structures are pure
        implementation tools, and perform no argument checking, no result
        screening, and no synchronization. They rely on user-level classes
        (see HashSet, for example) to do such things.

        Still, Slink is made `public' so that you can use it to build other
        kinds of containers
        
        Note that when K is specified, support for keys are enabled. When
        Identity is stipulated as 'true', those keys are compared using an
        identity-comparison instead of equality (using 'is').

*******************************************************************************/

private typedef int KeyDummy;

struct Slink (V, K=KeyDummy, bool Identity = false)
{
        alias Slink!(V, K)      Type;
        alias Type              *Ref;
        alias Compare!(V)       Comparator;


        Ref     next;           // pointer to next
        V       value;          // element value
                
        /***********************************************************************

                add support for keys also?
                
        ***********************************************************************/

        static if (!is(typeof(K) == KeyDummy))
        {
                K key;

                final Ref set (K k, V v, Ref n)
                {
                        key = k;
                        return set (v, n);
                }

                final int hash()
                {
                        return typeid(K).getHash(&key);
                }

                final Ref findKey (K key)
                {
                        static if (Identity == true)
                        {
                           for (auto p=this; p; p = p.next)
                                if (key is p.key)
                                    return p;
                        }
                        else
                        {
                           for (auto p=this; p; p = p.next)
                                if (key == p.key)
                                    return p;
                        }
                        return null;
                }

                final Ref findPair (K key, V value)
                {
                        static if (Identity == true)
                        {
                           for (auto p=this; p; p = p.next)
                                if (key is p.key && value == p.value)
                                    return p;
                        }
                        else
                        {
                           for (auto p=this; p; p = p.next)
                                if (key == p.key && value == p.value)
                                    return p;
                        }
                        return null;
                }

                final int indexKey (K key)
                {
                        int i = 0;
                        static if (Identity == true)
                        {
                           for (auto p=this; p; p = p.next, ++i)
                                if (key is p.key)
                                    return i;
                        }
                        else
                        {
                           for (auto p=this; p; p = p.next, ++i)
                                if (key == p.key)
                                    return i;
                        }
                        return -1;
                }

                final int indexPair (K key, V value)
                {
                        int i = 0;
                        static if (Identity == true)
                        {
                           for (auto p=this; p; p = p.next, ++i)
                                if (key is p.key && value == p.value)
                                    return i;
                        }
                        else
                        {
                           for (auto p=this; p; p = p.next, ++i)
                                if (key == p.key && value == p.value)
                                    return i;
                        }
                        return -1;
                }

                final int countKey (K key)
                {
                        int c = 0;
                        static if (Identity == true)
                        {
                           for (auto p=this; p; p = p.next)
                                if (key is p.key)
                                    ++c;
                        }
                        else
                        {
                           for (auto p=this; p; p = p.next)
                                if (key == p.key)
                                    ++c;
                        }
                        return c;
                }

                final int countPair (K key, V value)
                {
                        int c = 0;
                        static if (Identity == true)
                        {
                           for (auto p=this; p; p = p.next)
                                if (key is p.key && value == p.value)
                                    ++c;
                        }
                        else
                        {
                           for (auto p=this; p; p = p.next)
                                if (key == p.key && value == p.value)
                                    ++c;
                        }
                        return c;
                }
        }
        
        /***********************************************************************

                 Set to point to n as next cell

                 param: n, the new next cell
                        
        ***********************************************************************/

        final Ref set (V v, Ref n)
        {
                next = n;
                value = v;
                return this;
        }

        /***********************************************************************

                 Splice in p between current cell and whatever it was
                 previously pointing to

                 param: p, the cell to splice
                        
        ***********************************************************************/

        final void attach (Ref p)
        {
                if (p)
                    p.next = next;
                next = p;
        }

        /***********************************************************************

                Cause current cell to skip over the current next() one, 
                effectively removing the next element from the list
                        
        ***********************************************************************/

        final void detachNext()
        {
                if (next)
                    next = next.next;
        }

        /***********************************************************************

                 Linear search down the list looking for element
                 
                 param: element to look for
                 Returns: the cell containing element, or null if no such
                 
        ***********************************************************************/

        final Ref find (V element)
        {
                for (auto p = this; p; p = p.next)
                     if (element == p.value)
                         return p;
                return null;
        }

        /***********************************************************************

                Return the number of cells traversed to find first occurrence
                of a cell with element() element, or -1 if not present
                        
        ***********************************************************************/

        final int index (V element)
        {
                int i;
                for (auto p = this; p; p = p.next, ++i)
                     if (element == p.value)
                         return i;

                return -1;
        }

        /***********************************************************************

                Count the number of occurrences of element in list
                        
        ***********************************************************************/

        final int count (V element)
        {
                int c;
                for (auto p = this; p; p = p.next)
                     if (element == p.value)
                         ++c;
                return c;
        }

        /***********************************************************************

                 Return the number of cells in the list
                        
        ***********************************************************************/

        final int count ()
        {
                int c;
                for (auto p = this; p; p = p.next)
                     ++c;
                return c;
        }

        /***********************************************************************

                Return the cell representing the last element of the list
                (i.e., the one whose next() is null
                        
        ***********************************************************************/

        final Ref tail ()
        {
                auto p = this;
                while (p.next)
                       p = p.next;
                return p;
        }

        /***********************************************************************

                Return the nth cell of the list, or null if no such
                        
        ***********************************************************************/

        final Ref nth (int n)
        {
                auto p = this;
                for (int i; i < n; ++i)
                     p = p.next;
                return p;
        }

        /***********************************************************************

                Make a copy of the list; i.e., a new list containing new cells
                but including the same elements in the same order
                        
        ***********************************************************************/

        final Ref copy (Ref delegate() alloc)
        {
                auto newlist = dup (alloc);
                auto current = newlist;

                for (auto p = next; p; p = p.next)
                    {
                    current.next = p.dup (alloc);
                    current = current.next;
                    }
                current.next = null;
                return newlist;
        }

        /***********************************************************************

                dup is shallow; i.e., just makes a copy of the current cell
                        
        ***********************************************************************/

        private Ref dup (Ref delegate() alloc)
        {
                auto ret = alloc();
                static if (is(typeof(K) == KeyDummy))
                           ret.set (value, next);
                       else
                          ret.set (key, value, next);
                return ret;
        }

        /***********************************************************************

                Basic linkedlist merge algorithm.
                Merges the lists head by fst and snd with respect to cmp
         
                param: fst head of the first list
                param: snd head of the second list
                param: cmp a Comparator used to compare elements
                Returns: the merged ordered list
                        
        ***********************************************************************/

        static Ref merge (Ref fst, Ref snd, Comparator cmp)
        {
                auto a = fst;
                auto b = snd;
                Ref hd = null;
                Ref current = null;

                for (;;)
                    {
                    if (a is null)
                       {
                       if (hd is null)
                           hd = b;
                       else
                          current.next = b;
                       return hd;
                       }
                    else
                       if (b is null)
                          {
                          if (hd is null)
                              hd = a;
                          else
                             current.next = a;
                          return hd;
                          }

                    int diff = cmp (a.value, b.value);
                    if (diff <= 0)
                       {
                       if (hd is null)
                           hd = a;
                       else
                          current.next = a;
                       current = a;
                       a = a.next;
                       }
                    else
                       {
                       if (hd is null)
                           hd = b;
                       else
                          current.next = b;
                       current = b;
                       b = b.next;
                       }
                    }
        }

        /***********************************************************************

                Standard list splitter, used by sort.
                Splits the list in half. Returns the head of the second half

                param: s the head of the list
                Returns: the head of the second half

        ***********************************************************************/

        static Ref split (Ref s)
        {
                auto fast = s;
                auto slow = s;

                if (fast is null || fast.next is null)
                    return null;

                while (fast)
                      {
                      fast = fast.next;
                      if (fast && fast.next)
                         {
                         fast = fast.next;
                         slow = slow.next;
                         }
                      }

                auto r = slow.next;
                slow.next = null;
                return r;

        }

        /***********************************************************************

                 Standard merge sort algorithm
                 
                 param: s the list to sort
                 param: cmp, the comparator to use for ordering
                 Returns: the head of the sorted list
                        
        ***********************************************************************/

        static Ref sort (Ref s, Comparator cmp)
        {
                if (s is null || s.next is null)
                    return s;
                else
                   {
                   auto right = split (s);
                   auto left = s;
                   left = sort (left, cmp);
                   right = sort (right, cmp);
                   return merge (left, right, cmp);
                   }
        }

}

