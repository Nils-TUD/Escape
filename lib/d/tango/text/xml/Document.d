/*******************************************************************************

        Copyright: Copyright (C) 2007 Aaron Craelius and Kris Bell  
                   All rights reserved.

        License:   BSD style: $(LICENSE)

        version:   Initial release: February 2008      

        Authors:   Aaron, Kris

*******************************************************************************/

module tango.text.xml.Document;

package import tango.text.xml.PullParser;

version=discrete;

/*******************************************************************************

        Implements a DOM atop the XML parser, supporting document 
        parsing, tree traversal and ad-hoc tree manipulation.

        The DOM API is non-conformant, yet simple and functional in 
        style - locate a tree node of interest and operate upon or 
        around it. In all cases you will need a document instance to 
        begin, whereupon it may be populated either by parsing an 
        existing document or via API manipulation.

        This particular DOM employs a simple free-list to allocate
        each of the tree nodes, making it quite efficient at parsing
        XML documents. The tradeoff with such a scheme is that copying
        nodes from one document to another requires a little more care
        than otherwise. We felt this was a reasonable tradeoff, given
        the throughput gains vs the relative infrequency of grafting
        operations. For grafting within or across documents, please
        use the move() and copy() methods.

        Another simplification is related to entity transcoding. This
        is not performed internally, and becomes the responsibility
        of the client. That is, the client should perform appropriate
        entity transcoding as necessary. Paying the (high) transcoding 
        cost for all documents doesn't seem appropriate.

        Parse example
        ---
        auto doc = new Document!(char);
        doc.parse (content);

        auto print = new DocPrinter!(char);
        Stdout(print(doc)).newline;
        ---

        API example
        ---
        auto doc = new Document!(char);

        // attach an xml header
        doc.header;

        // attach an element with some attributes, plus 
        // a child element with an attached data value
        doc.tree.element   (null, "element")
                .attribute (null, "attrib1", "value")
                .attribute (null, "attrib2")
                .element   (null, "child", "value");

        auto print = new DocPrinter!(char);
        Stdout(print(doc)).newline;
        ---

        Note that the document tree() includes all nodes in the tree,
        and not just elements. Use doc.elements to address the topmost
        element instead. For example, adding an interior sibling to
        the prior illustration
        ---
        doc.elements.element (null, "sibling");
        ---

        Printing the name of the topmost (root) element:
        ---
        Stdout.formatln ("first element is '{}'", doc.elements.name);
        ---
        
        XPath examples:
        ---
        auto doc = new Document!(char);

        // attach an element with some attributes, plus 
        // a child element with an attached data value
        doc.tree.element   (null, "element")
                .attribute (null, "attrib1", "value")
                .attribute (null, "attrib2")
                .element   (null, "child", "value");

        // select named-elements
        auto set = doc.query["element"]["child"];

        // select all attributes named "attrib1"
        set = doc.query.descendant.attribute("attrib1");

        // select elements with one parent and a matching text value
        set = doc.query[].filter((doc.Node n) {return n.children.hasData("value");});
        ---

        Note that path queries are temporal - they do not retain content
        across mulitple queries. That is, the lifetime of a query result
        is limited unless you explicitly copy it. For example, this will 
        fail
        ---
        auto elements = doc.query["element"];
        auto children = elements["child"];
        ---

        The above will lose elements because the associated document reuses 
        node space for subsequent queries. In order to retain results, do this
        ---
        auto elements = doc.query["element"].dup;
        auto children = elements["child"];
        ---

        The above .dup is generally very small (a set of pointers only). On
        the other hand, recursive queries are fully supported
        ---
        set = doc.query[].filter((doc.Node n) {return n.query[].count > 1;});
        ---

        Typical usage tends to follow the following pattern, Where each query 
        result is processed before another is initiated
        ---
        foreach (node; doc.query.child("element"))
                {
                // do something with each node
                }
        ---

        Note that the parser is templated for char, wchar or dchar.
            
*******************************************************************************/

class Document(T) : package PullParser!(T)
{
        public alias NodeImpl*  Node;

        private Node            root; 
        private NodeImpl[]      list;
        private NodeImpl[][]    lists;
        private int             index,
                                chunks,
                                freelists;
        private XmlPath!(T)     xpath;

        /***********************************************************************
        
                Construct a DOM instance. The optional parameter indicates
                the initial number of nodes assigned to the freelist

        ***********************************************************************/

        this (uint nodes = 1000)
        {
                assert (nodes > 50);
                super (null);
                xpath = new XmlPath!(T);

                chunks = nodes;
                newlist;
                root = allocate;
                root.id = XmlNodeType.Document;
        }

        /***********************************************************************
        
                Return an xpath handle to query this document. This starts
                at the document root.

                See also Node.query

        ***********************************************************************/
        
        final XmlPath!(T).NodeSet query ()
        {
                return xpath.start (root);
        }

        /***********************************************************************
        
                Return the root document node, from which all other nodes
                are descended. 

                Returns null where there are no nodes in the document

        ***********************************************************************/
        
        final Node tree ()
        {
                return root;
        }

        /***********************************************************************
        
                Return the topmost element node, which is generally the
                root of the element tree.

                Returns null where there are no top-level element nodes

        ***********************************************************************/
        
        final Node elements ()
        {
                if (root)
                   {
                   auto node = root.lastChild;
                   while (node)
                          if (node.id is XmlNodeType.Element)
                              return node;
                          else
                             node = node.prev;
                   }
                return null;
        }

        /***********************************************************************
        
                Reset the freelist. Subsequent allocation of document nodes 
                will overwrite prior instances.

        ***********************************************************************/
        
        final Document reset ()
        {
                root.lastChild = root.firstChild = null;
                freelists = 0;
                newlist;
                index = 1;
version(d)
{
                freelists = 0;          // needed to align the codegen!
}
                return this;
        }

