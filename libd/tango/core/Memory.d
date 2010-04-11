/**
 * The memory module provides an interface to the garbage collector and to
 * any other OS or API-level memory management facilities.
 *
 * Copyright: Copyright (C) 2005-2006 Sean Kelly.  All rights reserved.
 * License:   BSD style: $(LICENSE)
 * Authors:   Sean Kelly
 */
module tango.core.Memory;


private
{
    extern (C) void gc_init();
    extern (C) void gc_term();

    extern (C) void gc_enable();
    extern (C) void gc_disable();
    extern (C) void gc_collect();
    extern (C) void gc_minimize();

    extern (C) uint gc_getAttr( void* p );
    extern (C) uint gc_setAttr( void* p, uint a );
    extern (C) uint gc_clrAttr( void* p, uint a );

    extern (C) void*  gc_malloc( size_t sz, uint ba = 0 );
    extern (C) void*  gc_calloc( size_t sz, uint ba = 0 );
    extern (C) void*  gc_realloc( void* p, size_t sz, uint ba = 0 );
    extern (C) size_t gc_extend( void* p, size_t mx, size_t sz );
    extern (C) size_t gc_reserve( size_t sz );
    extern (C) void   gc_free( void* p );

    extern (C) void*   gc_addrOf( void* p );
    extern (C) size_t  gc_sizeOf( void* p );

    struct BlkInfo_
    {
        void*  base;
        size_t size;
        uint   attr;
    }

    extern (C) BlkInfo_ gc_query( void* p );

    extern (C) void gc_addRoot( void* p );
    extern (C) void gc_addRange( void* p, size_t sz );

    extern (C) void gc_removeRoot( void* p );
    extern (C) void gc_removeRange( void* p );

    extern(C) Object gc_weakpointerGet (void* wp);
    extern(C) void*  gc_weakpointerCreate (Object r);
    extern(C) void   gc_weakpointerDestroy (void* wp);
}


/**
 * This struct encapsulates all garbage collection functionality for the D
 * programming language.
 */
struct GC
{
    /**
     * Enables the garbage collector if collections have previously been
     * suspended by a call to disable.  This function is reentrant, and
     * must be called once for every call to disable before the garbage
     * collector is enabled.
     */
    static void enable()
    {
        gc_enable();
    }


    /**
     * Disables the garbage collector.  This function is reentrant, but
     * enable must be called once for each call to disable.
     */
    static void disable()
    {
        gc_disable();
    }


    /**
     * Begins a full collection.  While the meaning of this may change based
     * on the garbage collector implementation, typical behavior is to scan
     * all stack segments for roots, mark accessible memory blocks as alive,
     * and then to reclaim free space.  This action may need to suspend all
     * running threads for at least part of the collection process.
     */
    static void collect()
    {
        gc_collect();
    }

    /**
     * Indicates that the managed memory space be minimized by returning free
     * physical memory to the operating system.  The amount of free memory
     * returned depends on the allocator design and on program behavior.
     */
    static void minimize()
    {
        gc_minimize();
    }


    /**
     * Elements for a bit field representing memory block attributes.  These
     * are manipulated via the getAttr, setAttr, clrAttr functions.
     */
    enum BlkAttr : uint
    {
        FINALIZE = 0b0000_0001, /// Finalize the data in this block on collect.
        NO_SCAN  = 0b0000_0010, /// Do not scan through this block on collect.
        NO_MOVE  = 0b0000_0100  /// Do not move this memory block on collect.
    }


    /**
     * Contains aggregate information about a block of managed memory.  The
     * purpose of this struct is to support a more efficient query style in
     * instances where detailed information is needed.
     *
     * base = A pointer to the base of the block in question.
     * size = The size of the block, calculated from base.
     * attr = Attribute bits set on the memory block.
     */
    alias BlkInfo_ BlkInfo;


    /**
     * Returns a bit field representing all block attributes set for the memory
     * referenced by p.  If p references memory not originally allocated by this
     * garbage collector, points to the interior of a memory block, or if p is
     * null, zero will be returned.
     *
     * Params:
     *  p = A pointer to the root of a valid memory block or to null.
     *
     * Returns:
     *  A bit field containing any bits set for the memory block referenced by
     *  p or zero on error.
     */
    static uint getAttr( void* p )
    {
        return gc_getAttr( p );
    }


    /**
     * Sets the specified bits for the memory references by p.  If p references
     * memory not originally allocated by this garbage collector, points to the
     * interior of a memory block, or if p is null, no action will be performed.
     *
     * Params:
     *  p = A pointer to the root of a valid memory block or to null.
     *  a = A bit field containing any bits to set for this memory block.
     *
     *  The result of a call to getAttr after the specified bits have been
     *  set.
     */
    static uint setAttr( void* p, uint a )
    {
        return gc_setAttr( p, a );
    }


    /**
     * Clears the specified bits for the memory references by p.  If p
     * references memory not originally allocated by this garbage collector,
     * points to the interior of a memory block, or if p is null, no action
     * will be performed.
     *
     * Params:
     *  p = A pointer to the root of a valid memory block or to null.
     *  a = A bit field containing any bits to clear for this memory block.
     *
     * Returns:
     *  The result of a call to getAttr after the specified bits have been
     *  cleared.
     */
    static uint clrAttr( void* p, uint a )
    {
        return gc_clrAttr( p, a );
    }


