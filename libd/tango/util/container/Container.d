/*******************************************************************************

        copyright:      Copyright (c) 2008 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Apr 2008: Initial release
                        Jan 2009: Added GCChunk allocator

        authors:        Kris, schveiguy

        Since:          0.99.7

*******************************************************************************/

module tango.util.container.Container;

private import tango.core.Memory;

private import tango.stdc.stdlib;
private import tango.stdc.string;

/*******************************************************************************

        Utility functions and constants

*******************************************************************************/

struct Container
{
        /***********************************************************************
        
               default initial number of buckets of a non-empty hashmap

        ***********************************************************************/
        
        static size_t defaultInitialBuckets = 31;

        /***********************************************************************

                default load factor for a non-empty hashmap. The hash 
                table is resized when the proportion of elements per 
                buckets exceeds this limit
        
        ***********************************************************************/
        
        static float defaultLoadFactor = 0.75f;

        /***********************************************************************
        
                generic value reaper, which does nothing

        ***********************************************************************/
        
        static void reap(V) (V v) {}

        /***********************************************************************
        
                generic key/value reaper, which does nothing

        ***********************************************************************/
        
        static void reap(K, V) (K k, V v) {}

        /***********************************************************************

                generic hash function, using the default hashing. Thanks
                to 'mwarning' for the optimization suggestion

        ***********************************************************************/

        static size_t hash(K) (K k, size_t length)
        {
                static if (is(K : int) || is(K : uint) || 
                           is(K : long) || is(K : ulong) || 
                           is(K : short) || is(K : ushort) ||
                           is(K : byte) || is(K : ubyte) ||
                           is(K : char) || is(K : wchar) || is (K : dchar))
                           return cast(size_t) (k % length);
                else
                   return (typeid(K).getHash(&k) & 0x7FFFFFFF) % length;
        }

        /***********************************************************************
        
                generic GC allocation manager
                
        ***********************************************************************/
        
        struct Collect(T)
        {
                /***************************************************************
        
                        allocate a T sized memory chunk
                        
                ***************************************************************/
        
                T* allocate ()
                {       
                        return cast(T*) GC.calloc (T.sizeof);
                }
        
                /***************************************************************
        
                        allocate an array of T sized memory chunks
                        
                ***************************************************************/
        
                T*[] allocate (size_t count)
                {
                        return new T*[count];
                }
        
                /***************************************************************
        
                        Invoked when a specific T[] is discarded
                        
                ***************************************************************/
        
                void collect (T* p)
                {
                        if (p)
                            delete p;
                }
        
                /***************************************************************
        
                        Invoked when a specific T[] is discarded
                        
                ***************************************************************/
        
                void collect (T*[] t)
                {      
                        if (t)
                            delete t;
                }

                /***************************************************************
        
                        Invoked when clear/reset is called on the host. 
                        This is a shortcut to clear everything allocated.
        
                        Should return true if supported, or false otherwise. 
                        False return will cause a series of discrete collect
                        calls

                ***************************************************************/
        
                bool collect (bool all = true)
                {
                        return false;
                }
        }        
        
                
        /***********************************************************************
        
                Malloc allocation manager.

                Note that, due to GC behaviour, you should not configure
                a custom allocator for containers holding anything managed
                by the GC. For example, you cannot use a MallocAllocator
                to manage a container of classes or strings where those 
                were allocated by the GC. Once something is owned by a GC
                then it's lifetime must be managed by GC-managed entities
                (otherwise the GC may think there are no live references
                and prematurely collect container contents).
        
                You can explicity manage the collection of keys and values
                yourself by providing a reaper delegate. For example, if 
                you use a MallocAllocator to manage key/value pairs which
                are themselves allocated via malloc, then you should also
                provide a reaper delegate to collect those as required.      
                
        ***********************************************************************/
        
        struct Malloc(T)
        {
                /***************************************************************
        
                        allocate an array of T sized memory chunks
                        
                ***************************************************************/
        
                T* allocate ()
                {
                        return cast(T*) calloc (1, T.sizeof);
                }
        
                /***************************************************************
        
                        allocate an array of T sized memory chunks
                        
                ***************************************************************/
        
                T*[] allocate (size_t count)
                {
                        return (cast(T**) calloc(count, (T*).sizeof)) [0 .. count];
                }
        
                /***************************************************************
        
                        Invoked when a specific T[] is discarded
                        
                ***************************************************************/
        
                void collect (T*[] t)
                {      
                        if (t.length)
                            free (t.ptr);
                }

                /***************************************************************
        
                        Invoked when a specific T[] is discarded
                        
                ***************************************************************/
        
                void collect (T* p)
                {       
                        if (p)
                            free (p);
                }
        
                /***************************************************************
        
                        Invoked when clear/reset is called on the host. 
                        This is a shortcut to clear everything allocated.
        
                        Should return true if supported, or false otherwise. 
                        False return will cause a series of discrete collect
                        calls
                        
                ***************************************************************/
        