        /***********************************************************************
        
               Prepend an XML header to the document tree

        ***********************************************************************/
        
        final Document header (T[] encoding = null)
        {
                if (encoding.length)
                    encoding = `xml version="1.0" encoding="`~encoding~`"`;
                else
                   encoding = `xml version="1.0" encoding="UTF-8"`;

                root.prepend (root.create(XmlNodeType.PI, encoding));
                return this;
        }

        /***********************************************************************
        
                Parse the given xml content, which will reuse any existing 
                node within this document. The resultant tree is retrieved
                via the document 'tree' attribute

        ***********************************************************************/
        
        final void parse(T[] xml)
        {       
                assert (xml);
                reset;
                super.reset (xml);
                auto cur = root;
                uint defNamespace;

                while (true) 
                      {
                      auto p = text.point;
                      switch (super.next) 
                             {
                             case XmlTokenType.EndElement:
                             case XmlTokenType.EndEmptyElement:
                                  assert (cur.host);
                                  cur.end = text.point;
                                  cur = cur.host;                      
                                  break;
        
                             case XmlTokenType.Data:
version (discrete)
{
                                  auto node = allocate;
                                  node.rawValue = super.rawValue;
                                  node.id = XmlNodeType.Data;
                                  cur.append (node);
}
else
{
                                  if (cur.rawValue.length is 0)
                                      cur.rawValue = super.rawValue;
                                  else
                                     // multiple data sections
                                     cur.data (super.rawValue);
}
                                  break;
        
                             case XmlTokenType.StartElement:
                                  auto node = allocate;
                                  node.host = cur;
                                  node.prefixed = super.prefix;
                                  node.id = XmlNodeType.Element;
                                  node.localName = super.localName;
                                  node.start = p;
                                
                                  // inline append
                                  if (cur.lastChild) 
                                     {
                                     cur.lastChild.nextSibling = node;
                                     node.prevSibling = cur.lastChild;
                                     cur.lastChild = node;
                                     }
                                  else 
                                     {
                                     cur.firstChild = node;
                                     cur.lastChild = node;
                                     }
                                  cur = node;
                                  break;
        
                             case XmlTokenType.Attribute:
                                  auto attr = allocate;
                                  attr.prefixed = super.prefix;
                                  attr.rawValue = super.rawValue;
                                  attr.localName = super.localName;
                                  attr.id = XmlNodeType.Attribute;
                                  cur.attrib (attr);
                                  break;
        
                             case XmlTokenType.PI:
                                  cur.pi_ (super.rawValue, p[0..text.point-p]);
                                  break;
        
                             case XmlTokenType.Comment:
                                  cur.comment_ (super.rawValue);
                                  break;
        
                             case XmlTokenType.CData:
                                  cur.cdata_ (super.rawValue);
                                  break;
        
                             case XmlTokenType.Doctype:
                                  cur.doctype_ (super.rawValue);
                                  break;
        
                             case XmlTokenType.Done:
                                  return;

                             default:
                                  break;
                             }
                      }
        }
        
        /***********************************************************************
        
                allocate a node from the freelist

        ***********************************************************************/

        private final Node allocate ()
        {
                if (index >= list.length)
                    newlist;

                auto p = &list[index++];
                p.start = p.end = null;
                p.doc = this;
                p.host =
                p.prevSibling = 
                p.nextSibling = 
                p.firstChild =
                p.lastChild = 
                p.firstAttr =
                p.lastAttr = null;
                p.rawValue = null;
                return p;
        }

        /***********************************************************************
        
                allocate a node from the freelist

        ***********************************************************************/

        private final void newlist ()
        {
                index = 0;
                if (freelists >= lists.length)
                   {
                   lists.length = lists.length + 1;
                   lists[$-1] = new NodeImpl [chunks];
                   }
                list = lists[freelists++];
        }

        /***********************************************************************
        
                foreach support for visiting and selecting nodes. 
                
                A fruct is a low-overhead mechanism for capturing context 
                relating to an opApply, and we use it here to sweep nodes
                when testing for various relationships.

                See Node.attributes and Node.children

        ***********************************************************************/
        
        private struct Visitor
        {
                private Node node;
        
                public alias value      data;
                public alias hasValue   hasData;

                /***************************************************************
                
                        Is there anything to visit here?

                        Time complexity: O(1)

                ***************************************************************/
        
                bool exist ()
                {
                        return node != null;
                }

                /***************************************************************
                
                        traverse sibling nodes

                ***************************************************************/
        
                int opApply (int delegate(ref Node) dg)
                {
                        int ret;

                        for (auto n=node; n; n = n.nextSibling)
                             if ((ret = dg(n)) != 0) 
                                  break;
                        return ret;
                }

                /***************************************************************
                
                        Locate a node with a matching name and/or prefix, 
                        and which passes an optional filter. Each of the
                        arguments will be ignored where they are null.

                        Time complexity: O(n)

                ***************************************************************/

                Node name (T[] prefix, T[] local, bool delegate(Node) dg=null)
                {
                        for (auto n=node; n; n = n.nextSibling)
                            {
                            if (local.ptr && local != n.localName)
                                continue;

                            if (prefix.ptr && prefix != n.prefixed)
                                continue;

                            if (dg.ptr && dg(n) is false)
                                continue;

                            return n;
                            }
                        return null;
                }

                /***************************************************************
                
                        Scan nodes for a matching name and/or prefix. Each 
                        of the arguments will be ignored where they are null.

                        Time complexity: O(n)

                ***************************************************************/
        
                bool hasName (T[] prefix, T[] local)
                {
                        return name (prefix, local) != null;
                }