    /**
     * Requests an aligned block of managed memory from the garbage collector.
     * This memory may be deleted at will with a call to free, or it may be
     * discarded and cleaned up automatically during a collection run.  If
     * allocation fails, this function will call onOutOfMemory which is
     * expected to throw an OutOfMemoryException.
     *
     * Params:
     *  sz = The desired allocation size in bytes.
     *  ba = A bitmask of the attributes to set on this block.
     *
     * Returns:
     *  A reference to the allocated memory or null if insufficient memory
     *  is available.
     *
     * Throws:
     *  OutOfMemoryException on allocation failure.
     */
    static void* malloc( size_t sz, uint ba = 0 )
    {
        return gc_malloc( sz, ba );
    }


    /**
     * Requests an aligned block of managed memory from the garbage collector,
     * which is initialized with all bits set to zero.  This memory may be
     * deleted at will with a call to free, or it may be discarded and cleaned
     * up automatically during a collection run.  If allocation fails, this
     * function will call onOutOfMemory which is expected to throw an
     * OutOfMemoryException.
     *
     * Params:
     *  sz = The desired allocation size in bytes.
     *  ba = A bitmask of the attributes to set on this block.
     *
     * Returns:
     *  A reference to the allocated memory or null if insufficient memory
     *  is available.
     *
     * Throws:
     *  OutOfMemoryException on allocation failure.
     */
    static void* calloc( size_t sz, uint ba = 0 )
    {
        return gc_calloc( sz, ba );
    }


    /**
     * If sz is zero, the memory referenced by p will be deallocated as if
     * by a call to free.  A new memory block of size sz will then be
     * allocated as if by a call to malloc, or the implementation may instead
     * resize the memory block in place.  The contents of the new memory block
     * will be the same as the contents of the old memory block, up to the
     * lesser of the new and old sizes.  Note that existing memory will only
     * be freed by realloc if sz is equal to zero.  The garbage collector is
     * otherwise expected to later reclaim the memory block if it is unused.
     * If allocation fails, this function will call onOutOfMemory which is
     * expected to throw an OutOfMemoryException.  If p references memory not
     * originally allocated by this garbage collector, or if it points to the
     * interior of a memory block, no action will be taken.  If ba is zero
     * (the default) and p references the head of a valid, known memory block
     * then any bits set on the current block will be set on the new block if a
     * reallocation is required.  If ba is not zero and p references the head
     * of a valid, known memory block then the bits in ba will replace those on
     * the current memory block and will also be set on the new block if a
     * reallocation is required.
     *
     * Params:
     *  p  = A pointer to the root of a valid memory block or to null.
     *  sz = The desired allocation size in bytes.
     *  ba = A bitmask of the attributes to set on this block.
     *
     * Returns:
     *  A reference to the allocated memory on success or null if sz is
     *  zero.  On failure, the original value of p is returned.
     *
     * Throws:
     *  OutOfMemoryException on allocation failure.
     */
    static void* realloc( void* p, size_t sz, uint ba = 0 )
    {
        return gc_realloc( p, sz, ba );
    }


    /**
     * Requests that the managed memory block referenced by p be extended in
     * place by at least mx bytes, with a desired extension of sz bytes.  If an
     * extension of the required size is not possible, if p references memory
     * not originally allocated by this garbage collector, or if p points to
     * the interior of a memory block, no action will be taken.
     *
     * Params:
     *  mx = The minimum extension size in bytes.
     *  sz = The  desired extension size in bytes.
     *
     * Returns:
     *  The size in bytes of the extended memory block referenced by p or zero
     *  if no extension occurred.
     */
    static size_t extend( void* p, size_t mx, size_t sz )
    {
        return gc_extend( p, mx, sz );
    }


    /**
     * Requests that at least sz bytes of memory be obtained from the operating
     * system and marked as free.
     *
     * Params:
     *  sz = The desired size in bytes.
     *
     * Returns:
     *  The actual number of bytes reserved or zero on error.
     */
    static size_t reserve( size_t sz )
    {
        return gc_reserve( sz );
    }


    /**
     * Deallocates the memory referenced by p.  If p is null, no action
     * occurs.  If p references memory not originally allocated by this
     * garbage collector, or if it points to the interior of a memory block,
     * no action will be taken.  The block will not be finalized regardless
     * of whether the FINALIZE attribute is set.  If finalization is desired,
     * use delete instead.
     *
     * Params:
     *  p = A pointer to the root of a valid memory block or to null.
     */
    static void free( void* p )
    {
        gc_free( p );
    }


    /**
     * Returns the base address of the memory block containing p.  This value
     * is useful to determine whether p is an interior pointer, and the result
     * may be passed to routines such as sizeOf which may otherwise fail.  If p
     * references memory not originally allocated by this garbage collector, if
     * p is null, or if the garbage collector does not support this operation,
     * null will be returned.
     *
     * Params:
     *  p = A pointer to the root or the interior of a valid memory block or to
     *      null.
     *
     * Returns:
     *  The base address of the memory block referenced by p or null on error.
     */
    static void* addrOf( void* p )
    {
        return gc_addrOf( p );
    }


