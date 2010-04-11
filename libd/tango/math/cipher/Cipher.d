/*******************************************************************************

        copyright:      Copyright (c) 2004 Regan Heath. All rights reserved

        license:        BSD style: see doc/license.txt for details

        version:        Initial release: Feb 2006

        author:         Regan Heath, Kris

        This module defines two abstract base classes, the primary one being
        Cipher which can be extended to provide concrete implementations of
        various ciphers (AKA hashing functions or algorithms). These ciphers
        produce digests (AKA hashes) hence the second abstract base class
        Digest. In addition CipherException is defined for exception
        handling.

*******************************************************************************/

module tango.math.cipher.Cipher;

private import tango.core.ByteSwap;

/*******************************************************************************

       This is an exception class to be thrown by Cipher's where required.

*******************************************************************************/

class CipherException : Exception
{
        /***********************************************************************

                Construct a CipherException.

                Params:
                msg = the exception text

                Remarks:
                Constructs a CipherException.

        ***********************************************************************/

        this(char[] msg) { super(msg); }
}


/*******************************************************************************

       This is the abstract base class for creating custom Digests.

*******************************************************************************/

class Digest
{
        /***********************************************************************

                Return the string representation

                Returns:
                the digest in string form

                Remarks:
                Formats the digest into hex encoded string form.

        ***********************************************************************/

        abstract char[] toUtf8();

        /***********************************************************************

                Return the binary representation

                Returns:
                the digest in binary form

                Remarks:
                Returns a void[] containing the binary representation of the digest.

        ***********************************************************************/

        abstract void[] toBinary();

        /***********************************************************************

                Represent an array as a hex encoded string

                Params:
                d = the array to represent

                Returns:
                the string representation

                Remarks:
                Represents any sized array of any sized numerical items as a
                hex encoded string.

        ***********************************************************************/

        protected char[] toHexString (ubyte[] src)
        {
                char[] result;
                static char[] hexdigits = "0123456789ABCDEF";

                if (src.length)
                   {
                   uint index = -1;
                   result = new char [src.length * 2];
                   foreach (b; src)
                           {
                           result [++index] = hexdigits [b >> 4];
                           result [++index] = hexdigits [b & 0x0f];
                           }
                   }

                return result;
        }
}

/*******************************************************************************

        The interface for Cipher consists of four main public methods and two
        usage patterns.

        The first and simplest usage pattern is to call sum() on the complete
        set of data, the cipher is performed immediately and a Digest is
        produced. For example:
        ---
        // create an MD5 cipher
        Md5Cipher cipher = new Md5Cipher();

        // process the data and produce a digest
        Md5Digest digest = cipher.sum("The quick brown fox jumps over the lazy dog");
        ---

        The second usage pattern involves three methods and can be used to
        process the data piece by piece (this makes it useful for cases
        involving streams of data). It begins with a new Cipher or a call to
        start() which initializes the cipher. Data is ciphered by one or more
        calls to update(), and finish() is called to complete the process and
        produce a Digest. For example:
        ---
        // create an MD5 cipher
        Md5Cipher cipher = new Md5Cipher();

        // process some data
        cipher.update("The quick brown fox");

        // process some more data
        cipher.update(" jumps over the lazy dog");

        // conclude cipher and produce digest
        Md5Digest digest = cipher.finish()
        ---

        When extending Cipher to create a custom cipher you will be required
        to implement a number of abstract methods, these include:
        ---
        public abstract Digest getDigest();
        protected abstract uint blockSize();
        protected abstract uint addSize();
        protected abstract void padMessage(ubyte[] data);
        protected abstract void transform(ubyte[] data);
        ---

        In addition there exist two further abstract methods, these methods
        have empty default implementations because in some cases they are not required:
        ---
        protected abstract void padLength(ubyte[] data, ulong length);
        protected abstract void extend();
        ---
        The method padLength() is required to implement the SHA series of
        ciphers and also the Tiger algorithm, extend() is required only to
        implement the MD2 cipher.

        The basic sequence of events as it happens internally is as follows:
        $(UL
        $(LI transform(), 0 or more times)
        $(LI padMessage())
        $(LI padLength())
        $(LI transform())
        $(LI extend())
        $(LI getDigest())
        )

*******************************************************************************/

class Cipher
{
        private uint    bytes;
        private ubyte[] buffer;

        /***********************************************************************

                Obtain the digest

                Returns:
                the digest

                Remarks:
                Returns a digest of the current cipher state, this may be the
                final digest, or a digest of the state between calls to update()

        ***********************************************************************/

        public abstract Digest getDigest();

        /***********************************************************************

                Cipher block size

                Returns:
                the block size

                Remarks:
                Specifies the size (in bytes) of the block of data to pass to
                each call to transform().

        ***********************************************************************/

        protected abstract uint blockSize();

        /***********************************************************************

                Length padding size

                Returns:
                the length padding size

                Remarks:
                Specifies the size (in bytes) of the padding which uses the
                length of the data which has been ciphered, this padding is
                carried out by the padLength method.

        ***********************************************************************/

        protected abstract uint addSize();

        /***********************************************************************

                Pads the cipher data

                Params:
                data = a slice of the cipher buffer to fill with padding

                Remarks:
                Fills the passed buffer slice with the appropriate padding for
                the final call to transform(). This padding will fill the cipher
                buffer up to blockSize()-addSize().

        ***********************************************************************/