                /***************************************************************
                
                        Locate a node with a matching name and/or prefix, 
                        and which matches a specified value. Each of the
                        arguments will be ignored where they are null.

                        Time complexity: O(n)

                ***************************************************************/
version (Filter)
{        
                Node value (T[] prefix, T[] local, T[] value)
                {
                        if (value.ptr)
                            return name (prefix, local, (Node n){return value == n.rawValue;});
                        return name (prefix, local);
                }
}
                /***************************************************************
        
                        Sweep nodes looking for a match, and returns either 
                        a node or null. See value(x,y,z) or name(x,y,z) for
                        additional filtering.

                        Time complexity: O(n)

                ***************************************************************/

                Node value (T[] match)
                {
                        if (match.ptr)
                            for (auto n=node; n; n = n.nextSibling)
                                 if (match == n.rawValue)
                                     return n;
                        return null;
                }

                /***************************************************************
                
                        Sweep the nodes looking for a value match. Returns 
                        true if found. See value(x,y,z) or name(x,y,z) for
                        additional filtering.

                        Time complexity: O(n)

                ***************************************************************/
        
                bool hasValue (T[] match)
                {
                        return value(match) != null;
                }
        }
        
        
        /***********************************************************************
        
                The node implementation

        ***********************************************************************/
        
        private struct NodeImpl
        {
                public void*            user;           /// open for usage
                package Document        doc;            // owning document
                package XmlNodeType     id;             // node type
                package T[]             prefixed;       // namespace
                package T[]             localName;      // name
                package T[]             rawValue;       // data value
                
                package Node            host,           // parent node
                                        prevSibling,    // prior
                                        nextSibling,    // next
                                        firstChild,     // head
                                        lastChild,      // tail
                                        firstAttr,      // head
                                        lastAttr;       // tail

                package T*              end,            // slice of the  ...
                                        start;          // original xml text 

                /***************************************************************
                
                        Return the hosting document

                ***************************************************************/
        
                Document document () 
                {
                        return doc;
                }
        
                /***************************************************************
                
                        Return the node type-id

                ***************************************************************/
        
                XmlNodeType type () 
                {
                        return id;
                }
        
                /***************************************************************
                
                        Return the parent, which may be null

                ***************************************************************/
        
                Node parent () 
                {
                        return host;
                }
        
                /***************************************************************
                
                        Return the first child, which may be null

                ***************************************************************/
                
                Node child () 
                {
                        return firstChild;
                }
        
                /***************************************************************
                
                        Return the last child, which may be null

                        Deprecated: exposes too much implementation detail. 
                                    Please file a ticket if you really need 
                                    this functionality

                ***************************************************************/
        
                deprecated Node childTail () 
                {
                        return lastChild;
                }
        
                /***************************************************************
                
                        Return the prior sibling, which may be null

                ***************************************************************/
        
                Node prev () 
                {
                        return prevSibling;
                }
        
                /***************************************************************
                
                        Return the next sibling, which may be null

                ***************************************************************/
        
                Node next () 
                {
                        return nextSibling;
                }
        
                /***************************************************************
                
                        Return the namespace prefix of this node (may be null)

                ***************************************************************/
        
                T[] prefix ()
                {
                        return prefixed;
                }

                /***************************************************************
                
                        Set the namespace prefix of this node (may be null)

                ***************************************************************/
        
                Node prefix (T[] replace)
                {
                        prefixed = replace;
                        return this;
                }

                /***************************************************************
                
                        Return the vanilla node name (sans prefix)

                ***************************************************************/
        
                T[] name ()
                {
                        return localName;
                }

                /***************************************************************
                
                        Set the vanilla node name (sans prefix)

                ***************************************************************/
        
                Node name (T[] replace)
                {
                        localName = replace;
                        return this;
                }

                /***************************************************************
                
                        Return the data content, which may be null

                ***************************************************************/
        
                T[] value ()
                {
version(discrete)
{
                        if (type is XmlNodeType.Element)
                            foreach (child; children)
                                     if (child.id is XmlNodeType.Data || 
                                         child.id is XmlNodeType.CData)
                                         return child.rawValue;
}
                        return rawValue;
                }
                
                /***************************************************************
                
                        Set the raw data content, which may be null

                ***************************************************************/
        
                void value (T[] val)
                {
version(discrete)
{
                        if (type is XmlNodeType.Element)
                            foreach (child; children)
                                     if (child.id is XmlNodeType.Data)
                                         return child.value (val);
}
                        rawValue = val; 
                        mutate;
                }
                
                /***************************************************************
                
                        Return the full node name, which is a combination 
                        of the prefix & local names. Nodes without a prefix 
                        will return local-name only

                ***************************************************************/
        
                T[] toString (T[] output = null)
                {
                        if (prefixed.length)
                           {
                           auto len = prefixed.length + localName.length + 1;
                           
                           // is the prefix already attached to the name?
                           if (prefixed.ptr + prefixed.length + 1 is localName.ptr &&
                               ':' is *(localName.ptr-1))
                               return prefixed.ptr [0 .. len];
       
                           // nope, copy the discrete segments into output
                           if (output.length < len)
                               output.length = len;
                           output[0..prefixed.length] = prefixed;
                           output[prefixed.length] = ':';
                           output[prefixed.length+1 .. len] = localName;
                           return output[0..len];
                           }

                        return localName;
                }
                
                /***************************************************************
                
                        Return the index of this node, or how many 
                        prior siblings it has. 

                        Time complexity: O(n) 

                ***************************************************************/
       
                uint position ()
                {
                        auto count = 0;
                        auto prior = prevSibling;
                        while (prior)
                               ++count, prior = prior.prevSibling;                        
                        return count;
                }
                
                /***************************************************************
                
                        Detach this node from its parent and siblings

                ***************************************************************/
        
                Node detach ()
                {
                        return remove;
                }

                /***************************************************************
        
                        Return an xpath handle to query this node

                        See also Document.query

                ***************************************************************/
        