                bool collect (bool all = true)
                {
                        return false;
                }
        }        
        
        
        /***********************************************************************
        
                Chunk allocator

                Can save approximately 30% memory for small elements (tested 
                with integer elements and a chunk size of 1000), and is at 
                least twice as fast at adding elements when compared to the 
                default allocator (approximately 50x faster with LinkedList)
        
                Note that, due to GC behaviour, you should not configure
                a custom allocator for containers holding anything managed
                by the GC. For example, you cannot use a MallocAllocator
                to manage a container of classes or strings where those 
                were allocated by the GC. Once something is owned by a GC
                then it's lifetime must be managed by GC-managed entities
                (otherwise the GC may think there are no live references
                and prematurely collect container contents).
        
                You can explicity manage the collection of keys and values
                yourself by providing a reaper delegate. For example, if 
                you use a MallocAllocator to manage key/value pairs which
                are themselves allocated via malloc, then you should also
                provide a reaper delegate to collect those as required.
        
        ***********************************************************************/
        
        struct Chunk(T)
        {
                static assert (T.sizeof >= (T*).sizeof, "The Chunk allocator can only be used for data sizes of at least " ~ ((T*).sizeof).stringof[0..$-1] ~ " bytes!");
                
                private T[]     list;
                private T[][]   lists;
                private size_t     index;
                private size_t     freelists;
                private size_t     presize = 0;
                private size_t     chunks = 1000;

                private struct Discarded
                {
                        Discarded* next;
                }
                private Discarded* discarded;
                
                /***************************************************************
        
                        set the chunk size and preallocate lists
                        
                ***************************************************************/
        
                void config (size_t chunks, size_t presize)
                {
                        this.chunks = chunks;
                        this.presize = presize;
                        lists.length = presize;

                        foreach (ref list; lists)
                                 list = block;
                }
        
                /***************************************************************
        
                        allocate an array of T sized memory chunks
                        
                ***************************************************************/

                T* allocate ()
                {
                        if (index >= list.length)
                            if (discarded)
                               {    
                               auto p = discarded;
                               discarded = p.next;
                               return cast(T*) p;
                               }
                            else
                               newlist;
       
                        return (&list[index++]);
                }
        
                /***************************************************************
        
                        allocate an array of T sized memory chunks
                        
                ***************************************************************/
        
                T*[] allocate (size_t count)
                {
                        return (cast(T**) calloc(count, (T*).sizeof)) [0 .. count];
                }
        
                /***************************************************************
        
                        Invoked when a specific T[] is discarded
                        
                ***************************************************************/
        
                void collect (T*[] t)
                {      
                        if (t.length)
                            free (t.ptr);
                }

                /***************************************************************
        
                        Invoked when a specific T is discarded
                        
                ***************************************************************/
        
                void collect (T* p)
                {      
                        if (p)
                           {
                           assert (T.sizeof >= (T*).sizeof);
                           auto d = cast(Discarded*) p;
                           d.next = discarded;
                           discarded = d;
                           }
                }
        
                /***************************************************************
        
                        Invoked when clear/reset is called on the host. 
                        This is a shortcut to clear everything allocated.
        
                        Should return true if supported, or false otherwise. 
                        False return will cause a series of discrete collect
                        calls
                        
                ***************************************************************/
        
                bool collect (bool all = true)
                {
                        freelists = 0;
                        newlist;
                        if (all)
                           {
                           foreach (list; lists)
                                    free (list.ptr);
                           lists.length = 0;
                           }
                        return true;
                }
              
                /***************************************************************
        
                        list manager
                        
                ***************************************************************/
        
                private void newlist ()
                {
                        index = 0;
                        if (freelists >= lists.length)
                           {
                           lists.length = lists.length + 1;
                           lists[$-1] = block;
                           }
                        list = lists[freelists++];
                }
        
                /***************************************************************
        
                        list allocator
                        
                ***************************************************************/
        
                private T[] block ()
                {
                        return (cast(T*) calloc (chunks, T.sizeof)) [0 .. chunks];
                }
        }        

        /***********************************************************************
        
                GCChunk allocator

                Like the Chunk allocator, this allocates elements in chunks,
                but allows you to allocate elements that can have GC pointers.

                Tests have shown about a 60% speedup when using the GC chunk
                allocator for a Hashmap!(int, int).
        
        ***********************************************************************/
        struct GCChunk(T, uint chunkSize)
        {
            static if(T.sizeof < (void*).sizeof)
            {
                static assert(false, "Error, allocator for " ~ T.stringof ~ " failed to instantiate");
            }

            /**
             * This is the form used to link recyclable elements together.
             */
            struct element
            {
                element *next;
            }

            /**
             * A chunk of elements
             */
            struct chunk
            {
                /**
                 * The next chunk in the chain
                 */
                chunk *next;

                /**
                 * The previous chunk in the chain.  Required for O(1) removal
                 * from the chain.
                 */
                chunk *prev;

                /**
                 * The linked list of free elements in the chunk.  This list is
                 * amended each time an element in this chunk is freed.
                 */
                element *freeList;

                /**
                 * The number of free elements in the freeList.  Used to determine
                 * whether this chunk can be given back to the GC
                 */
                uint numFree;

                /**
                 * The elements in the chunk.
                 */
                T[chunkSize] elems;

