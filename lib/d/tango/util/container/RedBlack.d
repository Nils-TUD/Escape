/*******************************************************************************

        copyright:      Copyright (c) 2008 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Apr 2008: Initial release

        authors:        Kris, tsalm

        Since:          0.99.7

        Based upon Doug Lea's Java collection package

*******************************************************************************/

module tango.util.container.RedBlack;

private import tango.util.container.model.IContainer;

private typedef int AttributeDummy;

/*******************************************************************************

        RedBlack implements basic capabilities of Red-Black trees,
        an efficient kind of balanced binary tree. The particular
        algorithms used are adaptations of those in Corman,
        Lieserson, and Rivest's <EM>Introduction to Algorithms</EM>.
        This class was inspired by (and code cross-checked with) a 
        similar class by Chuck McManis. The implementations of
        rebalancings during insertion and deletion are
        a little trickier than those versions since they
        don't swap Cell contents or use special dummy nilnodes. 

        Doug Lea

*******************************************************************************/

struct RedBlack (V, A = AttributeDummy)
{
        alias RedBlack!(V, A) Type;
        alias Type            *Ref;

        enum : bool {RED = false, BLACK = true}

        /**
         * Pointer to left child
        **/

        package Ref     left;

        /**
         * Pointer to right child
        **/

        package Ref     right;

        /**
         * Pointer to parent (null if root)
        **/

        package Ref     parent;

        package V       value;

        static if (!is(typeof(A) == AttributeDummy))
        {
               A        attribute;
        }

        /**
         * The node color (RED, BLACK)
        **/

        package bool    color;

        static if (!is(typeof(A) == AttributeDummy))
        {
                final Ref set (V v, A a)
                {
                        attribute = a;
                        return set (v);
                }
        }

        /**
         * Make a new Cell with given value, null links, and BLACK color.
         * Normally only called to establish a new root.
        **/

        final Ref set (V v)
        {
                value = v;
                left = null;
                right = null;
                parent = null;
                color = BLACK;
                return this;
        }

        /**
         * Return a new Ref with same value and color as self,
         * but with null links. (Since it is never OK to have
         * multiple identical links in a RB tree.)
        **/ 

        protected Ref dup (Ref delegate() alloc)
        {
                static if (is(typeof(A) == AttributeDummy))
                           auto t = alloc().set (value);
                       else
                          auto t = alloc().set (value, attribute);

                t.color = color;
                return t;
        }


        /**
         * See_Also: tango.util.collection.model.View.View.checkImplementation.
        **/
        public void checkImplementation ()
        {

                // It's too hard to check the property that every simple
                // path from node to leaf has same number of black nodes.
                // So restrict to the following

                assert(parent is null ||
                       this is parent.left ||
                       this is parent.right);

                assert(left is null ||
                       this is left.parent);

                assert(right is null ||
                       this is right.parent);

                assert(color is BLACK ||
                       (colorOf(left) is BLACK) && (colorOf(right) is BLACK));

                if (left !is null)
                        left.checkImplementation();
                if (right !is null)
                        right.checkImplementation();
        }

        /**
         * Return the minimum value of the current (sub)tree
        **/

        Ref leftmost ()
        {
                auto p = this;
                for ( ; p.left; p = p.left) {}
                return p;
        }

        /**
         * Return the maximum value of the current (sub)tree
        **/
        Ref rightmost ()
        {
                auto p = this;
                for ( ; p.right; p = p.right) {}
                return p;
        }

        /**
         * Return the root (parentless node) of the tree
        **/
        Ref root ()
        {
                auto p = this;
                for ( ; p.parent; p = p.parent) {}
                return p;
        }

        /**
         * Return true if node is a root (i.e., has a null parent)
        **/

        bool isRoot ()
        {
                return parent is null;
        }


        /**
         * Return the inorder successor, or null if no such
        **/

        Ref successor ()
        {
                if (right)
                    return right.leftmost;

                auto p = parent;
                auto ch = this;
                while (p && ch is p.right)
                      {
                      ch = p;
                      p = p.parent;
                      }
                return p;
        }