                final XmlPath!(T).NodeSet query ()
                {
                        return doc.xpath.start (this);
                }

                /***************************************************************
                
                        Return a foreach iterator for node children

                ***************************************************************/
        
                Visitor children () 
                {
                        Visitor v = {firstChild};
                        return v;
                }
        
                /***************************************************************
                
                        Return a foreach iterator for node attributes

                ***************************************************************/
        
                Visitor attributes () 
                {
                        Visitor v = {firstAttr};
                        return v;
                }
        
                /***************************************************************
                
                        Returns whether there are attributes present or not

                        Deprecated: use node.attributes.exist instead

                ***************************************************************/
        
                deprecated bool hasAttributes () 
                {
                        return firstAttr !is null;
                }
                               
                /***************************************************************
                
                        Returns whether there are children present or nor

                        Deprecated: use node.child or node.children.exist
                        instead

                ***************************************************************/
        
                deprecated bool hasChildren () 
                {
                        return firstChild !is null;
                }
                
                /***************************************************************
                
                        Duplicate the given sub-tree into place as a child 
                        of this node. 
                        
                        Returns a reference to the subtree

                ***************************************************************/
        
                Node copy (Node tree)
                {
                        assert (tree);
                        tree = tree.clone;
                        tree.migrate (document);

                        if (tree.id is XmlNodeType.Attribute)
                            attrib (tree);
                        else
                            append (tree);
                        return tree;
                }

                /***************************************************************
                
                        Relocate the given sub-tree into place as a child 
                        of this node. 
                        
                        Returns a reference to the subtree

                ***************************************************************/
        
                Node move (Node tree)
                {
                        tree.detach;
                        if (tree.doc is doc)
                           {
                           if (tree.id is XmlNodeType.Attribute)
                               attrib (tree);
                           else
                              append (tree);
                           }
                        else
                           tree = copy (tree);
                        return tree;
                }

                /***************************************************************
        
                        Appends a new (child) Element and returns a reference 
                        to it.

                ***************************************************************/
        
                Node element (T[] prefix, T[] local, T[] value = null)
                {
                        return element_ (prefix, local, value).mutate;
                }
        
                /***************************************************************
        
                        Attaches an Attribute and returns this, the host 

                ***************************************************************/
        
                Node attribute (T[] prefix, T[] local, T[] value = null)
                { 
                        return attribute_ (prefix, local, value).mutate;
                }
        
                /***************************************************************
        
                        Attaches a Data node and returns this, the host

                ***************************************************************/
        
                Node data (T[] data)
                {
                        return data_ (data).mutate;
                }
        
                /***************************************************************
        
                        Attaches a CData node and returns this, the host

                ***************************************************************/
        
                Node cdata (T[] cdata)
                {
                        return cdata_ (cdata).mutate;
                }
        
                /***************************************************************
        
                        Attaches a Comment node and returns this, the host

                ***************************************************************/
        
                Node comment (T[] comment)
                {
                        return comment_ (comment).mutate;
                }
        
                /***************************************************************
        
                        Attaches a Doctype node and returns this, the host

                ***************************************************************/
        
                Node doctype (T[] doctype)
                {
                        return doctype_ (doctype).mutate;
                }
        
                /***************************************************************
        
                        Attaches a PI node and returns this, the host

                ***************************************************************/
        
                Node pi (T[] pi)
                {
                        return pi_ (pi, null).mutate;
                }

                /***************************************************************
        
                        Attaches a child Element, and returns a reference 
                        to the child

                ***************************************************************/
        
                private Node element_ (T[] prefix, T[] local, T[] value = null)
                {
                        auto node = create (XmlNodeType.Element, null);
                        append (node.set (prefix, local));
version(discrete)
{
                        if (value.length)
                            node.data_ (value);
}
else
{
                        node.rawValue = value;
}
                        return node;
                }
        
                /***************************************************************
        
                        Attaches an Attribute, and returns the host

                ***************************************************************/
        
                private Node attribute_ (T[] prefix, T[] local, T[] value = null)
                { 
                        auto node = create (XmlNodeType.Attribute, value);
                        attrib (node.set (prefix, local));
                        return this;
                }
        
                /***************************************************************
        
                        Attaches a Data node, and returns the host

                ***************************************************************/
        
                private Node data_ (T[] data)
                {
                        append (create (XmlNodeType.Data, data));
                        return this;
                }
        
                /***************************************************************
        
                        Attaches a CData node, and returns the host

                ***************************************************************/
        
                private Node cdata_ (T[] cdata)
                {
                        append (create (XmlNodeType.CData, cdata));
                        return this;
                }
        
                /***************************************************************
        
                        Attaches a Comment node, and returns the host

                ***************************************************************/
        
                private Node comment_ (T[] comment)
                {
                        append (create (XmlNodeType.Comment, comment));
                        return this;
                }
        
                /***************************************************************
        
                        Attaches a PI node, and returns the host

                ***************************************************************/
        
                private Node pi_ (T[] pi, T[] patch)
                {
                        append (create(XmlNodeType.PI, pi).patch(patch));
                        return this;
                }
        
                /***************************************************************
        
                        Attaches a Doctype node, and returns the host

                ***************************************************************/
        
                private Node doctype_ (T[] doctype)
                {
                        append (create (XmlNodeType.Doctype, doctype));
                        return this;
                }
        
                /***************************************************************
                
                        Append an attribute to this node, The given attribute
                        cannot have an existing parent.

                ***************************************************************/
        
                private void attrib (Node node)
                {
                        assert (node.parent is null);
                        node.host = this;
                        if (lastAttr) 
                           {
                           lastAttr.nextSibling = node;
                           node.prevSibling = lastAttr;
                           lastAttr = node;
                           }
                        else 
                           firstAttr = lastAttr = node;
                }
        
