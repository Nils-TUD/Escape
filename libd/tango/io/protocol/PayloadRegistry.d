/*******************************************************************************

        copyright:      Copyright (c) 2004 Kris Bell. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Apr 2004: Initial release
                        Dec 2006: Outback version

        author:         Kris

*******************************************************************************/

module tango.io.protocol.PayloadRegistry;

private import  tango.core.Exception;

public  import  tango.io.protocol.model.IPayload;

/*******************************************************************************

        Bare framework for registering and creating serializable objects.
        Such objects are intended to be transported across a local network
        and re-instantiated at some destination node.

        Each IPayload exposes the means to write, or freeze, its content. An
        IPickleFactory provides the means to create a new instance of itself
        populated with thawed data. Frozen objects are uniquely identified
        by a guid exposed via the interface. Responsibility of maintaining
        uniqueness across said identifiers lies in the hands of the developer.

*******************************************************************************/

class PayloadRegistry
{
        private alias IPayload delegate() Factory;

        private static Factory[char[]] registry;


        /***********************************************************************

                This is a singleton: the constructor should not be exposed

        ***********************************************************************/

        private this () {}

        /***********************************************************************

                Add the provided Factory to the registry. Note that one
                cannot change a registration once it is placed. Neither
                can one remove registered item. This is done to avoid
                issues when trying to synchronize servers across
                a farm, which may still have live instances of "old"
                objects waiting to be passed around the cluster. New
                versions of an object should be given a distinct guid
                from the prior version; appending an incremental number
                may well be sufficient for your needs.

        ***********************************************************************/

        static synchronized void enroll (IPayload p)
        {
                if (p.guid in registry)
                    throw new PayloadException ("PayloadRegistry.enroll :: attempt to re-register a guid");

                registry[p.guid] = &p.create;
        }

        /***********************************************************************

                Synchronized Factory lookup of the guid

        ***********************************************************************/

        static synchronized Factory lookup (char[] guid)
        {
                auto factory = guid in registry;
                return (factory ? *factory : null);
        }

        /***********************************************************************

                Serialize an Object. Objects are written in Network-order,
                and are prefixed by the guid exposed via the IPayload
                interface. This guid is used to identify the appropriate
                factory when reconstructing the instance.

        ***********************************************************************/

        static void freeze (IWriter output, IPayload object)
        {
                output (object.guid);
                object.write (output);
        }

        /***********************************************************************

                Create a new instance of a registered class from the content
                made available via the given reader. The factory is located
                using the provided guid, which must match an enrolled factory.

                Note that only the factory lookup is synchronized, and not
                the instance construction itself. This is intentional, and
                limits how long the calling thread is stalled

        ***********************************************************************/

        static IPayload thaw (IReader input)
        {
                char[] guid;

                // locate appropriate factory and invoke it
                input (guid);
                auto factory = lookup (guid);

                if (factory is null)
                    throw new PayloadException ("PayloadRegistry.thaw :: attempt to unpickle via unregistered guid '"~guid~"'");

                auto o = factory ();
                o.read (input);
                return o;
        }
}




debug (UnitTest1)
{
        class Foo : IPayload
        {
                char[] guid ()
                {
                        return this.classinfo.name;
                }

                void write (IWriter output)
                {
                }

                void read (IReader reader)
                {
                }

                IPayload create ()
                {
                        return new Foo;
                }

                ulong time ()
                {
                    return 0;
                }

                void time (ulong time)
                {
                }

                void destroy ()
                {
                }
        }

        unittest
        {
                auto foo = new Foo;
                PayloadRegistry.enroll (foo);
                PayloadRegistry.freeze (null, foo);
                auto x = PayloadRegistry.thaw (null);
        }
}
