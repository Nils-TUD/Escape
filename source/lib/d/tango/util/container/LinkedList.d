/*******************************************************************************

        copyright:      Copyright (c) 2008 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Apr 2008: Initial release

        authors:        Kris

        Since:          0.99.7

        Based upon Doug Lea's Java collection package

*******************************************************************************/

module tango.util.container.LinkedList;

private import  tango.util.container.Slink;

public  import  tango.util.container.Container;

private import tango.util.container.model.IContainer;

/*******************************************************************************

        List of singly-linked values

        ---
	Iterator iterator ()
        int opApply (int delegate(ref V value) dg)

        V head ()
        V tail ()
        V head (V value)
        V tail (V value)
        V removeHead ()
        V removeTail ()

        bool contains (V value)
        size_t first (V value, size_t startingIndex = 0)
        size_t last (V value, size_t startingIndex = 0)

        LinkedList add (V value)
        LinkedList prepend (V value)
        size_t prepend (IContainer!(V) e)
        LinkedList append (V value)
        size_t append (IContainer!(V) e)
        LinkedList addAt (size_t index, V value)
        size_t addAt (size_t index, IContainer!(V) e)

        V get (size_t index)
        bool take (ref V v)
        size_t remove (V value, bool all)
        bool removeAt (size_t index)
        size_t removeRange (size_t fromIndex, size_t toIndex)
        size_t replace (V oldElement, V newElement, bool all)
        bool replaceAt (size_t index, V value)

        LinkedList clear ()
        LinkedList reset ()

        LinkedList subset (size_t from, size_t length = size_t.max)
        LinkedList dup ()

        size_t size ()
        bool isEmpty ()
        V[] toArray (V[] dst)
        LinkedList sort (Compare!(V) cmp)
        LinkedList check ()
        ---

*******************************************************************************/

