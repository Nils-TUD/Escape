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

    extern (C) uint gc_getAttr( void* p );
    extern (C) uint gc_setAttr( void* p, uint a );
    extern (C) uint gc_clrAttr( void* p, uint a );

    extern (C) void* gc_malloc( size_t sz, uint ba = 0 );
    extern (C) void* gc_calloc( size_t sz, uint ba = 0 );
    extern (C) void* gc_realloc( void* p, size_t sz, uint ba = 0 );
    extern (C) void gc_free( void* p );

    extern (C) size_t gc_sizeOf( void* p );

    extern (C) void gc_addRoot( void* p );
    extern (C) void gc_addRange( void* p, size_t sz );

    extern (C) void gc_removeRoot( void* p );
    extern (C) void gc_removeRange( void* p );
}


/**
 * This struct encapsulates all garbage collection functionality for the D
 * programming language.  Currently, the garbage collector is decided at
 * link time, but this design could adapt to dynamic garbage collector
 * loading with few semantic changes.
 */
struct GC
{
    /**
     * Enables the garbage collector if collections have previously been
     * suspended by a call to disable.  This function is reentrant, and
     * must be called once for every call to disable before the garbage
     * collector is enabled.
     */
    void enable()
    {
        gc_enable();
    }


    /**
     * Disables the garbage collector.  This function is reentrant, but
     * enable must be called once for each call to disable.
     */
    void disable()
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
    void collect()
    {
        gc_collect();
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
    uint getAttr( void* p )
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
    uint setAttr( void* p, uint a )
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
    uint clrAttr( void* p, uint a )
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
    void* malloc( size_t sz, uint ba = 0 )
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
    void* calloc( size_t sz, uint ba = 0 )
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
    void* realloc( void* p, size_t sz, uint ba = 0 )
    {
        return gc_realloc( p, sz, ba );
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
    void free( void* p )
    {
        gc_free( p );
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
    size_t sizeOf( void* p )
    {
        return gc_sizeOf( p );
    }


    /**
     * Adds the memory address referenced by p to an internal list of roots to
     * be scanned during a collection.  If p is null, no operation is
     * performed.
     *
     * Params:
     *  p = A pointer to a valid memory address or to null.
     */
    void addRoot( void* p )
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
    void addRange( void* p, size_t sz )
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
    void removeRoot( void* p )
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
    void removeRange( void* p )
    {
        gc_removeRange( p );
    }
}


/**
 * All GC routines are accessed through this variable.  This is done to
 * follow the established D coding style guidelines and to reduce the
 * impact of future design changes.
 */
GC gc;
