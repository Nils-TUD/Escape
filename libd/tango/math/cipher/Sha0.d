/*******************************************************************************

        copyright:      Copyright (c) 2004 Regan Heath. All rights reserved

        license:        BSD style: see doc/license.txt for details

        version:        Initial release: Feb 2006

        author:         Regan Heath, Kris

        This module implements the SHA-0 Algorithm described by Secure Hash
        Standard, FIPS PUB 180

*******************************************************************************/

module tango.math.cipher.Sha0;

public import tango.math.cipher.Cipher;

private import tango.core.ByteSwap;

/*******************************************************************************

*******************************************************************************/

class Sha0Digest : Digest
{
        private ubyte[20] digest;

        /***********************************************************************

                Construct an Sha0Digest.

                Remarks:
                Constructs a blank Sha0Digest.

        ***********************************************************************/

        this() { digest[] = 0; }

        /***********************************************************************

                Construct an Sha0Digest.

                Remarks:
                Constructs an Sha0Digest from binary data

        ***********************************************************************/

        this(void[] raw) { digest[] = cast(ubyte[])raw; }

        /***********************************************************************

                Construct an Sha0Digest.

                Remarks:
                Constructs an Sha0Digest from another Sha0Digest.

        ***********************************************************************/

        this(Sha0Digest rhs) { digest[] = rhs.digest[]; }

        /***********************************************************************

                Return the string representation

                Returns:
                the digest in string form

                Remarks:
                Formats the digest into hex encoded string form.

        ***********************************************************************/

        char[] toUtf8() { return toHexString(digest); }

        /***********************************************************************

                Return the binary representation

                Returns:
                the digest in binary form

                Remarks:
                Returns a void[] containing the binary representation of the digest.

        ***********************************************************************/

        void[] toBinary() { return cast(void[]) digest; }

        int opEquals(Object o) { Sha0Digest rhs = cast(Sha0Digest)o; assert(rhs); return digest == rhs.digest; }
}


/*******************************************************************************

*******************************************************************************/

class Sha0Cipher : Cipher
{
        protected uint[5]       context;

        private const ubyte     padChar = 0x80;
        private const uint      mask = 0x0000000F;

        /***********************************************************************

        ***********************************************************************/

        private static const uint[] K =
        [
                0x5A827999,
                0x6ED9EBA1,
                0x8F1BBCDC,
                0xCA62C1D6
        ];

        /***********************************************************************

        ***********************************************************************/

        private static const uint[5] initial =
        [
                0x67452301,
                0xEFCDAB89,
                0x98BADCFE,
                0x10325476,
                0xC3D2E1F0
        ];

        /***********************************************************************

                Construct an Sha0Cipher

        ***********************************************************************/

        this() { }

        /***********************************************************************

                Construct an Sha0Cipher

                Params:
                rhs = an existing Sha0Digest

                Remarks:
                Construct an Sha0Cipher from a previous Sha0Digest.

        ***********************************************************************/

        this(Sha0Digest rhs)
        {
                context[] = cast(uint[])rhs.toBinary();
                version (LittleEndian)
                        ByteSwap.swap32 (context.ptr, context.length * uint.sizeof);
        }

        /***********************************************************************

                Initialize the cipher

                Remarks:
                Returns the cipher state to it's initial value

        ***********************************************************************/

        override void start()
        {
                super.start();
                context[] = initial[];
        }

        /***********************************************************************

                Obtain the digest

                Returns:
                the digest

                Remarks:
                Returns a digest of the current cipher state, this may be the
                final digest, or a digest of the state between calls to update()

        ***********************************************************************/

        override Sha0Digest getDigest()
        {
                uint[5] digest = context[];
                version (LittleEndian)
                         ByteSwap.swap32 (digest.ptr, digest.length * uint.sizeof);
                return new Sha0Digest(digest);
        }

        /***********************************************************************

                Cipher block size

                Returns:
                the block size

                Remarks:
                Specifies the size (in bytes) of the block of data to pass to
                each call to transform(). For SHA0 the blockSize is 64.

        ***********************************************************************/

        protected override uint blockSize() { return 64; }

        /***********************************************************************

                Length padding size

                Returns:
                the length padding size

                Remarks:
                Specifies the size (in bytes) of the padding which uses the
                length of the data which has been ciphered, this padding is
                carried out by the padLength method. For SHA0 the addSize is 0.

        ***********************************************************************/

        protected override uint addSize()   { return 8;  }

        /***********************************************************************

                Pads the cipher data

                Params:
                data = a slice of the cipher buffer to fill with padding

                Remarks:
                Fills the passed buffer slice with the appropriate padding for
                the final call to transform(). This padding will fill the cipher
                buffer up to blockSize()-addSize().

        ***********************************************************************/