    /**
     * Returns the true size of the memory block referenced by p.  This value
     * represents the maximum number of bytes for which a call to realloc may
     * resize the existing block in place.  If p references memory not
     * originally allocated by this garbage collector, points to the interior
     * of a memory block, or if p is null, zero will be returned.
     *
     * Params:
     *  p = A pointer to the root of a valid memory block or to null.
     *
     * Returns:
     *  The size in bytes of the memory block referenced by p or zero on error.
     */
    static size_t sizeOf( void* p )
    {
        return gc_sizeOf( p );
    }


    /**
     * Returns aggregate information about the memory block containing p.  If p
     * references memory not originally allocated by this garbage collector, if
     * p is null, or if the garbage collector does not support this operation,
     * BlkInfo.init will be returned.  Typically, support for this operation
     * is dependent on support for addrOf.
     *
     * Params:
     *  p = A pointer to the root or the interior of a valid memory block or to
     *      null.
     *
     * Returns:
     *  Information regarding the memory block referenced by p or BlkInfo.init
     *  on error.
     */
    static BlkInfo query( void* p )
    {
        return gc_query( p );
    }


    /**
     * Adds the memory address referenced by p to an internal list of roots to
     * be scanned during a collection.  If p is null, no operation is
     * performed.
     *
     * Params:
     *  p = A pointer to a valid memory address or to null.
     */
    static void addRoot( void* p )
    {
        gc_addRoot( p );
    }


    /**
     * Adds the memory block referenced by p and of size sz to an internal list
     * of ranges to be scanned during a collection.  If p is null, no operation
     * is performed.
     *
     * Params:
     *  p  = A pointer to a valid memory address or to null.
     *  sz = The size in bytes of the block to add.  If sz is zero then the
     *       no operation will occur.  If p is null then sz must be zero.
     */
    static void addRange( void* p, size_t sz )
    {
        gc_addRange( p, sz );
    }


    /**
     * Removes the memory block referenced by p from an internal list of roots
     * to be scanned during a collection.  If p is null or does not represent
     * a value previously passed to add(void*) then no operation is performed.
     *
     *  p  = A pointer to a valid memory address or to null.
     */
    static void removeRoot( void* p )
    {
        gc_removeRoot( p );
    }


    /**
     * Removes the memory block referenced by p from an internal list of ranges
     * to be scanned during a collection.  If p is null or does not represent
     * a value previously passed to add(void*, size_t) then no operation is
     * performed.
     *
     * Params:
     *  p  = A pointer to a valid memory address or to null.
     */
    static void removeRange( void* p )
    {
        gc_removeRange( p );
    }
    

    /**
     * Create a weak pointer to the given object.
     * Returns a pointer to an opaque struct allocated in C memory.
     */
    static void* weakPointerCreate( Object o )
    {
        return gc_weakpointerCreate (o);
    }


    /**
     * Destroy a weak pointer returned by weakpointerCreate().
     * If null is passed, nothing happens.
     */
    static void weakPointerDestroy( void* p )
    {
        return gc_weakpointerDestroy (p);
    }


    /**
     * Query a weak pointer and return either the object passed to
     * weakpointerCreate, or null if it was free'd in the meantime.
     * If null is passed, null is returned.
     */
    static Object weakPointerGet( void* p )
    {
        return gc_weakpointerGet (p);
    }


    /**
    * returns the amount to allocate to keep some extra space
    * for large allocations the extra allocated space decreases, but is still enough
    * so that the number of reallocations when linearly growing stays logaritmic
    * Params:
    * newlength = the number of elements to allocate
    * elSize = size of one element
    */
    static size_t growLength (size_t newlength, size_t elSize=1)
    {   
        return growLength (newlength, elSize, 100, 0, 1);
    }
    

    /**
    * returns the amount to allocate to keep some extra space
    * for large allocations the extra allocated space decreases, but is still enough
    * so that the number of reallocations when linearly growing stays logaritmic
    * Params:
    * newlength = the number of elements to allocate
    * elSize = size of one element
    * a = maximum extra space in percent (the allocated space gets rounded up, so might be larger)
    * b = flatness factor, how fast the extra space decreases with array size (the larger the more constant)
    * minBits = minimum number of bits of newlength
    */
    static size_t growLength (size_t newlength, size_t elSize, size_t a, size_t b=0, size_t minBits=1)
    {
        static size_t log2(size_t c)
        {
            // could use the bsr bit op
            size_t i=1;
            while(c >>= 1)
                  ++i;
            return i;
        }

        size_t newext = 0;
        size_t newcap = newlength*elSize;
        long mult = 100 + a*(minBits+b) / (log2(newlength)+b);
        newext = elSize*cast(size_t)(((newcap * mult)+99) / 100);
        newcap = newext > newcap ? newext : newcap; // just to handle overflows
        return newcap;
    }
}
