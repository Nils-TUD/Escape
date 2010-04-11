/*******************************************************************************

        copyright:      Copyright (c) 2008 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Apr 2008: Initial release

        authors:        Kris

        Since:          0.99.7

        Based upon Doug Lea's Java collection package

*******************************************************************************/

module tango.util.container.SortedMap;

public  import  tango.util.container.Container;

private import  tango.util.container.RedBlack;

private import  tango.util.container.model.IContainer;

private import tango.core.Exception : NoSuchElementException;

/*******************************************************************************

        RedBlack trees of (key, value) pairs

        ---
        Iterator iterator (bool forward)
        Iterator iterator (K key, bool forward)
        int opApply (int delegate (ref V value) dg)
        int opApply (int delegate (ref K key, ref V value) dg)

        bool contains (V value)
        bool containsKey (K key)
        bool containsPair (K key, V value)
        bool keyOf (V value, ref K key)
        bool get (K key, ref V value)

        bool take (ref V v)
        bool take (K key, ref V v)
        bool removeKey (K key)
        size_t remove (V value, bool all)
        size_t remove (IContainer!(V) e, bool all)

        bool add (K key, V value)
        size_t replace (V oldElement, V newElement, bool all)
        bool replacePair (K key, V oldElement, V newElement)
        bool opIndexAssign (V element, K key)
        K    nearbyKey (K key, bool greater)
        V    opIndex (K key)
        V*   opIn_r (K key)

        size_t size ()
        bool isEmpty ()
        V[] toArray (V[] dst)
        SortedMap dup ()
        SortedMap clear ()
        SortedMap reset ()
        SortedMap comparator (Comparator c)
        ---

*******************************************************************************/