        /**
         * Return the inorder predecessor, or null if no such
        **/

        Ref predecessor ()
        {
                if (left)
                    return left.rightmost;

                auto p = parent;
                auto ch = this;
                while (p && ch is p.left)
                      {
                      ch = p;
                      p = p.parent;
                      }
                return p;
        }

        /**
         * Return the number of nodes in the subtree
        **/
        int size ()
        {
                auto c = 1;
                if (left)
                    c += left.size;
                if (right)
                    c += right.size;
                return c;
        }


        /**
         * Return node of current subtree containing value as value(), 
         * if it exists, else null. 
         * Uses Comparator cmp to find and to check equality.
        **/

        Ref find (V value, Compare!(V) cmp)
        {
                auto t = this;
                for (;;)
                    {
                    auto diff = cmp (value, t.value);
                    if (diff is 0)
                        return t;
                    else
                       if (diff < 0)
                           t = t.left;
                       else
                          t = t.right;
                    if (t is null)
                        break;
                    }
                return null;
        }


        /**
         * Return node of subtree matching "value" 
         * or, if none found, the one just after or before  
         * if it doesn't exist, return null
         * Uses Comparator cmp to find and to check equality.
        **/
        Ref findFirst (V value, Compare!(V) cmp, bool after = true)
        {
                auto t = this;
                auto tLower = this;
                auto tGreater  = this;
            
                for (;;)
                    {
                    auto diff = cmp (value, t.value);
                    if (diff is 0)
                        return t;
                   else
                      if (diff < 0)
                         {
                         tGreater = t;
                         t = t.left;
                         }
                      else
                         {
                         tLower = t;
                         t = t.right;
                         }
                   if (t is null)
                       break;
                   }
    
                if (after)
                   { 
                   if (cmp (value, tGreater.value) <= 0)
                       if (cmp (value, tGreater.value) < 0)
                           return tGreater;
                   }
                else
                   {
                   if (cmp (value, tLower.value) >= 0)
                       if (cmp (value, tLower.value) > 0)
                           return tLower;
                   }

                return null;
        }
        
        /**
         * Return number of nodes of current subtree containing value.
         * Uses Comparator cmp to find and to check equality.
        **/
        int count (V value, Compare!(V) cmp)
        {
                auto c = 0;
                auto t = this;
                while (t)
                      {
                      int diff = cmp (value, t.value);
                      if (diff is 0)
                         {
                         ++c;
                         if (t.left is null)
                             t = t.right;
                         else
                            if (t.right is null)
                                t = t.left;
                            else
                               {
                               c += t.right.count (value, cmp);
                               t = t.left;
                               }
                            }
                         else
                            if (diff < 0)
                                t = t.left;
                            else
                               t = t.right;
                         }
                return c;
        }
        
        static if (!is(typeof(A) == AttributeDummy))
        {
        Ref findAttribute (A attribute, Compare!(A) cmp)
        {
                auto t = this;

                while (t)
                      {
                      if (t.attribute == attribute)
                          return t;
                      else
                        if (t.right is null)
                            t = t.left;
                        else
                           if (t.left is null)
                               t = t.right;
                           else
                              {
                              auto p = t.left.findAttribute (attribute, cmp);

                              if (p !is null)
                                  return p;
                              else
                                 t = t.right;
                              }
                      }
                return null; // not reached
        }

        int countAttribute (A attrib, Compare!(A) cmp)
        {
                int c = 0;
                auto t = this;

                while (t)
                      {
                      if (t.attribute == attribute)
                          ++c;

                      if (t.right is null)
                          t = t.left;
                      else
                         if (t.left is null)
                             t = t.right;
                         else
                            {
                            c += t.left.countAttribute (attribute, cmp);
                            t = t.right;
                            }
                      }
                return c;
        }

        /**
         * find and return a cell holding (key, element), or null if no such
        **/
        Ref find (V value, A attribute, Compare!(V) cmp)
        {
                auto t = this;

                for (;;)
                    {
                    int diff = cmp (value, t.value);
                    if (diff is 0 && t.attribute == attribute)
                        return t;
                    else
                       if (diff <= 0)
                           t = t.left;
                       else
                          t = t.right;

                    if (t is null)
                        break;
                    }
                return null;
        }

        }



