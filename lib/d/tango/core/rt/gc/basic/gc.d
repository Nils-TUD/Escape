/**
 * This module contains the garbage collector front-end.
 *
 * Copyright: Copyright (C) 2005-2006 Digital Mars, www.digitalmars.com.
 *            All rights reserved.
 * License:
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, in both source and binary form, subject to the following
 *  restrictions:
 *
 *  o  The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 *  o  Altered source versions must be plainly marked as such, and must not
 *     be misrepresented as being the original software.
 *  o  This notice may not be removed or altered from any source
 *     distribution.
 * Authors:   Walter Bright, Sean Kelly
 */
module rt.gc.basic.gc;

private import rt.gc.basic.gcx;
private import rt.gc.basic.gcstats;
private import tango.stdc.stdlib;

version=GCCLASS;

version (GCCLASS)
    alias GC gc_t;
else
    alias GC* gc_t;

gc_t _gc;

private int _termCleanupLevel=1;

/// sets the cleanup level done by gc
/// (0: none, 1: fullCollect, 2: fullCollectNoStack (might crash daemonThreads))
/// result !=0 if the value was invalid
extern (C) int gc_setTermCleanupLevel(int cLevel){
    if (cLevel<0 || cLevel>2) return cLevel;
    _termCleanupLevel=cLevel;
    return 0;
}

/// returns the cleanup level done by gc
extern (C) int gc_getTermCleanupLevel(){
    return _termCleanupLevel;
}

version (DigitalMars) version(OSX) {
    extern(C) void _d_osx_image_init();
}

extern (C) void thread_init();

extern (C) void gc_init()
{
    version (GCCLASS)
    {   void* p;
        ClassInfo ci = GC.classinfo;

        p = tango.stdc.stdlib.malloc(ci.init.length);
        (cast(byte*)p)[0 .. ci.init.length] = ci.init[];
        _gc = cast(GC)p;
    }
    else
    {
        _gc = cast(GC*) tango.stdc.stdlib.calloc(1, GC.sizeof);
    }
    _gc.initialize();
    version (DigitalMars) version(OSX) {
        _d_osx_image_init();
    }
    // NOTE: The GC must initialize the thread library
    //       before its first collection.
    thread_init();
}

extern (C) void gc_term()
{
    if (_termCleanupLevel<1) {
        // no cleanup
    } else if (_termCleanupLevel==2){
        // a more complete cleanup
        // NOTE: There may be daemons threads still running when this routine is
        //       called.  If so, cleaning memory out from under then is a good
        //       way to make them crash horribly.
        //       Often this probably doesn't matter much since the app is
        //       supposed to be shutting down anyway, but for example tests might
        //       crash (and be considerd failed even if the test was ok).
        //       thus this is not the default and should be enabled by 
        //       I'm disabling cleanup for now until I can think about it some
        //       more.
        //
        _gc.fullCollectNoStack(); // not really a 'collect all' -- still scans
                                  // static data area, roots, and ranges.
        _gc.Dtor();
    } else {
        // default (safe) clenup
        _gc.fullCollect(); 
    }
}

extern (C) void gc_enable()
{
    _gc.enable();
}

extern (C) void gc_disable()
{
    _gc.disable();
}

extern (C) void gc_collect()
{
    _gc.fullCollect();
}


extern (C) void gc_minimize()
{
    _gc.minimize();
}

extern (C) uint gc_getAttr( void* p )
{
    return _gc.getAttr( p );
}

extern (C) uint gc_setAttr( void* p, uint a )
{
    return _gc.setAttr( p, a );
}

extern (C) uint gc_clrAttr( void* p, uint a )
{
    return _gc.clrAttr( p, a );
}

extern (C) void* gc_malloc( size_t sz, uint ba = 0 )
{
    return _gc.malloc( sz, ba );
}

extern (C) void* gc_calloc( size_t sz, uint ba = 0 )
{
    return _gc.calloc( sz, ba );
}

extern (C) void* gc_realloc( void* p, size_t sz, uint ba = 0 )
{
    return _gc.realloc( p, sz, ba );
}

extern (C) size_t gc_extend( void* p, size_t mx, size_t sz )
{
    return _gc.extend( p, mx, sz );
}

extern (C) size_t gc_reserve( size_t sz )
{
    return _gc.reserve( sz );
}

extern (C) void gc_free( void* p )
{
    _gc.free( p );
}

extern (C) void* gc_addrOf( void* p )
{
    return _gc.addrOf( p );
}

extern (C) size_t gc_sizeOf( void* p )
{
    return _gc.sizeOf( p );
}

extern (C) BlkInfo gc_query( void* p )
{
    return _gc.query( p );
}

// NOTE: This routine is experimental.  The stats or function name may change
//       before it is made officially available.
extern (C) GCStats gc_stats()
{
    GCStats stats = void;
    _gc.getStats( stats );
    return stats;
}

extern (C) void gc_addRoot( void* p )
{
    _gc.addRoot( p );
}

extern (C) void gc_addRange( void* p, size_t sz )
{
    _gc.addRange( p, sz );
}

extern (C) void gc_removeRoot( void *p )
{
    _gc.removeRoot( p );
}

extern (C) void gc_removeRange( void *p )
{
    _gc.removeRange( p );
}

extern (C) void* gc_weakpointerCreate( Object r )
{
    return _gc.weakpointerCreate(r);
}

extern (C) void gc_weakpointerDestroy( void* wp )
{
    _gc.weakpointerDestroy(wp);
}

extern (C) Object gc_weakpointerGet( void* wp )
{
    return _gc.weakpointerGet(wp);
}