                /**
                 * Allocate a T* from the free list.
                 */
                T *allocateFromFree()
                {
                    element *x = freeList;
                    freeList = x.next;
                    //
                    // clear the pointer, this clears the element as if it was
                    // newly allocated
                    //
                    x.next = null;
                    numFree--;
                    return cast(T*)x;
                }

                /**
                 * deallocate a T*, send it to the free list
                 *
                 * returns true if this chunk no longer has any used elements.
                 */
                bool deallocate(T *t)
                {
                    //
                    // clear the element so the GC does not interpret the element
                    // as pointing to anything else.
                    //
                    memset(t, 0, (T).sizeof);
                    element *x = cast(element *)t;
                    x.next = freeList;
                    freeList = x;
                    return (++numFree == chunkSize);
                }
            }

            /**
             * The chain of used chunks.  Used chunks have had all their elements
             * allocated at least once.
             */
            chunk *used;

            /**
             * The fresh chunk.  This is only used if no elements are available in
             * the used chain.
             */
            chunk *fresh;

            /**
             * The next element in the fresh chunk.  Because we don't worry about
             * the free list in the fresh chunk, we need to keep track of the next
             * fresh element to use.
             */
            uint nextFresh;

            /**
             * Allocate a T*
             */
            T* allocate()
            {
                if(used !is null && used.numFree > 0)
                {
                    //
                    // allocate one element of the used list
                    //
                    T* result = used.allocateFromFree();
                    if(used.numFree == 0)
                        //
                        // move used to the end of the list
                        //
                        used = used.next;
                    return result;
                }

                //
                // no used elements are available, allocate out of the fresh
                // elements
                //
                if(fresh is null)
                {
                    fresh = new chunk;
                    nextFresh = 0;
                }

                T* result = &fresh.elems[nextFresh];
                if(++nextFresh == chunkSize)
                {
                    if(used is null)
                    {
                        used = fresh;
                        fresh.next = fresh;
                        fresh.prev = fresh;
                    }
                    else
                    {
                        //
                        // insert fresh into the used chain
                        //
                        fresh.prev = used.prev;
                        fresh.next = used;
                        fresh.prev.next = fresh;
                        fresh.next.prev = fresh;
                        if(fresh.numFree != 0)
                        {
                            //
                            // can recycle elements from fresh
                            //
                            used = fresh;
                        }
                    }
                    fresh = null;
                }
                return result;
            }

            T*[] allocate(uint count)
            {
                return new T*[count];
            }


            /**
             * free a T*
             */
            void collect(T* t)
            {
                //
                // need to figure out which chunk t is in
                //
                chunk *cur = cast(chunk *)GC.addrOf(t);

                if(cur !is fresh && cur.numFree == 0)
                {
                    //
                    // move cur to the front of the used list, it has free nodes
                    // to be used.
                    //
                    if(cur !is used)
                    {
                        if(used.numFree != 0)
                        {
                            //
                            // first, unlink cur from its current location
                            //
                            cur.prev.next = cur.next;
                            cur.next.prev = cur.prev;

                            //
                            // now, insert cur before used.
                            //
                            cur.prev = used.prev;
                            cur.next = used;
                            used.prev = cur;
                            cur.prev.next = cur;
                        }
                        used = cur;
                    }
                }

                if(cur.deallocate(t))
                {
                    //
                    // cur no longer has any elements in use, it can be deleted.
                    //
                    if(cur.next is cur)
                    {
                        //
                        // only one element, don't free it.
                        //
                    }
                    else
                    {
                        //
                        // remove cur from list
                        //
                        if(used is cur)
                        {
                            //
                            // update used pointer
                            //
                            used = used.next;
                        }
                        cur.next.prev = cur.prev;
                        cur.prev.next = cur.next;
                        delete cur;
                    }
                }
            }

            void collect(T*[] t)
            {
                if(t)
                    delete t;
            }

            /**
             * Deallocate all chunks used by this allocator.  Depends on the GC to do
             * the actual collection
             */
            bool collect(bool all = true)
            {
                used = null;

                //
                // keep fresh around
                //
                if(fresh !is null)
                {
                    nextFresh = 0;
                    fresh.freeList = null;
                }

                return true;
            }

        }

        /***********************************************************************

                aliases to the correct Default allocator depending on how big
                the type is.  It makes less sense to use a GCChunk allocator
                if the type is going to be larger than a page (currently there
                is no way to get the page size from the GC, so we assume 4096
                bytes).  If not more than one unit can fit into a page, then
                we use the default GC allocator.

        ***********************************************************************/
        template DefaultCollect(T)
        {
            static if((T).sizeof + ((void*).sizeof * 3) + uint.sizeof >= 4095 / 2)
            {
                alias Collect!(T) DefaultCollect;
            }
            else
            {
                alias GCChunk!(T, (4095 - ((void *).sizeof * 3) - uint.sizeof) / (T).sizeof) DefaultCollect;
            }
            // TODO: see if we can automatically figure out whether a type has
            // any pointers in it, this would allow automatic usage of the
            // Chunk allocator for added speed.
        }
}


