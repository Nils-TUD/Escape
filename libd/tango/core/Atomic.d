/**
 * The atomic module is intended to provide some basic support for lock-free
 * concurrent programming.  Some common operations are defined, each of which
 * may be performed using the specified memory barrier or a less granular
 * barrier if the hardware does not support the version requested.  This
 * model is based on a design by Alexander Terekhov as outlined in
 * <a href=http://groups.google.com/groups?threadm=3E4820EE.6F408B25%40web.de>
 * this thread</a>.
 *
 * Copyright: Copyright (C) 2005-2006 Sean Kelly.  All rights reserved.
 * License:   BSD style: $(LICENSE)
 * Authors:   Sean Kelly
 */
module tango.core.Atomic;


////////////////////////////////////////////////////////////////////////////////
// Synchronization Options
////////////////////////////////////////////////////////////////////////////////


/**
 * Memory synchronization flag.  If the supplied option is not available on the
 * current platform then a stronger method will be used instead.
 */
enum msync
{
    raw,    /// not sequenced
    hlb,    /// hoist-load barrier
    hsb,    /// hoist-store barrier
    slb,    /// sink-load barrier
    ssb,    /// sink-store barrier
    acq,    /// hoist-load + hoist-store barrier
    rel,    /// sink-load + sink-store barrier
    seq,    /// fully sequenced (acq + rel)
}


////////////////////////////////////////////////////////////////////////////////
// Internal Type Checking
////////////////////////////////////////////////////////////////////////////////


