// D import file generated from 'core/Memory.d'
module tango.core.Memory;
private
{
    extern (C) 
{
    void gc_init();
}
    extern (C) 
{
    void gc_term();
}
    extern (C) 
{
    void gc_enable();
}
    extern (C) 
{
    void gc_disable();
}
    extern (C) 
{
    void gc_collect();
}
    extern (C) 
{
    uint gc_getAttr(void* p);
}
    extern (C) 
{
    uint gc_setAttr(void* p, uint a);
}
    extern (C) 
{
    uint gc_clrAttr(void* p, uint a);
}
    extern (C) 
{
    void* gc_malloc(size_t sz, uint ba = 0);
}
    extern (C) 
{
    void* gc_calloc(size_t sz, uint ba = 0);
}
    extern (C) 
{
    void* gc_realloc(void* p, size_t sz, uint ba = 0);
}
    extern (C) 
{
    void gc_free(void* p);
}
    extern (C) 
{
    size_t gc_sizeOf(void* p);
}
    extern (C) 
{
    void gc_addRoot(void* p);
}
    extern (C) 
{
    void gc_addRange(void* p, size_t sz);
}
    extern (C) 
{
    void gc_removeRoot(void* p);
}
    extern (C) 
{
    void gc_removeRange(void* p);
}
}
struct GC
{
    void enable()
{
gc_enable();
}
    void disable()
{
gc_disable();
}
    void collect()
{
gc_collect();
}
    enum BlkAttr : uint
{
FINALIZE = 1,
NO_SCAN = 2,
NO_MOVE = 4,
}
    uint getAttr(void* p)
{
return gc_getAttr(p);
}
    uint setAttr(void* p, uint a)
{
return gc_setAttr(p,a);
}
    uint clrAttr(void* p, uint a)
{
return gc_clrAttr(p,a);
}
    void* malloc(size_t sz, uint ba = 0)
{
return gc_malloc(sz,ba);
}
    void* calloc(size_t sz, uint ba = 0)
{
return gc_calloc(sz,ba);
}
    void* realloc(void* p, size_t sz, uint ba = 0)
{
return gc_realloc(p,sz,ba);
}
    void free(void* p)
{
gc_free(p);
}
    size_t sizeOf(void* p)
{
return gc_sizeOf(p);
}
    void addRoot(void* p)
{
gc_addRoot(p);
}
    void addRange(void* p, size_t sz)
{
gc_addRange(p,sz);
}
    void removeRoot(void* p)
{
gc_removeRoot(p);
}
    void removeRange(void* p)
{
gc_removeRange(p);
}
}
GC gc;