        protected abstract void padMessage(ubyte[] data);

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

        protected abstract void padLength(ubyte[] data, ulong length) {}

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

        protected abstract void transform(ubyte[] data);

        /***********************************************************************

                Final processing of cipher.

                Remarks:
                This method is called after the final transform just prior to
                the creation of the final digest. The MD2 algorithm requires
                an additional step at this stage. Future ciphers may or may not
                require this method.

        ***********************************************************************/

        protected abstract void extend() {}

        /***********************************************************************

                Construct a cipher

                Remarks:
                Constructs the internal buffer for use by the cipher, the buffer
                size (in bytes) is defined by the abstract method blockSize().

        ***********************************************************************/

        this()
        {
                buffer = new ubyte[blockSize()];
        }

        /***********************************************************************

                Cipher the complete set of data

                Params:
                data = the set of data to cipher

                Returns:
                the completed digest

                Remarks:
                Performs the cipher on the complete set of data and produces
                the final digest.

        ***********************************************************************/

        Digest sum(void[] data)
        {
                start();
                update(data);
                return finish();
        }

        /***********************************************************************

                Initialize the cipher

                Remarks:
                Returns the cipher state to its initial value

        ***********************************************************************/

        void start()
        {
                bytes = 0;
        }

        /***********************************************************************

                Cipher additional data

                Params:
                input = the data to cipher

                Remarks:
                Continues the cipher operation on the additional data.

        ***********************************************************************/

        void update(void[] input)
        {
                uint i;
                ubyte[] data = cast(ubyte[])input;

                i = bytes & (blockSize-1);
                bytes += data.length;

                if (data.length+i < blockSize) {
                        buffer[i..i+data.length] = data[];
                        return ;
                }

                buffer[i..blockSize] = data[0..blockSize-i];
                transform(buffer);

                for(i = blockSize-i; i+blockSize-1 < data.length; i += blockSize)
                        transform(data[i..i+blockSize]);

                buffer[0..data.length-i] = data[i..data.length];
        }

        /***********************************************************************

                Complete the cipher

                Returns:
                the completed digest

                Remarks:
                Concludes the cipher producing the final digest.

        ***********************************************************************/

        Digest finish()
        {
                uint i;

                i = bytes & (blockSize-1);
                if (i < blockSize-addSize)
                    padMessage(buffer[i..blockSize-addSize]);
                else
                   {
                   padMessage(buffer[i..blockSize]);
                   transform(buffer);
                   (cast(ubyte[])buffer)[] = 0;
                   }

                padLength(buffer[blockSize-addSize..blockSize],bytes);
                transform(buffer);

                extend();

                return getDigest();
        }

        /***********************************************************************

                Converts 8 bit to 32 bit Little Endian

                Params:
                input  = the source array
                output = the destination array

                Remarks:
                Converts an array of ubyte[] into uint[] in Little Endian byte order.

        ***********************************************************************/

        static final void littleEndian32(ubyte[] input, uint[] output)
        {
                assert(output.length == input.length/4);
                output[] = cast(uint[]) input;

                version (BigEndian)
                         ByteSwap.swap32 (output.ptr, output.length * uint.sizeof);
        }

        /***********************************************************************

                Converts 8 bit to 32 bit Big Endian

                Params:
                input  = the source array
                output = the destination array

                Remarks:
                Converts an array of ubyte[] into uint[] in Big Endian byte order.

        ***********************************************************************/

        static final void bigEndian32(ubyte[] input, uint[] output)
        {
                assert(output.length == input.length/4);
                output[] = cast(uint[]) input;

                version(LittleEndian)
                        ByteSwap.swap32 (output.ptr, output.length *  uint.sizeof);
        }

        /***********************************************************************

                Converts 8 bit to 64 bit Little Endian

                Params:
                input  = the source array
                output = the destination array

                Remarks:
                Converts an array of ubyte[] into ulong[] in Little Endian byte order.

        ***********************************************************************/

        static final void littleEndian64(ubyte[] input, ulong[] output)
        {
                assert(output.length == input.length/8);
                output[] = cast(ulong[]) input;

                version (BigEndian)
                         ByteSwap.swap64 (output.ptr, output.length * ulong.sizeof);
        }

        /***********************************************************************

                Converts 8 bit to 64 bit Big Endian

                Params:
                input  = the source array
                output = the destination array

                Remarks:
                Converts an array of ubyte[] into ulong[] in Big Endian byte order.

        ***********************************************************************/

        static final void bigEndian64(ubyte[] input, ulong[] output)
        {
                assert(output.length == input.length/8);
                output[] = cast(ulong[]) input;

                version (LittleEndian)
                         ByteSwap.swap64 (output.ptr, output.length * ulong.sizeof);
        }

        /***********************************************************************

                Rotate left by n

                Params:
                x = the value to rotate
                n = the amount to rotate by

                Remarks:
                Rotates a 32 bit value by the specified amount.

        ***********************************************************************/

        static final uint rotateLeft(uint x, uint n)
        {
                version (D_InlineAsm_X86)
                        version (DigitalMars)
                        {
                        asm {
                            naked;
                            mov ECX,EAX;
                            mov EAX,4[ESP];
                            rol EAX,CL;
                            ret 4;
                            }
                        }
                     else
                        return (x << n) | (x >> (32-n));
                else
                   return (x << n) | (x >> (32-n));
        }
}