private
{
  version( DDoc ) {} else
  {
    import tango.util.meta.Traits;


    template isValidAtomicType( T )
    {
        const bool isValidAtomicType = T.sizeof == byte.sizeof  ||
                                       T.sizeof == short.sizeof ||
                                       T.sizeof == int.sizeof   ||
                                       T.sizeof == long.sizeof;
    }


    template isValidNumericType( T )
    {
        const bool isValidNumericType = isIntegerType!( T ) ||
                                        isPointerType!( T );
    }


    template isHoistOp( msync ms )
    {
        const bool isHoistOp = ms == msync.hlb ||
                               ms == msync.hsb ||
                               ms == msync.acq ||
                               ms == msync.seq;
    }


    template isSinkOp( msync ms )
    {
        const bool isSinkOp = ms == msync.slb ||
                              ms == msync.ssb ||
                              ms == msync.rel ||
                              ms == msync.seq;
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
// DDoc Documentation for Atomic Functions
////////////////////////////////////////////////////////////////////////////////


version( DDoc )
{
    ////////////////////////////////////////////////////////////////////////////
    // Atomic Load
    ////////////////////////////////////////////////////////////////////////////


    /**
     * Supported msync values:
     *  msync.raw
     *  msync.hlb
     *  msync.acq
     *  msync.seq
     */
    template atomicLoad( msync ms, T )
    {
        /**
         * Refreshes the contents of 'val' from main memory.  This operation is
         * both lock-free and atomic.
         *
         * Params:
         *  val = The value to load.  This value must be properly aligned.
         *
         * Returns:
         *  The loaded value.
         */
        T atomicLoad( inout T val )
        {
            return val;
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Atomic Store
    ////////////////////////////////////////////////////////////////////////////


    /**
     * Supported msync values:
     *  msync.raw
     *  msync.ssb
     *  msync.acq
     *  msync.rel
     *  msync.seq
     */
    template atomicStore( msync ms, T )
    {
        /**
         * Stores 'newval' to the memory referenced by 'val'.  This operation
         * is both lock-free and atomic.
         *
         * Params:
         *  val     = The destination variable.
         *  newval  = The value to store.
         */
        void atomicStore( inout T val, T newval )
        {

        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Atomic StoreIf
    ////////////////////////////////////////////////////////////////////////////


    /**
     * Supported msync values:
     *  msync.raw
     *  msync.ssb
     *  msync.acq
     *  msync.rel
     *  msync.seq
     */
    template atomicStoreIf( msync ms, T )
    {
        /**
         * Stores 'newval' to the memory referenced by 'val' if val is equal to
         * 'equalTo'.  This operation is both lock-free and atomic.
         *
         * Params:
         *  val     = The destination variable.
         *  newval  = The value to store.
         *  equalTo = The comparison value.
         *
         * Returns:
         *  true if the store occurred, false if not.
         */
        bool atomicStoreIf( inout T val, T newval, T equalTo )
        {
            return false;
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Atomic Increment
    ////////////////////////////////////////////////////////////////////////////


    /**
     * Supported msync values:
     *  msync.raw
     *  msync.ssb
     *  msync.acq
     *  msync.rel
     *  msync.seq
     */
    template atomicIncrement( msync ms, T )
    {
        /**
         * This operation is only legal for built-in value and pointer types,
         * and is equivalent to an atomic "val = val + 1" operation.  This
         * function exists to facilitate use of the optimized increment
         * instructions provided by some architecures.  If no such instruction
         * exists on the target platform then the behavior will perform the
         * operation using more traditional means.  This operation is both
         * lock-free and atomic.
         *
         * Params:
         *  val = The value to increment.
         *
         * Returns:
         *  The result of an atomicLoad of val immediately following the
         *  increment operation.  This value is not required to be equal to the
         *  newly stored value.  Thus, competing writes are allowed to occur
         *  between the increment and successive load operation.
         */
        T atomicIncrement( inout T val )
        {
            return val;
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Atomic Decrement
    ////////////////////////////////////////////////////////////////////////////


    /**
     * Supported msync values:
     *  msync.raw
     *  msync.ssb
     *  msync.acq
     *  msync.rel
     *  msync.seq
     */
    template atomicDecrement( msync ms, T )
    {
        /**
         * This operation is only legal for built-in value and pointer types,
         * and is equivalent to an atomic "val = val - 1" operation.  This
         * function exists to facilitate use of the optimized decrement
         * instructions provided by some architecures.  If no such instruction
         * exists on the target platform then the behavior will perform the
         * operation using more traditional means.  This operation is both
         * lock-free and atomic.
         *
         * Params:
         *  val = The value to decrement.
         *
         * Returns:
         *  The result of an atomicLoad of val immediately following the
         *  increment operation.  This value is not required to be equal to the
         *  newly stored value.  Thus, competing writes are allowed to occur
         *  between the increment and successive load operation.
         */
        T atomicDecrement( inout T val )
        {
            return val;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
// x86 Atomic Function Implementation
////////////////////////////////////////////////////////////////////////////////


else version( D_InlineAsm_X86 )
{
    private
    {
        ////////////////////////////////////////////////////////////////////////
        // x86 Value Requirements
        ////////////////////////////////////////////////////////////////////////


        // NOTE: Strictly speaking, the x86 supports atomic operations on
        //       unaligned values.  However, this is far slower than the
        //       common case, so such behavior should be prohibited.
        template atomicValueIsProperlyAligned( T )
        {
            bool atomicValueIsProperlyAligned( size_t addr )
            {
                return addr % T.sizeof == 0;
            }
        }


        ////////////////////////////////////////////////////////////////////////
        // Atomic Load
        ////////////////////////////////////////////////////////////////////////


        template doAtomicLoad( bool membar, T )
        {
            static if( T.sizeof == byte.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 1 Byte Load
                ////////////////////////////////////////////////////////////////


                T doAtomicLoad( inout T val )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock;
                            mov AL, [EAX];
                        }
                    }
                    else
                    {
                        volatile
                        {
                            return val;
                        }
                    }
                }
            }
            else static if( T.sizeof == short.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 2 Byte Load
                ////////////////////////////////////////////////////////////////


                T doAtomicLoad( inout T val )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock;
                            mov AX, [EAX];
                        }
                    }
                    else
                    {
                        volatile
                        {
                            return val;
                        }
                    }
                }
            }
            else static if( T.sizeof == int.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 4 Byte Load
                ////////////////////////////////////////////////////////////////


                T doAtomicLoad( inout T val )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock;
                            mov EAX, [EAX];
                        }
                    }
                    else
                    {
                        volatile
                        {
                            return val;
                        }
                    }
                }
            }
            else static if( T.sizeof == long.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 8 Byte Load
                ////////////////////////////////////////////////////////////////


                version( X86_64 )
                {
                    ////////////////////////////////////////////////////////////
                    // 8 Byte Load on 64-Bit Processor
                    ////////////////////////////////////////////////////////////


                    T doAtomicLoad( inout T val )
                    in
                    {
                        assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                    }
                    body
                    {
                        static if( membar )
                        {
                            volatile asm
                            {
                                mov RAX, val;
                                lock;
                                mov RAX, [RAX];
                            }
                        }
                        else
                        {
                            volatile
                            {
                                return val;
                            }
                        }
                    }
                }
                else
                {
                    ////////////////////////////////////////////////////////////
                    // 8 Byte Load on 32-Bit Processor
                    ////////////////////////////////////////////////////////////


                    pragma( msg, "This operation is only available on 64-bit platforms." );
                    static assert( false );
                }
            }
            else
            {
                ////////////////////////////////////////////////////////////////
                // Not a 1, 2, 4, or 8 Byte Type
                ////////////////////////////////////////////////////////////////


                pragma( msg, "Invalid template type specified." );
                static assert( false );
            }
        }


        ////////////////////////////////////////////////////////////////////////
        // Atomic Store
        ////////////////////////////////////////////////////////////////////////


        template doAtomicStore( bool membar, T )
        {
            static if( T.sizeof == byte.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 1 Byte Store
                ////////////////////////////////////////////////////////////////


                void doAtomicStore( inout T val, T newval )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            mov BL, newval;
                            lock;
                            xchg [EAX], BL;
                        }
                    }
                    else
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            mov BL, newval;
                            mov [EAX], BL;
                        }
                    }
                }
            }
            else static if( T.sizeof == short.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 2 Byte Store
                ////////////////////////////////////////////////////////////////


                void doAtomicStore( inout T val, T newval )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            mov BX, newval;
                            lock;
                            xchg [EAX], BX;
                        }
                    }
                    else
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            mov BX, newval;
                            mov [EAX], BX;
                        }
                    }
                }
            }
            else static if( T.sizeof == int.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 4 Byte Store
                ////////////////////////////////////////////////////////////////


                void doAtomicStore( inout T val, T newval )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            mov EBX, newval;
                            lock;
                            xchg [EAX], EBX;
                        }
                    }
                    else
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            mov EBX, newval;
                            mov [EAX], EBX;
                        }
                    }
                }
            }
            else static if( T.sizeof == long.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 8 Byte Store
                ////////////////////////////////////////////////////////////////


                version( X86_64 )
                {
                    ////////////////////////////////////////////////////////////
                    // 8 Byte Store on 64-Bit Processor
                    ////////////////////////////////////////////////////////////


                    void doAtomicStore( inout T val, T newval )
                    in
                    {
                        assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                    }
                    body
                    {
                        static if( membar )
                        {
                            volatile asm
                            {
                                mov RAX, val;
                                mov RBX, newval;
                                lock;
                                xchg [RAX], RBX;
                            }
                        }
                        else
                        {
                            volatile asm
                            {
                                mov RAX, val;
                                mov RBX, newval;
                                mov [RAX], RBX;
                            }
                        }
                    }
                }
                else
                {
                    ////////////////////////////////////////////////////////////
                    // 8 Byte Store on 32-Bit Processor
                    ////////////////////////////////////////////////////////////


                    pragma( msg, "This operation is only available on 64-bit platforms." );
                    static assert( false );
                }
            }
            else
            {
                ////////////////////////////////////////////////////////////////
                // Not a 1, 2, 4, or 8 Byte Type
                ////////////////////////////////////////////////////////////////


                pragma( msg, "Invalid template type specified." );
                static assert( false );
            }
        }


        ////////////////////////////////////////////////////////////////////////
        // Atomic Store If
        ////////////////////////////////////////////////////////////////////////


        template doAtomicStoreIf( bool membar, T )
        {
            static if( T.sizeof == byte.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 1 Byte StoreIf
                ////////////////////////////////////////////////////////////////


                bool doAtomicStoreIf( inout T val, T newval, T equalTo )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov BL, newval;
                            mov AL, equalTo;
                            mov ECX, val;
                            lock;
                            cmpxchg [ECX], BL;
                            setz AL;
                        }
                    }
                    else
                    {
                        volatile asm
                        {
                            mov BL, newval;
                            mov AL, equalTo;
                            mov ECX, val;
                            lock; // lock needed to make this op atomic
                            cmpxchg [ECX], BL;
                            setz AL;
                        }
                    }
                }
            }
            else static if( T.sizeof == short.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 2 Byte StoreIf
                ////////////////////////////////////////////////////////////////


                bool doAtomicStoreIf( inout T val, T newval, T equalTo )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov BX, newval;
                            mov AX, equalTo;
                            mov ECX, val;
                            lock;
                            cmpxchg [ECX], BX;
                            setz AL;
                        }
                    }
                    else
                    {
                        volatile asm
                        {
                            mov BX, newval;
                            mov AX, equalTo;
                            mov ECX, val;
                            lock; // lock needed to make this op atomic
                            cmpxchg [ECX], BX;
                            setz AL;
                        }
                    }
                }
            }
            else static if( T.sizeof == int.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 4 Byte StoreIf
                ////////////////////////////////////////////////////////////////


                bool doAtomicStoreIf( inout T val, T newval, T equalTo )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov EBX, newval;
                            mov EAX, equalTo;
                            mov ECX, val;
                            lock;
                            cmpxchg [ECX], EBX;
                            setz AL;
                        }
                    }
                    else
                    {
                        volatile asm
                        {
                            mov EBX, newval;
                            mov EAX, equalTo;
                            mov ECX, val;
                            lock; // lock needed to make this op atomic
                            cmpxchg [ECX], EBX;
                            setz AL;
                        }
                    }
                }
            }
            else static if( T.sizeof == long.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 8 Byte StoreIf
                ////////////////////////////////////////////////////////////////


                version( X86_64 )
                {
                    ////////////////////////////////////////////////////////////
                    // 8 Byte StoreIf on 64-Bit Processor
                    ////////////////////////////////////////////////////////////


                    bool doAtomicStoreIf( inout T val, T newval, T equalTo )
                    in
                    {
                        assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                    }
                    body
                    {
                        static if( membar )
                        {
                            volatile asm
                            {
                                mov RBX, newval;
                                mov RAX, equalTo;
                                mov RCX, val;
                                lock;
                                cmpxchg [RCX], RBX;
                                setz AL;
                            }
                        }
                        else
                        {
                            volatile asm
                            {
                                mov RBX, newval;
                                mov RAX, equalTo;
                                mov RCX, val;
                                lock; // lock needed to make this op atomic
                                cmpxchg [RCX], RBX;
                                setz AL;
                            }
                        }
                    }
                }
                else
                {
                    ////////////////////////////////////////////////////////////
                    // 8 Byte StoreIf on 32-Bit Processor
                    ////////////////////////////////////////////////////////////


                    bool doAtomicStoreIf( inout T val, T newval, T equalTo )
                    in
                    {
                        assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                    }
                    body
                    {
                        static if( membar )
                        {
                            volatile asm
                            {
                                lea EDI, newval;
                                mov EBX, [EDI];
                                mov ECX, 4[EDI];
                                lea EDI, equalTo;
                                mov EAX, [EDI];
                                mov EDX, 4[EDI];
                                mov EDI, val;
                                lock;
                                cmpxch8b [EDI];
                                setz AL;
                            }
                        }
                        else
                        {
                            volatile asm
                            {
                                lea EDI, newval;
                                mov EBX, [EDI];
                                mov ECX, 4[EDI];
                                lea EDI, equalTo;
                                mov EAX, [EDI];
                                mov EDX, 4[EDI];
                                mov EDI, val;
                                lock; // lock needed to make this op atomic
                                cmpxch8b [EDI];
                                setz AL;
                            }
                        }
                    }
                }
            }
            else
            {
                ////////////////////////////////////////////////////////////////
                // Not a 1, 2, 4, or 8 Byte Type
                ////////////////////////////////////////////////////////////////


                pragma( msg, "Invalid template type specified." );
                static assert( false );
            }
        }


        ////////////////////////////////////////////////////////////////////////
        // Atomic Increment
        ////////////////////////////////////////////////////////////////////////


        template doAtomicIncrement( bool membar, T )
        {
            //
            // NOTE: This operation is only valid for integer or pointer types
            //
            static assert( isValidNumericType!(T) );

            static if( T.sizeof == byte.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 1 Byte Increment
                ////////////////////////////////////////////////////////////////


                T doAtomicIncrement( inout T val )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock;
                            inc [EAX];
                            mov AL, [EAX];
                        }
                    }
                    else
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock; // lock needed to make this op atomic
                            inc [EAX];
                            mov AL, [EAX];
                        }
                    }
                }
            }
            else static if( T.sizeof == short.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 2 Byte Increment
                ////////////////////////////////////////////////////////////////


                T doAtomicIncrement( inout T val )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock;
                            inc [EAX];
                            mov AX, [EAX];
                        }
                    }
                    else
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock; // lock needed to make this op atomic
                            inc [EAX];
                            mov AX, [EAX];
                        }
                    }
                }
            }
            else static if( T.sizeof == int.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 4 Byte Increment
                ////////////////////////////////////////////////////////////////


                T doAtomicIncrement( inout T val )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock;
                            inc [EAX];
                            mov EAX, [EAX];
                        }
                    }
                    else
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock; // lock needed to make this op atomic
                            inc [EAX];
                            mov EAX, [EAX];
                        }
                    }
                }
            }
            else static if( T.sizeof == long.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 8 Byte Increment
                ////////////////////////////////////////////////////////////////


                version( X86_64 )
                {
                    ////////////////////////////////////////////////////////////
                    // 8 Byte Increment on 64-Bit Processor
                    ////////////////////////////////////////////////////////////


                    T doAtomicIncrement( inout T val )
                    in
                    {
                        assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                    }
                    body
                    {
                        static if( membar )
                        {
                            volatile asm
                            {
                                mov RAX, val;
                                lock;
                                inc [RAX];
                                mov RAX, [RAX];
                            }
                        }
                        else
                        {
                            volatile asm
                            {
                                mov RAX, val;
                                lock; // lock needed to make this op atomic
                                inc [RAX];
                                mov RAX, [RAX];
                            }
                        }
                    }
                }
                else
                {
                    ////////////////////////////////////////////////////////////
                    // 8 Byte Increment on 32-Bit Processor
                    ////////////////////////////////////////////////////////////


                    pragma( msg, "This operation is only available on 64-bit platforms." );
                    static assert( false );
                }
            }
            else
            {
                ////////////////////////////////////////////////////////////////
                // Not a 1, 2, 4, or 8 Byte Type
                ////////////////////////////////////////////////////////////////


                pragma( msg, "Invalid template type specified." );
                static assert( false );
            }
        }


        ////////////////////////////////////////////////////////////////////////
        // Atomic Decrement
        ////////////////////////////////////////////////////////////////////////


        template doAtomicDecrement( bool membar, T )
        {
            //
            // NOTE: This operation is only valid for integer or pointer types
            //
            static assert( isValidNumericType!(T) );

            static if( T.sizeof == byte.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 1 Byte Decrement
                ////////////////////////////////////////////////////////////////


                T doAtomicDecrement( inout T val )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock;
                            dec [EAX];
                            mov AL, [EAX];
                        }
                    }
                    else
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock; // lock needed to make this op atomic
                            dec [EAX];
                            mov AL, [EAX];
                        }
                    }
                }
            }
            else static if( T.sizeof == short.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 2 Byte Decrement
                ////////////////////////////////////////////////////////////////


                T doAtomicDecrement( inout T val )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock;
                            dec [EAX];
                            mov AX, [EAX];
                        }
                    }
                    else
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock; // lock needed to make this op atomic
                            dec [EAX];
                            mov AX, [EAX];
                        }
                    }
                }
            }
            else static if( T.sizeof == int.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 4 Byte Decrement
                ////////////////////////////////////////////////////////////////


                T doAtomicDecrement( inout T val )
                in
                {
                    assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                }
                body
                {
                    static if( membar )
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock;
                            dec [EAX];
                            mov EAX, [EAX];
                        }
                    }
                    else
                    {
                        volatile asm
                        {
                            mov EAX, val;
                            lock; // lock needed to make this op atomic
                            dec [EAX];
                            mov EAX, [EAX];
                        }
                    }
                }
            }
            else static if( T.sizeof == long.sizeof )
            {
                ////////////////////////////////////////////////////////////////
                // 8 Byte Decrement
                ////////////////////////////////////////////////////////////////


                version( X86_64 )
                {
                    ////////////////////////////////////////////////////////////
                    // 8 Byte Decrement on 64-Bit Processor
                    ////////////////////////////////////////////////////////////


                    T doAtomicDecrement( inout T val )
                    in
                    {
                        assert( atomicValueIsProperlyAligned!(T)( cast(size_t) &val ) );
                    }
                    body
                    {
                        static if( membar )
                        {
                            volatile asm
                            {
                                mov RAX, val;
                                lock;
                                dec [RAX];
                                mov RAX, [RAX];
                            }
                        }
                        else
                        {
                            volatile asm
                            {
                                mov RAX, val;
                                lock; // lock needed to make this op atomic
                                dec [RAX];
                                mov RAX, [RAX];
                            }
                        }
                    }
                }
                else
                {
                    ////////////////////////////////////////////////////////////////
                    // 8 Byte Decrement on 32-Bit Processor
                    ////////////////////////////////////////////////////////////////


                    pragma( msg, "This operation is only available on 64-bit platforms." );
                    static assert( false );
                }
            }
            else
            {
                ////////////////////////////////////////////////////////////////
                // Not a 1, 2, 4, or 8 Byte Type
                ////////////////////////////////////////////////////////////////


                pragma( msg, "Invalid template type specified." );
                static assert( false );
            }
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // x86 Public Atomic Functions
    ////////////////////////////////////////////////////////////////////////////

    //
    // NOTE: x86 loads implicitly have acquire semantics so a membar is only necessary on release
    //

    template atomicLoad( msync ms : msync.raw, T ) { alias doAtomicLoad!(isSinkOp!(ms),T) atomicLoad; }
    template atomicLoad( msync ms : msync.hlb, T ) { alias doAtomicLoad!(isSinkOp!(ms),T) atomicLoad; }
    template atomicLoad( msync ms : msync.acq, T ) { alias doAtomicLoad!(isSinkOp!(ms),T) atomicLoad; }
    template atomicLoad( msync ms : msync.seq, T ) { alias doAtomicLoad!(true,T)          atomicLoad; }

    //
    // NOTE: x86 stores implicitly have release semantics so a membar is only necessary on acquires
    //

    template atomicStore( msync ms : msync.raw, T ) { alias doAtomicStore!(isHoistOp!(ms),T) atomicStore; }
    template atomicStore( msync ms : msync.ssb, T ) { alias doAtomicStore!(isHoistOp!(ms),T) atomicStore; }
    template atomicStore( msync ms : msync.acq, T ) { alias doAtomicStore!(isHoistOp!(ms),T) atomicStore; }
    template atomicStore( msync ms : msync.rel, T ) { alias doAtomicStore!(isHoistOp!(ms),T) atomicStore; }
    template atomicStore( msync ms : msync.seq, T ) { alias doAtomicStore!(true,T)           atomicStore; }

    template atomicStoreIf( msync ms : msync.raw, T ) { alias doAtomicStoreIf!(ms!=msync.raw,T) atomicStoreIf; }
    template atomicStoreIf( msync ms : msync.ssb, T ) { alias doAtomicStoreIf!(ms!=msync.raw,T) atomicStoreIf; }
    template atomicStoreIf( msync ms : msync.acq, T ) { alias doAtomicStoreIf!(ms!=msync.raw,T) atomicStoreIf; }
    template atomicStoreIf( msync ms : msync.rel, T ) { alias doAtomicStoreIf!(ms!=msync.raw,T) atomicStoreIf; }
    template atomicStoreIf( msync ms : msync.seq, T ) { alias doAtomicStoreIf!(true,T)          atomicStoreIf; }

    template atomicIncrement( msync ms : msync.raw, T ) { alias doAtomicIncrement!(ms!=msync.raw,T) atomicIncrement; }
    template atomicIncrement( msync ms : msync.ssb, T ) { alias doAtomicIncrement!(ms!=msync.raw,T) atomicIncrement; }
    template atomicIncrement( msync ms : msync.acq, T ) { alias doAtomicIncrement!(ms!=msync.raw,T) atomicIncrement; }
    template atomicIncrement( msync ms : msync.rel, T ) { alias doAtomicIncrement!(ms!=msync.raw,T) atomicIncrement; }
    template atomicIncrement( msync ms : msync.seq, T ) { alias doAtomicIncrement!(true,T)          atomicIncrement; }

    template atomicDecrement( msync ms : msync.raw, T ) { alias doAtomicDecrement!(ms!=msync.raw,T) atomicDecrement; }
    template atomicDecrement( msync ms : msync.ssb, T ) { alias doAtomicDecrement!(ms!=msync.raw,T) atomicDecrement; }
    template atomicDecrement( msync ms : msync.acq, T ) { alias doAtomicDecrement!(ms!=msync.raw,T) atomicDecrement; }
    template atomicDecrement( msync ms : msync.rel, T ) { alias doAtomicDecrement!(ms!=msync.raw,T) atomicDecrement; }
    template atomicDecrement( msync ms : msync.seq, T ) { alias doAtomicDecrement!(true,T)          atomicDecrement; }
}