class LinkedList (V, alias Reap = Container.reap, 
                     alias Heap = Container.DefaultCollect) 
                     : IContainer!(V)
{
        // use this type for Allocator configuration
        private alias Slink!(V) Type;
        
        private alias Type*     Ref;
        private alias V*        VRef;

        private alias Heap!(Type) Alloc;

        // number of elements contained
        private size_t          count;

        // configured heap manager
        private Alloc           heap;
        
        // mutation tag updates on each change
        private size_t          mutation;

        // head of the list. Null if empty
        private Ref             list;

        /***********************************************************************

                Create a new empty list

        ***********************************************************************/

        this ()
        {
                this (null, 0);
        }

        /***********************************************************************

                Special version of constructor needed by dup

        ***********************************************************************/

        protected this (Ref l, size_t c)
        {
                list = l;
                count = c;
        }

        /***********************************************************************

                Clean up when deleted

        ***********************************************************************/

        ~this ()
        {
                reset;
        }

        /***********************************************************************

                Return the configured allocator
                
        ***********************************************************************/

        final Alloc allocator ()
        {
                return heap;
        }

        /***********************************************************************

                Return a generic iterator for contained elements
                
        ***********************************************************************/

        final Iterator iterator ()
        {
                Iterator i = void;
                i.mutation = mutation;
                i.node = list ? *(i.hook = &list) : null;
                i.prior = null;
                i.owner = this;
                return i;
        }

        /***********************************************************************


        ***********************************************************************/

        final int opApply (int delegate(ref V value) dg)
        {
                return iterator.opApply (dg);
        }

        /***********************************************************************

                Return the number of elements contained
                
        ***********************************************************************/

        final size_t size ()
        {
                return count;
        }

        /***********************************************************************

                Build an independent copy of the list.
                The elements themselves are not cloned

        ***********************************************************************/

        final LinkedList dup ()
        {
                return new LinkedList!(V, Reap, Heap) (list ? list.copy(&heap.allocate) : null, count);
        }

        /***********************************************************************

                Time complexity: O(n)

        ***********************************************************************/

        final bool contains (V value)
        {
                if (count is 0)
                    return false;

                return list.find(value) !is null;
        }

        /***********************************************************************

                 Time complexity: O(1)

        ***********************************************************************/

        final V head ()
        {
                return firstCell.value;
        }

        /***********************************************************************

                 Time complexity: O(n)

        ***********************************************************************/

        final V tail ()
        {
                return lastCell.value;
        }

        /***********************************************************************

                 Time complexity: O(n)

        ***********************************************************************/

        final V get (size_t index)
        {
                return cellAt(index).value;
        }

        /***********************************************************************

                 Time complexity: O(n)
                 Returns size_t.max if no element found.

        ***********************************************************************/

        final size_t first (V value, size_t startingIndex = 0)
        {
                if (list is null || startingIndex >= count)
                    return size_t.max;

                if (startingIndex < 0)
                    startingIndex = 0;

                auto p = list.nth (startingIndex);
                if (p)
                   {
                   auto i = p.index (value);
                   if (i >= 0)
                       return i + startingIndex;
                   }
                return size_t.max;
        }

        /***********************************************************************

                 Time complexity: O(n)
                 Returns size_t.max if no element found.

        ***********************************************************************/

        final size_t last (V value, size_t startingIndex = 0)
        {
                if (list is null)
                    return size_t.max;

                auto i = 0;
                if (startingIndex >= count)
                    startingIndex = count - 1;

                auto index = size_t.max;
                auto p = list;
                while (i <= startingIndex && p)
                      {
                      if (p.value == value)
                          index = i;
                      ++i;
                      p = p.next;
                      }
                return index;
        }

        /***********************************************************************

                 Time complexity: O(length)

        ***********************************************************************/

        final LinkedList subset (size_t from, size_t length = size_t.max)
        {
                Ref newlist = null;

                if (length > 0)
                   {
                   auto p = cellAt (from);
                   auto current = newlist = heap.allocate.set (p.value, null);
         
                   for (auto i = 1; i < length; ++i)
                        if ((p = p.next) is null)
                             length = i;
                        else
                           {
                           current.attach (heap.allocate.set (p.value, null));
                           current = current.next;
                           }
                   }

                return new LinkedList (newlist, length);
        }

        /***********************************************************************

                 Time complexity: O(n)

        ***********************************************************************/

        final LinkedList clear ()
        {
                return clear (false);
        }

        /***********************************************************************

                Reset the HashMap contents and optionally configure a new
                heap manager. We cannot guarantee to clean up reconfigured 
                allocators, so be sure to invoke reset() before discarding
                this class

                Time complexity: O(n)
                
        ***********************************************************************/

        final LinkedList reset ()
        {
                return clear (true);
        }

        /***********************************************************************
        
                Takes the first value on the list

                Time complexity: O(1)

        ***********************************************************************/

        final bool take (ref V v)
        {
                if (count)
                   {
                   v = head;
                   removeHead;
                   return true;
                   }
                return false;
        }

        /***********************************************************************

                Uses a merge-sort-based algorithm.

                Time complexity: O(n log n)

        ***********************************************************************/

        final LinkedList sort (Compare!(V) cmp)
        {
                if (list)
                   {
                   list = Ref.sort (list, cmp);
                   mutate;
                   }
                return this;
        }

        /***********************************************************************

                Time complexity: O(1)

        ***********************************************************************/

        final LinkedList add (V value)
        {
                return prepend (value);
        }

        /***********************************************************************

                Time complexity: O(1)

        ***********************************************************************/

        final LinkedList prepend (V value)
        {
                list = heap.allocate.set (value, list);
                increment;
                return this;
        }

        /***********************************************************************

                Time complexity: O(n)

        ***********************************************************************/

        final size_t remove (V value, bool all = false)
        {
                auto c = count;
                if (c)
                   {
                   auto p = list;
                   auto trail = p;

                   while (p)
                         {
                         auto n = p.next;
                         if (p.value == value)
                            {
                            decrement (p);
                            if (p is list)
                               {
                               list = n;
                               trail = n;
                               }
                            else
                               trail.next = n;

                            if (!all || count is 0)
                                 break;
                            else
                               p = n;
                            }
                         else
                            {
                            trail = p;
                            p = n;
                            }
                         }
                   }
                return c - count;
        }

        /***********************************************************************

                Time complexity: O(n)

        ***********************************************************************/

        final size_t replace (V oldElement, V newElement, bool all = false)
        {
                size_t c;
                if (count && oldElement != newElement)
                   {
                   auto p = list.find (oldElement);
                   while (p)
                         {
                         ++c;
                         mutate;
                         p.value = newElement;
                         if (!all)
                              break;
                         p = p.find (oldElement);
                         }
                   }
                return c;
        }

        /***********************************************************************

                 Time complexity: O(1)

        ***********************************************************************/

        final V head (V value)
        {
                auto cell = firstCell;
                auto v = cell.value;
                cell.value = value;
                mutate;
                return v;
        }

        /***********************************************************************

                 Time complexity: O(1)

        ***********************************************************************/

        final V removeHead ()
        {
                auto p = firstCell;
                auto v = p.value;
                list = p.next;
                decrement (p);
                return v;
        }

        /***********************************************************************

                 Time complexity: O(n)

        ***********************************************************************/

        final LinkedList append (V value)
        {
                if (list is null)
                    prepend (value);
                else
                   {
                   list.tail.next = heap.allocate.set (value, null);
                   increment;
                   }
                return this;
        }

        /***********************************************************************

                 Time complexity: O(n)

        ***********************************************************************/

        final V tail (V value)
        {
                auto p = lastCell;
                auto v = p.value;
                p.value = value;
                mutate;
                return v;
        }

        /***********************************************************************

                 Time complexity: O(n)

        ***********************************************************************/

        final V removeTail ()
        {
                if (firstCell.next is null)
                    return removeHead;

                auto trail = list;
                auto p = trail.next;

                while (p.next)
                      {
                      trail = p;
                      p = p.next;
                      }
                trail.next = null;
                auto v = p.value;
                decrement (p);
                return v;
        }

        /***********************************************************************

                Time complexity: O(n)

        ***********************************************************************/

        final LinkedList addAt (size_t index, V value)
        {
                if (index is 0)
                    prepend (value);
                else
                   {
                   cellAt(index - 1).attach (heap.allocate.set(value, null));
                   increment;
                   }
                return this;
        }

        /***********************************************************************

                 Time complexity: O(n)

        ***********************************************************************/

        final LinkedList removeAt (size_t index)
        {
                if (index is 0)
                    removeHead;
                else
                   {
                   auto p = cellAt (index - 1);
                   auto t = p.next;
                   p.detachNext;
                   decrement (t);
                   }
                return this;
        }

        /***********************************************************************

                 Time complexity: O(n)

        ***********************************************************************/

        final LinkedList replaceAt (size_t index, V value)
        {
                cellAt(index).value = value;
                mutate;
                return this;
        }

        /***********************************************************************

                 Time complexity: O(number of elements in e)

        ***********************************************************************/

        final size_t prepend (IContainer!(V) e)
        {
                auto c = count;
                splice_ (e, null, list);
                return count - c;
        }

        /***********************************************************************

                 Time complexity: O(n + number of elements in e)

        ***********************************************************************/

        final size_t append (IContainer!(V) e)
        {
                auto c = count;
                if (list is null)
                    splice_ (e, null, null);
                else
                   splice_ (e, list.tail, null);
                return count - c;
        }

        /***********************************************************************

                Time complexity: O(n + number of elements in e)

        ***********************************************************************/

        final size_t addAt (size_t index, IContainer!(V) e)
        {
                auto c = count;
                if (index is 0)
                    splice_ (e, null, list);
                else
                   {
                   auto p = cellAt (index - 1);
                   splice_ (e, p, p.next);
                   }
                return count - c;
        }

        /***********************************************************************

                Time complexity: O(n)

        ***********************************************************************/

        final size_t removeRange (size_t fromIndex, size_t toIndex)
        {
                auto c = count;
                if (fromIndex <= toIndex)
                   {
                   if (fromIndex is 0)
                      {
                      auto p = firstCell;
                      for (size_t i = fromIndex; i <= toIndex; ++i)
                           p = p.next;
                      list = p;
                      }
                   else
                      {
                      auto f = cellAt (fromIndex - 1);
                      auto p = f;
                      for (size_t i = fromIndex; i <= toIndex; ++i)
                           p = p.next;
                      f.next = p.next;
                      }

                  count -= (toIndex - fromIndex + 1);
                  mutate;
                  }
                return c - count;
        }

        /***********************************************************************

                Copy and return the contained set of values in an array, 
                using the optional dst as a recipient (which is resized 
                as necessary).

                Returns a slice of dst representing the container values.
                
                Time complexity: O(n)
                
        ***********************************************************************/

        final V[] toArray (V[] dst = null)
        {
                if (dst.length < count)
                    dst.length = count;

                size_t i = 0;
                foreach (v; this)
                         dst[i++] = v;
                return dst [0 .. count];                        
        }

        /***********************************************************************

                Is this container empty?
                
                Time complexity: O(1)
                
        ***********************************************************************/

        final bool isEmpty ()
        {
                return count is 0;
        }

        /***********************************************************************

        ***********************************************************************/

        final LinkedList check ()
        {
                assert(((count is 0) is (list is null)));
                assert((list is null || list.count is size));

                size_t c = 0;
                for (Ref p = list; p; p = p.next)
                    {
                    assert(instances(p.value) > 0);
                    assert(contains(p.value));
                    ++c;
                    }
                assert(c is count);
                return this;
        }

        /***********************************************************************

                 Time complexity: O(n)

        ***********************************************************************/

        private size_t instances (V value)
        {
                if (count is 0)
                    return 0;

                return list.count (value);
        }

        /***********************************************************************

        ***********************************************************************/

        private Ref firstCell ()
        {
                checkIndex (0);
                return list;
        }

        /***********************************************************************

        ***********************************************************************/

        private Ref lastCell ()
        {
                checkIndex (0);
                return list.tail;
        }

        /***********************************************************************

        ***********************************************************************/

        private Ref cellAt (size_t index)
        {
                checkIndex (index);
                return list.nth (index);
        }

        /***********************************************************************

        ***********************************************************************/

        private void checkIndex (size_t index)
        {
                if (index >= count)
                    throw new Exception ("out of range");
        }

        /***********************************************************************

                Splice elements of e between hd and tl. If hd 
                is null return new hd

                Returns the count of new elements added

        ***********************************************************************/

        private void splice_ (IContainer!(V) e, Ref hd, Ref tl)
        {
                Ref newlist = null;
                Ref current = null;

                foreach (v; e)
                        {
                        increment;

                        auto p = heap.allocate.set (v, null);
                        if (newlist is null)
                            newlist = p;
                        else
                           current.next = p;
                        current = p;
                        }

                if (current)
                   {
                   current.next = tl;

                   if (hd is null)
                       list = newlist;
                   else
                      hd.next = newlist;
                   }
        }

        /***********************************************************************

                 Time complexity: O(n)

        ***********************************************************************/

        private LinkedList clear (bool all)
        {
                mutate;

                // collect each node if we can't collect all at once
                if (heap.collect(all) is false && count)
                   {
                   auto p = list;
                   while (p)
                         {
                         auto n = p.next;
                         decrement (p);
                         p = n;
                         }
                   }
        
                list = null;
                count = 0;
                return this;
        }

        /***********************************************************************

                new element was added
                
        ***********************************************************************/

        private void increment ()
        {
                ++mutation;
                ++count;
        }
        
        /***********************************************************************

                element was removed
                
        ***********************************************************************/

        private void decrement (Ref p)
        {
                Reap (p.value);
                heap.collect (p);
                ++mutation;
                --count;
        }
        
        /***********************************************************************

                set was changed
                
        ***********************************************************************/

        private void mutate ()
        {
                ++mutation;
        }

        /***********************************************************************

                List iterator

        ***********************************************************************/

        private struct Iterator
        {
                Ref             node;
                Ref*            hook,
                                prior;
                LinkedList      owner;
                size_t          mutation;

                /***************************************************************

                        Did the container change underneath us?

                ***************************************************************/

                bool valid ()
                {
                        return owner.mutation is mutation;
                }               

                /***************************************************************

                        Accesses the next value, and returns false when
                        there are no further values to traverse

                ***************************************************************/

                bool next (ref V v)
                {
                        auto n = next;
                        return (n) ? v = *n, true : false;
                }
                
                /***************************************************************

                        Return a pointer to the next value, or null when
                        there are no further values to traverse

                ***************************************************************/

                V* next ()
                {
                        V* r;
                        if (node)
                           {
                           prior = hook;
                           r = &node.value;
                           node = *(hook = &node.next);
                           }
                        return r;
                }

                /***************************************************************

                        Insert a new value before the node about to be
                        iterated (or after the node that was just iterated).

                ***************************************************************/

                void insert(V value)
                {
                        // insert a node previous to the node that we are
                        // about to iterate.
                        *hook = owner.heap.allocate.set(value, *hook);
                        node = *hook;
                        next();

                        // update the count of the owner, and ignore this
                        // change in the mutation.
                        owner.increment;
                        mutation++;
                }

                /***************************************************************

                        Insert a new value before the value that was just
                        iterated.

                        Returns true if the prior node existed and the
                        insertion worked.  False otherwise.

                ***************************************************************/

                bool insertPrior(V value)
                {
                    if(prior)
                    {
                        // insert a node previous to the node that we just
                        // iterated.
                        *prior = owner.heap.allocate.set(value, *prior);
                        prior = &(*prior).next;

                        // update the count of the owner, and ignore this
                        // change in the mutation.
                        owner.increment;
                        mutation++;
                        return true;
                    }
                    return false;
                }

                /***************************************************************

                        Foreach support

                ***************************************************************/

                int opApply (int delegate(ref V value) dg)
                {
                        int result;

                        auto n = node;
                        while (n)
                              {
                              prior = hook;
                              hook = &n.next;
                              if ((result = dg(n.value)) != 0)
                                   break;
                              n = *hook;
                              }
                        node = n;
                        return result;
                }                               

                /***************************************************************

                        Remove value at the current iterator location

                ***************************************************************/

                bool remove ()
                {
                        if (prior)
                           {
                           auto p = *prior;
                           *prior = p.next;
                           owner.decrement (p);
                           hook = prior;
                           prior = null;

                           // ignore this change
                           ++mutation;
                           return true;
                           }
                        return false;
                }
        }
}


