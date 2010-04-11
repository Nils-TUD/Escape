/*******************************************************************************

        copyright:      Copyright (c) 2004 Regan Heath. All rights reserved

        license:        BSD style: see doc/license.txt for details

        version:        Initial release: Feb 2006

        author:         Regan Heath, Kris

        This module implements the SHA-1 Algorithm described by Secure Hash
        Standard, FIPS PUB 180-1, and RFC 3174 US Secure Hash Algorithm 1
        (SHA1). D. Eastlake 3rd, P. Jones. September 2001.

*******************************************************************************/

module tango.math.cipher.Sha1;

public import tango.math.cipher.Sha0;

private import tango.core.ByteSwap;

/*******************************************************************************

*******************************************************************************/

class Sha1Digest : Sha0Digest
{
        /***********************************************************************

                Construct an Sha1Digest.

                Remarks:
                Constructs a blank Sha1Digest.

        ***********************************************************************/

        this() { }

        /***********************************************************************

                Construct an Sha1Digest.

                Remarks:
                Constructs an Sha1Digest from binary data

        ***********************************************************************/

        this(void[] raw) { super(raw); }

        /***********************************************************************

                Construct an Sha1Digest.

                Remarks:
                Constructs an Sha1Digest from another Sha1Digest.

        ***********************************************************************/

        this(Sha1Digest rhs) { super(rhs); }
}


/*******************************************************************************

*******************************************************************************/

class Sha1Cipher : Sha0Cipher
{
        /***********************************************************************

                Construct an Sha1Cipher

        ***********************************************************************/

        this() { }

        /***********************************************************************

                Construct an Sha1Cipher

                Params:
                rhs = an existing Sha1Digest

                Remarks:
                Construct an Sha1Cipher from a previous Sha1Digest.

        ***********************************************************************/

        this(Sha1Digest rhs) {
                context[] = cast(uint[])rhs.toBinary();
                version (LittleEndian)
                        ByteSwap.swap32 (context.ptr, context.length * uint.sizeof);
        }

        /***********************************************************************

                Obtain the digest

                Returns:
                the digest

                Remarks:
                Returns a digest of the current cipher state, this may be the
                final digest, or a digest of the state between calls to update()

        ***********************************************************************/

        override Sha1Digest getDigest()
        {
                uint[5] digest = context[];
                version (LittleEndian)
                        ByteSwap.swap32 (digest.ptr, digest.length * uint.sizeof);
                return new Sha1Digest(digest);

        }

        /***********************************************************************

        ***********************************************************************/

        protected override void expand (uint W[], uint s)
        {
                W[s] = rotateLeft(W[(s+13)&mask] ^ W[(s+8)&mask] ^ W[(s+2)&mask] ^ W[s],1);
        }
}


/*******************************************************************************

*******************************************************************************/

version (UnitTest)
{
unittest {
        static char[][] strings = [
                "abc",
                "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                "a",
                "0123456701234567012345670123456701234567012345670123456701234567"
        ];
        static char[][] results = [
                "A9993E364706816ABA3E25717850C26C9CD0D89D",
                "84983E441C3BD26EBAAE4AA1F95129E5E54670F1",
                "34AA973CD4C4DAA4F61EEB2BDBAD27316534016F",
                "DEA356A2CDDD90C7A7ECEDC5EBB563934F460452"
        ];
        static int[] repeat = [
                1,
                1,
                1000000,
                10
        ];

        Sha1Cipher h = new Sha1Cipher();
        Sha1Digest d,e;

        foreach(int i, char[] s; strings) {
                h.start();
                for(int r = 0; r < repeat[i]; r++)
                        h.update(s);

                d = cast(Sha1Digest)h.finish();
                assert(d.toUtf8() == results[i],"Cipher:("~s~")("~d.toUtf8()~")!=("~results[i]~")");

                e = new Sha1Digest(d);
                assert(d == e,"Digest from Digest:("~d.toUtf8()~")!=("~e.toUtf8()~")");

                e = new Sha1Digest(d.toBinary());
                assert(d == e,"Digest from Digest binary:("~d.toUtf8()~")!=("~e.toUtf8()~")");

                h = new Sha1Cipher(d);
                e = h.getDigest();
                assert(d == e,"Digest from Cipher continue:("~d.toUtf8()~")!=("~e.toUtf8()~")");
        }
}
}
