/**
 * This module contains a minimal garbage collector implementation according to
 * Tango requirements.  This library is mostly intended to serve as an example,
 * but it is usable in applications which do not rely on a garbage collector
 * to clean up memory (ie. when dynamic array resizing is not used, and all
 * memory allocated with 'new' is freed deterministically with 'delete').
 *
 * Please note that block attribute data must be tracked, or at a minimum, the
 * FINALIZE bit must be tracked for any allocated memory block because calling
 * cr_finalize on a non-object block can result in an access violation.  In the
 * allocator below, this tracking is done via a leading uint bitmask.  A real
 * allocator may do better to store this data separately, similar to the basic
 * GC normally used by Tango.
 *
 * Copyright: Public Domain
 * License:   Public Domain
 * Authors:   Sean Kelly
 */

private import tango.stdc.stdlib;
private import tango.stdc.string; // for memset

private
{
    extern (C) void thread_init();
    extern (C) void cr_finalize( void* p, bool det = true );

    enum BlkAttr : uint
    {
        FINALIZE = 0b0000_0001,
        NO_SCAN  = 0b0000_0010,
        NO_MOVE  = 0b0000_0100,
        ALL_BITS = 0b1111_1111
    }

    alias typeof(BlkAttr.ALL_BITS) blk_t;

    //
    // NOTE: A mark/sweep GC might find the following functions useful
    //
    /+
    extern (C) void* cr_stackBottom();
    extern (C) void* cr_stackTop();

    extern (C) bool thread_needLock();
    extern (C) void thread_suspendAll();
    extern (C) void thread_resumeAll();

    alias void delegate( void*, void* ) scanFn;
    extern (C) void cr_scanStaticData( scanFn scan );
    extern (C) void thread_scanAll( scanFn fn, void* curStackTop = null );
    +/
}

extern (C) void gc_init()
{
    // NOTE: The GC must initialize the thread library
    //       before its first collection, and always
    //       before returning from gc_init().
    thread_init();
}

extern (C) void gc_term()
{

}

extern (C) void gc_enable()
{

}

extern (C) void gc_disable()
{

}

extern (C) void gc_collect()
{

}

extern (C) uint gc_getAttr( void* p )
{
    if( p is null )
        return 0;
    return *((cast(blk_t*) p) - 1) & BlkAttr.ALL_BITS;
}

extern (C) uint gc_setAttr( void* p, uint a )
{
    if( p is null )
        return 0;
    return *((cast(blk_t*) p) - 1) |= a & BlkAttr.ALL_BITS;
}

extern (C) uint gc_clrAttr( void* p, uint a )
{
    if( p is null )
        return 0;
    return *((cast(blk_t*) p) - 1) &= ~a & BlkAttr.ALL_BITS;
}

extern (C) void* gc_malloc( size_t sz, uint ba = 0 )
{
    blk_t* p = cast(blk_t*) malloc( sz + blk_t.sizeof );

    if( p is null )
        return p;
    gc_setAttr( p + 1, ba );
    return p + 1;
}

extern (C) void* gc_calloc( size_t sz, uint ba = 0 )
{
    blk_t* p = cast(blk_t*) calloc( 1, sz + blk_t.sizeof );

    if( p is null )
        return p;
    gc_setAttr( p + 1, ba );
    return p + 1;
}

extern (C) void* gc_realloc( void* p, size_t sz, uint ba = 0 )
{
    blk_t* i = (cast(blk_t*) p) - (p ? 1 : 0);

    i = cast(blk_t*) realloc( i, sz + blk_t.sizeof );
    if( i is null )
        return i;
    gc_setAttr( i, ba );
    return i + 1;
}

extern (C) void gc_free( void* p )
{
    if( p !is null )
    {
        blk_t* i = (cast(blk_t*) p) - 1;

        if( *i & BlkAttr.FINALIZE )
            cr_finalize( p );
        free( i );
    }
}

extern (C) size_t gc_sizeOf( void* p )
{
    return 0;
}

extern (C) void gc_addRoot( void* p )
{

}

extern (C) void gc_addRange( void* p, size_t sz )
{

}

extern (C) void gc_removeRoot( void *p )
{

}

extern (C) void gc_removeRange( void *p )
{

}