        /**
         * Return a new subtree containing each value of current subtree
        **/

        Ref copyTree (Ref delegate() alloc)
        {
                auto t = dup (alloc);

                if (left)
                   {
                   t.left = left.copyTree (alloc);
                   t.left.parent = t;
                   }

                if (right)
                   {
                   t.right = right.copyTree (alloc);
                   t.right.parent = t;
                   }

                return t;
        }


        /**
         * There's no generic value insertion. Instead find the
         * place you want to add a node and then invoke insertLeft
         * or insertRight.
         * <P>
         * Insert Cell as the left child of current node, and then
         * rebalance the tree it is in.
         * @param Cell the Cell to add
         * @param root, the root of the current tree
         * Returns: the new root of the current tree. (Rebalancing
         * can change the root!)
        **/


        Ref insertLeft (Ref cell, Ref root)
        {
                left = cell;
                cell.parent = this;
                return cell.fixAfterInsertion (root);
        }

        /**
         * Insert Cell as the right child of current node, and then
         * rebalance the tree it is in.
         * @param Cell the Cell to add
         * @param root, the root of the current tree
         * Returns: the new root of the current tree. (Rebalancing
         * can change the root!)
        **/

        Ref insertRight (Ref cell, Ref root)
        {
                right = cell;
                cell.parent = this;
                return cell.fixAfterInsertion (root);
        }


        /**
         * Delete the current node, and then rebalance the tree it is in
         * @param root the root of the current tree
         * Returns: the new root of the current tree. (Rebalancing
         * can change the root!)
        **/


        Ref remove (Ref root)
        {
                // handle case where we are only node
                if (left is null && right is null && parent is null)
                    return null;

                // if strictly internal, swap places with a successor
                if (left && right)
                   {
                   auto s = successor;

                   // To work nicely with arbitrary subclasses of Ref, we don't want to
                   // just copy successor's fields. since we don't know what
                   // they are.  Instead we swap positions _in the tree.
                   root = swapPosition (this, s, root);
                   }

                // Start fixup at replacement node (normally a child).
                // But if no children, fake it by using self

                if (left is null && right is null)
                   {
                   if (color is BLACK)
                       root = this.fixAfterDeletion (root);

                   // Unlink  (Couldn't before since fixAfterDeletion needs parent ptr)
                   if (parent)
                      {
                      if (this is parent.left)
                          parent.left = null;
                      else
                         if (this is parent.right)
                             parent.right = null;
                      parent = null;
                      }
                   }
                else
                   {
                   auto replacement = left;
                   if (replacement is null)
                       replacement = right;

                   // link replacement to parent
                   replacement.parent = parent;

                   if (parent is null)
                       root = replacement;
                   else
                      if (this is parent.left)
                          parent.left = replacement;
                      else
                         parent.right = replacement;

                   left = null;
                   right = null;
                   parent = null;
        
                   // fix replacement
                   if (color is BLACK)
                       root = replacement.fixAfterDeletion (root);
                   }
                return root;
        }

        /**
         * Swap the linkages of two nodes in a tree.
         * Return new root, in case it changed.
        **/

