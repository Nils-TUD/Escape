/*******************************************************************************

        copyright:      Copyright (c) 2008 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Apr 2008: Initial release

        authors:        Kris

        Since:          0.99.7

        Based upon Doug Lea's Java collection package

*******************************************************************************/

module tango.util.container.HashMap;

private import tango.util.container.Slink;

public  import tango.util.container.Container;

private import tango.util.container.model.IContainer;

private import tango.core.Exception : NoSuchElementException;

/*******************************************************************************

        Hash table implementation of a Map

        ---
        Iterator iterator ()
        int opApply (int delegate(ref V value) dg)
        int opApply (int delegate(ref K key, ref V value) dg)

        bool get (K key, ref V element)
        bool keyOf (V value, ref K key)
        bool contains (V element)
        bool containsPair (K key, V element)

        bool removeKey (K key)
        bool take (ref V element)
        bool take (K key, ref V element)
        size_t remove (V element, bool all)
        size_t remove (IContainer!(V) e, bool all)
        size_t replace (V oldElement, V newElement, bool all)
        bool replacePair (K key, V oldElement, V newElement)

        bool add (K key, V element)
        bool opIndexAssign (V element, K key)
        V    opIndex (K key)
        V*   opIn_r (K key)

        size_t size ()
        bool isEmpty ()
        V[] toArray (V[] dst)
        HashMap dup ()
        HashMap clear ()
        HashMap reset ()
        size_t buckets ()
        float threshold ()
        void buckets (size_t cap)
        void threshold (float desired)
        Allocator allocator()
        ---

*******************************************************************************/

