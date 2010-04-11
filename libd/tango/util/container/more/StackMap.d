/*******************************************************************************

        copyright:      Copyright (c) 2008 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
        
        version:        Initial release: April 2008      
        
        author:         Kris

        Since:          0.99.7

*******************************************************************************/

module tango.util.container.more.StackMap;

private import tango.stdc.stdlib;

private import tango.util.container.HashMap;

public  import tango.util.container.Container;

/******************************************************************************

        StackMap extends the basic hashmap type by adding a limit to 
        the number of items contained at any given time. In addition, 
        StackMap retains the order in which elements were added, and
        employs that during foreach() traversal. Additions to the map
        beyond the capacity will result in an exception being thrown.

        You can push and pop things just like an typical stack, and/or
        traverse the entries in the order they were added, though with 
        the additional capability of retrieving and/or removing by key.

        Note also that a push/add operation will replace an existing 
        entry with the same key, exposing a twist on the stack notion.

******************************************************************************/

class StackMap (K, V, alias Hash = Container.hash, 
                      alias Reap = Container.reap, 
                      alias Heap = Container.Collect) 
{
        private alias QueuedEntry       Type;
        private alias Type              *Ref;
        private alias HashMap!(K, Ref, Hash, reaper, Heap) Map;
        private Map                     hash;
        private Type[]                  links;

        // extents of queue
        private Ref                     head,
                                        tail,
                                        start;
        // dimension of queue
        private uint                    capacity;

       /**********************************************************************

                Construct a cache with the specified maximum number of 
                entries. Additions to the cache beyond this number will
                throw an exception

        **********************************************************************/

        this (uint capacity)
        {
                hash = new Map;
                this.capacity = capacity;
                hash.buckets (capacity, 0.75);
                //links = (cast(Ref) calloc(capacity, Type.sizeof))[0..capacity];
                links.length = capacity;

                // create empty list
                head = tail = &links[0];
                foreach (ref link; links[1..$])
                        {
                        link.prev = tail;
                        tail.next = &link;
                        tail = &link;
                        }
        }

        /***********************************************************************

                clean up when done
                
        ***********************************************************************/

        ~this ()
        {
                //free (links.ptr);
        }

        /***********************************************************************

                Reaping callback for the hashmap, acting as a trampoline

        ***********************************************************************/

        static void reaper(K, R) (K k, R r) 
        {
                Reap (k, r.value);
        }

        /***********************************************************************


        ***********************************************************************/

        final uint size ()
        {
                return hash.size;
        }

        /***********************************************************************


        ***********************************************************************/

        final void clear ()
        {
                hash.clear;
                start = null;
        }

        /**********************************************************************

                Place an entry into the cache and associate it with the
                provided key. Note that there can be only one entry for
                any particular key. If two entries are added with the 
                same key, the second effectively overwrites the first.

                Returns true if we added a new entry; false if we just
                replaced an existing one. A replacement does not change
                the order of the keys, and thus does not change stack
                ordering.

        **********************************************************************/

        final bool push (K key, V value)
        {
                return add (key, value);
        }

        /**********************************************************************

                Remove and return the more recent addition to the stack

        **********************************************************************/

        bool popHead (ref K key, ref V value)
        {
                if (start)
                   {
                   key = head.key;
                   return take (key, value);
                   }
                return false;
        }

        /**********************************************************************

                Remove and return the oldest addition to the stack

        **********************************************************************/

        bool popTail (ref K key, ref V value)
        {
                if (start)
                   {
                   key = start.key;
                   return take (key, value);
                   }
                return false;
        }

        /***********************************************************************

                Iterate from the oldest to the most recent additions

        ***********************************************************************/

        final int opApply (int delegate(ref K key, ref V value) dg)
        {
                        K   key;
                        V   value;
                        int result;

                        auto node = start;
                        while (node)
                              {
                              key = node.key;
                              value = node.value;
                              if ((result = dg(key, value)) != 0)
                                   break;
                              node = node.prev;
                              }
                        return result;
        }

        /**********************************************************************

                Place an entry into the cache and associate it with the
                provided key. Note that there can be only one entry for
                any particular key. If two entries are added with the 
                same key, the second effectively overwrites the first.

                Returns true if we added a new entry; false if we just
                replaced an existing one. A replacement does not change
                the order of the keys, and thus does not change stack
                ordering.

        **********************************************************************/

        final bool add (K key, V value)
        {
                Ref entry = null;

                // already in the list? -- replace entry
                if (hash.get (key, entry))
                   {
                   entry.set (key, value);
                   return false;
                   }

                // create a new entry at the list head 
                entry = addEntry (key, value);
                if (start is null)
                    start = entry;
                return true;
        }

        /**********************************************************************

                Get the cache entry identified by the given key

        **********************************************************************/

        bool get (K key, ref V value)
        {
                Ref entry = null;

                if (hash.get (key, entry))
                   {
                   value = entry.value;
                   return true;
                   }
                return false;
        }

        /**********************************************************************

                Remove (and return) the cache entry associated with the 
                provided key. Returns false if there is no such entry.

        **********************************************************************/

        final bool take (K key, ref V value)
        {
                Ref entry = null;
                if (hash.get (key, entry))
                   {
                   value = entry.value;

                   if (entry is start)
                       start = entry.prev;
                
                   // don't actually kill the list entry -- just place
                   // it at the list 'tail' ready for subsequent reuse
                   deReference (entry);

                   // remove the entry from hash
                   hash.removeKey (key);
                   return true;
                   }
                return false;
        }

        /**********************************************************************

                Place a cache entry at the tail of the queue. This makes
                it the least-recently referenced.

        **********************************************************************/

        private Ref deReference (Ref entry)
        {
                if (entry !is tail)
                   {
                   // adjust head
                   if (entry is head)
                       head = entry.next;

                   // move to tail
                   entry.extract;
                   tail = entry.append (tail);
                   }
                return entry;
        }

        /**********************************************************************

                Move a cache entry to the head of the queue. This makes
                it the most-recently referenced.

        **********************************************************************/

        private Ref reReference (Ref entry)
        {
                if (entry !is head)
                   {
                   // adjust tail
                   if (entry is tail)
                       tail = entry.prev;

                   // move to head
                   entry.extract;
                   head = entry.prepend (head);
                   }
                return entry;
        }

        /**********************************************************************

                Add an entry into the queue. An exception is thrown if 
                the queue is full

        **********************************************************************/

        private Ref addEntry (K key, V value)
        {
                if (hash.size < capacity)
                   {
                   hash.add (key, tail);
                   return reReference (tail.set (key, value));
                   }

                throw new Exception ("Full");
        }

        /**********************************************************************
        
                A doubly-linked list entry, used as a wrapper for queued 
                cache entries. 
        
        **********************************************************************/
        
        private struct QueuedEntry
        {
                private K               key;
                private Ref             prev,
                                        next;
                private V               value;
        
                /**************************************************************
        
                        Set this linked-list entry with the given arguments. 

                **************************************************************/
        
                Ref set (K key, V value)
                {
                        this.value = value;
                        this.key = key;
                        return this;
                }
        
                /**************************************************************
        
                        Insert this entry into the linked-list just in 
                        front of the given entry.
        
                **************************************************************/
        
                Ref prepend (Ref before)
                {
                        if (before)
                           {
                           prev = before.prev;
        
                           // patch 'prev' to point at me
                           if (prev)
                               prev.next = this;
        
                           //patch 'before' to point at me
                           next = before;
                           before.prev = this;
                           }
                        return this;
                }
        
                /**************************************************************
                        
                        Add this entry into the linked-list just after 
                        the given entry.
        
                **************************************************************/
        
                Ref append (Ref after)
                {
                        if (after)
                           {
                           next = after.next;
        
                           // patch 'next' to point at me
                           if (next)
                               next.prev = this;
        
                           //patch 'after' to point at me
                           prev = after;
                           after.next = this;
                           }
                        return this;
                }
        
                /**************************************************************
        
                        Remove this entry from the linked-list. The 
                        previous and next entries are patched together 
                        appropriately.
        
                **************************************************************/
        
                Ref extract ()
                {
                        // make 'prev' and 'next' entries see each other
                        if (prev)
                            prev.next = next;
        
                        if (next)
                            next.prev = prev;
        
                        // Murphy's law 
                        next = prev = null;
                        return this;
                }
        }
}