                /***************************************************************
                
                        Append a node to this one. The given node cannot
                        have an existing parent.

                ***************************************************************/
        
                private void append (Node node)
                {
                        assert (node.parent is null);
                        node.host = this;
                        if (lastChild) 
                           {
                           lastChild.nextSibling = node;
                           node.prevSibling = lastChild;
                           lastChild = node;
                           }
                        else 
                           firstChild = lastChild = node;                  
                }

                /***************************************************************
                
                        Prepend a node to this one. The given node cannot
                        have an existing parent.

                ***************************************************************/
        
                private void prepend (Node node)
                {
                        assert (node.parent is null);
                        node.host = this;
                        if (firstChild) 
                           {
                           firstChild.prevSibling = node;
                           node.nextSibling = firstChild;
                           firstChild = node;
                           }
                        else 
                           firstChild = lastChild = node;
                }
                
                /***************************************************************
        
                        Configure node values
        
                ***************************************************************/
        
                private Node set (T[] prefix, T[] local)
                {
                        this.localName = local;
                        this.prefixed = prefix;
                        return this;
                }
        
                /***************************************************************
        
                        Creates and returns a child Element node

                ***************************************************************/
        
                private Node create (XmlNodeType type, T[] value)
                {
                        auto node = document.allocate;
                        node.rawValue = value;
                        node.id = type;
                        return node;
                }
        
                /***************************************************************
                
                        Detach this node from its parent and siblings

                ***************************************************************/
        
                private Node remove()
                {
                        if (! host) 
                              return this;
                        
                        mutate;
                        if (prevSibling && nextSibling) 
                           {
                           prevSibling.nextSibling = nextSibling;
                           nextSibling.prevSibling = prevSibling;
                           prevSibling = null;
                           nextSibling = null;
                           host = null;
                           }
                        else 
                           if (nextSibling)
                              {
                              debug assert(host.firstChild == this);
                              parent.firstChild = nextSibling;
                              nextSibling.prevSibling = null;
                              nextSibling = null;
                              host = null;
                              }
                           else 
                              if (type != XmlNodeType.Attribute)
                                 {
                                 if (prevSibling)
                                    {
                                    debug assert(host.lastChild == this);
                                    host.lastChild = prevSibling;
                                    prevSibling.nextSibling = null;
                                    prevSibling = null;
                                    host = null;
                                    }
                                 else
                                    {
                                    debug assert(host.firstChild == this);
                                    debug assert(host.lastChild == this);
                                    host.firstChild = null;
                                    host.lastChild = null;
                                    host = null;
                                    }
                                 }
                              else
                                 {
                                 if (prevSibling)
                                    {
                                    debug assert(host.lastAttr == this);
                                    host.lastAttr = prevSibling;
                                    prevSibling.nextSibling = null;
                                    prevSibling = null;
                                    host = null;
                                    }
                                 else
                                    {
                                    debug assert(host.firstAttr == this);
                                    debug assert(host.lastAttr == this);
                                    host.firstAttr = null;
                                    host.lastAttr = null;
                                    host = null;
                                    }
                                 }

                        return this;
                }

                /***************************************************************
        
                        Patch the serialization text, causing DocPrinter
                        to ignore the subtree of this node, and instead
                        emit the provided text as raw XML output.

                        Warning: this function does *not* copy the provided 
                        text, and may be removed from future revisions

                ***************************************************************/
        
                private Node patch (T[] text)
                {
                        end = text.ptr + text.length;
                        start = text.ptr;
                        return this;
                }
        
                /***************************************************************

                        purge serialization cache for this node and its
                        ancestors

                ***************************************************************/
        
                private Node mutate ()
                {
                        auto node = this;
                        do {
                           node.end = null;
                           } while ((node = node.host) !is null);

                        return this;
                }

                /***************************************************************
                
                        Duplicate a single node

                ***************************************************************/
        
                private Node dup ()
                {
                        return create(type, rawValue.dup).set(prefixed.dup, localName.dup);
                }

                /***************************************************************
                
                        Duplicate a subtree

                ***************************************************************/
        
                private Node clone ()
                {
                        auto p = dup;

                        foreach (attr; attributes)
                                 p.attrib (attr.dup);
                        foreach (child; children)
                                 p.append (child.clone);
                        return p;
                }

                /***************************************************************

                        Reset the document host for this subtree

                ***************************************************************/
        
                private void migrate (Document host)
                {
                        this.doc = host;
                        foreach (attr; attributes)
                                 attr.migrate (host);
                        foreach (child; children)
                                 child.migrate (host);
                }
        }
}


