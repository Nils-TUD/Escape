/*******************************************************************************

        copyright:      Copyright (c) 2009 Tango. All rights reserved

        license:        BSD style: see doc/license.txt for details

        version:        Initial release: Sep 2009

        author:         Kai Nacke

        This module implements the Ripemd256 algorithm by Hans Dobbertin, 
        Antoon Bosselaers and Bart Preneel.

        See http://homes.esat.kuleuven.be/~bosselae/ripemd160.html for more
        information.
        
        The implementation is based on:        
        RIPEMD-160 software written by Antoon Bosselaers, 
 		available at http://www.esat.kuleuven.ac.be/~cosicart/ps/AB-9601/

*******************************************************************************/

module tango.util.digest.Ripemd256;

private import tango.util.digest.MerkleDamgard;

public  import tango.util.digest.Digest;

/*******************************************************************************

*******************************************************************************/

final class Ripemd256 : MerkleDamgard
{
        private uint[8]        context;
        private const uint     padChar = 0x80;

        /***********************************************************************

        ***********************************************************************/

        private static const uint[8] initial =
        [
  				0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476,
  				0x76543210, 0xfedcba98, 0x89abcdef, 0x01234567
        ];
        
        /***********************************************************************

        	Construct a Ripemd256

         ***********************************************************************/

        this() { }

        /***********************************************************************

        	The size of a Ripemd256 digest is 32 bytes
        
         ***********************************************************************/

        override uint digestSize() {return 32;}


        /***********************************************************************

        	Initialize the cipher

        	Remarks:
        		Returns the cipher state to it's initial value

         ***********************************************************************/