class SortedMap (K, V, alias Reap = Container.reap, 
                       alias Heap = Container.DefaultCollect) 
                       : IContainer!(V)
{
        // use this type for Allocator configuration
        public alias RedBlack!(K, V)    Type;
        private alias Type              *Ref;

        private alias Heap!(Type)       Alloc;
        private alias Compare!(K)       Comparator;

        // root of the tree. Null if empty.
        package Ref                     tree;

        // configured heap manager
        private Alloc                   heap;

        // Comparators used for ordering
        private Comparator              cmp;
        private Compare!(V)             cmpElem;

        private size_t                  count,
                                        mutation;


        /***********************************************************************

                Make an empty tree, using given Comparator for ordering
                 
        ***********************************************************************/

        public this (Comparator c = null)
        {
                this (c, 0);
        }

        /***********************************************************************

                Special version of constructor needed by dup()
                 
        ***********************************************************************/

        private this (Comparator c, size_t n)
        {       
                count = n;
                cmpElem = &compareElem;
                cmp = (c is null) ? &compareKey : c;
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

        final Iterator iterator (bool forward = true)
        {
                Iterator i = void;
                i.node = count ? (forward ? tree.leftmost : tree.rightmost) : null;
                i.bump = forward ? &Iterator.fore : &Iterator.back;
                i.mutation = mutation;
                i.owner = this;
                i.prior = null;
                return i;
        }
      
        /***********************************************************************

                Return an iterator which return all elements matching 
                or greater/lesser than the key in argument. The second
                argument dictates traversal direction.

                Return a generic iterator for contained elements
                
        ***********************************************************************/

        final Iterator iterator (K key, bool forward)
        {
                Iterator i = iterator (forward);
                i.node = count ? tree.findFirst(key, cmp, forward) : null;
                return i;
        }

        /***********************************************************************

                Return the number of elements contained
                
        ***********************************************************************/

        final size_t size ()
        {
                return count;
        }
        
        /***********************************************************************

                Create an independent copy. Does not clone elements
                 
        ***********************************************************************/

        final SortedMap dup ()
        {
                auto clone = new SortedMap!(K, V, Reap, Heap) (cmp, count);
                if (count)
                    clone.tree = tree.copyTree (&clone.heap.allocate);

                return clone;
        }

        /***********************************************************************

                Time complexity: O(log n)
                        
        ***********************************************************************/

        final bool contains (V value)
        {
                if (count is 0)
                    return false;
                return tree.findAttribute (value, cmpElem) !is null;
        }

        /***********************************************************************
        
        ***********************************************************************/
        
        final int opApply (int delegate (ref V value) dg)
        {
                return iterator.opApply ((ref K k, ref V v) {return dg(v);});
        }


        /***********************************************************************
        
        ***********************************************************************/
        
        final int opApply (int delegate (ref K key, ref V value) dg)
        {
                return iterator.opApply (dg);
        }

        /***********************************************************************

                Use a new Comparator. Causes a reorganization
                 
        ***********************************************************************/

        final SortedMap comparator (Comparator c)
        {
                if (cmp !is c)
                   {
                   cmp = (c is null) ? &compareKey : c;

                   if (count !is 0)
                      {       
                      // must rebuild tree!
                      mutate;
                      auto t = tree.leftmost;
                      tree = null;
                      count = 0;
                      
                      while (t)
                            {
                            add_ (t.value, t.attribute, false);
                            t = t.successor;
                            }
                      }
                   }
                return this;
        }

        /***********************************************************************

                Time complexity: O(log n)
                 
        ***********************************************************************/

        final bool containsKey (K key)
        {
                if (count is 0)
                    return false;

                return tree.find (key, cmp) !is null;
        }

        /***********************************************************************

                Time complexity: O(n)
                 
        ***********************************************************************/

        final bool containsPair (K key, V value)
        {
                if (count is 0)
                    return false;

                return tree.find (key, value, cmp) !is null;
        }

        /***********************************************************************

                Return the value associated with Key key. 

                param: key a key
                Returns: whether the key is contained or not
                 
        ***********************************************************************/

        final bool get (K key, ref V value)
        {
                if (count)
                   {
                   auto p = tree.find (key, cmp);
                   if (p)
                      {
                      value = p.attribute;
                      return true;
                      }
                   }
                return false;
        }

        /***********************************************************************

                Return the value of the key exactly matching the provided
                key or, if none, the key just after/before it based on the
                setting of the second argument
    
                param: key a key
                param: after indicates whether to look beyond or before
                       the given key, where there is no exact match
                throws: NoSuchElementException if none found
                returns: a pointer to the value, or null if not present
             
        ***********************************************************************/

        K nearbyKey (K key, bool after)
        {
                if (count)
                   {
                   auto p = tree.findFirst (key, cmp, after);
                   if (p)
                       return p.value;
                   }

                noSuchElement ("no such key");
                assert (0);
        }

        /***********************************************************************
        
                Return the first key of the map

                throws: NoSuchElementException where the map is empty
                     
        ***********************************************************************/

        K firstKey ()
        {
                if (count)
                    return tree.leftmost.value;

                noSuchElement ("no such key");
                assert (0);
        }

        /***********************************************************************
        
                Return the last key of the map

                throws: NoSuchElementException where the map is empty
                     
        ***********************************************************************/

        K lastKey ()
        {
                if (count)
                    return tree.rightmost.value;

                noSuchElement ("no such key");
                assert (0);
        }

        /***********************************************************************

                Return the value associated with Key key. 

                param: key a key
                Returns: a pointer to the value, or null if not present
                 
        ***********************************************************************/

        final V* opIn_r (K key)
        {
                if (count)
                   {
                   auto p = tree.find (key, cmp);
                   if (p)
                       return &p.attribute;
                   }
                return null;
        }

        /***********************************************************************

                Time complexity: O(n)
                 
        ***********************************************************************/

        final bool keyOf (V value, ref K key)
        {
                if (count is 0)
                    return false;

                auto p = tree.findAttribute (value, cmpElem);
                if (p is null)
                    return false;

                key = p.value;
                return true;
        }

        /***********************************************************************

                Time complexity: O(n)
                 
        ***********************************************************************/

        final SortedMap clear ()
        {
                return clear (false);
        }

        /***********************************************************************

                Reset the SortedMap contents. This releases more memory 
                than clear() does

                Time complexity: O(n)
                
        ***********************************************************************/

        final SortedMap reset ()
        {
                return clear (true);
        }

        /***********************************************************************

        ************************************************************************/

        final size_t remove (IContainer!(V) e, bool all)
        {
                auto c = count;
                foreach (v; e)
                         remove (v, all);
                return c - count;
        }

        /***********************************************************************

                Time complexity: O(n
                 
        ***********************************************************************/

        final size_t remove (V value, bool all = false)
        {       
                size_t i = count;
                if (count)
                   {
                   auto p = tree.findAttribute (value, cmpElem);
                   while (p)
                         {
                         tree = p.remove (tree);
                         decrement (p);
                         if (!all || count is 0)
                             break;
                         p = tree.findAttribute (value, cmpElem);
                         }
                   }
                return i - count;
        }

        /***********************************************************************

                Time complexity: O(n)
                 
        ***********************************************************************/

        final size_t replace (V oldElement, V newElement, bool all = false)
        {
                size_t c;

                if (count)
                   {
                   auto p = tree.findAttribute (oldElement, cmpElem);
                   while (p)
                         {
                         ++c;
                         mutate;
                         p.attribute = newElement;
                         if (!all)
                              break;
                         p = tree.findAttribute (oldElement, cmpElem);
                         }
                   }
                return c;
        }

        /***********************************************************************

                Time complexity: O(log n)

                Takes the value associated with the least key.
                 
        ***********************************************************************/

        final bool take (ref V v)
        {
                if (count)
                   {
                   auto p = tree.leftmost;
                   v = p.attribute;
                   tree = p.remove (tree);
                   decrement (p);
                   return true;
                   }
                return false;
        }

        /***********************************************************************

                Time complexity: O(log n)
                        
        ***********************************************************************/

        final bool take (K key, ref V value)
        {
                if (count)
                   {
                   auto p = tree.find (key, cmp);
                   if (p)
                      {
                      value = p.attribute;
                      tree = p.remove (tree);
                      decrement (p);
                      return true;
                      }
                   }
                return false;
        }

        /***********************************************************************

                Time complexity: O(log n)

                Returns true if inserted, false where an existing key 
                exists and was updated instead
                 
        ***********************************************************************/

        final bool add (K key, V value)
        {
                return add_ (key, value, true);
        }

        /***********************************************************************

                Time complexity: O(log n)

                Returns true if inserted, false where an existing key 
                exists and was updated instead
                 
        ***********************************************************************/

        final bool opIndexAssign (V element, K key)
        {
                return add (key, element);
        }

        /***********************************************************************

                Operator retreival function

                Throws NoSuchElementException where key is missing

        ***********************************************************************/

        final V opIndex (K key)
        {
                auto p = opIn_r (key);
                if (p)
                    return *p;

                noSuchElement ("missing or invalid key");
                assert (0);
        }

        /***********************************************************************

                Time complexity: O(log n)
                        
        ***********************************************************************/

        final bool removeKey (K key)
        {
                V value;
                
                return take (key, value);
        }

        /***********************************************************************

                Time complexity: O(log n)
                 
        ***********************************************************************/

        final bool replacePair (K key, V oldElement, V newElement)
        {
                if (count)
                   {
                   auto p = tree.find (key, oldElement, cmp);
                   if (p)
                      {
                      p.attribute = newElement;
                      mutate;
                      return true;
                      }
                   }
                return false;
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
                foreach (k, v; this)
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

        final SortedMap check ()
        {
                assert(cmp !is null);
                assert(((count is 0) is (tree is null)));
                assert((tree is null || tree.size() is count));

                if (tree)
                   {
                   tree.checkImplementation;
                   auto t = tree.leftmost;
                   K last = K.init;

                   while (t)
                         {
                         auto v = t.value;
                         assert((last is K.init || cmp(last, v) <= 0));
                         last = v;
                         t = t.successor;
                         }
                   }
                return this;
        }

            
        /***********************************************************************

                 
        ***********************************************************************/

        private void noSuchElement (char[] msg)
        {
                throw new NoSuchElementException (msg);
        }

        /***********************************************************************

                Time complexity: O(log n)
                 
        ***********************************************************************/

        private size_t instances (V value)
        {
                if (count is 0)
                     return 0;
                return tree.countAttribute (value, cmpElem);
        }

        /***********************************************************************

                Returns true where an element is added, false where an 
                existing key is found
                 
        ***********************************************************************/

        private final bool add_ (K key, V value, bool checkOccurrence)
        {
                if (tree is null)
                   {
                   tree = heap.allocate.set (key, value);
                   increment;
                   }
                else
                   {
                   auto t = tree;
                   for (;;)
                       {
                       int diff = cmp (key, t.value);
                       if (diff is 0 && checkOccurrence)
                          {
                          if (t.attribute != value)
                             {
                             t.attribute = value;
                             mutate;
                             }
                          return false;
                          }
                       else
                          if (diff <= 0)
                             {
                             if (t.left)
                                 t = t.left;
                             else
                                {
                                tree = t.insertLeft (heap.allocate.set(key, value), tree);
                                increment;
                                break;
                                }
                             }
                          else
                             {
                             if (t.right)
                                 t = t.right;
                             else
                                {
                                tree = t.insertRight (heap.allocate.set(key, value), tree);
                                increment;
                                break;
                                }
                             }
                       }
                   }

                return true;
        }

        /***********************************************************************

                Time complexity: O(n)
                 
        ***********************************************************************/

        private SortedMap clear (bool all)
        {
                mutate;

                // collect each node if we can't collect all at once
                if (heap.collect(all) is false & count)                 
                   {
                   auto node = tree.leftmost;
                   while (node)
                         {
                         auto next = node.successor;
                         decrement (node);
                         node = next;
                         }
                   }

                count = 0;
                tree = null;
                return this;
        }

        /***********************************************************************

                Time complexity: O(log n)
                        
        ***********************************************************************/

        private void remove (Ref node)
        {
                tree = node.remove (tree);
                decrement (node);
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
                Reap (p.value, p.attribute);
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

                The default key comparator

                @param fst first argument
                @param snd second argument

                Returns: a negative number if fst is less than snd; a
                positive number if fst is greater than snd; else 0
                 
        ***********************************************************************/

        private static int compareKey (ref K fst, ref K snd)
        {
                if (fst is snd)
                    return 0;

                return typeid(K).compare (&fst, &snd);
        }


        /***********************************************************************

                The default value comparator

                @param fst first argument
                @param snd second argument

                Returns: a negative number if fst is less than snd; a
                positive number if fst is greater than snd; else 0
                 
        ***********************************************************************/

        private static int compareElem(ref V fst, ref V snd)
        {
                if (fst is snd)
                    return 0;

                return typeid(V).compare (&fst, &snd);
        }

        /***********************************************************************

                Iterator with no filtering

        ***********************************************************************/

        private struct Iterator
        {
                Ref function(Ref) bump;
                Ref               node,
                                  prior;
                SortedMap         owner;
                size_t            mutation;

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

                bool next (ref K k, ref V v)
                {
                        auto n = next (k);
                        return (n) ? v = *n, true : false;
                }
                
                /***************************************************************

                        Return a pointer to the next value, or null when
                        there are no further values to traverse

                ***************************************************************/

                V* next (ref K k)
                {
                        V* r;

                        if (node)
                           {
                           prior = node;
                           k = node.value;
                           r = &node.attribute;
                           node = bump (node);
                           }
                        return r;
                }

                /***************************************************************

                        Foreach support

                ***************************************************************/

                int opApply (int delegate(ref K key, ref V value) dg)
                {
                        int result;

                        auto n = node;
                        while (n)
                              {
                              prior = n;
                              auto next = bump (n);
                              if ((result = dg(n.value, n.attribute)) != 0)
                                   break;
                              n = next;
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
                           owner.remove (prior);

                           // ignore this change
                           ++mutation;
                           return true;
                           }

                        prior = null;
                        return false;
                }

                /***************************************************************

                ***************************************************************/

                Iterator reverse ()
                {
                        if (bump is &fore)
                            bump = &back;
                        else
                           bump = &fore;
                        return *this;
                }

                /***************************************************************

                ***************************************************************/

                private static Ref fore (Ref p)
                {
                        return p.successor;
                }

                /***************************************************************

                ***************************************************************/

                private static Ref back (Ref p)
                {
                        return p.predecessor;
                }
        }
}



/*******************************************************************************

*******************************************************************************/

debug (SortedMap)
{
        import tango.io.Stdout;
        import tango.core.Thread;
        import tango.time.StopWatch;
        import tango.math.random.Kiss;

        void main()
        {
                // usage examples ...
                auto map = new SortedMap!(char[], int);
                map.add ("foo", 1);
                map.add ("bar", 2);
                map.add ("wumpus", 3);

                // implicit generic iteration
                foreach (key, value; map)
                         Stdout.formatln ("{}:{}", key, value);

                // explicit iteration
                foreach (key, value; map.iterator("foo", false))
                         Stdout.formatln ("{}:{}", key, value);

                // generic iteration with optional remove
                auto s = map.iterator;
                foreach (key, value; s)
                        {} // s.remove;

                // incremental iteration, with optional remove
                char[] k;
                int    v;
                auto iterator = map.iterator;
                while (iterator.next(k, v))
                      {} //iterator.remove;
                
                // incremental iteration, with optional failfast
                auto it = map.iterator;
                while (it.valid && it.next(k, v))
                      {}

                // remove specific element
                map.removeKey ("wumpus");

                // remove first element ...
                while (map.take(v))
                       Stdout.formatln ("taking {}, {} left", v, map.size);
                
                
                // setup for benchmark, with a set of integers. We
                // use a chunk allocator, and presize the bucket[]
                auto test = new SortedMap!(int, int, Container.reap, Container.Chunk);
                test.allocator.config (1000, 500);
                const count = 500_000;
                StopWatch w;
                
                auto keys = new int[count];
                foreach (ref vv; keys)
                         vv = Kiss.instance.toInt(int.max);

                // benchmark adding
                w.start;
                for (int i=count; i--;)
                     test.add(keys[i], i);
                Stdout.formatln ("{} adds: {}/s", test.size, test.size/w.stop);

                // benchmark reading
                w.start;
                for (int i=count; i--;)
                     test.get(keys[i], v);
                Stdout.formatln ("{} lookups: {}/s", test.size, test.size/w.stop);

                // benchmark adding without allocation overhead
                test.clear;
                w.start;
                for (int i=count; i--;)
                     test.add(keys[i], i);
                Stdout.formatln ("{} adds (after clear): {}/s", test.size, test.size/w.stop);

                // benchmark duplication
                w.start;
                auto dup = test.dup;
                Stdout.formatln ("{} element dup: {}/s", dup.size, dup.size/w.stop);

                // benchmark iteration
                w.start;
                foreach (key, value; test) {}
                Stdout.formatln ("{} element iteration: {}/s", test.size, test.size/w.stop);

                test.check;
        }
}