/*******************************************************************************

        XPath support 

        Provides support for common XPath axis and filtering functions,
        via a native-D interface instead of typical interpreted notation.

        The general idea here is to generate a NodeSet consisting of those
        tree-nodes which satisfy a filtering function. The direction, or
        axis, of tree traversal is governed by one of several predefined
        operations. All methods facilitiate call-chaining, where each step 
        returns a new NodeSet instance to be operated upon.

        The set of nodes themselves are collected in a freelist, avoiding
        heap-activity and making good use of D array-slicing facilities.

        XPath examples
        ---
        auto doc = new Document!(char);

        // attach an element with some attributes, plus 
        // a child element with an attached data value
        doc.tree.element   (null, "element")
                .attribute (null, "attrib1", "value")
                .attribute (null, "attrib2")
                .element   (null, "child", "value");

        // select named-elements
        auto set = doc.query["element"]["child"];

        // select all attributes named "attrib1"
        set = doc.query.descendant.attribute("attrib1");

        // select elements with one parent and a matching text value
        set = doc.query[].filter((doc.Node n) {return n.children.hasData("value");});
        ---

        Note that path queries are temporal - they do not retain content
        across mulitple queries. That is, the lifetime of a query result
        is limited unless you explicitly copy it. For example, this will 
        fail to operate as one might expect
        ---
        auto elements = doc.query["element"];
        auto children = elements["child"];
        ---

        The above will lose elements, because the associated document reuses 
        node space for subsequent queries. In order to retain results, do this
        ---
        auto elements = doc.query["element"].dup;
        auto children = elements["child"];
        ---

        The above .dup is generally very small (a set of pointers only). On
        the other hand, recursive queries are fully supported
        ---
        set = doc.query[].filter((doc.Node n) {return n.query[].count > 1;});
        ---
  
        Typical usage tends to exhibit the following pattern, Where each query 
        result is processed before another is initiated
        ---
        foreach (node; doc.query.child("element"))
                {
                // do something with each node
                }
        ---

        Supported axis include:
        ---
        .child                  immediate children
        .parent                 immediate parent 
        .next                   following siblings
        .prev                   prior siblings
        .ancestor               all parents
        .descendant             all descendants
        .data                   text children
        .cdata                  cdata children
        .attribute              attribute children
        ---

        Each of the above accept an optional string, which is used in an
        axis-specific way to filter nodes. For instance, a .child("food") 
        will filter <food> child elements. These variants are shortcuts
        to using a filter to post-process a result. Each of the above also
        have variants which accept a delegate instead.

        In general, you traverse an axis and operate upon the results. The
        operation applied may be another axis traversal, or a filtering 
        step. All steps can be, and generally should be chained together. 
        Filters are implemented via a delegate mechanism
        ---
        .filter (bool delegate(Node))
        ---

        Where the delegate returns true if the node passes the filter. An
        example might be selecting all nodes with a specific attribute
        ---
        auto set = doc.query.descendant.filter (
                    (doc.Node n){return n.attributes.hasName (null, "test");}
                   );
        ---

        Obviously this is not as clean and tidy as true XPath notation, but 
        that can be wrapped atop this API instead. The benefit here is one 
        of raw throughput - important for some applications. 

        Note that every operation returns a discrete result. Methods first()
        and last() also return a set of one or zero elements. Some language
        specific extensions are provided for too
        ---
        * .child() can be substituted with [] notation instead

        * [] notation can be used to index a specific element, like .nth()

        * the .nodes attribute exposes an underlying Node[], which may be
          sliced or traversed in the usual D manner
        ---

       Other (query result) utility methods include
       ---
       .dup
       .first
       .last
       .opIndex
       .nth
       .count
       .opApply
       ---

       XmlPath itself needs to be a class in order to avoid forward-ref issues.

*******************************************************************************/

private class XmlPath(T)
{       
        public alias Document!(T) Doc;          /// the typed document
        public alias Doc.Node     Node;         /// generic document node
         
        private Node[]          freelist;
        private uint            freeIndex,
                                markIndex;
        private uint            recursion;

        /***********************************************************************
        
                Prime a query

                Returns a NodeSet containing just the given node, which
                can then be used to cascade results into subsequent NodeSet
                instances.

        ***********************************************************************/
        
        final NodeSet start (Node root)
        {
                // we have to support recursion which may occur within
                // a filter callback
                if (recursion is 0)
                   {
                   if (freelist.length is 0)
                       freelist.length = 256;
                   freeIndex = 0;
                   }

                NodeSet set = {this};
                auto mark = freeIndex;
                allocate(root);
                return set.assign (mark);
        }

        /***********************************************************************
        
                This is the meat of XPath support. All of the NodeSet
                operators exist here, in order to enable call-chaining.

                Note that some of the axis do double-duty as a filter 
                also. This is just a convenience factor, and doesn't 
                change the underlying mechanisms.

        ***********************************************************************/
        
        struct NodeSet
        {
                private XmlPath host;
                public  Node[]  nodes;  /// array of selected nodes
               
                /***************************************************************
        
                        Return a duplicate NodeSet

                ***************************************************************/
        
                NodeSet dup ()
                {
                        NodeSet copy = {host};
                        copy.nodes = nodes.dup;
                        return copy;
                }

                /***************************************************************
        
                        Return the number of selected nodes in the set

                ***************************************************************/
        
                uint count ()
                {
                        return nodes.length;
                }

                /***************************************************************
        
                        Return a set containing just the first node of
                        the current set

                ***************************************************************/
        
                NodeSet first ()
                {
                        return nth (0);
                }

                /***************************************************************
       
                        Return a set containing just the last node of
                        the current set

                ***************************************************************/
        
                NodeSet last ()
                {       
                        auto i = nodes.length;
                        if (i > 0)
                            --i;
                        return nth (i);
                }

                /***************************************************************
        
                        Return a set containing just the nth node of
                        the current set

                ***************************************************************/
        
                NodeSet opIndex (uint i)
                {
                        return nth (i);
                }

                /***************************************************************
        
                        Return a set containing just the nth node of
                        the current set
        
                ***************************************************************/
        
                NodeSet nth (uint index)
                {
                        NodeSet set = {host};
                        auto mark = host.mark;
                        if (index < nodes.length)
                            host.allocate (nodes [index]);
                        return set.assign (mark);
                }

                /***************************************************************
        
                        Return a set containing all child elements of the 
                        nodes within this set
        
                ***************************************************************/
        
                NodeSet opSlice ()
                {
                        return child();
                }

                /***************************************************************
        
                        Return a set containing all child elements of the 
                        nodes within this set, which match the given name

                ***************************************************************/
        
                NodeSet opIndex (T[] name)
                {
                        return child (name);
                }

                /***************************************************************
        
                        Return a set containing all parent elements of the 
                        nodes within this set, which match the optional name

                ***************************************************************/
        
                NodeSet parent (T[] name = null)
                {
                        if (name.ptr)
                            return parent ((Node node){return node.name == name;});
                        return parent (&always);
                }

                /***************************************************************
        
                        Return a set containing all data nodes of the 
                        nodes within this set, which match the optional
                        value

                ***************************************************************/
        
