/*******************************************************************************

        copyright:      Copyright (c) 2008 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Apr 2008: Initial release

        authors:        Kris

        Since:          0.99.7

        Based upon Doug Lea's Java collection package

*******************************************************************************/

module tango.util.container.Clink;

/*******************************************************************************

        Clinks are links that are always arranged in circular lists.

*******************************************************************************/

struct Clink (V)
{
        alias Clink!(V)    Type;
        alias Type         *Ref;

        Ref     prev,           // pointer to prev
                next;           // pointer to next
        V       value;          // element value

        /***********************************************************************

                 Set to point to ourselves
                        
        ***********************************************************************/

        Ref set (V v)
        {
                return set (v, this, this);
        }

        /***********************************************************************

                 Set to point to n as next cell and p as the prior cell

                 param: n, the new next cell
                 param: p, the new prior cell
                        
        ***********************************************************************/

        Ref set (V v, Ref p, Ref n)
        {
                value = v;
                prev = p;
                next = n;
                return this;
        }

        /**
         * Return true if current cell is the only one on the list
        **/

        bool singleton()
        {
                return next is this;
        }

        void linkNext (Ref p)
        {
                if (p)
                   {
                   next.prev = p;
                   p.next = next;
                   p.prev = this;
                   next = p;
                   }
        }

        /**
         * Make a cell holding v and link it immediately after current cell
        **/

        void addNext (V v, Ref delegate() alloc)
        {
                auto p = alloc().set (v, this, next);
                next.prev = p;
                next = p;
        }

        /**
         * make a node holding v, link it before the current cell, and return it
        **/

        Ref addPrev (V v, Ref delegate() alloc)
        {
                auto p = prev;
                auto c = alloc().set (v, p, this);
                p.next = c;
                prev = c;
                return c;
        }

        /**
         * link p before current cell
        **/

        void linkPrev (Ref p)
        {
                if (p)
                   {
                   prev.next = p;
                   p.prev = prev;
                   p.next = this;
                   prev = p;
                   }
        }

        /**
         * return the number of cells in the list
        **/

        int size()
        {
                int c = 0;
                auto p = this;
                do {
                   ++c;
                   p = p.next;
                   } while (p !is this);
                return c;
        }

        /**
         * return the first cell holding element found in a circular traversal starting
         * at current cell, or null if no such
        **/

        Ref find (V element)
        {
                auto p = this;
                do {
                   if (element == p.value)
                       return p;
                   p = p.next;
                   } while (p !is this);
                return null;
        }

        /**
         * return the number of cells holding element found in a circular
         * traversal
        **/

        int count (V element)
        {
                int c = 0;
                auto p = this;
                do {
                   if (element == p.value)
                       ++c;
                   p = p.next;
                   } while (p !is this);
                return c;
        }

        /**
         * return the nth cell traversed from here. It may wrap around.
        **/

        Ref nth (int n)
        {
                auto p = this;
                for (int i = 0; i < n; ++i)
                     p = p.next;
                return p;
        }


        /**
         * Unlink the next cell.
         * This has no effect on the list if isSingleton()
        **/

        void unlinkNext ()
        {
                auto nn = next.next;
                nn.prev = this;
                next = nn;
        }

        /**
         * Unlink the previous cell.
         * This has no effect on the list if isSingleton()
        **/

        void unlinkPrev ()
        {
                auto pp = prev.prev;
                pp.next = this;
                prev = pp;
        }


        /**
         * Unlink self from list it is in.
         * Causes it to be a singleton
        **/

        void unlink ()
        {
                auto p = prev;
                auto n = next;
                p.next = n;
                n.prev = p;
                prev = this;
                next = this;
        }

        /**
         * Make a copy of the list and return new head. 
        **/

        Ref copyList (Ref delegate() alloc)
        {
                auto hd = this;

                auto newlist = alloc().set (hd.value, null, null);
                auto current = newlist;

                for (auto p = next; p !is hd; p = p.next)
                     {
                     current.next = alloc().set (p.value, current, null);
                     current = current.next;
                     }
                newlist.prev = current;
                current.next = newlist;
                return newlist;
        }
}


