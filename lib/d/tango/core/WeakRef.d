/******************************************************************************

        copyright:      Copyright (c) 2009 Tango. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Jan 2010: Initial release

        author:         wm4, kris

******************************************************************************/

module tango.core.WeakRef;

private import tango.core.Memory;

/******************************************************************************

        A generic WeakReference

******************************************************************************/

alias WeakReference!(Object) WeakRef;

/******************************************************************************

        Implements a Weak reference. The get() method returns null once 
        the object pointed to has been collected

******************************************************************************/

class WeakReference (T : Object) 
{
        public alias get opCall;        /// alternative get() call
        private void* weakpointer;      // what the GC gives us back

        /**********************************************************************
        
                initializes a weak reference
        
        **********************************************************************/

        this (T obj) 
        {
                weakpointer = GC.weakPointerCreate (obj);
        }

        /**********************************************************************
        
                clean up when we are no longer referenced

        **********************************************************************/

        ~this () 
        {
                clear;
        }

        /**********************************************************************
        
                host a different object reference 

        **********************************************************************/

        final void set (T obj) 
        {
                clear;
                weakpointer = GC.weakPointerCreate (obj);
        }

        /**********************************************************************
        
                clear the weak reference - get() will always return null
        
        **********************************************************************/

        final void clear () 
        {
                GC.weakPointerDestroy (weakpointer);
                weakpointer = null;
        }

        /**********************************************************************
        
                returns the weak reference - returns null if the object 
                was deallocated in the meantime

        **********************************************************************/

        final T get () 
        {
                return cast(T) GC.weakPointerGet (weakpointer);
        }
}


/******************************************************************************

        Note this requires -g (with dmd) in order for the GC.collect call
        to operate as desired 

******************************************************************************/

debug (WeakRef)
{
        import tango.core.Memory;

        WeakRef make ()
        {       
                return new WeakRef (new Object);
        }

        void main()
        {
                auto o = new Object;
                auto r = new WeakRef (o);
                assert (r() is o);
                delete o;
                assert (r() is null);

                auto r1 = make;
                assert (r1.get);
                GC.collect;
                assert (r1() is null);
        }
}
