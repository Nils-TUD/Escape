/*******************************************************************************

        copyright:      Copyright (c) 2008 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)
        
        version:        Initial release: April 2008      
        
        author:         Kris

        Since:          0.99.7

*******************************************************************************/

module tango.util.container.more.CacheMap;

private import tango.stdc.stdlib;

private import tango.util.container.HashMap;

public  import tango.util.container.Container;

/******************************************************************************

        CacheMap extends the basic hashmap type by adding a limit to 
        the number of items contained at any given time. In addition, 
        CacheMap sorts the cache entries such that those entries 
        frequently accessed are at the head of the queue, and those
        least frequently accessed are at the tail. When the queue 
        becomes full, old entries are dropped from the tail and are 
        reused to house new cache entries. 

        In other words, it retains MRU items while dropping LRU when
        capacity is reached.

        This is great for keeping commonly accessed items around, while
        limiting the amount of memory used. Typically, the queue size 
        would be set in the thousands

******************************************************************************/

class CacheMap (K, V, alias Hash = Container.hash, 
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
                                        tail;
        // dimension of queue
        private uint                    capacity;

       /**********************************************************************

                Construct a cache with the specified maximum number of 
                entries. Additions to the cache beyond this number will
                reuse the slot of the least-recently-referenced cache
                entry. 

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
//                free (links.ptr);
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

                Iterate from MRU to LRU entries

        ***********************************************************************/

        final int opApply (int delegate(ref K key, ref V value) dg)
        {
                        K   key;
                        V   value;
                        int result;

                        auto node = head;
                        auto i = hash.size;
                        while (i--)
                              {
                              key = node.key;
                              value = node.value;
                              if ((result = dg(key, value)) != 0)
                                   break;
                              node = node.next;
                              }
                        return result;
        }

        /**********************************************************************

                Get the cache entry identified by the given key

        **********************************************************************/

        bool get (K key, ref V value)
        {
                Ref entry = null;

                // if we find 'key' then move it to the list head
                if (hash.get (key, entry))
                   {
                   value = entry.value;
                   reReference (entry);
                   return true;
                   }
                return false;
        }

        /**********************************************************************

                Place an entry into the cache and associate it with the
                provided key. Note that there can be only one entry for
                any particular key. If two entries are added with the 
                same key, the second effectively overwrites the first.

                Returns true if we added a new entry; false if we just
                replaced an existing one

        **********************************************************************/

        final bool add (K key, V value)
        {
                Ref entry = null;

                // already in the list? -- replace entry
                if (hash.get (key, entry))
                   {
                   // set the new item for this key and move to list head
                   reReference (entry.set (key, value));
                   return false;
                   }

                // create a new entry at the list head 
                addEntry (key, value);
                return true;
        }

        /**********************************************************************

                Remove the cache entry associated with the provided key. 
                Returns false if there is no such entry.

        **********************************************************************/

        final bool take (K key)
        {
                V value;

                return take (key, value);
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

                Add an entry into the queue. If the queue is full, the
                least-recently-referenced entry is reused for the new
                addition. 

        **********************************************************************/

        private Ref addEntry (K key, V value)
        {
                assert (capacity);

                if (hash.size < capacity)
                    hash.add (key, tail);
                else
                   {
                   // we're re-using a prior QueuedEntry, so reap and
                   // relocate the existing hash-table entry first
                   Reap (tail.key, tail.value);
                   if (! hash.replaceKey (tail.key, key))
                         throw new Exception ("key missing!");
                   }

                // place at head of list
                return reReference (tail.set (key, value));
        }

        /**********************************************************************
        
                A doubly-linked list entry, used as a wrapper for queued 
                cache entries. Note that this class itself is a valid cache 
                entry.
        
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

debug (CacheMap)
{
        import tango.io.Stdout;
        import tango.core.Memory;
        import tango.time.StopWatch;

        void main()
        {
                int v;
                auto map = new CacheMap!(char[], int)(2);
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
                auto test = new CacheMap!(int, int, Container.hash, Container.reap, Container.Chunk) (1000);
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

                test.hash.check;
        }
}
        