                NodeSet data (T[] value = null)
                {
                        if (value.ptr)
                            return child ((Node node){return node.value == value;}, 
                                           XmlNodeType.Data);
                        return child (&always, XmlNodeType.Data);
                }

                /***************************************************************
        
                        Return a set containing all cdata nodes of the 
                        nodes within this set, which match the optional
                        value

                ***************************************************************/
        
                NodeSet cdata (T[] value = null)
                {
                        if (value.ptr)
                            return child ((Node node){return node.value == value;}, 
                                           XmlNodeType.CData);
                        return child (&always, XmlNodeType.CData);
                }

                /***************************************************************
        
                        Return a set containing all attributes of the 
                        nodes within this set, which match the optional
                        name

                ***************************************************************/
        
                NodeSet attribute (T[] name = null)
                {
                        if (name.ptr)
                            return attribute ((Node node){return node.name == name;});
                        return attribute (&always);
                }

                /***************************************************************
        
                        Return a set containing all descendant elements of 
                        the nodes within this set, which match the given name

                ***************************************************************/
        
                NodeSet descendant (T[] name = null)
                {
                        if (name.ptr)
                            return descendant ((Node node){return node.name == name;});
                        return descendant (&always);
                }

                /***************************************************************
        
                        Return a set containing all child elements of the 
                        nodes within this set, which match the optional name

                ***************************************************************/
        
                NodeSet child (T[] name = null)
                {
                        if (name.ptr)
                            return child ((Node node){return node.name == name;});
                        return  child (&always);
                }

                /***************************************************************
        
                        Return a set containing all ancestor elements of 
                        the nodes within this set, which match the optional
                        name

                ***************************************************************/
        
                NodeSet ancestor (T[] name = null)
                {
                        if (name.ptr)
                            return ancestor ((Node node){return node.name == name;});
                        return ancestor (&always);
                }

                /***************************************************************
        
                        Return a set containing all prior sibling elements of 
                        the nodes within this set, which match the optional
                        name

                ***************************************************************/
        
                NodeSet prev (T[] name = null)
                {
                        if (name.ptr)
                            return prev ((Node node){return node.name == name;});
                        return prev (&always);
                }

                /***************************************************************
        
                        Return a set containing all subsequent sibling 
                        elements of the nodes within this set, which 
                        match the optional name

                ***************************************************************/
        
                NodeSet next (T[] name = null)
                {
                        if (name.ptr)
                            return next ((Node node){return node.name == name;});
                        return next (&always);
                }

                /***************************************************************
        
                        Return a set containing all nodes within this set
                        which pass the filtering test

                ***************************************************************/
        
                NodeSet filter (bool delegate(Node) filter)
                {
                        NodeSet set = {host};
                        auto mark = host.mark;

                        foreach (node; nodes)
                                 test (filter, node);
                        return set.assign (mark);
                }

                /***************************************************************
        
                        Return a set containing all child nodes of 
                        the nodes within this set which pass the 
                        filtering test

                ***************************************************************/
        
                NodeSet child (bool delegate(Node) filter, 
                               XmlNodeType type = XmlNodeType.Element)
                {
                        NodeSet set = {host};
                        auto mark = host.mark;

                        foreach (parent; nodes)
                                 foreach (child; parent.children)
                                          if (child.id is type)
                                              test (filter, child);
                        return set.assign (mark);
                }

                /***************************************************************
        
                        Return a set containing all attribute nodes of 
                        the nodes within this set which pass the given
                        filtering test

                ***************************************************************/
        
                NodeSet attribute (bool delegate(Node) filter)
                {
                        NodeSet set = {host};
                        auto mark = host.mark;

                        foreach (node; nodes)
                                 foreach (attr; node.attributes)
                                          test (filter, attr);
                        return set.assign (mark);
                }

                /***************************************************************
        
                        Return a set containing all descendant nodes of 
                        the nodes within this set, which pass the given
                        filtering test

                ***************************************************************/
        
                NodeSet descendant (bool delegate(Node) filter, 
                                    XmlNodeType type = XmlNodeType.Element)
                {
                        void traverse (Node parent)
                        {
                                 foreach (child; parent.children)
                                         {
                                         if (child.id is type)
                                             test (filter, child);
                                         if (child.firstChild)
                                             traverse (child);
                                         }                                                
                        }

                        NodeSet set = {host};
                        auto mark = host.mark;

                        foreach (node; nodes)
                                 traverse (node);
                        return set.assign (mark);
                }

                /***************************************************************
        
                        Return a set containing all parent nodes of 
                        the nodes within this set which pass the given
                        filtering test

                ***************************************************************/
        
                NodeSet parent (bool delegate(Node) filter)
                {
                        NodeSet set = {host};
                        auto mark = host.mark;

                        foreach (node; nodes)
                                {
                                auto p = node.parent;
                                if (p && p.id != XmlNodeType.Document && !set.has(p))
                                   {
                                   test (filter, p);
                                   // continually update our set of nodes, so
                                   // that set.has() can see a prior entry.
                                   // Ideally we'd avoid invoking test() on
                                   // prior nodes, but I don't feel the added
                                   // complexity is warranted
                                   set.nodes = host.slice (mark);
                                   }
                                }
                        return set.assign (mark);
                }

                /***************************************************************
        
                        Return a set containing all ancestor nodes of 
                        the nodes within this set, which pass the given
                        filtering test

                ***************************************************************/
        