        override void reset()
        {
        	super.reset();
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

        override void createDigest(ubyte[] buf)
        {
            version (BigEndian)
            	ByteSwap.swap32 (context.ptr, context.length * uint.sizeof);

        	buf[] = cast(ubyte[]) context;
        }


        /***********************************************************************

         	block size

        	Returns:
        	the block size

        	Remarks:
        	Specifies the size (in bytes) of the block of data to pass to
        	each call to transform(). For Ripemd256 the blockSize is 64.

         ***********************************************************************/

        protected override uint blockSize() { return 64; }

        /***********************************************************************

        	Length padding size

        	Returns:
        	the length padding size

        	Remarks:
        	Specifies the size (in bytes) of the padding which uses the
        	length of the data which has been ciphered, this padding is
        	carried out by the padLength method. For Ripemd256 the addSize is 8.

         ***********************************************************************/

        protected uint addSize()   { return 8;  }

        /***********************************************************************

        	Pads the cipher data

        	Params:
        	data = a slice of the cipher buffer to fill with padding

        	Remarks:
        	Fills the passed buffer slice with the appropriate padding for
        	the final call to transform(). This padding will fill the cipher
        	buffer up to blockSize()-addSize().

         ***********************************************************************/

        protected override void padMessage(ubyte[] at)
        {
        	at[0] = padChar;
        	at[1..at.length] = 0;
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

        protected override void padLength(ubyte[] at, ulong length)
        {
        	length <<= 3;
        	littleEndian64((cast(ubyte*)&length)[0..8],cast(ulong[]) at); 
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
        	uint al, bl, cl, dl;
        	uint ar, br, cr, dr;
            uint[16] x;
            uint t;

            littleEndian32(input,x);

            al = context[0];
            bl = context[1];
            cl = context[2];
            dl = context[3];
            ar = context[4];
            br = context[5];
            cr = context[6];
            dr = context[7];

            // Round 1 and parallel round 1
            al = rotateLeft(al + (bl ^ cl ^ dl) + x[0], 11);
            ar = rotateLeft(ar + ((br & dr) | (cr & ~(dr))) + x[5] + 0x50a28be6, 8);
            dl = rotateLeft(dl + (al ^ bl ^ cl) + x[1], 14);
            dr = rotateLeft(dr + ((ar & cr) | (br & ~(cr))) + x[14] + 0x50a28be6, 9);
            cl = rotateLeft(cl + (dl ^ al ^ bl) + x[2], 15);
            cr = rotateLeft(cr + ((dr & br) | (ar & ~(br))) + x[7] + 0x50a28be6, 9);
            bl = rotateLeft(bl + (cl ^ dl ^ al) + x[3], 12);
            br = rotateLeft(br + ((cr & ar) | (dr & ~(ar))) + x[0] + 0x50a28be6, 11);
            al = rotateLeft(al + (bl ^ cl ^ dl) + x[4], 5);
            ar = rotateLeft(ar + ((br & dr) | (cr & ~(dr))) + x[9] + 0x50a28be6, 13);
            dl = rotateLeft(dl + (al ^ bl ^ cl) + x[5], 8);
            dr = rotateLeft(dr + ((ar & cr) | (br & ~(cr))) + x[2] + 0x50a28be6, 15);
            cl = rotateLeft(cl + (dl ^ al ^ bl) + x[6], 7);
            cr = rotateLeft(cr + ((dr & br) | (ar & ~(br))) + x[11] + 0x50a28be6, 15);
            bl = rotateLeft(bl + (cl ^ dl ^ al) + x[7], 9);
            br = rotateLeft(br + ((cr & ar) | (dr & ~(ar))) + x[4] + 0x50a28be6, 5);
            al = rotateLeft(al + (bl ^ cl ^ dl) + x[8], 11);
            ar = rotateLeft(ar + ((br & dr) | (cr & ~(dr))) + x[13] + 0x50a28be6, 7);
            dl = rotateLeft(dl + (al ^ bl ^ cl) + x[9], 13);
            dr = rotateLeft(dr + ((ar & cr) | (br & ~(cr))) + x[6] + 0x50a28be6, 7);
            cl = rotateLeft(cl + (dl ^ al ^ bl) + x[10], 14);
            cr = rotateLeft(cr + ((dr & br) | (ar & ~(br))) + x[15] + 0x50a28be6, 8);
            bl = rotateLeft(bl + (cl ^ dl ^ al) + x[11], 15);
            br = rotateLeft(br + ((cr & ar) | (dr & ~(ar))) + x[8] + 0x50a28be6, 11);
            al = rotateLeft(al + (bl ^ cl ^ dl) + x[12], 6);
            ar = rotateLeft(ar + ((br & dr) | (cr & ~(dr))) + x[1] + 0x50a28be6, 14);
            dl = rotateLeft(dl + (al ^ bl ^ cl) + x[13], 7);
            dr = rotateLeft(dr + ((ar & cr) | (br & ~(cr))) + x[10] + 0x50a28be6, 14);
            cl = rotateLeft(cl + (dl ^ al ^ bl) + x[14], 9);
            cr = rotateLeft(cr + ((dr & br) | (ar & ~(br))) + x[3] + 0x50a28be6, 12);
            bl = rotateLeft(bl + (cl ^ dl ^ al) + x[15], 8);
            br = rotateLeft(br + ((cr & ar) | (dr & ~(ar))) + x[12] + 0x50a28be6, 6);
            
            t = al; al = ar; ar = t;
            
            // Round 2 and parallel round 2
            al = rotateLeft(al + (((cl ^ dl) & bl) ^ dl) + x[7] + 0x5a827999, 7);
            ar = rotateLeft(ar + ((br | ~(cr)) ^ dr) + x[6] + 0x5c4dd124, 9);
            dl = rotateLeft(dl + (((bl ^ cl) & al) ^ cl) + x[4] + 0x5a827999, 6);
            dr = rotateLeft(dr + ((ar | ~(br)) ^ cr) + x[11] + 0x5c4dd124, 13);
            cl = rotateLeft(cl + (((al ^ bl) & dl) ^ bl) + x[13] + 0x5a827999, 8);
            cr = rotateLeft(cr + ((dr | ~(ar)) ^ br) + x[3] + 0x5c4dd124, 15);
            bl = rotateLeft(bl + (((dl ^ al) & cl) ^ al) + x[1] + 0x5a827999, 13);
            br = rotateLeft(br + ((cr | ~(dr)) ^ ar) + x[7] + 0x5c4dd124, 7);
            al = rotateLeft(al + (((cl ^ dl) & bl) ^ dl) + x[10] + 0x5a827999, 11);
            ar = rotateLeft(ar + ((br | ~(cr)) ^ dr) + x[0] + 0x5c4dd124, 12);
            dl = rotateLeft(dl + (((bl ^ cl) & al) ^ cl) + x[6] + 0x5a827999, 9);
            dr = rotateLeft(dr + ((ar | ~(br)) ^ cr) + x[13] + 0x5c4dd124, 8);
            cl = rotateLeft(cl + (((al ^ bl) & dl) ^ bl) + x[15] + 0x5a827999, 7);
            cr = rotateLeft(cr + ((dr | ~(ar)) ^ br) + x[5] + 0x5c4dd124, 9);
            bl = rotateLeft(bl + (((dl ^ al) & cl) ^ al) + x[3] + 0x5a827999, 15);
            br = rotateLeft(br + ((cr | ~(dr)) ^ ar) + x[10] + 0x5c4dd124, 11);
            al = rotateLeft(al + (((cl ^ dl) & bl) ^ dl) + x[12] + 0x5a827999, 7);
            ar = rotateLeft(ar + ((br | ~(cr)) ^ dr) + x[14] + 0x5c4dd124, 7);
            dl = rotateLeft(dl + (((bl ^ cl) & al) ^ cl) + x[0] + 0x5a827999, 12);
            dr = rotateLeft(dr + ((ar | ~(br)) ^ cr) + x[15] + 0x5c4dd124, 7);
            cl = rotateLeft(cl + (((al ^ bl) & dl) ^ bl) + x[9] + 0x5a827999, 15);
            cr = rotateLeft(cr + ((dr | ~(ar)) ^ br) + x[8] + 0x5c4dd124, 12);
            bl = rotateLeft(bl + (((dl ^ al) & cl) ^ al) + x[5] + 0x5a827999, 9);
            br = rotateLeft(br + ((cr | ~(dr)) ^ ar) + x[12] + 0x5c4dd124, 7);
            al = rotateLeft(al + (((cl ^ dl) & bl) ^ dl) + x[2] + 0x5a827999, 11);
            ar = rotateLeft(ar + ((br | ~(cr)) ^ dr) + x[4] + 0x5c4dd124, 6);
            dl = rotateLeft(dl + (((bl ^ cl) & al) ^ cl) + x[14] + 0x5a827999, 7);
            dr = rotateLeft(dr + ((ar | ~(br)) ^ cr) + x[9] + 0x5c4dd124, 15);
            cl = rotateLeft(cl + (((al ^ bl) & dl) ^ bl) + x[11] + 0x5a827999, 13);
            cr = rotateLeft(cr + ((dr | ~(ar)) ^ br) + x[1] + 0x5c4dd124, 13);
            bl = rotateLeft(bl + (((dl ^ al) & cl) ^ al) + x[8] + 0x5a827999, 12);
            br = rotateLeft(br + ((cr | ~(dr)) ^ ar) + x[2] + 0x5c4dd124, 11);
            
            t = bl; bl = br; br = t;
            
            // Round 3 and parallel round 3
            al = rotateLeft(al + ((bl | ~(cl)) ^ dl) + x[3] + 0x6ed9eba1, 11);
            ar = rotateLeft(ar + (((cr ^ dr) & br) ^ dr) + x[15] + 0x6d703ef3, 9);
            dl = rotateLeft(dl + ((al | ~(bl)) ^ cl) + x[10] + 0x6ed9eba1, 13);
            dr = rotateLeft(dr + (((br ^ cr) & ar) ^ cr) + x[5] + 0x6d703ef3, 7);
            cl = rotateLeft(cl + ((dl | ~(al)) ^ bl) + x[14] + 0x6ed9eba1, 6);
            cr = rotateLeft(cr + (((ar ^ br) & dr) ^ br) + x[1] + 0x6d703ef3, 15);
            bl = rotateLeft(bl + ((cl | ~(dl)) ^ al) + x[4] + 0x6ed9eba1, 7);
            br = rotateLeft(br + (((dr ^ ar) & cr) ^ ar) + x[3] + 0x6d703ef3, 11);
            al = rotateLeft(al + ((bl | ~(cl)) ^ dl) + x[9] + 0x6ed9eba1, 14);
            ar = rotateLeft(ar + (((cr ^ dr) & br) ^ dr) + x[7] + 0x6d703ef3, 8);
            dl = rotateLeft(dl + ((al | ~(bl)) ^ cl) + x[15] + 0x6ed9eba1, 9);
            dr = rotateLeft(dr + (((br ^ cr) & ar) ^ cr) + x[14] + 0x6d703ef3, 6);
            cl = rotateLeft(cl + ((dl | ~(al)) ^ bl) + x[8] + 0x6ed9eba1, 13);
            cr = rotateLeft(cr + (((ar ^ br) & dr) ^ br) + x[6] + 0x6d703ef3, 6);
            bl = rotateLeft(bl + ((cl | ~(dl)) ^ al) + x[1] + 0x6ed9eba1, 15);
            br = rotateLeft(br + (((dr ^ ar) & cr) ^ ar) + x[9] + 0x6d703ef3, 14);
            al = rotateLeft(al + ((bl | ~(cl)) ^ dl) + x[2] + 0x6ed9eba1, 14);
            ar = rotateLeft(ar + (((cr ^ dr) & br) ^ dr) + x[11] + 0x6d703ef3, 12);
            dl = rotateLeft(dl + ((al | ~(bl)) ^ cl) + x[7] + 0x6ed9eba1, 8);
            dr = rotateLeft(dr + (((br ^ cr) & ar) ^ cr) + x[8] + 0x6d703ef3, 13);
            cl = rotateLeft(cl + ((dl | ~(al)) ^ bl) + x[0] + 0x6ed9eba1, 13);
            cr = rotateLeft(cr + (((ar ^ br) & dr) ^ br) + x[12] + 0x6d703ef3, 5);
            bl = rotateLeft(bl + ((cl | ~(dl)) ^ al) + x[6] + 0x6ed9eba1, 6);
            br = rotateLeft(br + (((dr ^ ar) & cr) ^ ar) + x[2] + 0x6d703ef3, 14);
            al = rotateLeft(al + ((bl | ~(cl)) ^ dl) + x[13] + 0x6ed9eba1, 5);
            ar = rotateLeft(ar + (((cr ^ dr) & br) ^ dr) + x[10] + 0x6d703ef3, 13);
            dl = rotateLeft(dl + ((al | ~(bl)) ^ cl) + x[11] + 0x6ed9eba1, 12);
            dr = rotateLeft(dr + (((br ^ cr) & ar) ^ cr) + x[0] + 0x6d703ef3, 13);
            cl = rotateLeft(cl + ((dl | ~(al)) ^ bl) + x[5] + 0x6ed9eba1, 7);
            cr = rotateLeft(cr + (((ar ^ br) & dr) ^ br) + x[4] + 0x6d703ef3, 7);
            bl = rotateLeft(bl + ((cl | ~(dl)) ^ al) + x[12] + 0x6ed9eba1, 5);
            br = rotateLeft(br + (((dr ^ ar) & cr) ^ ar) + x[13] + 0x6d703ef3, 5);
            
            t = cl; cl = cr; cr = t;
            
            // Round 4 and parallel round 4
            al = rotateLeft(al + ((bl & dl) | (cl & ~(dl))) + x[1] + 0x8f1bbcdc, 11);
            ar = rotateLeft(ar + (br ^ cr ^ dr) + x[8], 15);
            dl = rotateLeft(dl + ((al & cl) | (bl & ~(cl))) + x[9] + 0x8f1bbcdc, 12);
            dr = rotateLeft(dr + (ar ^ br ^ cr) + x[6], 5);
            cl = rotateLeft(cl + ((dl & bl) | (al & ~(bl))) + x[11] + 0x8f1bbcdc, 14);
            cr = rotateLeft(cr + (dr ^ ar ^ br) + x[4], 8);
            bl = rotateLeft(bl + ((cl & al) | (dl & ~(al))) + x[10] + 0x8f1bbcdc, 15);
            br = rotateLeft(br + (cr ^ dr ^ ar) + x[1], 11);
            al = rotateLeft(al + ((bl & dl) | (cl & ~(dl))) + x[0] + 0x8f1bbcdc, 14);
            ar = rotateLeft(ar + (br ^ cr ^ dr) + x[3], 14);
            dl = rotateLeft(dl + ((al & cl) | (bl & ~(cl))) + x[8] + 0x8f1bbcdc, 15);
            dr = rotateLeft(dr + (ar ^ br ^ cr) + x[11], 14);
            cl = rotateLeft(cl + ((dl & bl) | (al & ~(bl))) + x[12] + 0x8f1bbcdc, 9);
            cr = rotateLeft(cr + (dr ^ ar ^ br) + x[15], 6);
            bl = rotateLeft(bl + ((cl & al) | (dl & ~(al))) + x[4] + 0x8f1bbcdc, 8);
            br = rotateLeft(br + (cr ^ dr ^ ar) + x[0], 14);
            al = rotateLeft(al + ((bl & dl) | (cl & ~(dl))) + x[13] + 0x8f1bbcdc, 9);
            ar = rotateLeft(ar + (br ^ cr ^ dr) + x[5], 6);
            dl = rotateLeft(dl + ((al & cl) | (bl & ~(cl))) + x[3] + 0x8f1bbcdc, 14);
            dr = rotateLeft(dr + (ar ^ br ^ cr) + x[12], 9);
            cl = rotateLeft(cl + ((dl & bl) | (al & ~(bl))) + x[7] + 0x8f1bbcdc, 5);
            cr = rotateLeft(cr + (dr ^ ar ^ br) + x[2], 12);
            bl = rotateLeft(bl + ((cl & al) | (dl & ~(al))) + x[15] + 0x8f1bbcdc, 6);
            br = rotateLeft(br + (cr ^ dr ^ ar) + x[13], 9);
            al = rotateLeft(al + ((bl & dl) | (cl & ~(dl))) + x[14] + 0x8f1bbcdc, 8);
            ar = rotateLeft(ar + (br ^ cr ^ dr) + x[9], 12);
            dl = rotateLeft(dl + ((al & cl) | (bl & ~(cl))) + x[5] + 0x8f1bbcdc, 6);
            dr = rotateLeft(dr + (ar ^ br ^ cr) + x[7], 5);
            cl = rotateLeft(cl + ((dl & bl) | (al & ~(bl))) + x[6] + 0x8f1bbcdc, 5);
            cr = rotateLeft(cr + (dr ^ ar ^ br) + x[10], 15);
            bl = rotateLeft(bl + ((cl & al) | (dl & ~(al))) + x[2] + 0x8f1bbcdc, 12);
            br = rotateLeft(br + (cr ^ dr ^ ar) + x[14], 8);
            
            // Do not swap dl and dr; simply add the right value to context 

            context[0] += al;
            context[1] += bl;
            context[2] += cl;
            context[3] += dr;
            context[4] += ar;
            context[5] += br;
            context[6] += cr;
            context[7] += dl;

            x[] = 0;
        }

}

/*******************************************************************************

*******************************************************************************/

debug(UnitTest)
{
    unittest
    {
    static char[][] strings =
    [
            "",
            "a",
            "abc",
            "message digest",
            "abcdefghijklmnopqrstuvwxyz",
            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
            "12345678901234567890123456789012345678901234567890123456789012345678901234567890"
    ];

    static char[][] results =
    [
            "02ba4c4e5f8ecd1877fc52d64d30e37a2d9774fb1e5d026380ae0168e3c5522d",
            "f9333e45d857f5d90a91bab70a1eba0cfb1be4b0783c9acfcd883a9134692925",
            "afbd6e228b9d8cbbcef5ca2d03e6dba10ac0bc7dcbe4680e1e42d2e975459b65",
            "87e971759a1ce47a514d5c914c392c9018c7c46bc14465554afcdf54a5070c0e",
            "649d3034751ea216776bf9a18acc81bc7896118a5197968782dd1fd97d8d5133",
            "3843045583aac6c8c8d9128573e7a9809afb2a0f34ccc36ea9e72f16f6368e3f",
            "5740a408ac16b720b84424ae931cbb1fe363d1d0bf4017f1a89f7ea6de77a0b8",
            "06fdcc7a409548aaf91368c06a6275b553e3f099bf0ea4edfd6778df89a890dd"
    ];

    Ripemd256 h = new Ripemd256();

    foreach (int i, char[] s; strings)
            {
            h.update(cast(ubyte[]) s);
            char[] d = h.hexDigest;

            assert(d == results[i],":("~s~")("~d~")!=("~results[i]~")");
            }

    char[] s = new char[1000000];
    for (auto i = 0; i < s.length; i++) s[i] = 'a';
    char[] result = "ac953744e10e31514c150d4d8d7b677342e33399788296e43ae4850ce4f97978";
    h.update(cast(ubyte[]) s);
    char[] d = h.hexDigest;

    assert(d == result,":(1 million times \"a\")("~d~")!=("~result~")");
    }
	
}