/*******************************************************************************

*******************************************************************************/

debug (LinkedList)
{
        import tango.io.Stdout;
        import tango.core.Thread;
        import tango.time.StopWatch;

        void main()
        {
                // usage examples ...
                auto set = new LinkedList!(char[]);
                set.add ("foo");
                set.add ("bar");
                set.add ("wumpus");

                // implicit generic iteration
                foreach (value; set)
                         Stdout (value).newline;

                // explicit generic iteration   
                foreach (value; set.iterator)
                         Stdout.formatln ("{}", value);

                // generic iteration with optional remove and insert
                auto s = set.iterator;
                foreach (value; s)
                {
                         if (value == "foo")
                             s.remove;
                         if (value == "bar")
                             s.insertPrior("bloomper");
                         if (value == "wumpus")
                             s.insert("rumple");
                }

                set.check();

                // incremental iteration, with optional remove
                char[] v;
                auto iterator = set.iterator;
                while (iterator.next(v))
                      {} //iterator.remove;
                
                // incremental iteration, with optional failfast
                auto it = set.iterator;
                while (it.valid && it.next(v))
                      {}

                // remove specific element
                set.remove ("wumpus");

                // remove first element ...
                while (set.take(v))
                       Stdout.formatln ("taking {}, {} left", v, set.size);
                
                
                // setup for benchmark, with a set of integers. We
                // use a chunk allocator, and presize the bucket[]
                auto test = new LinkedList!(int, Container.reap, Container.Chunk);
                test.allocator.config (2000, 500);
                const count = 1_000_000;
                StopWatch w;

                // benchmark adding
                w.start;
                for (int i=count; i--;)
                     test.prepend(i);
                Stdout.formatln ("{} adds: {}/s", test.size, test.size/w.stop);

                // benchmark adding without allocation overhead
                test.clear;
                w.start;
                for (int i=count; i--;)
                     test.prepend(i);
                Stdout.formatln ("{} adds (after clear): {}/s", test.size, test.size/w.stop);

                // benchmark duplication
                w.start;
                auto dup = test.dup;
                Stdout.formatln ("{} element dup: {}/s", dup.size, dup.size/w.stop);

                // benchmark iteration
                w.start;
                auto xx = test.iterator;
                int ii;
                while (xx.next()) {}
                Stdout.formatln ("{} element iteration: {}/s", test.size, test.size/w.stop);

                // benchmark iteration
                w.start;
                foreach (v; test) {}
                Stdout.formatln ("{} foreach iteration: {}/s", test.size, test.size/w.stop);


                // benchmark iteration
                w.start;             
                foreach (ref iii; test) {} 
                Stdout.formatln ("{} pointer iteration: {}/s", test.size, test.size/w.stop);

                test.check;
        }
}
                
