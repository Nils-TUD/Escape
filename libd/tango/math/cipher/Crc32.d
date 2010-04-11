/*******************************************************************************

        copyright:      Copyright (c) 2006 James Pelcis. All rights reserved

        license:        BSD style: $(LICENSE)

        version:        Initial release: August 2006

        author:         James Pelcis

*******************************************************************************/

module tango.math.cipher.Crc32;

public import tango.math.cipher.Cipher;

/** */
class Crc32Digest : Digest
{
        private uint digest;

        /**
         * Create a new digest.
         *
         * Params:
         *      digest = An existing Crc32Digest.
         */
        this (Crc32Digest digest)
        {
                this.digest = digest.digest;
        }

        /**
         * Create a new digest.
         *
         * Params:
         *      digest = A uint representing the digest.
         */
        this (uint digest)
        {
                this.digest = digest;
        }

        /**
         * Create a new digest.
         *
         * Params:
         *      digest = A ubyte[] representing the digest.  digest.length must be
         *      4.
         */
        this (ubyte[] digest)
        {
                assert (digest.length == 4);
                this.digest = digest[0] << 24 + digest[1] << 16 + digest[2] << 8 + digest[3];
        }

        /**
         * Return the string representation of the digest.
         *
         * The string is hex encoded.
         *
         * Returns:
         *      The digest in string form.
         */
        override char[] toUtf8 ()
        {
                return toHexString(cast(ubyte[])toBinary());
        }

        /**
         * Return the binary representation of the digest as void[].
         *
         * Returns:
         *      The digest in binary form.
         */
        override void[] toBinary ()
        {
                ubyte[] result;
                result.length = 4;
                result[0] = cast(ubyte)((digest & 0xFF000000) >> 24);
                result[1] = cast(ubyte)((digest & 0x00FF0000) >> 16);
                result[2] = cast(ubyte)((digest & 0x0000FF00) >> 8);
                result[3] = cast(ubyte)(digest & 0x000000FF);
                return result;
        }

        /**
         * Return the binary representation of the digest as a uint.
         *
         * Returns:
         *      The digest in binary form.
         */
        uint opCast ()
        {
                return digest;
        }

        /**
         * Test for equality with another Crc32Digest.
         *
         * Params:
         *      o = The object to test against.
         *
         * Returns:
         *      true if they are equal and false otherwise.
         */
        int opEquals (Object o)
        {
                Crc32Digest rhs = cast(Crc32Digest)o;
                assert (rhs !is null);
                return digest == rhs.digest;
        }
}

/** */
class Crc32Cipher : Cipher
{
        private uint[256] CrcTable;
        private uint CrcPolynomial;
        private uint CrcResult = uint.max;

        /**
         * Create a new Crc32Cipher based on an existing Crc32Cipher.
         *
         * Params:
         *      cipher = An existing Crc32Cipher.
         */
        this (Crc32Cipher cipher)
        {
                this.CrcTable[0 .. 255] = cipher.CrcTable[0 .. 255];
                this.CrcPolynomial = cipher.CrcPolynomial;
                this.CrcResult = cipher.CrcResult;
        }

        /**
         * Prepare Crc32Cipher to checksum the data with a given polynomial.
         *
         * Params:
         *      CrcPolynomial = The magic CRC number to base calculations on.  The
         *      default is the one compatible with ZIP files.
         */
        this (uint CrcPolynomial = 0xEDB88320U)
        {
                this.CrcPolynomial = CrcPolynomial;
                for (size_t i = 0; i < 256; i++)
                {
                        uint CrcValue = i;
                        for (size_t j = 8; j > 0; j--)
                        {
                                if (CrcValue & 1) {
                                        CrcValue &= 0xFFFFFFFE;
                                        CrcValue /= 2;
                                        CrcValue &= 0x7FFFFFFF;
                                        CrcValue ^= CrcPolynomial;
                                }
                                else
                                {
                                        CrcValue &= 0xFFFFFFFE;
                                        CrcValue /= 2;
                                        CrcValue &= 0x7FFFFFFF;
                                }
                        }
                        CrcTable[i] = CrcValue;
                }
        }

        /**
         * Get the digest in its current state.
         *
         * Returns:
         *      The current digest.
         */
        override Crc32Digest getDigest ()
        {
                return new Crc32Digest(~CrcResult);
        }

        /**
         * Return the cipher state to its initial state
         */
        override void start ()
        {
                CrcResult = CrcResult.init;
        }

        /**
         * Calculate the CRC-32 checksum of data combined with the results of all
         * previous calls to calculateChecksum.
         *
         * Params:
         *      data = The data to calculate the checksum of.
         */
        override void update (void[] input)
        {
                foreach (ubyte value; cast(ubyte[])input)
                {
                        size_t iLookup = CrcResult & 0xFF;
                        iLookup ^= value;
                        CrcResult &= 0xFFFFFF00;
                        CrcResult /= 0x100;
                        CrcResult &= 16777215;
                        CrcResult ^= CrcTable[iLookup];
                }
        }

        /**
         * Complete the cipher.
         *
         * Returns:
         *      The completed digest.
         */
        override Crc32Digest finish ()
        {
                return getDigest();
        }

        /**
         * Get the magic CRC number that calculations are based on.
         *
         * Returns:
         *      The magic CRC number that calculations are based on.
         */
        uint returnPolynomial ()
        {
                return CrcPolynomial;
        }

        protected override uint blockSize ()
        {
                return 0;
        }

        protected override uint addSize ()
        {
                return 0;
        }

        protected override void padMessage (ubyte[] data)
        {
        }

        protected override void padLength (ubyte[] data, ulong length)
        {
        }

        protected override void transform (ubyte[] data)
        {
        }

        protected override void extend ()
        {
        }
}