                NodeSet ancestor (bool delegate(Node) filter)
                {
                        NodeSet set = {host};
                        auto mark = host.mark;

                        void traverse (Node child)
                        {
                                auto p = child.host;
                                if (p && p.id != XmlNodeType.Document && !set.has(p))
                                   {
                                   test (filter, p);
                                   // continually update our set of nodes, so
                                   // that set.has() can see a prior entry.
                                   // Ideally we'd avoid invoking test() on
                                   // prior nodes, but I don't feel the added
                                   // complexity is warranted
                                   set.nodes = host.slice (mark);
                                   traverse (p);
                                   }
                        }

                        foreach (node; nodes)
                                 traverse (node);
                        return set.assign (mark);
                }

                /***************************************************************
        
                        Return a set containing all following siblings 
                        of the ones within this set, which pass the given
                        filtering test

                ***************************************************************/
        
                NodeSet next (bool delegate(Node) filter, 
                              XmlNodeType type = XmlNodeType.Element)
                {
                        NodeSet set = {host};
                        auto mark = host.mark;

                        foreach (node; nodes)
                                {
                                auto p = node.nextSibling;
                                while (p)
                                      {
                                      if (p.id is type)
                                          test (filter, p);
                                      p = p.nextSibling;
                                      }
                                }
                        return set.assign (mark);
                }

                /***************************************************************
        
                        Return a set containing all prior sibling nodes 
                        of the ones within this set, which pass the given
                        filtering test

                ***************************************************************/
        
                NodeSet prev (bool delegate(Node) filter, 
                              XmlNodeType type = XmlNodeType.Element)
                {
                        NodeSet set = {host};
                        auto mark = host.mark;

                        foreach (node; nodes)
                                {
                                auto p = node.prevSibling;
                                while (p)
                                      {
                                      if (p.id is type)
                                          test (filter, p);
                                      p = p.prevSibling;
                                      }
                                }
                        return set.assign (mark);
                }

                /***************************************************************
                
                        Traverse the nodes of this set

                ***************************************************************/
        
                int opApply (int delegate(ref Node) dg)
                {
                        int ret;

                        foreach (node; nodes)
                                 if ((ret = dg (node)) != 0) 
                                      break;
                        return ret;
                }

                /***************************************************************
        
                        Common predicate
                                
                ***************************************************************/
        
                private bool always (Node node)
                {
                        return true;
                }

                /***************************************************************
        
                        Assign a slice of the freelist to this NodeSet

                ***************************************************************/
        
                private NodeSet assign (uint mark)
                {
                        nodes = host.slice (mark);
                        return *this;
                }

                /***************************************************************
        
                        Execute a filter on the given node. We have to
                        deal with potential query recusion, so we set
                        all kinda crap to recover from that

                ***************************************************************/
        
                private void test (bool delegate(Node) filter, Node node)
                {
                        auto pop = host.push;
                        auto add = filter (node);
                        host.pop (pop);
                        if (add)
                            host.allocate (node);
                }

                /***************************************************************
        
                        We typically need to filter ancestors in order
                        to avoid duplicates, so this is used for those
                        purposes                        

                ***************************************************************/
        
                private bool has (Node p)
                {
                        foreach (node; nodes)
                                 if (node is p)
                                     return true;
                        return false;
                }
        }

        /***********************************************************************

                Return the current freelist index
                        
        ***********************************************************************/
        
        private uint mark ()
        {       
                return freeIndex;
        }

        /***********************************************************************

                Recurse and save the current state
                        
        ***********************************************************************/
        
        private uint push ()
        {       
                ++recursion;
                return freeIndex;
        }

        /***********************************************************************

                Restore prior state
                        
        ***********************************************************************/
        
        private void pop (uint prior)
        {       
                freeIndex = prior;
                --recursion;
        }

        /***********************************************************************
        
                Return a slice of the freelist

        ***********************************************************************/
        
        private Node[] slice (uint mark)
        {
                assert (mark <= freeIndex);
                return freelist [mark .. freeIndex];
        }

        /***********************************************************************
        
                Allocate an entry in the freelist, expanding as necessary

        ***********************************************************************/
        
        private uint allocate (Node node)
        {
                if (freeIndex >= freelist.length)
                    freelist.length = freelist.length + freelist.length / 2;

                freelist[freeIndex] = node;
                return ++freeIndex;
        }
}



/*******************************************************************************

        Specification for an XML serializer

*******************************************************************************/

interface IXmlPrinter(T)
{
        public alias Document!(T) Doc;          /// the typed document
        public alias Doc.Node Node;             /// generic document node
        public alias print opCall;              /// alias for print method

        /***********************************************************************
        
                Generate a text representation of the document tree

        ***********************************************************************/
        
        T[] print (Doc doc);
        
        /***********************************************************************
        
                Generate a representation of the given node-subtree 

        ***********************************************************************/
        
        void print (Node root, void delegate(T[][]...) emit);
}



debug (Document)
{
        import tango.io.Stdout;
        import tango.text.xml.DocPrinter;

        void main()
        {
                auto doc = new Document!(char);

                // attach an xml header
                doc.header;

                // attach an element with some attributes, plus 
                // a child element with an attached data value
                doc.tree.element   (null, "root")
                        .attribute (null, "attrib1", "value")
                        .attribute (null, "attrib2", "other")
                        .element   (null, "child")
                        .cdata     ("some text");

                // attach a sibling to the interior elements
                doc.elements.element (null, "sibling");
        
                bool foo (doc.Node node)
                {
                        node = node.attributes.name(null, "attrib1");
                        return node && "value" == node.value;
                }

                foreach (node; doc.query.descendant("root").filter(&foo).child)
                         Stdout.formatln(">> {}", node.name);

                foreach (node; doc.elements.attributes)
                         Stdout.formatln("<< {}", node.name);
                         
                foreach (node; doc.elements.children)
                         Stdout.formatln("<< {}", node.name);
                         
                foreach (node; doc.query.descendant.cdata)
                         Stdout.formatln ("{}: {}", node.parent.name, node.value);

                // emit the result
                auto print = new DocPrinter!(char);
                Stdout(print(doc)).newline;
        }
}