/*******************************************************************************

*******************************************************************************/

debug (StackMap)
{
        import tango.io.Stdout;
        import tango.core.Memory;
        import tango.time.StopWatch;

        void main()
        {
                int v;
                auto map = new StackMap!(char[], int)(3);
                map.add ("foo", 1);
                map.add ("bar", 2);
                map.add ("wumpus", 3);
                foreach (k, v; map)
                         Stdout.formatln ("{} {}", k, v);

                Stdout.newline;
                map.get ("bar", v);
                foreach (k, v; map)
                         Stdout.formatln ("{} {}", k, v);

                Stdout.newline;
                map.get ("bar", v);
                foreach (k, v; map)
                         Stdout.formatln ("{} {}", k, v);

                Stdout.newline;
                map.get ("foo", v);
                foreach (k, v; map)
                         Stdout.formatln ("{} {}", k, v);

                Stdout.newline;
                map.get ("wumpus", v);
                foreach (k, v; map)
                         Stdout.formatln ("{} {}", k, v);


                // setup for benchmark, with a cache of integers
                auto test = new StackMap!(int, int, Container.hash, Container.reap, Container.Chunk) (1000000);
                const count = 1_000_000;
                StopWatch w;

                // benchmark adding
                w.start;
                for (int i=count; i--;)
                     test.add (i, i);
                Stdout.formatln ("{} adds: {}/s", count, count/w.stop);

                // benchmark reading
                w.start;
                for (int i=count; i--;)
                     test.get (i, v);
                Stdout.formatln ("{} lookups: {}/s", count, count/w.stop);

                // benchmark iteration
                w.start;
                foreach (key, value; test) {}
                Stdout.formatln ("{} element iteration: {}/s", test.size, test.size/w.stop);
        }
}
        