        static Ref swapPosition (Ref x, Ref y, Ref root)
        {
                /* Too messy. TODO: find sequence of assigments that are always OK */

                auto px = x.parent;
                bool xpl = px !is null && x is px.left;
                auto lx = x.left;
                auto rx = x.right;

                auto py = y.parent;
                bool ypl = py !is null && y is py.left;
                auto ly = y.left;
                auto ry = y.right;

                if (x is py)
                   {
                   y.parent = px;
                   if (px !is null)
                       if (xpl)
                           px.left = y;
                       else
                          px.right = y;

                   x.parent = y;
                   if (ypl)
                      {
                      y.left = x;
                      y.right = rx;
                      if (rx !is null)
                      rx.parent = y;
                      }
                   else
                      {
                      y.right = x;
                      y.left = lx;
                      if (lx !is null)
                      lx.parent = y;
                      }

                   x.left = ly;
                   if (ly !is null)
                       ly.parent = x;

                   x.right = ry;
                   if (ry !is null)
                       ry.parent = x;
                   }
                else
                   if (y is px)
                      {
                      x.parent = py;
                      if (py !is null)
                          if (ypl)
                              py.left = x;
                          else
                             py.right = x;

                      y.parent = x;
                      if (xpl)
                         {
                         x.left = y;
                         x.right = ry;
                         if (ry !is null)
                             ry.parent = x;
                         }
                      else
                         {
                         x.right = y;
                         x.left = ly;
                         if (ly !is null)
                             ly.parent = x;
                         }

                      y.left = lx;
                      if (lx !is null)
                          lx.parent = y;

                      y.right = rx;
                      if (rx !is null)
                          rx.parent = y;
                      }
                   else
                      {
                      x.parent = py;
                      if (py !is null)
                          if (ypl)
                              py.left = x;
                          else
                             py.right = x;

                      x.left = ly;
                      if (ly !is null)
                          ly.parent = x;

                      x.right = ry;
                      if (ry !is null)
                          ry.parent = x;
        
                      y.parent = px;
                      if (px !is null)
                          if (xpl)
                              px.left = y;
                          else
                             px.right = y;

                      y.left = lx;
                      if (lx !is null)
                          lx.parent = y;

                      y.right = rx;
                      if (rx !is null)
                          rx.parent = y;
                      }

                bool c = x.color;
                x.color = y.color;
                y.color = c;

                if (root is x)
                    root = y;
                else
                   if (root is y)
                       root = x;
                return root;
        }



        /**
         * Return color of node p, or BLACK if p is null
         * (In the CLR version, they use
         * a special dummy `nil' node for such purposes, but that doesn't
         * work well here, since it could lead to creating one such special
         * node per real node.)
         *
        **/

        static bool colorOf (Ref p)
        {
                return (p is null) ? BLACK : p.color;
        }

        /**
         * return parent of node p, or null if p is null
        **/
        static Ref parentOf (Ref p)
        {
                return (p is null) ? null : p.parent;
        }

        /**
         * Set the color of node p, or do nothing if p is null
        **/

        static void setColor (Ref p, bool c)
        {
                if (p !is null)
                    p.color = c;
        }

        /**
         * return left child of node p, or null if p is null
        **/

        static Ref leftOf (Ref p)
        {
                return (p is null) ? null : p.left;
        }

        /**
         * return right child of node p, or null if p is null
        **/

        static Ref rightOf (Ref p)
        {
                return (p is null) ? null : p.right;
        }


        /** From CLR **/
        package Ref rotateLeft (Ref root)
        {
                auto r = right;
                right = r.left;

                if (r.left)
                    r.left.parent = this;

                r.parent = parent;
                if (parent is null)
                    root = r;
                else
                   if (parent.left is this)
                       parent.left = r;
                   else
                      parent.right = r;

                r.left = this;
                parent = r;
                return root;
        }

        /** From CLR **/
        package Ref rotateRight (Ref root)
        {
                auto l = left;
                left = l.right;

                if (l.right !is null)
                   l.right.parent = this;

                l.parent = parent;
                if (parent is null)
                    root = l;
                else
                   if (parent.right is this)
                       parent.right = l;
                   else
                      parent.left = l;

                l.right = this;
                parent = l;
                return root;
        }