////////////////////////////////////////////////////////////////////////////////
// Atomic
////////////////////////////////////////////////////////////////////////////////


/**
 * This class represents a value which will be subject to competing access.
 * All accesses to this value will be synchronized with main memory, and
 * various memory barriers may be employed for instruction ordering.  Any
 * primitive type of size equal to or smaller than the memory bus size is
 * allowed, so 32-bit machines may use values with size <= int.sizeof and
 * 64-bit machines may use values with size <= long.sizeof.  The one exception
 * to this rule is that architectures that support DCAS will allow double-wide
 * storeIf operations.  The 32-bit x86 architecture, for example, supports
 * 64-bit storeIf operations.
 */
struct Atomic( T )
{
    ////////////////////////////////////////////////////////////////////////////
    // Atomic Load
    ////////////////////////////////////////////////////////////////////////////


    template load( msync ms )
    {
        static assert( ms == msync.raw || ms == msync.hlb ||
                       ms == msync.acq || ms == msync.seq,
                       "ms must be one of: msync.raw, msync.hlb, msync.acq, msync.seq" );

        /**
         * Refreshes the contents of this value from main memory.  This
         * operation is both lock-free and atomic.
         *
         * Returns:
         *  The loaded value.
         */
        T load()
        {
            return atomicLoad!(ms,T)( m_val );
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Atomic Store
    ////////////////////////////////////////////////////////////////////////////


    template store( msync ms )
    {
        static assert( ms == msync.raw || ms == msync.ssb ||
                       ms == msync.acq || ms == msync.rel ||
                       ms == msync.seq,
                       "ms must be one of: msync.raw, msync.ssb, msync.acq, msync.rel, msync.seq" );

        /**
         * Stores 'newval' to the memory referenced by this value.  This
         * operation is both lock-free and atomic.
         *
         * Params:
         *  newval  = The value to store.
         */
        void store( T newval )
        {
            atomicStore!(ms,T)( m_val, newval );
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Atomic StoreIf
    ////////////////////////////////////////////////////////////////////////////


    template storeIf( msync ms )
    {
        static assert( ms == msync.raw || ms == msync.ssb ||
                       ms == msync.acq || ms == msync.rel ||
                       ms == msync.seq,
                       "ms must be one of: msync.raw, msync.ssb, msync.acq, msync.rel, msync.seq" );

        /**
         * Stores 'newval' to the memory referenced by this value if val is
         * equal to 'equalTo'.  This operation is both lock-free and atomic.
         *
         * Params:
         *  newval  = The value to store.
         *  equalTo = The comparison value.
         *
         * Returns:
         *  true if the store occurred, false if not.
         */
        bool storeIf( T newval, T equalTo )
        {
            return atomicStoreIf!(ms,T)( m_val, newval, equalTo );
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Numeric Functions
    ////////////////////////////////////////////////////////////////////////////


    /**
     * The following additional functions are available for integer types.
     */
    static if( isValidNumericType!(T) )
    {
        ////////////////////////////////////////////////////////////////////////
        // Atomic Increment
        ////////////////////////////////////////////////////////////////////////


        template increment( msync ms )
        {
            static assert( ms == msync.raw || ms == msync.ssb ||
                           ms == msync.acq || ms == msync.rel ||
                           ms == msync.seq,
                           "ms must be one of: msync.raw, msync.ssb, msync.acq, msync.rel, msync.seq" );

            /**
             * This operation is only legal for built-in value and pointer
             * types, and is equivalent to an atomic "val = val + 1" operation.
             * This function exists to facilitate use of the optimized
             * increment instructions provided by some architecures.  If no
             * such instruction exists on the target platform then the
             * behavior will perform the operation using more traditional
             * means.  This operation is both lock-free and atomic.
             *
             * Returns:
             *  The result of an atomicLoad of val immediately following the
             *  increment operation.  This value is not required to be equal to
             *  the newly stored value.  Thus, competing writes are allowed to
             *  occur between the increment and successive load operation.
             */
            T increment()
            {
                return atomicIncrement!(ms,T)( m_val );
            }
        }


        ////////////////////////////////////////////////////////////////////////
        // Atomic Decrement
        ////////////////////////////////////////////////////////////////////////


        template decrement( msync ms )
        {
            static assert( ms == msync.raw || ms == msync.ssb ||
                           ms == msync.acq || ms == msync.rel ||
                           ms == msync.seq,
                           "ms must be one of: msync.raw, msync.ssb, msync.acq, msync.rel, msync.seq" );

            /**
             * This operation is only legal for built-in value and pointer
             * types, and is equivalent to an atomic "val = val - 1" operation.
             * This function exists to facilitate use of the optimized
             * decrement instructions provided by some architecures.  If no
             * such instruction exists on the target platform then the behavior
             * will perform the operation using more traditional means.  This
             * operation is both lock-free and atomic.
             *
             * Returns:
             *  The result of an atomicLoad of val immediately following the
             *  increment operation.  This value is not required to be equal to
             *  the newly stored value.  Thus, competing writes are allowed to
             *  occur between the increment and successive load operation.
             */
            T decrement()
            {
                return atomicDecrement!(ms,T)( m_val );
            }
        }
    }

private:
    T   m_val;
}


////////////////////////////////////////////////////////////////////////////////
// Support Code for Unit Tests
////////////////////////////////////////////////////////////////////////////////


private
{
  version( DDoc ) {} else
  {
    template testLoad( msync ms, T )
    {
        void testLoad( T val = T.init + 1 )
        {
            T          base;
            Atomic!(T) atom;

            assert( atom.load!(ms)() == base );
            base        = val;
            atom.m_val  = val;
            assert( atom.load!(ms)() == base );
        }
    }


    template testStore( msync ms, T )
    {
        void testStore( T val = T.init + 1 )
        {
            T          base;
            Atomic!(T) atom;

            assert( atom.m_val == base );
            base = val;
            atom.store!(ms)( base );
            assert( atom.m_val == base );
        }
    }


    template testStoreIf( msync ms, T )
    {
        void testStoreIf( T val = T.init + 1 )
        {
            T          base;
            Atomic!(T) atom;

            assert( atom.m_val == base );
            base = val;
            atom.storeIf!(ms)( base, val );
            assert( atom.m_val != base );
            atom.storeIf!(ms)( base, T.init );
            assert( atom.m_val == base );
        }
    }


    template testIncrement( msync ms, T )
    {
        void testIncrement( T val = T.init + 1 )
        {
            T          base = val;
            T          incr = val;
            Atomic!(T) atom;

            atom.m_val = val;
            assert( atom.m_val == base && incr == base );
            base = cast(T)( base + 1 );
            incr = atom.increment!(ms)();
            assert( atom.m_val == base && incr == base );
        }
    }


    template testDecrement( msync ms, T )
    {
        void testDecrement( T val = T.init + 1 )
        {
            T          base = val;
            T          decr = val;
            Atomic!(T) atom;

            atom.m_val = val;
            assert( atom.m_val == base && decr == base );
            base = cast(T)( base - 1 );
            decr = atom.decrement!(ms)();
            assert( atom.m_val == base && decr == base );
        }
    }


    template testType( T )
    {
        void testType( T val = T.init  +1 )
        {
            testLoad!(msync.raw, T)( val );
            testLoad!(msync.acq, T)( val );

            testStore!(msync.raw, T)( val );
            testStore!(msync.acq, T)( val );
            testStore!(msync.rel, T)( val );

            testStoreIf!(msync.raw, T)( val );
            testStoreIf!(msync.acq, T)( val );

            static if( isValidNumericType!(T) )
            {
                testIncrement!(msync.raw, T)( val );
                testIncrement!(msync.acq, T)( val );

                testDecrement!(msync.raw, T)( val );
                testDecrement!(msync.acq, T)( val );
            }
        }
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
// Unit Tests
////////////////////////////////////////////////////////////////////////////////


debug( UnitTest )
{
  unittest
  {
    testType!(bool)();

    testType!(byte)();
    testType!(ubyte)();

    testType!(short)();
    testType!(ushort)();

    testType!(int)();
    testType!(uint)();

    int x;
    testType!(void*)( &x );

    //
    // long
    //

    version( X86_64 )
    {
        testLoad!(msync.raw, long)();
        testLoad!(msync.acq, long)();
        testLoad!(msync.seq, long)();

        testStore!(msync.raw, long)();
        testStore!(msync.acq, long)();
        testStore!(msync.rel, long)();
        testStore!(msync.seq, long)();

        testIncrement!(msync.raw, long)();
        testDecrement!(msync.acq, long)();
        testDecrement!(msync.seq, long)();
    }
    version( X86 )
    {
        /+
         + TODO: long is not properly aligned on x86
         +
        testStoreIf!(msync.raw, long)();
        testStoreIf!(msync.acq, long)();
        testStoreIf!(msync.seq, long)();
         +/
    }

    //
    // ulong
    //

    version( X86_64 )
    {
        testLoad!(msync.raw, ulong)();
        testLoad!(msync.acq, ulong)();
        testLoad!(msync.seq, ulong)();

        testStore!(msync.raw, ulong)();
        testStore!(msync.acq, ulong)();
        testStore!(msync.rel, ulong)();
        testStore!(msync.seq, ulong)();

        testIncrement!(msync.raw, ulong)();
        testDecrement!(msync.acq, ulong)();
        testDecrement!(msync.seq, ulong)();
    }
    version( X86 )
    {
        /+
         + TODO: ulong is not properly aligned on x86
         +
        testStoreIf!(msync.raw, ulong)();
        testStoreIf!(msync.acq, ulong)();
        testStoreIf!(msync.seq, ulong)();
         +/
    }
  }
}
