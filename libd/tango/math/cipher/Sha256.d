/*******************************************************************************

        copyright:      Copyright (c) 2004 Regan Heath. All rights reserved

        license:        BSD style: see doc/license.txt for details

        version:        Initial release: Feb 2006

        author:         Regan Heath, Kris

        This module implements the SHA-256 Algorithm described by Secure Hash
        Standard, FIPS PUB 180-2

*******************************************************************************/

module tango.math.cipher.Sha256;

public import tango.math.cipher.Cipher;

private import tango.core.ByteSwap;

/*******************************************************************************

*******************************************************************************/

class Sha256Digest : Digest
{
        private uint[8] digest;

        /***********************************************************************

                Construct an Sha256Digest.

                Remarks:
                Constructs a blank Sha256Digest.

        ***********************************************************************/

        this() { digest[] = 0; }

        /***********************************************************************

                Construct an Sha256Digest.

                Remarks:
                Constructs an Sha256Digest from binary data

        ***********************************************************************/

        this(void[] raw) { digest[] = cast(uint[]) raw; }

        /***********************************************************************

                Construct an Sha256Digest.

                Remarks:
                Constructs an Sha256Digest from another Sha256Digest.

        ***********************************************************************/

        this(Sha256Digest rhs) { digest[] = rhs.digest[]; }

        /***********************************************************************

                Return the string representation

                Returns:
                the digest in string form

                Remarks:
                Formats the digest into hex encoded string form.

        ***********************************************************************/

        char[] toUtf8() { return toHexString (cast(ubyte[]) digest); }

        /***********************************************************************

                Return the binary representation

                Returns:
                the digest in binary form

                Remarks:
                Returns a void[] containing the binary representation of the digest.

        ***********************************************************************/

        void[] toBinary() { return cast(void[]) digest; }

        int opEquals(Object o) { Sha256Digest rhs = cast(Sha256Digest)o; assert(rhs); return digest == rhs.digest; }
}


/*******************************************************************************

*******************************************************************************/

class Sha256Cipher : Cipher
{
        private uint[8]         context;
        private const uint      padChar = 0x80;

        /***********************************************************************

                Construct an Sha256Cipher

        ***********************************************************************/

        this() { }

        /***********************************************************************

                Construct an Sha1Cipher

                Params:
                rhs = an existing Sha1Digest

                Remarks:
                Construct an Sha1Cipher from a previous Sha1Digest.

        ***********************************************************************/

        this(Sha256Digest rhs)
        {
                context[] = cast(uint[]) rhs.toBinary();
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

        override Sha256Digest getDigest()
        {
                uint[8] digest = context[];
                version (LittleEndian)
                        ByteSwap.swap32 (digest.ptr, digest.length * uint.sizeof);
                return new Sha256Digest(digest);
        }

        /***********************************************************************

                Cipher block size

                Returns:
                the block size

                Remarks:
                Specifies the size (in bytes) of the block of data to pass to
                each call to transform(). For SHA256 the blockSize is 64.

        ***********************************************************************/

        protected override uint blockSize() { return 64; }

        /***********************************************************************

                Length padding size

                Returns:
                the length padding size

                Remarks:
                Specifies the size (in bytes) of the padding which uses the
                length of the data which has been ciphered, this padding is
                carried out by the padLength method. For SHA256 the addSize is 8.

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
                        data[$-j-1] = cast(ubyte) (length >> j*8);
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
                uint[64] W;
                uint a,b,c,d,e,f,g,h;
                uint j,t1,t2;

                a = context[0];
                b = context[1];
                c = context[2];
                d = context[3];
                e = context[4];
                f = context[5];
                g = context[6];
                h = context[7];

                bigEndian32(input,W[0..16]);
                for(j = 16; j < 64; j++) {
                        W[j] = mix1(W[j-2]) + W[j-7] + mix0(W[j-15]) + W[j-16];
                }

                for(j = 0; j < 64; j++) {
                        t1 = h + sum1(e) + Ch(e,f,g) + K[j] + W[j];
                        t2 = sum0(a) + Maj(a,b,c);
                        h = g;
                        g = f;
                        f = e;
                        e = d + t1;
                        d = c;
                        c = b;
                        b = a;
                        a = t1 + t2;
                }

                context[0] += a;
                context[1] += b;
                context[2] += c;
                context[3] += d;
                context[4] += e;
                context[5] += f;
                context[6] += g;
                context[7] += h;
        }

        /***********************************************************************

        ***********************************************************************/

        private static uint Ch(uint x, uint y, uint z)
        {
                return (x&y)^(~x&z);
        }

        /***********************************************************************

        ***********************************************************************/

        private static uint Maj(uint x, uint y, uint z)
        {
                return (x&y)^(x&z)^(y&z);
        }

        /***********************************************************************

        ***********************************************************************/

        private static uint sum0(uint x)
        {
                return rotateRight(x,2)^rotateRight(x,13)^rotateRight(x,22);
        }

        /***********************************************************************

        ***********************************************************************/

        private static uint sum1(uint x)
        {
                return rotateRight(x,6)^rotateRight(x,11)^rotateRight(x,25);
        }

        /***********************************************************************

        ***********************************************************************/

        private static uint mix0(uint x)
        {
                return rotateRight(x,7)^rotateRight(x,18)^shiftRight(x,3);
        }

        /***********************************************************************

        ***********************************************************************/

        private static uint mix1(uint x)
        {
                return rotateRight(x,17)^rotateRight(x,19)^shiftRight(x,10);
        }

        /***********************************************************************

        ***********************************************************************/

        private static uint rotateRight(uint x, uint n)
        {
                return (x >> n) | (x << (32-n));
        }

        /***********************************************************************

        ***********************************************************************/

        private static uint shiftRight(uint x, uint n)
        {
                return x >> n;
        }
}


/*******************************************************************************

*******************************************************************************/

private static uint[] K = [
                0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        ];

/*******************************************************************************

*******************************************************************************/

private static const uint[8] initial = [
                0x6a09e667,
                0xbb67ae85,
                0x3c6ef372,
                0xa54ff53a,
                0x510e527f,
                0x9b05688c,
                0x1f83d9ab,
                0x5be0cd19
        ];


/*******************************************************************************

*******************************************************************************/

version (UnitTest)
{
unittest {
        static char[][] strings = [
                "abc",
                "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
        ];
        static char[][] results = [
                "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD",
                "248D6A61D20638B8E5C026930C3E6039A33CE45964FF2167F6ECEDD419DB06C1"
        ];

        Sha256Cipher h = new Sha256Cipher();
        Sha256Digest d,e;

        foreach(int i, char[] s; strings) {
                d = cast(Sha256Digest)h.sum(s);
                assert(d.toUtf8() == results[i],"Cipher:("~s~")("~d.toUtf8()~")!=("~results[i]~")");

                e = new Sha256Digest(d);
                assert(d == e,"Digest from Digest:("~d.toUtf8()~")!=("~e.toUtf8()~")");

                e = new Sha256Digest(d.toBinary());
                assert(d == e,"Digest from Digest binary:("~d.toUtf8()~")!=("~e.toUtf8()~")");

                h = new Sha256Cipher(d);
                e = h.getDigest();
                assert(d == e,"Digest from Cipher continue:("~d.toUtf8()~")!=("~e.toUtf8()~")");
        }
}
}