        protected override void padMessage(ubyte[] data)
        {
                data[0] = padChar;
                data[1..$] = 0;
        }

        /***********************************************************************

                Performs the length padding

                Params:
                data   = the slice of the cipher buffer to fill with padding
                length = the length of the data which has been ciphered

                Remarks:
                Fills the passed buffer slice with addSize() bytes of padding
                based on the length in bytes of the input data which has been
                ciphered.

        ***********************************************************************/

        protected override void padLength(ubyte[] data, ulong length)
        {
                length <<= 3;
                for(int j = data.length-1; j >= 0; j--)
                        data[$-j-1] = cast(ubyte) (length >> j*data.length);
        }

        /***********************************************************************

                Performs the cipher on a block of data

                Params:
                data = the block of data to cipher

                Remarks:
                The actual cipher algorithm is carried out by this method on
                the passed block of data. This method is called for every
                blockSize() bytes of input data and once more with the remaining
                data padded to blockSize().

        ***********************************************************************/

        protected override void transform(ubyte[] input)
        {
                uint A,B,C,D,E,TEMP;
                uint[16] W;
                uint s;

                bigEndian32(input,W);

                A = context[0];
                B = context[1];
                C = context[2];
                D = context[3];
                E = context[4];

                /* Method 1
                for(uint t = 16; t < 80; t++) {
                        W[t] = rotateLeft(W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16],1);
                }
                for(uint t = 0; t < 80; t++) {
                        TEMP = rotateLeft(A,5) + f(t,B,C,D) + E + W[t] + K[t/20];
                        E = D;
                        D = C;
                        C = rotateLeft(B,30);
                        B = A;
                        A = TEMP;
                }
                */

                /* Method 2 */
                for(uint t = 0; t < 80; t++) {
                        s = t & mask;
                        if (t >= 16) expand(W,s);
                        TEMP = rotateLeft(A,5) + f(t,B,C,D) + E + W[s] + K[t/20];
                        E = D; D = C; C = rotateLeft(B,30); B = A; A = TEMP;
                }

                context[0] += A;
                context[1] += B;
                context[2] += C;
                context[3] += D;
                context[4] += E;
        }

        /***********************************************************************

        ***********************************************************************/

        protected void expand(uint W[], uint s)
        {
                W[s] = W[(s+13)&mask] ^ W[(s+8)&mask] ^ W[(s+2)&mask] ^ W[s];
        }

        /***********************************************************************

        ***********************************************************************/

        private static uint f(uint t, uint B, uint C, uint D)
        {
                if (t < 20) return (B & C) | ((~B) & D);
                else if (t < 40) return B ^ C ^ D;
                else if (t < 60) return (B & C) | (B & D) | (C & D);
                else return B ^ C ^ D;
        }
}

/*******************************************************************************

*******************************************************************************/

version (UnitTest)
{
unittest {
        static char[][] strings = [
                "",
                "abc",
                "message digest",
                "abcdefghijklmnopqrstuvwxyz",
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
                "12345678901234567890123456789012345678901234567890123456789012345678901234567890"
        ];
        static char[][] results = [
                "F96CEA198AD1DD5617AC084A3D92C6107708C0EF",
                "0164B8A914CD2A5E74C4F7FF082C4D97F1EDF880",
                "C1B0F222D150EBB9AA36A40CAFDC8BCBED830B14",
                "B40CE07A430CFD3C033039B9FE9AFEC95DC1BDCD",
                "79E966F7A3A990DF33E40E3D7F8F18D2CAEBADFA",
                "4AA29D14D171522ECE47BEE8957E35A41F3E9CFF",
        ];

        Sha0Cipher h = new Sha0Cipher();
        Sha0Digest d,e;

        foreach(int i, char[] s; strings) {
                d = cast(Sha0Digest)h.sum(s);
                assert(d.toUtf8() == results[i],"Cipher:("~s~")("~d.toUtf8()~")!=("~results[i]~")");

                e = new Sha0Digest(d);
                assert(d == e,"Digest from Digest:("~d.toUtf8()~")!=("~e.toUtf8()~")");

                e = new Sha0Digest(d.toBinary());
                assert(d == e,"Digest from Digest binary:("~d.toUtf8()~")!=("~e.toUtf8()~")");

                h = new Sha0Cipher(d);
                e = h.getDigest();
                assert(d == e,"Digest from Cipher continue:("~d.toUtf8()~")!=("~e.toUtf8()~")");
        }
}
}