        /** From CLR **/
        package Ref fixAfterInsertion (Ref root)
        {
                color = RED;
                auto x = this;

                while (x && x !is root && x.parent.color is RED)
                      {
                      if (parentOf(x) is leftOf(parentOf(parentOf(x))))
                         {
                         auto y = rightOf(parentOf(parentOf(x)));
                         if (colorOf(y) is RED)
                            {
                            setColor(parentOf(x), BLACK);
                            setColor(y, BLACK);
                            setColor(parentOf(parentOf(x)), RED);
                            x = parentOf(parentOf(x));
                            }
                         else
                            {
                            if (x is rightOf(parentOf(x)))
                               {
                               x = parentOf(x);
                               root = x.rotateLeft(root);
                               }

                            setColor(parentOf(x), BLACK);
                            setColor(parentOf(parentOf(x)), RED);
                            if (parentOf(parentOf(x)) !is null)
                                root = parentOf(parentOf(x)).rotateRight(root);
                            }
                         }
                      else
                         {
                         auto y = leftOf(parentOf(parentOf(x)));
                         if (colorOf(y) is RED)
                            {
                            setColor(parentOf(x), BLACK);
                            setColor(y, BLACK);
                            setColor(parentOf(parentOf(x)), RED);
                            x = parentOf(parentOf(x));
                            }
                         else
                            {
                            if (x is leftOf(parentOf(x)))
                               {
                               x = parentOf(x);
                               root = x.rotateRight(root);
                               }
        
                            setColor(parentOf(x), BLACK);
                            setColor(parentOf(parentOf(x)), RED);
                            if (parentOf(parentOf(x)) !is null)
                                root = parentOf(parentOf(x)).rotateLeft(root);
                            }
                         }
                      }
                root.color = BLACK;
                return root;
        }



        /** From CLR **/
        package Ref fixAfterDeletion(Ref root)
        {
                auto x = this;
                while (x !is root && colorOf(x) is BLACK)
                      {
                      if (x is leftOf(parentOf(x)))
                         {
                         auto sib = rightOf(parentOf(x));
                         if (colorOf(sib) is RED)
                            {
                            setColor(sib, BLACK);
                            setColor(parentOf(x), RED);
                            root = parentOf(x).rotateLeft(root);
                            sib = rightOf(parentOf(x));
                            }
                         if (colorOf(leftOf(sib)) is BLACK && colorOf(rightOf(sib)) is BLACK)
                            {
                            setColor(sib, RED);
                            x = parentOf(x);
                            }
                         else
                            {
                            if (colorOf(rightOf(sib)) is BLACK)
                               {
                               setColor(leftOf(sib), BLACK);
                               setColor(sib, RED);
                               root = sib.rotateRight(root);
                               sib = rightOf(parentOf(x));
                               }

                            setColor(sib, colorOf(parentOf(x)));
                            setColor(parentOf(x), BLACK);
                            setColor(rightOf(sib), BLACK);
                            root = parentOf(x).rotateLeft(root);
                            x = root;
                            }
                         }
                      else
                         {
                         auto sib = leftOf(parentOf(x));
                         if (colorOf(sib) is RED)
                            {
                            setColor(sib, BLACK);
                            setColor(parentOf(x), RED);
                            root = parentOf(x).rotateRight(root);
                            sib = leftOf(parentOf(x));
                            }

                         if (colorOf(rightOf(sib)) is BLACK && colorOf(leftOf(sib)) is BLACK)
                            {
                            setColor(sib, RED);
                            x = parentOf(x);
                            }
                         else
                            {
                            if (colorOf(leftOf(sib)) is BLACK)
                               {
                               setColor(rightOf(sib), BLACK);
                               setColor(sib, RED);
                               root = sib.rotateLeft(root);
                               sib = leftOf(parentOf(x));
                               }

                            setColor(sib, colorOf(parentOf(x)));
                            setColor(parentOf(x), BLACK);
                            setColor(leftOf(sib), BLACK);
                            root = parentOf(x).rotateRight(root);
                            x = root;
                            }
                         }
                      }

                setColor(x, BLACK);
                return root;
        }
}