class HashMap (K, V, alias Hash = Container.hash, 
                     alias Reap = Container.reap, 
                     alias Heap = Container.DefaultCollect) 
                     : IContainer!(V)
{
        // bucket types
        private alias Slink!(V, K) Type;
        private alias Type         *Ref;

        // allocator type
        private alias Heap!(Type)  Alloc;

        // each table entry is a linked list, or null
        private Ref                table[];
        
        // number of elements contained
        private size_t             count;

        // the threshold load factor
        private float              loadFactor;

        // configured heap manager
        private Alloc              heap;
        
        // mutation tag updates on each change
        private size_t             mutation;

        /***********************************************************************

                Construct a HashMap instance

        ***********************************************************************/

        this (float f = Container.defaultLoadFactor)
        {
                loadFactor = f;
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
                i.table = table;
                i.owner = this;
                i.cell = null;
                i.row = 0;
                return i;
        }

        /***********************************************************************


        ***********************************************************************/

        final int opApply (int delegate(ref K key, ref V value) dg)
        {
                return iterator.opApply (dg);
        }

        /***********************************************************************


        ***********************************************************************/

        final int opApply (int delegate(ref V value) dg)
        {
                return iterator.opApply ((ref K k, ref V v) {return dg(v);});
        }

        /***********************************************************************

                Return the number of elements contained
                
        ***********************************************************************/

        final size_t size ()
        {
                return count;
        }
        
        /***********************************************************************

                Add a new element to the set. Does not add if there is an
                equivalent already present. Returns true where an element
                is added, false where it already exists (and was possibly
                updated).
                
                Time complexity: O(1) average; O(n) worst.
                
        ***********************************************************************/

        final bool add (K key, V element)
        {
                if (table is null)
                    resize (Container.defaultInitialBuckets);

                auto hd = &table [Hash (key, table.length)];
                auto node = *hd;
                
                if (node is null)
                   {
                   *hd = heap.allocate.set (key, element, null);
                   increment;
                   }
                else
                   {
                   auto p = node.findKey (key);
                   if (p)
                      {
                      if (element != p.value)
                         {
                         p.value = element;
                         mutate;
                         }
                      return false;
                      }
                   else
                      {
                      *hd = heap.allocate.set (key, element, node);
                      increment;

                      // we only check load factor on add to nonempty bin
                      checkLoad; 
                      }
                   }
                return true;
        }

        /***********************************************************************

                Add a new element to the set. Does not add if there is an
                equivalent already present. Returns true where an element
                is added, false where it already exists (and was possibly
                updated). This variation invokes the given retain function
                when the key does not already exist. You would typically
                use that to duplicate a char[], or whatever is required.
                
                Time complexity: O(1) average; O(n) worst.
                
        ***********************************************************************/

        final bool add (K key, V element, K function(K) retain)
        {
                if (table is null)
                    resize (Container.defaultInitialBuckets);

                auto hd = &table [Hash (key, table.length)];
                auto node = *hd;
                
                if (node is null)
                   {
                   *hd = heap.allocate.set (retain(key), element, null);
                   increment;
                   }
                else
                   {
                   auto p = node.findKey (key);
                   if (p)
                      {
                      if (element != p.value)
                         {
                         p.value = element;
                         mutate;
                         }
                      return false;
                      }
                   else
                      {
                      *hd = heap.allocate.set (retain(key), element, node);
                      increment;

                      // we only check load factor on add to nonempty bin
                      checkLoad; 
                      }
                   }
                return true;
        }

        /***********************************************************************

                Return the element associated with key

                param: a key
                param: a value reference (where returned value will reside)
                Returns: whether the key is contained or not
        
        ************************************************************************/

        final bool get (K key, ref V element)
        {
                if (count)
                   {
                   auto p = table [Hash (key, table.length)];
                   if (p && (p = p.findKey(key)) !is null)
                      {
                      element = p.value;
                      return true;
                      }
                   }
                return false;
        }

        /***********************************************************************

                Return the element associated with key

                param: a key
                Returns: a pointer to the located value, or null if not found
        
        ************************************************************************/

        final V* opIn_r (K key)
        {
                if (count)
                   {
                   auto p = table [Hash (key, table.length)];
                   if (p && (p = p.findKey(key)) !is null)
                       return &p.value;
                   }
                return null;
        }

        /***********************************************************************

                Does this set contain the given element?
        
                Time complexity: O(1) average; O(n) worst
                
        ***********************************************************************/

        final bool contains (V element)
        {
                return instances (element) > 0;
        }

        /***********************************************************************

                Time complexity: O(n).
        
        ************************************************************************/
        
        final bool keyOf (V value, ref K key)
        {
                if (count)
                    foreach (list; table)
                            if (list)
                               {
                               auto p = list.find (value);
                               if (p)
                                  {
                                  key = p.key;
                                  return true;
                                  }
                               }
                return false;
        }

        /***********************************************************************

                Time complexity: O(1) average; O(n) worst.
                
        ***********************************************************************/
        
        final bool containsKey (K key)
        {
                if (count)
                   {
                   auto p = table[Hash (key, table.length)];
                   if (p && p.findKey(key))
                       return true;
                   }
                return false;
        }

        /***********************************************************************

                Time complexity: O(1) average; O(n) worst.
        
        ***********************************************************************/
        
        final bool containsPair (K key, V element)
        {
                if (count)
                   {                    
                   auto p = table[Hash (key, table.length)];
                   if (p && p.findPair (key, element))
                       return true;
                   }
                return false;
        }

        /***********************************************************************

                Make an independent copy of the container. Does not clone
                elements
                
                Time complexity: O(n)
                
        ***********************************************************************/

        final HashMap dup ()
        {
                auto clone = new HashMap!(K, V, Hash, Reap, Heap) (loadFactor);

                if (count)
                   {
                   clone.buckets (buckets);

                   foreach (key, value; iterator)
                            clone.add (key, value);
                   }
                return clone;
        }

        /***********************************************************************

                Time complexity: O(1) average; O(n) worst.
        
        ***********************************************************************/
        
        final bool removeKey (K key)
        {
                V value;

                return take (key, value);
        }

        /***********************************************************************

                Time complexity: O(1) average; O(n) worst.
        
        ***********************************************************************/
        
        final bool replaceKey (K key, K replace)
        {
                if (count)
                   {
                   auto h = Hash (key, table.length);
                   auto hd = table[h];
                   auto trail = hd;
                   auto p = hd;

                   while (p)
                         {
                         auto n = p.next;
                         if (key == p.key)
                            {
                            if (p is hd)
                                table[h] = n;
                            else
                               trail.detachNext;
                            
                            // inject into new location
                            h = Hash (replace, table.length);
                            table[h] = p.set (replace, p.value, table[h]);
                            return true;
                            }
                         else
                            {
                            trail = p;
                            p = n;
                            }
                         }
                   }
                return false;
        }

        /***********************************************************************

                Time complexity: O(1) average; O(n) worst.
        
        ***********************************************************************/
        
        final bool replacePair (K key, V oldElement, V newElement)
        {
                if (count)
                   {
                   auto p = table [Hash (key, table.length)];
                   if (p)
                      {
                      auto c = p.findPair (key, oldElement);
                      if (c)
                         {
                         c.value = newElement;
                         mutate;
                         return true;
                         }
                      }
                   }
                return false;
        }

        /***********************************************************************

                Remove and expose the first element. Returns false when no
                more elements are contained
        
                Time complexity: O(n)

        ***********************************************************************/

        final bool take (ref V element)
        {
                if (count)
                    foreach (ref list; table)
                             if (list)
                                {
                                auto p = list;
                                element = p.value;
                                list = p.next;
                                decrement (p);
                                return true;
                                }
                return false;
        }

        /***********************************************************************

                Remove and expose the element associated with key

                param: a key
                param: a value reference (where returned value will reside)
                Returns: whether the key is contained or not
        
                Time complexity: O(1) average, O(n) worst

        ***********************************************************************/
        
        final bool take (K key, ref V value)
        {
                if (count)
                   {
                   auto p = &table [Hash (key, table.length)];
                   auto n = *p;

                   while ((n = *p) !is null)
                           if (key == n.key)
                              {
                              *p = n.next;
                              value = n.value;
                              decrement (n);
                              return true;
                              } 
                           else
                              p = &n.next;
                   }
                return false;
        }

        /***********************************************************************

                Operator shortcut for assignment

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
                throw new NoSuchElementException ("missing or invalid key");
        }

        /***********************************************************************

                Remove a set of values 

        ************************************************************************/

        final size_t remove (IContainer!(V) e, bool all = false)
        {
                auto i = count;
                foreach (value; e)
                         remove (value, all);
                return i - count;
        }

        /***********************************************************************

                Removes element instances, and returns the number of elements
                removed
                
                Time complexity: O(1) average; O(n) worst
        
        ************************************************************************/

        final size_t remove (V element, bool all = false)
        {
                auto i = count;
                
                if (i)
                    foreach (ref node; table)
                            {                         
                            auto p = node;
                            auto trail = node;

                            while (p)
                                  {     
                                  auto n = p.next;
                                  if (element == p.value)
                                     {
                                     decrement (p);
                                     if (p is node)
                                        {
                                        node = n;
                                        trail = n;
                                        }
                                     else
                                        trail.next = n;

                                     if (! all)
                                           return i - count;
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

                return i - count;
        }

        /***********************************************************************

                Replace instances of oldElement with newElement, and returns
                the number of replacements

                Time complexity: O(n).
                
        ************************************************************************/

        final size_t replace (V oldElement, V newElement, bool all = false)
        {
                size_t i;
                
                if (count && oldElement != newElement)
                    foreach (node; table)
                             while (node && (node = node.find(oldElement)) !is null)
                                   {
                                   ++i;
                                   mutate;
                                   node.value = newElement;
                                   if (! all)
                                         return i;
                                   }
                return i;
        }
        
        /***********************************************************************

                Clears the HashMap contents. Various attributes are
                retained, such as the internal table itself. Invoke
                reset() to drop everything.

                Time complexity: O(n)
                
        ***********************************************************************/

        final HashMap clear ()
        {
                return clear (false);
        }

        /***********************************************************************

                Reset the HashMap contents. This releases more memory 
                than clear() does

                Time complexity: O(n)
                
        ***********************************************************************/

        final HashMap reset ()
        {
                clear (true);
                heap.collect (table);
                table = null;
                return this;
        }

        /***********************************************************************

                Return the number of buckets

                Time complexity: O(1)

        ***********************************************************************/

        final size_t buckets ()
        {
                return table ? table.length : 0;
        }

        /***********************************************************************

                Set the desired number of buckets in the hash table. Any 
                value greater than or equal to one is OK.

                If different than current buckets, causes a version change
                
                Time complexity: O(n)

        ***********************************************************************/

        final void buckets (size_t cap)
        {
                if (cap < Container.defaultInitialBuckets)
                    cap = Container.defaultInitialBuckets;

                if (cap !is buckets)
                    resize (cap);
        }

        /***********************************************************************

                Set the number of buckets for the given threshold
                and resize as required
                
                Time complexity: O(n)

        ***********************************************************************/

        final void buckets (size_t cap, float threshold)
        {
                loadFactor = threshold;
                buckets (cast(size_t)(cap / threshold) + 1);
        }

        /***********************************************************************

                Return the current load factor threshold

                The Hash table occasionally checka against the load factor
                resizes itself if it has gone past it.

                Time complexity: O(1)

        ***********************************************************************/

        final float threshold ()
        {
                return loadFactor;
        }

        /***********************************************************************

                Set the resize threshold, and resize as required
                Set the current desired load factor. Any value greater 
                than 0 is OK. The current load is checked against it, 
                possibly causing a resize.
                
                Time complexity: O(n)
                
        ***********************************************************************/

        final void threshold (float desired)
        {
                assert (desired > 0.0);
                loadFactor = desired;
                if (table)
                    checkLoad;
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

                Sanity check

        ***********************************************************************/
                        
        final HashMap check ()
        {
                assert(!(table is null && count !is 0));
                assert((table is null || table.length > 0));
                assert(loadFactor > 0.0f);

                if (table)
                   {
                   size_t c = 0;
                   for (size_t i=0; i < table.length; ++i)
                        for (auto p = table[i]; p; p = p.next)
                            {
                            ++c;
                            assert(contains(p.value));
                            assert(containsKey(p.key));
                            assert(instances(p.value) >= 1);
                            assert(containsPair(p.key, p.value));
                            assert(Hash (p.key, table.length) is i);
                            }
                   assert(c is count);
                   }
                return this;
        }

        /***********************************************************************

                Count the element instances in the set (there can only be
                0 or 1 instances in a Set).
                
                Time complexity: O(n)
                
        ***********************************************************************/

        private size_t instances (V element)
        {
                size_t c = 0;
                foreach (node; table)
                         if (node)
                             c += node.count (element);
                return c;
        }

        /***********************************************************************

                 Check to see if we are past load factor threshold. If so,
                 resize so that we are at half of the desired threshold.
                 
        ***********************************************************************/

        private HashMap checkLoad ()
        {
                float fc = count;
                float ft = table.length;
                if (fc / ft > loadFactor)
                    resize (2 * cast(size_t)(fc / loadFactor) + 1);
                return this;
        }

        /***********************************************************************

                resize table to new capacity, rehashing all elements
                
        ***********************************************************************/

        private void resize (size_t newCap)
        {
                // Stdout.formatln ("resize {}", newCap);
                auto newtab = heap.allocate (newCap);
                mutate;

                foreach (bucket; table)
                         while (bucket)
                               {
                               auto n = bucket.next;
                               auto h = Hash (bucket.key, newCap);
                               bucket.next = newtab[h];
                               newtab[h] = bucket;
                               bucket = n;
                               }

                // release the prior table and assign new one
                heap.collect (table);
                table = newtab;
        }

        /***********************************************************************

                Remove the indicated node. We need to traverse buckets
                for this, since we're singly-linked only. Better to save
                the per-node memory than to gain a little on each remove

                Used by iterators only
                 
        ***********************************************************************/

        private bool removeNode (Ref node, Ref* list)
        {
                auto p = list;
                auto n = *p;

                while ((n = *p) !is null)
                        if (n is node)
                           {
                           *p = n.next;
                           decrement (n);
                           return true;
                           } 
                        else
                           p = &n.next;
                return false;
        }

        /***********************************************************************

                Clears the HashMap contents. Various attributes are
                retained, such as the internal table itself. Invoke
                reset() to drop everything.

                Time complexity: O(n)
                
        ***********************************************************************/

        private final HashMap clear (bool all)
        {
                mutate;

                // collect each node if we can't collect all at once
                if (heap.collect(all) is false)
                    foreach (v; table)
                             while (v)
                                   {
                                   auto n = v.next;
                                   decrement (v);
                                   v = n;
                                   }

                // retain table, but remove bucket chains
                foreach (ref v; table)
                         v = null;

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
                Reap (p.key, p.value);
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

                Iterator with no filtering

        ***********************************************************************/

        private struct Iterator
        {
                size_t  row;
                Ref     cell,
                        prior;
                Ref[]   table;
                HashMap owner;
                size_t  mutation;

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
                        while (cell is null)
                               if (row < table.length)
                                   cell = table [row++];
                               else
                                  return null;
  
                        prior = cell;
                        k = cell.key;
                        cell = cell.next;
                        return &prior.value;

                }

                /***************************************************************

                        Foreach support

                ***************************************************************/

                int opApply (int delegate(ref K key, ref V value) dg)
                {
                        int result;

                        auto c = cell;
                        loop: while (true)
                              {
                              while (c is null)
                                     if (row < table.length)
                                         c = table [row++];
                                     else
                                        break loop;
  
                              prior = c;
                              c = c.next;
                              if ((result = dg(prior.key, prior.value)) != 0)
                                   break loop;
                              }

                        cell = c;
                        return result;
                }                               

                /***************************************************************

                        Remove value at the current iterator location

                ***************************************************************/

                bool remove ()
                {
                        if (prior)
                            if (owner.removeNode (prior, &table[row-1]))
                               {
                               // ignore this change
                               ++mutation;
                               return true;
                               }

                        prior = null;
                        return false;
                }
        }
}


/*******************************************************************************

*******************************************************************************/

debug (HashMap)
{
        import tango.io.Stdout;
        import tango.core.Memory;
        import tango.time.StopWatch;

        void main()
        {
                // usage examples ...
                auto map = new HashMap!(char[], int);
                map.add ("foo", 1);
                map.add ("bar", 2);
                map.add ("wumpus", 3);

                // implicit generic iteration
                foreach (key, value; map)
                         Stdout.formatln ("{}:{}", key, value);

                // explicit generic iteration
                foreach (key, value; map.iterator)
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
                auto test = new HashMap!(int, int, Container.hash, Container.reap, Container.Chunk);
                test.allocator.config (1000, 1000);
                test.buckets = 1_500_000;
                const count = 1_000_000;
                StopWatch w;

                // benchmark adding
                w.start;
                for (int i=count; i--;)
                     test.add(i, i);
                Stdout.formatln ("{} adds: {}/s", test.size, test.size/w.stop);

                // benchmark reading
                w.start;
                for (int i=count; i--;)
                     test.get(i, v);
                Stdout.formatln ("{} lookups: {}/s", test.size, test.size/w.stop);

                // benchmark adding without allocation overhead
                test.clear;
                w.start;
                for (int i=count; i--;)
                     test.add(i, i);
                Stdout.formatln ("{} adds (after clear): {}/s", test.size, test.size/w.stop);

                // benchmark duplication
                w.start;
                auto dup = test.dup;
                Stdout.formatln ("{} element dup: {}/s", dup.size, dup.size/w.stop);

                // benchmark iteration
                w.start;
                foreach (key, value; test) {}
                Stdout.formatln ("{} element iteration: {}/s", test.size, test.size/w.stop);

                GC.collect;
                test.check;

                auto aa = new HashMap!(long, int, Container.hash, Container.reap, Container.Chunk);
                aa.allocator.config (5000, 1000);
                aa.buckets = 7_500_000;
                w.start;
                for (int i=5_000_000; i--;)
                     aa.add (i, 0);
                Stdout.formatln ("{} test iteration: {}/s", aa.size, aa.size/w.stop);

        }
}
