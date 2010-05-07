/*******************************************************************************

        copyright:      Copyright (c) 2009 Tango. All rights reserved

        license:        BSD style: see doc/license.txt for details

        version:        Initial release: Jan 2010

        author:         Kai Nacke

        This module implements the Whirlpool algorithm by Paulo S.L.M. Barreto 
        and Vincent Rijmen.
        
        See https://www.cosic.esat.kuleuven.ac.be/nessie/workshop/submissions/whirlpool.zip
        for more information.

*******************************************************************************/

module tango.io.digest.Whirlpool;

private import tango.util.digest.MerkleDamgard;

public  import tango.util.digest.Digest;

/*******************************************************************************

*******************************************************************************/

private static const int INTERNAL_ROUNDS = 10;

final class Whirlpool : MerkleDamgard
{
		private ulong hash[8];    /* the hashing state */

        private const uint     padChar = 0x80;

        /***********************************************************************

        	Construct a Whirlpool

         ***********************************************************************/

        this() { }

        /***********************************************************************

        	The size of a Whirlpool digest is 64 bytes
        
         ***********************************************************************/

        override uint digestSize() {return 64;}

        /***********************************************************************

        	Initialize the cipher

        	Remarks:
        		Returns the cipher state to it's initial value

         ***********************************************************************/

        override void reset()
        {
        	super.reset();
        	hash[] = 0L;
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
            version (LittleEndian)
            	ByteSwap.swap64 (hash.ptr, hash.length * ulong.sizeof);

        	buf[] = cast(ubyte[]) hash;
        }

        /***********************************************************************

         	block size

        	Returns:
        	the block size

        	Remarks:
        	Specifies the size (in bytes) of the block of data to pass to
        	each call to transform(). For Whirlpool the blockSize is 64.

         ***********************************************************************/

        protected override uint blockSize() { return 64; }

        /***********************************************************************

        	Length padding size

        	Returns:
        	the length padding size

        	Remarks:
        	Specifies the size (in bytes) of the padding which uses the
        	length of the data which has been ciphered, this padding is
        	carried out by the padLength method. For Whirlpool the addSize is 8.

         ***********************************************************************/

        protected uint addSize()   { return 32;  }

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
        	ulong buffer[4];
        	buffer[0..2] = 0L;
        	buffer[3] = length << 3;
        	bigEndian64((cast(ubyte*)&buffer)[0..ulong.sizeof*4],cast(ulong[]) at); 
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
            ulong block[8];    /* mu(buffer) */

            /*
             * map the buffer to a block:
             */
            bigEndian64(input,block);
            
            /*
             * compute and apply K^0 to the cipher state:
             */
            ulong K[8] = hash[]; 			/* the round key */
            ulong state[8] = block[] ^ K[];	/* the cipher state */

            /*
             * iterate over all rounds:
             */
            for (auto r = 1; r <= INTERNAL_ROUNDS; r++) {
                ulong L[8];
                /*
                 * compute K^r from K^{r-1}:
                 */
                L[0] =
                    C0[cast(ubyte)(K[0] >>> 56)] ^
                    C1[cast(ubyte)(K[7] >>> 48)] ^
                    C2[cast(ubyte)(K[6] >>> 40)] ^
                    C3[cast(ubyte)(K[5] >>> 32)] ^
                    C4[cast(ubyte)(K[4] >>> 24)] ^
                    C5[cast(ubyte)(K[3] >>> 16)] ^
                    C6[cast(ubyte)(K[2] >>>  8)] ^
                    C7[cast(ubyte)(K[1]      )] ^
                    rc[r];
                L[1] =
                    C0[cast(ubyte)(K[1] >>> 56)] ^
                    C1[cast(ubyte)(K[0] >>> 48)] ^
                    C2[cast(ubyte)(K[7] >>> 40)] ^
                    C3[cast(ubyte)(K[6] >>> 32)] ^
                    C4[cast(ubyte)(K[5] >>> 24)] ^
                    C5[cast(ubyte)(K[4] >>> 16)] ^
                    C6[cast(ubyte)(K[3] >>>  8)] ^
                    C7[cast(ubyte)(K[2]      )];
                L[2] =
                    C0[cast(ubyte)(K[2] >>> 56)] ^
                    C1[cast(ubyte)(K[1] >>> 48)] ^
                    C2[cast(ubyte)(K[0] >>> 40)] ^
                    C3[cast(ubyte)(K[7] >>> 32)] ^
                    C4[cast(ubyte)(K[6] >>> 24)] ^
                    C5[cast(ubyte)(K[5] >>> 16)] ^
                    C6[cast(ubyte)(K[4] >>>  8)] ^
                    C7[cast(ubyte)(K[3]      )];
                L[3] =
                    C0[cast(ubyte)(K[3] >>> 56)] ^
                    C1[cast(ubyte)(K[2] >>> 48)] ^
                    C2[cast(ubyte)(K[1] >>> 40)] ^
                    C3[cast(ubyte)(K[0] >>> 32)] ^
                    C4[cast(ubyte)(K[7] >>> 24)] ^
                    C5[cast(ubyte)(K[6] >>> 16)] ^
                    C6[cast(ubyte)(K[5] >>>  8)] ^
                    C7[cast(ubyte)(K[4]      )];
                L[4] =
                    C0[cast(ubyte)(K[4] >>> 56)] ^
                    C1[cast(ubyte)(K[3] >>> 48)] ^
                    C2[cast(ubyte)(K[2] >>> 40)] ^
                    C3[cast(ubyte)(K[1] >>> 32)] ^
                    C4[cast(ubyte)(K[0] >>> 24)] ^
                    C5[cast(ubyte)(K[7] >>> 16)] ^
                    C6[cast(ubyte)(K[6] >>>  8)] ^
                    C7[cast(ubyte)(K[5]      )];
                L[5] =
                    C0[cast(ubyte)(K[5] >>> 56)] ^
                    C1[cast(ubyte)(K[4] >>> 48)] ^
                    C2[cast(ubyte)(K[3] >>> 40)] ^
                    C3[cast(ubyte)(K[2] >>> 32)] ^
                    C4[cast(ubyte)(K[1] >>> 24)] ^
                    C5[cast(ubyte)(K[0] >>> 16)] ^
                    C6[cast(ubyte)(K[7] >>>  8)] ^
                    C7[cast(ubyte)(K[6]      )];
                L[6] =
                    C0[cast(ubyte)(K[6] >>> 56)] ^
                    C1[cast(ubyte)(K[5] >>> 48)] ^
                    C2[cast(ubyte)(K[4] >>> 40)] ^
                    C3[cast(ubyte)(K[3] >>> 32)] ^
                    C4[cast(ubyte)(K[2] >>> 24)] ^
                    C5[cast(ubyte)(K[1] >>> 16)] ^
                    C6[cast(ubyte)(K[0] >>>  8)] ^
                    C7[cast(ubyte)(K[7]      )];
                L[7] =
                    C0[cast(ubyte)(K[7] >>> 56)] ^
                    C1[cast(ubyte)(K[6] >>> 48)] ^
                    C2[cast(ubyte)(K[5] >>> 40)] ^
                    C3[cast(ubyte)(K[4] >>> 32)] ^
                    C4[cast(ubyte)(K[3] >>> 24)] ^
                    C5[cast(ubyte)(K[2] >>> 16)] ^
                    C6[cast(ubyte)(K[1] >>>  8)] ^
                    C7[cast(ubyte)(K[0]      )];
                K[] = L[];
                
                /*
                 * apply the r-th round transformation:
                 */
                L[0] =
                    C0[cast(ubyte)(state[0] >>> 56)] ^
                    C1[cast(ubyte)(state[7] >>> 48)] ^
                    C2[cast(ubyte)(state[6] >>> 40)] ^
                    C3[cast(ubyte)(state[5] >>> 32)] ^
                    C4[cast(ubyte)(state[4] >>> 24)] ^
                    C5[cast(ubyte)(state[3] >>> 16)] ^
                    C6[cast(ubyte)(state[2] >>>  8)] ^
                    C7[cast(ubyte)(state[1]      )] ^
                    K[0];
                L[1] =
                    C0[cast(ubyte)(state[1] >>> 56)] ^
                    C1[cast(ubyte)(state[0] >>> 48)] ^
                    C2[cast(ubyte)(state[7] >>> 40)] ^
                    C3[cast(ubyte)(state[6] >>> 32)] ^
                    C4[cast(ubyte)(state[5] >>> 24)] ^
                    C5[cast(ubyte)(state[4] >>> 16)] ^
                    C6[cast(ubyte)(state[3] >>>  8)] ^
                    C7[cast(ubyte)(state[2]      )] ^
                    K[1];
                L[2] =
                    C0[cast(ubyte)(state[2] >>> 56)] ^
                    C1[cast(ubyte)(state[1] >>> 48)] ^
                    C2[cast(ubyte)(state[0] >>> 40)] ^
                    C3[cast(ubyte)(state[7] >>> 32)] ^
                    C4[cast(ubyte)(state[6] >>> 24)] ^
                    C5[cast(ubyte)(state[5] >>> 16)] ^
                    C6[cast(ubyte)(state[4] >>>  8)] ^
                    C7[cast(ubyte)(state[3]      )] ^
                    K[2];
                L[3] =
                    C0[cast(ubyte)(state[3] >>> 56)] ^
                    C1[cast(ubyte)(state[2] >>> 48)] ^
                    C2[cast(ubyte)(state[1] >>> 40)] ^
                    C3[cast(ubyte)(state[0] >>> 32)] ^
                    C4[cast(ubyte)(state[7] >>> 24)] ^
                    C5[cast(ubyte)(state[6] >>> 16)] ^
                    C6[cast(ubyte)(state[5] >>>  8)] ^
                    C7[cast(ubyte)(state[4]      )] ^
                    K[3];
                L[4] =
                    C0[cast(ubyte)(state[4] >>> 56)] ^
                    C1[cast(ubyte)(state[3] >>> 48)] ^
                    C2[cast(ubyte)(state[2] >>> 40)] ^
                    C3[cast(ubyte)(state[1] >>> 32)] ^
                    C4[cast(ubyte)(state[0] >>> 24)] ^
                    C5[cast(ubyte)(state[7] >>> 16)] ^
                    C6[cast(ubyte)(state[6] >>>  8)] ^
                    C7[cast(ubyte)(state[5]      )] ^
                    K[4];
                L[5] =
                    C0[cast(ubyte)(state[5] >>> 56)] ^
                    C1[cast(ubyte)(state[4] >>> 48)] ^
                    C2[cast(ubyte)(state[3] >>> 40)] ^
                    C3[cast(ubyte)(state[2] >>> 32)] ^
                    C4[cast(ubyte)(state[1] >>> 24)] ^
                    C5[cast(ubyte)(state[0] >>> 16)] ^
                    C6[cast(ubyte)(state[7] >>>  8)] ^
                    C7[cast(ubyte)(state[6]      )] ^
                    K[5];
                L[6] =
                    C0[cast(ubyte)(state[6] >>> 56)] ^
                    C1[cast(ubyte)(state[5] >>> 48)] ^
                    C2[cast(ubyte)(state[4] >>> 40)] ^
                    C3[cast(ubyte)(state[3] >>> 32)] ^
                    C4[cast(ubyte)(state[2] >>> 24)] ^
                    C5[cast(ubyte)(state[1] >>> 16)] ^
                    C6[cast(ubyte)(state[0] >>>  8)] ^
                    C7[cast(ubyte)(state[7]      )] ^
                    K[6];
                L[7] =
                    C0[cast(ubyte)(state[7] >>> 56)] ^
                    C1[cast(ubyte)(state[6] >>> 48)] ^
                    C2[cast(ubyte)(state[5] >>> 40)] ^
                    C3[cast(ubyte)(state[4] >>> 32)] ^
                    C4[cast(ubyte)(state[3] >>> 24)] ^
                    C5[cast(ubyte)(state[2] >>> 16)] ^
                    C6[cast(ubyte)(state[1] >>>  8)] ^
                    C7[cast(ubyte)(state[0]      )] ^
                    K[7];
                state[] = L[];
            }

            /*
             * apply the Miyaguchi-Preneel compression function:
             */
            hash[] ^= state[] ^ block[];
        }
}

/*******************************************************************************

*******************************************************************************/

private static const ulong C0[256] = 
[
    0x18186018c07830d8, 0x23238c2305af4626, 0xc6c63fc67ef991b8, 0xe8e887e8136fcdfb,
    0x878726874ca113cb, 0xb8b8dab8a9626d11, 0x0101040108050209, 0x4f4f214f426e9e0d,
    0x3636d836adee6c9b, 0xa6a6a2a6590451ff, 0xd2d26fd2debdb90c, 0xf5f5f3f5fb06f70e,
    0x7979f979ef80f296, 0x6f6fa16f5fcede30, 0x91917e91fcef3f6d, 0x52525552aa07a4f8,
    0x60609d6027fdc047, 0xbcbccabc89766535, 0x9b9b569baccd2b37, 0x8e8e028e048c018a,
    0xa3a3b6a371155bd2, 0x0c0c300c603c186c, 0x7b7bf17bff8af684, 0x3535d435b5e16a80,
    0x1d1d741de8693af5, 0xe0e0a7e05347ddb3, 0xd7d77bd7f6acb321, 0xc2c22fc25eed999c,
    0x2e2eb82e6d965c43, 0x4b4b314b627a9629, 0xfefedffea321e15d, 0x575741578216aed5,
    0x15155415a8412abd, 0x7777c1779fb6eee8, 0x3737dc37a5eb6e92, 0xe5e5b3e57b56d79e,
    0x9f9f469f8cd92313, 0xf0f0e7f0d317fd23, 0x4a4a354a6a7f9420, 0xdada4fda9e95a944,
    0x58587d58fa25b0a2, 0xc9c903c906ca8fcf, 0x2929a429558d527c, 0x0a0a280a5022145a,
    0xb1b1feb1e14f7f50, 0xa0a0baa0691a5dc9, 0x6b6bb16b7fdad614, 0x85852e855cab17d9,
    0xbdbdcebd8173673c, 0x5d5d695dd234ba8f, 0x1010401080502090, 0xf4f4f7f4f303f507,
    0xcbcb0bcb16c08bdd, 0x3e3ef83eedc67cd3, 0x0505140528110a2d, 0x676781671fe6ce78,
    0xe4e4b7e47353d597, 0x27279c2725bb4e02, 0x4141194132588273, 0x8b8b168b2c9d0ba7,
    0xa7a7a6a7510153f6, 0x7d7de97dcf94fab2, 0x95956e95dcfb3749, 0xd8d847d88e9fad56,
    0xfbfbcbfb8b30eb70, 0xeeee9fee2371c1cd, 0x7c7ced7cc791f8bb, 0x6666856617e3cc71,
    0xdddd53dda68ea77b, 0x17175c17b84b2eaf, 0x4747014702468e45, 0x9e9e429e84dc211a,
    0xcaca0fca1ec589d4, 0x2d2db42d75995a58, 0xbfbfc6bf9179632e, 0x07071c07381b0e3f,
    0xadad8ead012347ac, 0x5a5a755aea2fb4b0, 0x838336836cb51bef, 0x3333cc3385ff66b6,
    0x636391633ff2c65c, 0x02020802100a0412, 0xaaaa92aa39384993, 0x7171d971afa8e2de,
    0xc8c807c80ecf8dc6, 0x19196419c87d32d1, 0x494939497270923b, 0xd9d943d9869aaf5f,
    0xf2f2eff2c31df931, 0xe3e3abe34b48dba8, 0x5b5b715be22ab6b9, 0x88881a8834920dbc,
    0x9a9a529aa4c8293e, 0x262698262dbe4c0b, 0x3232c8328dfa64bf, 0xb0b0fab0e94a7d59,
    0xe9e983e91b6acff2, 0x0f0f3c0f78331e77, 0xd5d573d5e6a6b733, 0x80803a8074ba1df4,
    0xbebec2be997c6127, 0xcdcd13cd26de87eb, 0x3434d034bde46889, 0x48483d487a759032,
    0xffffdbffab24e354, 0x7a7af57af78ff48d, 0x90907a90f4ea3d64, 0x5f5f615fc23ebe9d,
    0x202080201da0403d, 0x6868bd6867d5d00f, 0x1a1a681ad07234ca, 0xaeae82ae192c41b7,
    0xb4b4eab4c95e757d, 0x54544d549a19a8ce, 0x93937693ece53b7f, 0x222288220daa442f,
    0x64648d6407e9c863, 0xf1f1e3f1db12ff2a, 0x7373d173bfa2e6cc, 0x12124812905a2482,
    0x40401d403a5d807a, 0x0808200840281048, 0xc3c32bc356e89b95, 0xecec97ec337bc5df,
    0xdbdb4bdb9690ab4d, 0xa1a1bea1611f5fc0, 0x8d8d0e8d1c830791, 0x3d3df43df5c97ac8,
    0x97976697ccf1335b, 0x0000000000000000, 0xcfcf1bcf36d483f9, 0x2b2bac2b4587566e,
    0x7676c57697b3ece1, 0x8282328264b019e6, 0xd6d67fd6fea9b128, 0x1b1b6c1bd87736c3,
    0xb5b5eeb5c15b7774, 0xafaf86af112943be, 0x6a6ab56a77dfd41d, 0x50505d50ba0da0ea,
    0x45450945124c8a57, 0xf3f3ebf3cb18fb38, 0x3030c0309df060ad, 0xefef9bef2b74c3c4,
    0x3f3ffc3fe5c37eda, 0x55554955921caac7, 0xa2a2b2a2791059db, 0xeaea8fea0365c9e9,
    0x656589650fecca6a, 0xbabad2bab9686903, 0x2f2fbc2f65935e4a, 0xc0c027c04ee79d8e,
    0xdede5fdebe81a160, 0x1c1c701ce06c38fc, 0xfdfdd3fdbb2ee746, 0x4d4d294d52649a1f,
    0x92927292e4e03976, 0x7575c9758fbceafa, 0x06061806301e0c36, 0x8a8a128a249809ae,
    0xb2b2f2b2f940794b, 0xe6e6bfe66359d185, 0x0e0e380e70361c7e, 0x1f1f7c1ff8633ee7,
    0x6262956237f7c455, 0xd4d477d4eea3b53a, 0xa8a89aa829324d81, 0x96966296c4f43152,
    0xf9f9c3f99b3aef62, 0xc5c533c566f697a3, 0x2525942535b14a10, 0x59597959f220b2ab,
    0x84842a8454ae15d0, 0x7272d572b7a7e4c5, 0x3939e439d5dd72ec, 0x4c4c2d4c5a619816,
    0x5e5e655eca3bbc94, 0x7878fd78e785f09f, 0x3838e038ddd870e5, 0x8c8c0a8c14860598,
    0xd1d163d1c6b2bf17, 0xa5a5aea5410b57e4, 0xe2e2afe2434dd9a1, 0x616199612ff8c24e,
    0xb3b3f6b3f1457b42, 0x2121842115a54234, 0x9c9c4a9c94d62508, 0x1e1e781ef0663cee,
    0x4343114322528661, 0xc7c73bc776fc93b1, 0xfcfcd7fcb32be54f, 0x0404100420140824,
    0x51515951b208a2e3, 0x99995e99bcc72f25, 0x6d6da96d4fc4da22, 0x0d0d340d68391a65,
    0xfafacffa8335e979, 0xdfdf5bdfb684a369, 0x7e7ee57ed79bfca9, 0x242490243db44819,
    0x3b3bec3bc5d776fe, 0xabab96ab313d4b9a, 0xcece1fce3ed181f0, 0x1111441188552299,
    0x8f8f068f0c890383, 0x4e4e254e4a6b9c04, 0xb7b7e6b7d1517366, 0xebeb8beb0b60cbe0,
    0x3c3cf03cfdcc78c1, 0x81813e817cbf1ffd, 0x94946a94d4fe3540, 0xf7f7fbf7eb0cf31c,
    0xb9b9deb9a1676f18, 0x13134c13985f268b, 0x2c2cb02c7d9c5851, 0xd3d36bd3d6b8bb05,
    0xe7e7bbe76b5cd38c, 0x6e6ea56e57cbdc39, 0xc4c437c46ef395aa, 0x03030c03180f061b,
    0x565645568a13acdc, 0x44440d441a49885e, 0x7f7fe17fdf9efea0, 0xa9a99ea921374f88,
    0x2a2aa82a4d825467, 0xbbbbd6bbb16d6b0a, 0xc1c123c146e29f87, 0x53535153a202a6f1,
    0xdcdc57dcae8ba572, 0x0b0b2c0b58271653, 0x9d9d4e9d9cd32701, 0x6c6cad6c47c1d82b,
    0x3131c43195f562a4, 0x7474cd7487b9e8f3, 0xf6f6fff6e309f115, 0x464605460a438c4c,
    0xacac8aac092645a5, 0x89891e893c970fb5, 0x14145014a04428b4, 0xe1e1a3e15b42dfba,
    0x16165816b04e2ca6, 0x3a3ae83acdd274f7, 0x6969b9696fd0d206, 0x09092409482d1241,
    0x7070dd70a7ade0d7, 0xb6b6e2b6d954716f, 0xd0d067d0ceb7bd1e, 0xeded93ed3b7ec7d6,
    0xcccc17cc2edb85e2, 0x424215422a578468, 0x98985a98b4c22d2c, 0xa4a4aaa4490e55ed,
    0x2828a0285d885075, 0x5c5c6d5cda31b886, 0xf8f8c7f8933fed6b, 0x8686228644a411c2,
];

/*******************************************************************************

*******************************************************************************/

private static const ulong C1[256] = 
[
    0xd818186018c07830, 0x2623238c2305af46, 0xb8c6c63fc67ef991, 0xfbe8e887e8136fcd,
    0xcb878726874ca113, 0x11b8b8dab8a9626d, 0x0901010401080502, 0x0d4f4f214f426e9e,
    0x9b3636d836adee6c, 0xffa6a6a2a6590451, 0x0cd2d26fd2debdb9, 0x0ef5f5f3f5fb06f7,
    0x967979f979ef80f2, 0x306f6fa16f5fcede, 0x6d91917e91fcef3f, 0xf852525552aa07a4,
    0x4760609d6027fdc0, 0x35bcbccabc897665, 0x379b9b569baccd2b, 0x8a8e8e028e048c01,
    0xd2a3a3b6a371155b, 0x6c0c0c300c603c18, 0x847b7bf17bff8af6, 0x803535d435b5e16a,
    0xf51d1d741de8693a, 0xb3e0e0a7e05347dd, 0x21d7d77bd7f6acb3, 0x9cc2c22fc25eed99,
    0x432e2eb82e6d965c, 0x294b4b314b627a96, 0x5dfefedffea321e1, 0xd5575741578216ae,
    0xbd15155415a8412a, 0xe87777c1779fb6ee, 0x923737dc37a5eb6e, 0x9ee5e5b3e57b56d7,
    0x139f9f469f8cd923, 0x23f0f0e7f0d317fd, 0x204a4a354a6a7f94, 0x44dada4fda9e95a9,
    0xa258587d58fa25b0, 0xcfc9c903c906ca8f, 0x7c2929a429558d52, 0x5a0a0a280a502214,
    0x50b1b1feb1e14f7f, 0xc9a0a0baa0691a5d, 0x146b6bb16b7fdad6, 0xd985852e855cab17,
    0x3cbdbdcebd817367, 0x8f5d5d695dd234ba, 0x9010104010805020, 0x07f4f4f7f4f303f5,
    0xddcbcb0bcb16c08b, 0xd33e3ef83eedc67c, 0x2d0505140528110a, 0x78676781671fe6ce,
    0x97e4e4b7e47353d5, 0x0227279c2725bb4e, 0x7341411941325882, 0xa78b8b168b2c9d0b,
    0xf6a7a7a6a7510153, 0xb27d7de97dcf94fa, 0x4995956e95dcfb37, 0x56d8d847d88e9fad,
    0x70fbfbcbfb8b30eb, 0xcdeeee9fee2371c1, 0xbb7c7ced7cc791f8, 0x716666856617e3cc,
    0x7bdddd53dda68ea7, 0xaf17175c17b84b2e, 0x454747014702468e, 0x1a9e9e429e84dc21,
    0xd4caca0fca1ec589, 0x582d2db42d75995a, 0x2ebfbfc6bf917963, 0x3f07071c07381b0e,
    0xacadad8ead012347, 0xb05a5a755aea2fb4, 0xef838336836cb51b, 0xb63333cc3385ff66,
    0x5c636391633ff2c6, 0x1202020802100a04, 0x93aaaa92aa393849, 0xde7171d971afa8e2,
    0xc6c8c807c80ecf8d, 0xd119196419c87d32, 0x3b49493949727092, 0x5fd9d943d9869aaf,
    0x31f2f2eff2c31df9, 0xa8e3e3abe34b48db, 0xb95b5b715be22ab6, 0xbc88881a8834920d,
    0x3e9a9a529aa4c829, 0x0b262698262dbe4c, 0xbf3232c8328dfa64, 0x59b0b0fab0e94a7d,
    0xf2e9e983e91b6acf, 0x770f0f3c0f78331e, 0x33d5d573d5e6a6b7, 0xf480803a8074ba1d,
    0x27bebec2be997c61, 0xebcdcd13cd26de87, 0x893434d034bde468, 0x3248483d487a7590,
    0x54ffffdbffab24e3, 0x8d7a7af57af78ff4, 0x6490907a90f4ea3d, 0x9d5f5f615fc23ebe,
    0x3d202080201da040, 0x0f6868bd6867d5d0, 0xca1a1a681ad07234, 0xb7aeae82ae192c41,
    0x7db4b4eab4c95e75, 0xce54544d549a19a8, 0x7f93937693ece53b, 0x2f222288220daa44,
    0x6364648d6407e9c8, 0x2af1f1e3f1db12ff, 0xcc7373d173bfa2e6, 0x8212124812905a24,
    0x7a40401d403a5d80, 0x4808082008402810, 0x95c3c32bc356e89b, 0xdfecec97ec337bc5,
    0x4ddbdb4bdb9690ab, 0xc0a1a1bea1611f5f, 0x918d8d0e8d1c8307, 0xc83d3df43df5c97a,
    0x5b97976697ccf133, 0x0000000000000000, 0xf9cfcf1bcf36d483, 0x6e2b2bac2b458756,
    0xe17676c57697b3ec, 0xe68282328264b019, 0x28d6d67fd6fea9b1, 0xc31b1b6c1bd87736,
    0x74b5b5eeb5c15b77, 0xbeafaf86af112943, 0x1d6a6ab56a77dfd4, 0xea50505d50ba0da0,
    0x5745450945124c8a, 0x38f3f3ebf3cb18fb, 0xad3030c0309df060, 0xc4efef9bef2b74c3,
    0xda3f3ffc3fe5c37e, 0xc755554955921caa, 0xdba2a2b2a2791059, 0xe9eaea8fea0365c9,
    0x6a656589650fecca, 0x03babad2bab96869, 0x4a2f2fbc2f65935e, 0x8ec0c027c04ee79d,
    0x60dede5fdebe81a1, 0xfc1c1c701ce06c38, 0x46fdfdd3fdbb2ee7, 0x1f4d4d294d52649a,
    0x7692927292e4e039, 0xfa7575c9758fbcea, 0x3606061806301e0c, 0xae8a8a128a249809,
    0x4bb2b2f2b2f94079, 0x85e6e6bfe66359d1, 0x7e0e0e380e70361c, 0xe71f1f7c1ff8633e,
    0x556262956237f7c4, 0x3ad4d477d4eea3b5, 0x81a8a89aa829324d, 0x5296966296c4f431,
    0x62f9f9c3f99b3aef, 0xa3c5c533c566f697, 0x102525942535b14a, 0xab59597959f220b2,
    0xd084842a8454ae15, 0xc57272d572b7a7e4, 0xec3939e439d5dd72, 0x164c4c2d4c5a6198,
    0x945e5e655eca3bbc, 0x9f7878fd78e785f0, 0xe53838e038ddd870, 0x988c8c0a8c148605,
    0x17d1d163d1c6b2bf, 0xe4a5a5aea5410b57, 0xa1e2e2afe2434dd9, 0x4e616199612ff8c2,
    0x42b3b3f6b3f1457b, 0x342121842115a542, 0x089c9c4a9c94d625, 0xee1e1e781ef0663c,
    0x6143431143225286, 0xb1c7c73bc776fc93, 0x4ffcfcd7fcb32be5, 0x2404041004201408,
    0xe351515951b208a2, 0x2599995e99bcc72f, 0x226d6da96d4fc4da, 0x650d0d340d68391a,
    0x79fafacffa8335e9, 0x69dfdf5bdfb684a3, 0xa97e7ee57ed79bfc, 0x19242490243db448,
    0xfe3b3bec3bc5d776, 0x9aabab96ab313d4b, 0xf0cece1fce3ed181, 0x9911114411885522,
    0x838f8f068f0c8903, 0x044e4e254e4a6b9c, 0x66b7b7e6b7d15173, 0xe0ebeb8beb0b60cb,
    0xc13c3cf03cfdcc78, 0xfd81813e817cbf1f, 0x4094946a94d4fe35, 0x1cf7f7fbf7eb0cf3,
    0x18b9b9deb9a1676f, 0x8b13134c13985f26, 0x512c2cb02c7d9c58, 0x05d3d36bd3d6b8bb,
    0x8ce7e7bbe76b5cd3, 0x396e6ea56e57cbdc, 0xaac4c437c46ef395, 0x1b03030c03180f06,
    0xdc565645568a13ac, 0x5e44440d441a4988, 0xa07f7fe17fdf9efe, 0x88a9a99ea921374f,
    0x672a2aa82a4d8254, 0x0abbbbd6bbb16d6b, 0x87c1c123c146e29f, 0xf153535153a202a6,
    0x72dcdc57dcae8ba5, 0x530b0b2c0b582716, 0x019d9d4e9d9cd327, 0x2b6c6cad6c47c1d8,
    0xa43131c43195f562, 0xf37474cd7487b9e8, 0x15f6f6fff6e309f1, 0x4c464605460a438c,
    0xa5acac8aac092645, 0xb589891e893c970f, 0xb414145014a04428, 0xbae1e1a3e15b42df,
    0xa616165816b04e2c, 0xf73a3ae83acdd274, 0x066969b9696fd0d2, 0x4109092409482d12,
    0xd77070dd70a7ade0, 0x6fb6b6e2b6d95471, 0x1ed0d067d0ceb7bd, 0xd6eded93ed3b7ec7,
    0xe2cccc17cc2edb85, 0x68424215422a5784, 0x2c98985a98b4c22d, 0xeda4a4aaa4490e55,
    0x752828a0285d8850, 0x865c5c6d5cda31b8, 0x6bf8f8c7f8933fed, 0xc28686228644a411,
];

/*******************************************************************************

*******************************************************************************/

private static const ulong C2[256] = 
[
    0x30d818186018c078, 0x462623238c2305af, 0x91b8c6c63fc67ef9, 0xcdfbe8e887e8136f,
    0x13cb878726874ca1, 0x6d11b8b8dab8a962, 0x0209010104010805, 0x9e0d4f4f214f426e,
    0x6c9b3636d836adee, 0x51ffa6a6a2a65904, 0xb90cd2d26fd2debd, 0xf70ef5f5f3f5fb06,
    0xf2967979f979ef80, 0xde306f6fa16f5fce, 0x3f6d91917e91fcef, 0xa4f852525552aa07,
    0xc04760609d6027fd, 0x6535bcbccabc8976, 0x2b379b9b569baccd, 0x018a8e8e028e048c,
    0x5bd2a3a3b6a37115, 0x186c0c0c300c603c, 0xf6847b7bf17bff8a, 0x6a803535d435b5e1,
    0x3af51d1d741de869, 0xddb3e0e0a7e05347, 0xb321d7d77bd7f6ac, 0x999cc2c22fc25eed,
    0x5c432e2eb82e6d96, 0x96294b4b314b627a, 0xe15dfefedffea321, 0xaed5575741578216,
    0x2abd15155415a841, 0xeee87777c1779fb6, 0x6e923737dc37a5eb, 0xd79ee5e5b3e57b56,
    0x23139f9f469f8cd9, 0xfd23f0f0e7f0d317, 0x94204a4a354a6a7f, 0xa944dada4fda9e95,
    0xb0a258587d58fa25, 0x8fcfc9c903c906ca, 0x527c2929a429558d, 0x145a0a0a280a5022,
    0x7f50b1b1feb1e14f, 0x5dc9a0a0baa0691a, 0xd6146b6bb16b7fda, 0x17d985852e855cab,
    0x673cbdbdcebd8173, 0xba8f5d5d695dd234, 0x2090101040108050, 0xf507f4f4f7f4f303,
    0x8bddcbcb0bcb16c0, 0x7cd33e3ef83eedc6, 0x0a2d050514052811, 0xce78676781671fe6,
    0xd597e4e4b7e47353, 0x4e0227279c2725bb, 0x8273414119413258, 0x0ba78b8b168b2c9d,
    0x53f6a7a7a6a75101, 0xfab27d7de97dcf94, 0x374995956e95dcfb, 0xad56d8d847d88e9f,
    0xeb70fbfbcbfb8b30, 0xc1cdeeee9fee2371, 0xf8bb7c7ced7cc791, 0xcc716666856617e3,
    0xa77bdddd53dda68e, 0x2eaf17175c17b84b, 0x8e45474701470246, 0x211a9e9e429e84dc,
    0x89d4caca0fca1ec5, 0x5a582d2db42d7599, 0x632ebfbfc6bf9179, 0x0e3f07071c07381b,
    0x47acadad8ead0123, 0xb4b05a5a755aea2f, 0x1bef838336836cb5, 0x66b63333cc3385ff,
    0xc65c636391633ff2, 0x041202020802100a, 0x4993aaaa92aa3938, 0xe2de7171d971afa8,
    0x8dc6c8c807c80ecf, 0x32d119196419c87d, 0x923b494939497270, 0xaf5fd9d943d9869a,
    0xf931f2f2eff2c31d, 0xdba8e3e3abe34b48, 0xb6b95b5b715be22a, 0x0dbc88881a883492,
    0x293e9a9a529aa4c8, 0x4c0b262698262dbe, 0x64bf3232c8328dfa, 0x7d59b0b0fab0e94a,
    0xcff2e9e983e91b6a, 0x1e770f0f3c0f7833, 0xb733d5d573d5e6a6, 0x1df480803a8074ba,
    0x6127bebec2be997c, 0x87ebcdcd13cd26de, 0x68893434d034bde4, 0x903248483d487a75,
    0xe354ffffdbffab24, 0xf48d7a7af57af78f, 0x3d6490907a90f4ea, 0xbe9d5f5f615fc23e,
    0x403d202080201da0, 0xd00f6868bd6867d5, 0x34ca1a1a681ad072, 0x41b7aeae82ae192c,
    0x757db4b4eab4c95e, 0xa8ce54544d549a19, 0x3b7f93937693ece5, 0x442f222288220daa,
    0xc86364648d6407e9, 0xff2af1f1e3f1db12, 0xe6cc7373d173bfa2, 0x248212124812905a,
    0x807a40401d403a5d, 0x1048080820084028, 0x9b95c3c32bc356e8, 0xc5dfecec97ec337b,
    0xab4ddbdb4bdb9690, 0x5fc0a1a1bea1611f, 0x07918d8d0e8d1c83, 0x7ac83d3df43df5c9,
    0x335b97976697ccf1, 0x0000000000000000, 0x83f9cfcf1bcf36d4, 0x566e2b2bac2b4587,
    0xece17676c57697b3, 0x19e68282328264b0, 0xb128d6d67fd6fea9, 0x36c31b1b6c1bd877,
    0x7774b5b5eeb5c15b, 0x43beafaf86af1129, 0xd41d6a6ab56a77df, 0xa0ea50505d50ba0d,
    0x8a5745450945124c, 0xfb38f3f3ebf3cb18, 0x60ad3030c0309df0, 0xc3c4efef9bef2b74,
    0x7eda3f3ffc3fe5c3, 0xaac755554955921c, 0x59dba2a2b2a27910, 0xc9e9eaea8fea0365,
    0xca6a656589650fec, 0x6903babad2bab968, 0x5e4a2f2fbc2f6593, 0x9d8ec0c027c04ee7,
    0xa160dede5fdebe81, 0x38fc1c1c701ce06c, 0xe746fdfdd3fdbb2e, 0x9a1f4d4d294d5264,
    0x397692927292e4e0, 0xeafa7575c9758fbc, 0x0c3606061806301e, 0x09ae8a8a128a2498,
    0x794bb2b2f2b2f940, 0xd185e6e6bfe66359, 0x1c7e0e0e380e7036, 0x3ee71f1f7c1ff863,
    0xc4556262956237f7, 0xb53ad4d477d4eea3, 0x4d81a8a89aa82932, 0x315296966296c4f4,
    0xef62f9f9c3f99b3a, 0x97a3c5c533c566f6, 0x4a102525942535b1, 0xb2ab59597959f220,
    0x15d084842a8454ae, 0xe4c57272d572b7a7, 0x72ec3939e439d5dd, 0x98164c4c2d4c5a61,
    0xbc945e5e655eca3b, 0xf09f7878fd78e785, 0x70e53838e038ddd8, 0x05988c8c0a8c1486,
    0xbf17d1d163d1c6b2, 0x57e4a5a5aea5410b, 0xd9a1e2e2afe2434d, 0xc24e616199612ff8,
    0x7b42b3b3f6b3f145, 0x42342121842115a5, 0x25089c9c4a9c94d6, 0x3cee1e1e781ef066,
    0x8661434311432252, 0x93b1c7c73bc776fc, 0xe54ffcfcd7fcb32b, 0x0824040410042014,
    0xa2e351515951b208, 0x2f2599995e99bcc7, 0xda226d6da96d4fc4, 0x1a650d0d340d6839,
    0xe979fafacffa8335, 0xa369dfdf5bdfb684, 0xfca97e7ee57ed79b, 0x4819242490243db4,
    0x76fe3b3bec3bc5d7, 0x4b9aabab96ab313d, 0x81f0cece1fce3ed1, 0x2299111144118855,
    0x03838f8f068f0c89, 0x9c044e4e254e4a6b, 0x7366b7b7e6b7d151, 0xcbe0ebeb8beb0b60,
    0x78c13c3cf03cfdcc, 0x1ffd81813e817cbf, 0x354094946a94d4fe, 0xf31cf7f7fbf7eb0c,
    0x6f18b9b9deb9a167, 0x268b13134c13985f, 0x58512c2cb02c7d9c, 0xbb05d3d36bd3d6b8,
    0xd38ce7e7bbe76b5c, 0xdc396e6ea56e57cb, 0x95aac4c437c46ef3, 0x061b03030c03180f,
    0xacdc565645568a13, 0x885e44440d441a49, 0xfea07f7fe17fdf9e, 0x4f88a9a99ea92137,
    0x54672a2aa82a4d82, 0x6b0abbbbd6bbb16d, 0x9f87c1c123c146e2, 0xa6f153535153a202,
    0xa572dcdc57dcae8b, 0x16530b0b2c0b5827, 0x27019d9d4e9d9cd3, 0xd82b6c6cad6c47c1,
    0x62a43131c43195f5, 0xe8f37474cd7487b9, 0xf115f6f6fff6e309, 0x8c4c464605460a43,
    0x45a5acac8aac0926, 0x0fb589891e893c97, 0x28b414145014a044, 0xdfbae1e1a3e15b42,
    0x2ca616165816b04e, 0x74f73a3ae83acdd2, 0xd2066969b9696fd0, 0x124109092409482d,
    0xe0d77070dd70a7ad, 0x716fb6b6e2b6d954, 0xbd1ed0d067d0ceb7, 0xc7d6eded93ed3b7e,
    0x85e2cccc17cc2edb, 0x8468424215422a57, 0x2d2c98985a98b4c2, 0x55eda4a4aaa4490e,
    0x50752828a0285d88, 0xb8865c5c6d5cda31, 0xed6bf8f8c7f8933f, 0x11c28686228644a4,
];

/*******************************************************************************

*******************************************************************************/

private static const ulong C3[256] = 
[
    0x7830d818186018c0, 0xaf462623238c2305, 0xf991b8c6c63fc67e, 0x6fcdfbe8e887e813,
    0xa113cb878726874c, 0x626d11b8b8dab8a9, 0x0502090101040108, 0x6e9e0d4f4f214f42,
    0xee6c9b3636d836ad, 0x0451ffa6a6a2a659, 0xbdb90cd2d26fd2de, 0x06f70ef5f5f3f5fb,
    0x80f2967979f979ef, 0xcede306f6fa16f5f, 0xef3f6d91917e91fc, 0x07a4f852525552aa,
    0xfdc04760609d6027, 0x766535bcbccabc89, 0xcd2b379b9b569bac, 0x8c018a8e8e028e04,
    0x155bd2a3a3b6a371, 0x3c186c0c0c300c60, 0x8af6847b7bf17bff, 0xe16a803535d435b5,
    0x693af51d1d741de8, 0x47ddb3e0e0a7e053, 0xacb321d7d77bd7f6, 0xed999cc2c22fc25e,
    0x965c432e2eb82e6d, 0x7a96294b4b314b62, 0x21e15dfefedffea3, 0x16aed55757415782,
    0x412abd15155415a8, 0xb6eee87777c1779f, 0xeb6e923737dc37a5, 0x56d79ee5e5b3e57b,
    0xd923139f9f469f8c, 0x17fd23f0f0e7f0d3, 0x7f94204a4a354a6a, 0x95a944dada4fda9e,
    0x25b0a258587d58fa, 0xca8fcfc9c903c906, 0x8d527c2929a42955, 0x22145a0a0a280a50,
    0x4f7f50b1b1feb1e1, 0x1a5dc9a0a0baa069, 0xdad6146b6bb16b7f, 0xab17d985852e855c,
    0x73673cbdbdcebd81, 0x34ba8f5d5d695dd2, 0x5020901010401080, 0x03f507f4f4f7f4f3,
    0xc08bddcbcb0bcb16, 0xc67cd33e3ef83eed, 0x110a2d0505140528, 0xe6ce78676781671f,
    0x53d597e4e4b7e473, 0xbb4e0227279c2725, 0x5882734141194132, 0x9d0ba78b8b168b2c,
    0x0153f6a7a7a6a751, 0x94fab27d7de97dcf, 0xfb374995956e95dc, 0x9fad56d8d847d88e,
    0x30eb70fbfbcbfb8b, 0x71c1cdeeee9fee23, 0x91f8bb7c7ced7cc7, 0xe3cc716666856617,
    0x8ea77bdddd53dda6, 0x4b2eaf17175c17b8, 0x468e454747014702, 0xdc211a9e9e429e84,
    0xc589d4caca0fca1e, 0x995a582d2db42d75, 0x79632ebfbfc6bf91, 0x1b0e3f07071c0738,
    0x2347acadad8ead01, 0x2fb4b05a5a755aea, 0xb51bef838336836c, 0xff66b63333cc3385,
    0xf2c65c636391633f, 0x0a04120202080210, 0x384993aaaa92aa39, 0xa8e2de7171d971af,
    0xcf8dc6c8c807c80e, 0x7d32d119196419c8, 0x70923b4949394972, 0x9aaf5fd9d943d986,
    0x1df931f2f2eff2c3, 0x48dba8e3e3abe34b, 0x2ab6b95b5b715be2, 0x920dbc88881a8834,
    0xc8293e9a9a529aa4, 0xbe4c0b262698262d, 0xfa64bf3232c8328d, 0x4a7d59b0b0fab0e9,
    0x6acff2e9e983e91b, 0x331e770f0f3c0f78, 0xa6b733d5d573d5e6, 0xba1df480803a8074,
    0x7c6127bebec2be99, 0xde87ebcdcd13cd26, 0xe468893434d034bd, 0x75903248483d487a,
    0x24e354ffffdbffab, 0x8ff48d7a7af57af7, 0xea3d6490907a90f4, 0x3ebe9d5f5f615fc2,
    0xa0403d202080201d, 0xd5d00f6868bd6867, 0x7234ca1a1a681ad0, 0x2c41b7aeae82ae19,
    0x5e757db4b4eab4c9, 0x19a8ce54544d549a, 0xe53b7f93937693ec, 0xaa442f222288220d,
    0xe9c86364648d6407, 0x12ff2af1f1e3f1db, 0xa2e6cc7373d173bf, 0x5a24821212481290,
    0x5d807a40401d403a, 0x2810480808200840, 0xe89b95c3c32bc356, 0x7bc5dfecec97ec33,
    0x90ab4ddbdb4bdb96, 0x1f5fc0a1a1bea161, 0x8307918d8d0e8d1c, 0xc97ac83d3df43df5,
    0xf1335b97976697cc, 0x0000000000000000, 0xd483f9cfcf1bcf36, 0x87566e2b2bac2b45,
    0xb3ece17676c57697, 0xb019e68282328264, 0xa9b128d6d67fd6fe, 0x7736c31b1b6c1bd8,
    0x5b7774b5b5eeb5c1, 0x2943beafaf86af11, 0xdfd41d6a6ab56a77, 0x0da0ea50505d50ba,
    0x4c8a574545094512, 0x18fb38f3f3ebf3cb, 0xf060ad3030c0309d, 0x74c3c4efef9bef2b,
    0xc37eda3f3ffc3fe5, 0x1caac75555495592, 0x1059dba2a2b2a279, 0x65c9e9eaea8fea03,
    0xecca6a656589650f, 0x686903babad2bab9, 0x935e4a2f2fbc2f65, 0xe79d8ec0c027c04e,
    0x81a160dede5fdebe, 0x6c38fc1c1c701ce0, 0x2ee746fdfdd3fdbb, 0x649a1f4d4d294d52,
    0xe0397692927292e4, 0xbceafa7575c9758f, 0x1e0c360606180630, 0x9809ae8a8a128a24,
    0x40794bb2b2f2b2f9, 0x59d185e6e6bfe663, 0x361c7e0e0e380e70, 0x633ee71f1f7c1ff8,
    0xf7c4556262956237, 0xa3b53ad4d477d4ee, 0x324d81a8a89aa829, 0xf4315296966296c4,
    0x3aef62f9f9c3f99b, 0xf697a3c5c533c566, 0xb14a102525942535, 0x20b2ab59597959f2,
    0xae15d084842a8454, 0xa7e4c57272d572b7, 0xdd72ec3939e439d5, 0x6198164c4c2d4c5a,
    0x3bbc945e5e655eca, 0x85f09f7878fd78e7, 0xd870e53838e038dd, 0x8605988c8c0a8c14,
    0xb2bf17d1d163d1c6, 0x0b57e4a5a5aea541, 0x4dd9a1e2e2afe243, 0xf8c24e616199612f,
    0x457b42b3b3f6b3f1, 0xa542342121842115, 0xd625089c9c4a9c94, 0x663cee1e1e781ef0,
    0x5286614343114322, 0xfc93b1c7c73bc776, 0x2be54ffcfcd7fcb3, 0x1408240404100420,
    0x08a2e351515951b2, 0xc72f2599995e99bc, 0xc4da226d6da96d4f, 0x391a650d0d340d68,
    0x35e979fafacffa83, 0x84a369dfdf5bdfb6, 0x9bfca97e7ee57ed7, 0xb44819242490243d,
    0xd776fe3b3bec3bc5, 0x3d4b9aabab96ab31, 0xd181f0cece1fce3e, 0x5522991111441188,
    0x8903838f8f068f0c, 0x6b9c044e4e254e4a, 0x517366b7b7e6b7d1, 0x60cbe0ebeb8beb0b,
    0xcc78c13c3cf03cfd, 0xbf1ffd81813e817c, 0xfe354094946a94d4, 0x0cf31cf7f7fbf7eb,
    0x676f18b9b9deb9a1, 0x5f268b13134c1398, 0x9c58512c2cb02c7d, 0xb8bb05d3d36bd3d6,
    0x5cd38ce7e7bbe76b, 0xcbdc396e6ea56e57, 0xf395aac4c437c46e, 0x0f061b03030c0318,
    0x13acdc565645568a, 0x49885e44440d441a, 0x9efea07f7fe17fdf, 0x374f88a9a99ea921,
    0x8254672a2aa82a4d, 0x6d6b0abbbbd6bbb1, 0xe29f87c1c123c146, 0x02a6f153535153a2,
    0x8ba572dcdc57dcae, 0x2716530b0b2c0b58, 0xd327019d9d4e9d9c, 0xc1d82b6c6cad6c47,
    0xf562a43131c43195, 0xb9e8f37474cd7487, 0x09f115f6f6fff6e3, 0x438c4c464605460a,
    0x2645a5acac8aac09, 0x970fb589891e893c, 0x4428b414145014a0, 0x42dfbae1e1a3e15b,
    0x4e2ca616165816b0, 0xd274f73a3ae83acd, 0xd0d2066969b9696f, 0x2d12410909240948,
    0xade0d77070dd70a7, 0x54716fb6b6e2b6d9, 0xb7bd1ed0d067d0ce, 0x7ec7d6eded93ed3b,
    0xdb85e2cccc17cc2e, 0x578468424215422a, 0xc22d2c98985a98b4, 0x0e55eda4a4aaa449,
    0x8850752828a0285d, 0x31b8865c5c6d5cda, 0x3fed6bf8f8c7f893, 0xa411c28686228644,
];

/*******************************************************************************

*******************************************************************************/

private static const ulong C4[256] = 
[
    0xc07830d818186018, 0x05af462623238c23, 0x7ef991b8c6c63fc6, 0x136fcdfbe8e887e8,
    0x4ca113cb87872687, 0xa9626d11b8b8dab8, 0x0805020901010401, 0x426e9e0d4f4f214f,
    0xadee6c9b3636d836, 0x590451ffa6a6a2a6, 0xdebdb90cd2d26fd2, 0xfb06f70ef5f5f3f5,
    0xef80f2967979f979, 0x5fcede306f6fa16f, 0xfcef3f6d91917e91, 0xaa07a4f852525552,
    0x27fdc04760609d60, 0x89766535bcbccabc, 0xaccd2b379b9b569b, 0x048c018a8e8e028e,
    0x71155bd2a3a3b6a3, 0x603c186c0c0c300c, 0xff8af6847b7bf17b, 0xb5e16a803535d435,
    0xe8693af51d1d741d, 0x5347ddb3e0e0a7e0, 0xf6acb321d7d77bd7, 0x5eed999cc2c22fc2,
    0x6d965c432e2eb82e, 0x627a96294b4b314b, 0xa321e15dfefedffe, 0x8216aed557574157,
    0xa8412abd15155415, 0x9fb6eee87777c177, 0xa5eb6e923737dc37, 0x7b56d79ee5e5b3e5,
    0x8cd923139f9f469f, 0xd317fd23f0f0e7f0, 0x6a7f94204a4a354a, 0x9e95a944dada4fda,
    0xfa25b0a258587d58, 0x06ca8fcfc9c903c9, 0x558d527c2929a429, 0x5022145a0a0a280a,
    0xe14f7f50b1b1feb1, 0x691a5dc9a0a0baa0, 0x7fdad6146b6bb16b, 0x5cab17d985852e85,
    0x8173673cbdbdcebd, 0xd234ba8f5d5d695d, 0x8050209010104010, 0xf303f507f4f4f7f4,
    0x16c08bddcbcb0bcb, 0xedc67cd33e3ef83e, 0x28110a2d05051405, 0x1fe6ce7867678167,
    0x7353d597e4e4b7e4, 0x25bb4e0227279c27, 0x3258827341411941, 0x2c9d0ba78b8b168b,
    0x510153f6a7a7a6a7, 0xcf94fab27d7de97d, 0xdcfb374995956e95, 0x8e9fad56d8d847d8,
    0x8b30eb70fbfbcbfb, 0x2371c1cdeeee9fee, 0xc791f8bb7c7ced7c, 0x17e3cc7166668566,
    0xa68ea77bdddd53dd, 0xb84b2eaf17175c17, 0x02468e4547470147, 0x84dc211a9e9e429e,
    0x1ec589d4caca0fca, 0x75995a582d2db42d, 0x9179632ebfbfc6bf, 0x381b0e3f07071c07,
    0x012347acadad8ead, 0xea2fb4b05a5a755a, 0x6cb51bef83833683, 0x85ff66b63333cc33,
    0x3ff2c65c63639163, 0x100a041202020802, 0x39384993aaaa92aa, 0xafa8e2de7171d971,
    0x0ecf8dc6c8c807c8, 0xc87d32d119196419, 0x7270923b49493949, 0x869aaf5fd9d943d9,
    0xc31df931f2f2eff2, 0x4b48dba8e3e3abe3, 0xe22ab6b95b5b715b, 0x34920dbc88881a88,
    0xa4c8293e9a9a529a, 0x2dbe4c0b26269826, 0x8dfa64bf3232c832, 0xe94a7d59b0b0fab0,
    0x1b6acff2e9e983e9, 0x78331e770f0f3c0f, 0xe6a6b733d5d573d5, 0x74ba1df480803a80,
    0x997c6127bebec2be, 0x26de87ebcdcd13cd, 0xbde468893434d034, 0x7a75903248483d48,
    0xab24e354ffffdbff, 0xf78ff48d7a7af57a, 0xf4ea3d6490907a90, 0xc23ebe9d5f5f615f,
    0x1da0403d20208020, 0x67d5d00f6868bd68, 0xd07234ca1a1a681a, 0x192c41b7aeae82ae,
    0xc95e757db4b4eab4, 0x9a19a8ce54544d54, 0xece53b7f93937693, 0x0daa442f22228822,
    0x07e9c86364648d64, 0xdb12ff2af1f1e3f1, 0xbfa2e6cc7373d173, 0x905a248212124812,
    0x3a5d807a40401d40, 0x4028104808082008, 0x56e89b95c3c32bc3, 0x337bc5dfecec97ec,
    0x9690ab4ddbdb4bdb, 0x611f5fc0a1a1bea1, 0x1c8307918d8d0e8d, 0xf5c97ac83d3df43d,
    0xccf1335b97976697, 0x0000000000000000, 0x36d483f9cfcf1bcf, 0x4587566e2b2bac2b,
    0x97b3ece17676c576, 0x64b019e682823282, 0xfea9b128d6d67fd6, 0xd87736c31b1b6c1b,
    0xc15b7774b5b5eeb5, 0x112943beafaf86af, 0x77dfd41d6a6ab56a, 0xba0da0ea50505d50,
    0x124c8a5745450945, 0xcb18fb38f3f3ebf3, 0x9df060ad3030c030, 0x2b74c3c4efef9bef,
    0xe5c37eda3f3ffc3f, 0x921caac755554955, 0x791059dba2a2b2a2, 0x0365c9e9eaea8fea,
    0x0fecca6a65658965, 0xb9686903babad2ba, 0x65935e4a2f2fbc2f, 0x4ee79d8ec0c027c0,
    0xbe81a160dede5fde, 0xe06c38fc1c1c701c, 0xbb2ee746fdfdd3fd, 0x52649a1f4d4d294d,
    0xe4e0397692927292, 0x8fbceafa7575c975, 0x301e0c3606061806, 0x249809ae8a8a128a,
    0xf940794bb2b2f2b2, 0x6359d185e6e6bfe6, 0x70361c7e0e0e380e, 0xf8633ee71f1f7c1f,
    0x37f7c45562629562, 0xeea3b53ad4d477d4, 0x29324d81a8a89aa8, 0xc4f4315296966296,
    0x9b3aef62f9f9c3f9, 0x66f697a3c5c533c5, 0x35b14a1025259425, 0xf220b2ab59597959,
    0x54ae15d084842a84, 0xb7a7e4c57272d572, 0xd5dd72ec3939e439, 0x5a6198164c4c2d4c,
    0xca3bbc945e5e655e, 0xe785f09f7878fd78, 0xddd870e53838e038, 0x148605988c8c0a8c,
    0xc6b2bf17d1d163d1, 0x410b57e4a5a5aea5, 0x434dd9a1e2e2afe2, 0x2ff8c24e61619961,
    0xf1457b42b3b3f6b3, 0x15a5423421218421, 0x94d625089c9c4a9c, 0xf0663cee1e1e781e,
    0x2252866143431143, 0x76fc93b1c7c73bc7, 0xb32be54ffcfcd7fc, 0x2014082404041004,
    0xb208a2e351515951, 0xbcc72f2599995e99, 0x4fc4da226d6da96d, 0x68391a650d0d340d,
    0x8335e979fafacffa, 0xb684a369dfdf5bdf, 0xd79bfca97e7ee57e, 0x3db4481924249024,
    0xc5d776fe3b3bec3b, 0x313d4b9aabab96ab, 0x3ed181f0cece1fce, 0x8855229911114411,
    0x0c8903838f8f068f, 0x4a6b9c044e4e254e, 0xd1517366b7b7e6b7, 0x0b60cbe0ebeb8beb,
    0xfdcc78c13c3cf03c, 0x7cbf1ffd81813e81, 0xd4fe354094946a94, 0xeb0cf31cf7f7fbf7,
    0xa1676f18b9b9deb9, 0x985f268b13134c13, 0x7d9c58512c2cb02c, 0xd6b8bb05d3d36bd3,
    0x6b5cd38ce7e7bbe7, 0x57cbdc396e6ea56e, 0x6ef395aac4c437c4, 0x180f061b03030c03,
    0x8a13acdc56564556, 0x1a49885e44440d44, 0xdf9efea07f7fe17f, 0x21374f88a9a99ea9,
    0x4d8254672a2aa82a, 0xb16d6b0abbbbd6bb, 0x46e29f87c1c123c1, 0xa202a6f153535153,
    0xae8ba572dcdc57dc, 0x582716530b0b2c0b, 0x9cd327019d9d4e9d, 0x47c1d82b6c6cad6c,
    0x95f562a43131c431, 0x87b9e8f37474cd74, 0xe309f115f6f6fff6, 0x0a438c4c46460546,
    0x092645a5acac8aac, 0x3c970fb589891e89, 0xa04428b414145014, 0x5b42dfbae1e1a3e1,
    0xb04e2ca616165816, 0xcdd274f73a3ae83a, 0x6fd0d2066969b969, 0x482d124109092409,
    0xa7ade0d77070dd70, 0xd954716fb6b6e2b6, 0xceb7bd1ed0d067d0, 0x3b7ec7d6eded93ed,
    0x2edb85e2cccc17cc, 0x2a57846842421542, 0xb4c22d2c98985a98, 0x490e55eda4a4aaa4,
    0x5d8850752828a028, 0xda31b8865c5c6d5c, 0x933fed6bf8f8c7f8, 0x44a411c286862286,
];

/*******************************************************************************

*******************************************************************************/

private static const ulong C5[256] = 
[
    0x18c07830d8181860, 0x2305af462623238c, 0xc67ef991b8c6c63f, 0xe8136fcdfbe8e887,
    0x874ca113cb878726, 0xb8a9626d11b8b8da, 0x0108050209010104, 0x4f426e9e0d4f4f21,
    0x36adee6c9b3636d8, 0xa6590451ffa6a6a2, 0xd2debdb90cd2d26f, 0xf5fb06f70ef5f5f3,
    0x79ef80f2967979f9, 0x6f5fcede306f6fa1, 0x91fcef3f6d91917e, 0x52aa07a4f8525255,
    0x6027fdc04760609d, 0xbc89766535bcbcca, 0x9baccd2b379b9b56, 0x8e048c018a8e8e02,
    0xa371155bd2a3a3b6, 0x0c603c186c0c0c30, 0x7bff8af6847b7bf1, 0x35b5e16a803535d4,
    0x1de8693af51d1d74, 0xe05347ddb3e0e0a7, 0xd7f6acb321d7d77b, 0xc25eed999cc2c22f,
    0x2e6d965c432e2eb8, 0x4b627a96294b4b31, 0xfea321e15dfefedf, 0x578216aed5575741,
    0x15a8412abd151554, 0x779fb6eee87777c1, 0x37a5eb6e923737dc, 0xe57b56d79ee5e5b3,
    0x9f8cd923139f9f46, 0xf0d317fd23f0f0e7, 0x4a6a7f94204a4a35, 0xda9e95a944dada4f,
    0x58fa25b0a258587d, 0xc906ca8fcfc9c903, 0x29558d527c2929a4, 0x0a5022145a0a0a28,
    0xb1e14f7f50b1b1fe, 0xa0691a5dc9a0a0ba, 0x6b7fdad6146b6bb1, 0x855cab17d985852e,
    0xbd8173673cbdbdce, 0x5dd234ba8f5d5d69, 0x1080502090101040, 0xf4f303f507f4f4f7,
    0xcb16c08bddcbcb0b, 0x3eedc67cd33e3ef8, 0x0528110a2d050514, 0x671fe6ce78676781,
    0xe47353d597e4e4b7, 0x2725bb4e0227279c, 0x4132588273414119, 0x8b2c9d0ba78b8b16,
    0xa7510153f6a7a7a6, 0x7dcf94fab27d7de9, 0x95dcfb374995956e, 0xd88e9fad56d8d847,
    0xfb8b30eb70fbfbcb, 0xee2371c1cdeeee9f, 0x7cc791f8bb7c7ced, 0x6617e3cc71666685,
    0xdda68ea77bdddd53, 0x17b84b2eaf17175c, 0x4702468e45474701, 0x9e84dc211a9e9e42,
    0xca1ec589d4caca0f, 0x2d75995a582d2db4, 0xbf9179632ebfbfc6, 0x07381b0e3f07071c,
    0xad012347acadad8e, 0x5aea2fb4b05a5a75, 0x836cb51bef838336, 0x3385ff66b63333cc,
    0x633ff2c65c636391, 0x02100a0412020208, 0xaa39384993aaaa92, 0x71afa8e2de7171d9,
    0xc80ecf8dc6c8c807, 0x19c87d32d1191964, 0x497270923b494939, 0xd9869aaf5fd9d943,
    0xf2c31df931f2f2ef, 0xe34b48dba8e3e3ab, 0x5be22ab6b95b5b71, 0x8834920dbc88881a,
    0x9aa4c8293e9a9a52, 0x262dbe4c0b262698, 0x328dfa64bf3232c8, 0xb0e94a7d59b0b0fa,
    0xe91b6acff2e9e983, 0x0f78331e770f0f3c, 0xd5e6a6b733d5d573, 0x8074ba1df480803a,
    0xbe997c6127bebec2, 0xcd26de87ebcdcd13, 0x34bde468893434d0, 0x487a75903248483d,
    0xffab24e354ffffdb, 0x7af78ff48d7a7af5, 0x90f4ea3d6490907a, 0x5fc23ebe9d5f5f61,
    0x201da0403d202080, 0x6867d5d00f6868bd, 0x1ad07234ca1a1a68, 0xae192c41b7aeae82,
    0xb4c95e757db4b4ea, 0x549a19a8ce54544d, 0x93ece53b7f939376, 0x220daa442f222288,
    0x6407e9c86364648d, 0xf1db12ff2af1f1e3, 0x73bfa2e6cc7373d1, 0x12905a2482121248,
    0x403a5d807a40401d, 0x0840281048080820, 0xc356e89b95c3c32b, 0xec337bc5dfecec97,
    0xdb9690ab4ddbdb4b, 0xa1611f5fc0a1a1be, 0x8d1c8307918d8d0e, 0x3df5c97ac83d3df4,
    0x97ccf1335b979766, 0x0000000000000000, 0xcf36d483f9cfcf1b, 0x2b4587566e2b2bac,
    0x7697b3ece17676c5, 0x8264b019e6828232, 0xd6fea9b128d6d67f, 0x1bd87736c31b1b6c,
    0xb5c15b7774b5b5ee, 0xaf112943beafaf86, 0x6a77dfd41d6a6ab5, 0x50ba0da0ea50505d,
    0x45124c8a57454509, 0xf3cb18fb38f3f3eb, 0x309df060ad3030c0, 0xef2b74c3c4efef9b,
    0x3fe5c37eda3f3ffc, 0x55921caac7555549, 0xa2791059dba2a2b2, 0xea0365c9e9eaea8f,
    0x650fecca6a656589, 0xbab9686903babad2, 0x2f65935e4a2f2fbc, 0xc04ee79d8ec0c027,
    0xdebe81a160dede5f, 0x1ce06c38fc1c1c70, 0xfdbb2ee746fdfdd3, 0x4d52649a1f4d4d29,
    0x92e4e03976929272, 0x758fbceafa7575c9, 0x06301e0c36060618, 0x8a249809ae8a8a12,
    0xb2f940794bb2b2f2, 0xe66359d185e6e6bf, 0x0e70361c7e0e0e38, 0x1ff8633ee71f1f7c,
    0x6237f7c455626295, 0xd4eea3b53ad4d477, 0xa829324d81a8a89a, 0x96c4f43152969662,
    0xf99b3aef62f9f9c3, 0xc566f697a3c5c533, 0x2535b14a10252594, 0x59f220b2ab595979,
    0x8454ae15d084842a, 0x72b7a7e4c57272d5, 0x39d5dd72ec3939e4, 0x4c5a6198164c4c2d,
    0x5eca3bbc945e5e65, 0x78e785f09f7878fd, 0x38ddd870e53838e0, 0x8c148605988c8c0a,
    0xd1c6b2bf17d1d163, 0xa5410b57e4a5a5ae, 0xe2434dd9a1e2e2af, 0x612ff8c24e616199,
    0xb3f1457b42b3b3f6, 0x2115a54234212184, 0x9c94d625089c9c4a, 0x1ef0663cee1e1e78,
    0x4322528661434311, 0xc776fc93b1c7c73b, 0xfcb32be54ffcfcd7, 0x0420140824040410,
    0x51b208a2e3515159, 0x99bcc72f2599995e, 0x6d4fc4da226d6da9, 0x0d68391a650d0d34,
    0xfa8335e979fafacf, 0xdfb684a369dfdf5b, 0x7ed79bfca97e7ee5, 0x243db44819242490,
    0x3bc5d776fe3b3bec, 0xab313d4b9aabab96, 0xce3ed181f0cece1f, 0x1188552299111144,
    0x8f0c8903838f8f06, 0x4e4a6b9c044e4e25, 0xb7d1517366b7b7e6, 0xeb0b60cbe0ebeb8b,
    0x3cfdcc78c13c3cf0, 0x817cbf1ffd81813e, 0x94d4fe354094946a, 0xf7eb0cf31cf7f7fb,
    0xb9a1676f18b9b9de, 0x13985f268b13134c, 0x2c7d9c58512c2cb0, 0xd3d6b8bb05d3d36b,
    0xe76b5cd38ce7e7bb, 0x6e57cbdc396e6ea5, 0xc46ef395aac4c437, 0x03180f061b03030c,
    0x568a13acdc565645, 0x441a49885e44440d, 0x7fdf9efea07f7fe1, 0xa921374f88a9a99e,
    0x2a4d8254672a2aa8, 0xbbb16d6b0abbbbd6, 0xc146e29f87c1c123, 0x53a202a6f1535351,
    0xdcae8ba572dcdc57, 0x0b582716530b0b2c, 0x9d9cd327019d9d4e, 0x6c47c1d82b6c6cad,
    0x3195f562a43131c4, 0x7487b9e8f37474cd, 0xf6e309f115f6f6ff, 0x460a438c4c464605,
    0xac092645a5acac8a, 0x893c970fb589891e, 0x14a04428b4141450, 0xe15b42dfbae1e1a3,
    0x16b04e2ca6161658, 0x3acdd274f73a3ae8, 0x696fd0d2066969b9, 0x09482d1241090924,
    0x70a7ade0d77070dd, 0xb6d954716fb6b6e2, 0xd0ceb7bd1ed0d067, 0xed3b7ec7d6eded93,
    0xcc2edb85e2cccc17, 0x422a578468424215, 0x98b4c22d2c98985a, 0xa4490e55eda4a4aa,
    0x285d8850752828a0, 0x5cda31b8865c5c6d, 0xf8933fed6bf8f8c7, 0x8644a411c2868622,
];

/*******************************************************************************

*******************************************************************************/

private static const ulong C6[256] = 
[
    0x6018c07830d81818, 0x8c2305af46262323, 0x3fc67ef991b8c6c6, 0x87e8136fcdfbe8e8,
    0x26874ca113cb8787, 0xdab8a9626d11b8b8, 0x0401080502090101, 0x214f426e9e0d4f4f,
    0xd836adee6c9b3636, 0xa2a6590451ffa6a6, 0x6fd2debdb90cd2d2, 0xf3f5fb06f70ef5f5,
    0xf979ef80f2967979, 0xa16f5fcede306f6f, 0x7e91fcef3f6d9191, 0x5552aa07a4f85252,
    0x9d6027fdc0476060, 0xcabc89766535bcbc, 0x569baccd2b379b9b, 0x028e048c018a8e8e,
    0xb6a371155bd2a3a3, 0x300c603c186c0c0c, 0xf17bff8af6847b7b, 0xd435b5e16a803535,
    0x741de8693af51d1d, 0xa7e05347ddb3e0e0, 0x7bd7f6acb321d7d7, 0x2fc25eed999cc2c2,
    0xb82e6d965c432e2e, 0x314b627a96294b4b, 0xdffea321e15dfefe, 0x41578216aed55757,
    0x5415a8412abd1515, 0xc1779fb6eee87777, 0xdc37a5eb6e923737, 0xb3e57b56d79ee5e5,
    0x469f8cd923139f9f, 0xe7f0d317fd23f0f0, 0x354a6a7f94204a4a, 0x4fda9e95a944dada,
    0x7d58fa25b0a25858, 0x03c906ca8fcfc9c9, 0xa429558d527c2929, 0x280a5022145a0a0a,
    0xfeb1e14f7f50b1b1, 0xbaa0691a5dc9a0a0, 0xb16b7fdad6146b6b, 0x2e855cab17d98585,
    0xcebd8173673cbdbd, 0x695dd234ba8f5d5d, 0x4010805020901010, 0xf7f4f303f507f4f4,
    0x0bcb16c08bddcbcb, 0xf83eedc67cd33e3e, 0x140528110a2d0505, 0x81671fe6ce786767,
    0xb7e47353d597e4e4, 0x9c2725bb4e022727, 0x1941325882734141, 0x168b2c9d0ba78b8b,
    0xa6a7510153f6a7a7, 0xe97dcf94fab27d7d, 0x6e95dcfb37499595, 0x47d88e9fad56d8d8,
    0xcbfb8b30eb70fbfb, 0x9fee2371c1cdeeee, 0xed7cc791f8bb7c7c, 0x856617e3cc716666,
    0x53dda68ea77bdddd, 0x5c17b84b2eaf1717, 0x014702468e454747, 0x429e84dc211a9e9e,
    0x0fca1ec589d4caca, 0xb42d75995a582d2d, 0xc6bf9179632ebfbf, 0x1c07381b0e3f0707,
    0x8ead012347acadad, 0x755aea2fb4b05a5a, 0x36836cb51bef8383, 0xcc3385ff66b63333,
    0x91633ff2c65c6363, 0x0802100a04120202, 0x92aa39384993aaaa, 0xd971afa8e2de7171,
    0x07c80ecf8dc6c8c8, 0x6419c87d32d11919, 0x39497270923b4949, 0x43d9869aaf5fd9d9,
    0xeff2c31df931f2f2, 0xabe34b48dba8e3e3, 0x715be22ab6b95b5b, 0x1a8834920dbc8888,
    0x529aa4c8293e9a9a, 0x98262dbe4c0b2626, 0xc8328dfa64bf3232, 0xfab0e94a7d59b0b0,
    0x83e91b6acff2e9e9, 0x3c0f78331e770f0f, 0x73d5e6a6b733d5d5, 0x3a8074ba1df48080,
    0xc2be997c6127bebe, 0x13cd26de87ebcdcd, 0xd034bde468893434, 0x3d487a7590324848,
    0xdbffab24e354ffff, 0xf57af78ff48d7a7a, 0x7a90f4ea3d649090, 0x615fc23ebe9d5f5f,
    0x80201da0403d2020, 0xbd6867d5d00f6868, 0x681ad07234ca1a1a, 0x82ae192c41b7aeae,
    0xeab4c95e757db4b4, 0x4d549a19a8ce5454, 0x7693ece53b7f9393, 0x88220daa442f2222,
    0x8d6407e9c8636464, 0xe3f1db12ff2af1f1, 0xd173bfa2e6cc7373, 0x4812905a24821212,
    0x1d403a5d807a4040, 0x2008402810480808, 0x2bc356e89b95c3c3, 0x97ec337bc5dfecec,
    0x4bdb9690ab4ddbdb, 0xbea1611f5fc0a1a1, 0x0e8d1c8307918d8d, 0xf43df5c97ac83d3d,
    0x6697ccf1335b9797, 0x0000000000000000, 0x1bcf36d483f9cfcf, 0xac2b4587566e2b2b,
    0xc57697b3ece17676, 0x328264b019e68282, 0x7fd6fea9b128d6d6, 0x6c1bd87736c31b1b,
    0xeeb5c15b7774b5b5, 0x86af112943beafaf, 0xb56a77dfd41d6a6a, 0x5d50ba0da0ea5050,
    0x0945124c8a574545, 0xebf3cb18fb38f3f3, 0xc0309df060ad3030, 0x9bef2b74c3c4efef,
    0xfc3fe5c37eda3f3f, 0x4955921caac75555, 0xb2a2791059dba2a2, 0x8fea0365c9e9eaea,
    0x89650fecca6a6565, 0xd2bab9686903baba, 0xbc2f65935e4a2f2f, 0x27c04ee79d8ec0c0,
    0x5fdebe81a160dede, 0x701ce06c38fc1c1c, 0xd3fdbb2ee746fdfd, 0x294d52649a1f4d4d,
    0x7292e4e039769292, 0xc9758fbceafa7575, 0x1806301e0c360606, 0x128a249809ae8a8a,
    0xf2b2f940794bb2b2, 0xbfe66359d185e6e6, 0x380e70361c7e0e0e, 0x7c1ff8633ee71f1f,
    0x956237f7c4556262, 0x77d4eea3b53ad4d4, 0x9aa829324d81a8a8, 0x6296c4f431529696,
    0xc3f99b3aef62f9f9, 0x33c566f697a3c5c5, 0x942535b14a102525, 0x7959f220b2ab5959,
    0x2a8454ae15d08484, 0xd572b7a7e4c57272, 0xe439d5dd72ec3939, 0x2d4c5a6198164c4c,
    0x655eca3bbc945e5e, 0xfd78e785f09f7878, 0xe038ddd870e53838, 0x0a8c148605988c8c,
    0x63d1c6b2bf17d1d1, 0xaea5410b57e4a5a5, 0xafe2434dd9a1e2e2, 0x99612ff8c24e6161,
    0xf6b3f1457b42b3b3, 0x842115a542342121, 0x4a9c94d625089c9c, 0x781ef0663cee1e1e,
    0x1143225286614343, 0x3bc776fc93b1c7c7, 0xd7fcb32be54ffcfc, 0x1004201408240404,
    0x5951b208a2e35151, 0x5e99bcc72f259999, 0xa96d4fc4da226d6d, 0x340d68391a650d0d,
    0xcffa8335e979fafa, 0x5bdfb684a369dfdf, 0xe57ed79bfca97e7e, 0x90243db448192424,
    0xec3bc5d776fe3b3b, 0x96ab313d4b9aabab, 0x1fce3ed181f0cece, 0x4411885522991111,
    0x068f0c8903838f8f, 0x254e4a6b9c044e4e, 0xe6b7d1517366b7b7, 0x8beb0b60cbe0ebeb,
    0xf03cfdcc78c13c3c, 0x3e817cbf1ffd8181, 0x6a94d4fe35409494, 0xfbf7eb0cf31cf7f7,
    0xdeb9a1676f18b9b9, 0x4c13985f268b1313, 0xb02c7d9c58512c2c, 0x6bd3d6b8bb05d3d3,
    0xbbe76b5cd38ce7e7, 0xa56e57cbdc396e6e, 0x37c46ef395aac4c4, 0x0c03180f061b0303,
    0x45568a13acdc5656, 0x0d441a49885e4444, 0xe17fdf9efea07f7f, 0x9ea921374f88a9a9,
    0xa82a4d8254672a2a, 0xd6bbb16d6b0abbbb, 0x23c146e29f87c1c1, 0x5153a202a6f15353,
    0x57dcae8ba572dcdc, 0x2c0b582716530b0b, 0x4e9d9cd327019d9d, 0xad6c47c1d82b6c6c,
    0xc43195f562a43131, 0xcd7487b9e8f37474, 0xfff6e309f115f6f6, 0x05460a438c4c4646,
    0x8aac092645a5acac, 0x1e893c970fb58989, 0x5014a04428b41414, 0xa3e15b42dfbae1e1,
    0x5816b04e2ca61616, 0xe83acdd274f73a3a, 0xb9696fd0d2066969, 0x2409482d12410909,
    0xdd70a7ade0d77070, 0xe2b6d954716fb6b6, 0x67d0ceb7bd1ed0d0, 0x93ed3b7ec7d6eded,
    0x17cc2edb85e2cccc, 0x15422a5784684242, 0x5a98b4c22d2c9898, 0xaaa4490e55eda4a4,
    0xa0285d8850752828, 0x6d5cda31b8865c5c, 0xc7f8933fed6bf8f8, 0x228644a411c28686,
];

/*******************************************************************************

*******************************************************************************/

private static const ulong C7[256] = 
[
    0x186018c07830d818, 0x238c2305af462623, 0xc63fc67ef991b8c6, 0xe887e8136fcdfbe8,
    0x8726874ca113cb87, 0xb8dab8a9626d11b8, 0x0104010805020901, 0x4f214f426e9e0d4f,
    0x36d836adee6c9b36, 0xa6a2a6590451ffa6, 0xd26fd2debdb90cd2, 0xf5f3f5fb06f70ef5,
    0x79f979ef80f29679, 0x6fa16f5fcede306f, 0x917e91fcef3f6d91, 0x525552aa07a4f852,
    0x609d6027fdc04760, 0xbccabc89766535bc, 0x9b569baccd2b379b, 0x8e028e048c018a8e,
    0xa3b6a371155bd2a3, 0x0c300c603c186c0c, 0x7bf17bff8af6847b, 0x35d435b5e16a8035,
    0x1d741de8693af51d, 0xe0a7e05347ddb3e0, 0xd77bd7f6acb321d7, 0xc22fc25eed999cc2,
    0x2eb82e6d965c432e, 0x4b314b627a96294b, 0xfedffea321e15dfe, 0x5741578216aed557,
    0x155415a8412abd15, 0x77c1779fb6eee877, 0x37dc37a5eb6e9237, 0xe5b3e57b56d79ee5,
    0x9f469f8cd923139f, 0xf0e7f0d317fd23f0, 0x4a354a6a7f94204a, 0xda4fda9e95a944da,
    0x587d58fa25b0a258, 0xc903c906ca8fcfc9, 0x29a429558d527c29, 0x0a280a5022145a0a,
    0xb1feb1e14f7f50b1, 0xa0baa0691a5dc9a0, 0x6bb16b7fdad6146b, 0x852e855cab17d985,
    0xbdcebd8173673cbd, 0x5d695dd234ba8f5d, 0x1040108050209010, 0xf4f7f4f303f507f4,
    0xcb0bcb16c08bddcb, 0x3ef83eedc67cd33e, 0x05140528110a2d05, 0x6781671fe6ce7867,
    0xe4b7e47353d597e4, 0x279c2725bb4e0227, 0x4119413258827341, 0x8b168b2c9d0ba78b,
    0xa7a6a7510153f6a7, 0x7de97dcf94fab27d, 0x956e95dcfb374995, 0xd847d88e9fad56d8,
    0xfbcbfb8b30eb70fb, 0xee9fee2371c1cdee, 0x7ced7cc791f8bb7c, 0x66856617e3cc7166,
    0xdd53dda68ea77bdd, 0x175c17b84b2eaf17, 0x47014702468e4547, 0x9e429e84dc211a9e,
    0xca0fca1ec589d4ca, 0x2db42d75995a582d, 0xbfc6bf9179632ebf, 0x071c07381b0e3f07,
    0xad8ead012347acad, 0x5a755aea2fb4b05a, 0x8336836cb51bef83, 0x33cc3385ff66b633,
    0x6391633ff2c65c63, 0x020802100a041202, 0xaa92aa39384993aa, 0x71d971afa8e2de71,
    0xc807c80ecf8dc6c8, 0x196419c87d32d119, 0x4939497270923b49, 0xd943d9869aaf5fd9,
    0xf2eff2c31df931f2, 0xe3abe34b48dba8e3, 0x5b715be22ab6b95b, 0x881a8834920dbc88,
    0x9a529aa4c8293e9a, 0x2698262dbe4c0b26, 0x32c8328dfa64bf32, 0xb0fab0e94a7d59b0,
    0xe983e91b6acff2e9, 0x0f3c0f78331e770f, 0xd573d5e6a6b733d5, 0x803a8074ba1df480,
    0xbec2be997c6127be, 0xcd13cd26de87ebcd, 0x34d034bde4688934, 0x483d487a75903248,
    0xffdbffab24e354ff, 0x7af57af78ff48d7a, 0x907a90f4ea3d6490, 0x5f615fc23ebe9d5f,
    0x2080201da0403d20, 0x68bd6867d5d00f68, 0x1a681ad07234ca1a, 0xae82ae192c41b7ae,
    0xb4eab4c95e757db4, 0x544d549a19a8ce54, 0x937693ece53b7f93, 0x2288220daa442f22,
    0x648d6407e9c86364, 0xf1e3f1db12ff2af1, 0x73d173bfa2e6cc73, 0x124812905a248212,
    0x401d403a5d807a40, 0x0820084028104808, 0xc32bc356e89b95c3, 0xec97ec337bc5dfec,
    0xdb4bdb9690ab4ddb, 0xa1bea1611f5fc0a1, 0x8d0e8d1c8307918d, 0x3df43df5c97ac83d,
    0x976697ccf1335b97, 0x0000000000000000, 0xcf1bcf36d483f9cf, 0x2bac2b4587566e2b,
    0x76c57697b3ece176, 0x82328264b019e682, 0xd67fd6fea9b128d6, 0x1b6c1bd87736c31b,
    0xb5eeb5c15b7774b5, 0xaf86af112943beaf, 0x6ab56a77dfd41d6a, 0x505d50ba0da0ea50,
    0x450945124c8a5745, 0xf3ebf3cb18fb38f3, 0x30c0309df060ad30, 0xef9bef2b74c3c4ef,
    0x3ffc3fe5c37eda3f, 0x554955921caac755, 0xa2b2a2791059dba2, 0xea8fea0365c9e9ea,
    0x6589650fecca6a65, 0xbad2bab9686903ba, 0x2fbc2f65935e4a2f, 0xc027c04ee79d8ec0,
    0xde5fdebe81a160de, 0x1c701ce06c38fc1c, 0xfdd3fdbb2ee746fd, 0x4d294d52649a1f4d,
    0x927292e4e0397692, 0x75c9758fbceafa75, 0x061806301e0c3606, 0x8a128a249809ae8a,
    0xb2f2b2f940794bb2, 0xe6bfe66359d185e6, 0x0e380e70361c7e0e, 0x1f7c1ff8633ee71f,
    0x62956237f7c45562, 0xd477d4eea3b53ad4, 0xa89aa829324d81a8, 0x966296c4f4315296,
    0xf9c3f99b3aef62f9, 0xc533c566f697a3c5, 0x25942535b14a1025, 0x597959f220b2ab59,
    0x842a8454ae15d084, 0x72d572b7a7e4c572, 0x39e439d5dd72ec39, 0x4c2d4c5a6198164c,
    0x5e655eca3bbc945e, 0x78fd78e785f09f78, 0x38e038ddd870e538, 0x8c0a8c148605988c,
    0xd163d1c6b2bf17d1, 0xa5aea5410b57e4a5, 0xe2afe2434dd9a1e2, 0x6199612ff8c24e61,
    0xb3f6b3f1457b42b3, 0x21842115a5423421, 0x9c4a9c94d625089c, 0x1e781ef0663cee1e,
    0x4311432252866143, 0xc73bc776fc93b1c7, 0xfcd7fcb32be54ffc, 0x0410042014082404,
    0x515951b208a2e351, 0x995e99bcc72f2599, 0x6da96d4fc4da226d, 0x0d340d68391a650d,
    0xfacffa8335e979fa, 0xdf5bdfb684a369df, 0x7ee57ed79bfca97e, 0x2490243db4481924,
    0x3bec3bc5d776fe3b, 0xab96ab313d4b9aab, 0xce1fce3ed181f0ce, 0x1144118855229911,
    0x8f068f0c8903838f, 0x4e254e4a6b9c044e, 0xb7e6b7d1517366b7, 0xeb8beb0b60cbe0eb,
    0x3cf03cfdcc78c13c, 0x813e817cbf1ffd81, 0x946a94d4fe354094, 0xf7fbf7eb0cf31cf7,
    0xb9deb9a1676f18b9, 0x134c13985f268b13, 0x2cb02c7d9c58512c, 0xd36bd3d6b8bb05d3,
    0xe7bbe76b5cd38ce7, 0x6ea56e57cbdc396e, 0xc437c46ef395aac4, 0x030c03180f061b03,
    0x5645568a13acdc56, 0x440d441a49885e44, 0x7fe17fdf9efea07f, 0xa99ea921374f88a9,
    0x2aa82a4d8254672a, 0xbbd6bbb16d6b0abb, 0xc123c146e29f87c1, 0x535153a202a6f153,
    0xdc57dcae8ba572dc, 0x0b2c0b582716530b, 0x9d4e9d9cd327019d, 0x6cad6c47c1d82b6c,
    0x31c43195f562a431, 0x74cd7487b9e8f374, 0xf6fff6e309f115f6, 0x4605460a438c4c46,
    0xac8aac092645a5ac, 0x891e893c970fb589, 0x145014a04428b414, 0xe1a3e15b42dfbae1,
    0x165816b04e2ca616, 0x3ae83acdd274f73a, 0x69b9696fd0d20669, 0x092409482d124109,
    0x70dd70a7ade0d770, 0xb6e2b6d954716fb6, 0xd067d0ceb7bd1ed0, 0xed93ed3b7ec7d6ed,
    0xcc17cc2edb85e2cc, 0x4215422a57846842, 0x985a98b4c22d2c98, 0xa4aaa4490e55eda4,
    0x28a0285d88507528, 0x5c6d5cda31b8865c, 0xf8c7f8933fed6bf8, 0x86228644a411c286,
];

/*******************************************************************************

*******************************************************************************/

private static const ulong rc[INTERNAL_ROUNDS + 1] = 
[
    0x0000000000000000,
    0x1823c6e887b8014f,
    0x36a6d2f5796f9152,
    0x60bc9b8ea30c7b35,
    0x1de0d7c22e4bfe57,
    0x157737e59ff04ada,
    0x58c9290ab1a06b85,
    0xbd5d10f4cb3e0567,
    0xe427418ba77d95d8,
    0xfbee7c66dd17479e,
    0xca2dbf07ad5a8333,
];

/*******************************************************************************

*******************************************************************************/

debug(UnitTest)
{
    unittest
    {
    	void testISOVectors()
    	{
    	    // The ISO test vectors and results
    	    static char[][] strings =
    	    [
    	            "",
    	            "a",
    	            "abc",
    	            "message digest",
    	            "abcdefghijklmnopqrstuvwxyz",
    	            "abcdbcdecdefdefgefghfghighijhijk",
    	            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
    	            "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
     	            "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" // Not an ISO vector, but from FIPS
    	    ];

    	    static char[][] results =
    	    [
    	            "19fa61d75522a4669b44e39c1d2e1726c530232130d407f89afee0964997f7a73e83be698b288febcf88e3e03c4f0757ea8964e59b63d93708b138cc42a66eb3",
    	            "8aca2602792aec6f11a67206531fb7d7f0dff59413145e6973c45001d0087b42d11bc645413aeff63a42391a39145a591a92200d560195e53b478584fdae231a",
    	            "4e2448a4c6f486bb16b6562c73b4020bf3043e3a731bce721ae1b303d97e6d4c7181eebdb6c57e277d0e34957114cbd6c797fc9d95d8b582d225292076d4eef5",
    	            "378c84a4126e2dc6e56dcc7458377aac838d00032230f53ce1f5700c0ffb4d3b8421557659ef55c106b4b52ac5a4aaa692ed920052838f3362e86dbd37a8903e",
    	            "f1d754662636ffe92c82ebb9212a484a8d38631ead4238f5442ee13b8054e41b08bf2a9251c30b6a0b8aae86177ab4a6f68f673e7207865d5d9819a3dba4eb3b",
    	            "2a987ea40f917061f5d6f0a0e4644f488a7a5a52deee656207c562f988e95c6916bdc8031bc5be1b7b947639fe050b56939baaa0adff9ae6745b7b181c3be3fd",
    	            "dc37e008cf9ee69bf11f00ed9aba26901dd7c28cdec066cc6af42e40f82f3a1e08eba26629129d8fb7cb57211b9281a65517cc879d7b962142c65f5a7af01467",
    	            "466ef18babb0154d25b9d38a6414f5c08784372bccb204d6549c4afadb6014294d5bd8df2a6c44e538cd047b2681a51a2c60481e88c5a20b2c2a80cf3a9a083b",
     	            "526b2394d85683e24b29acd0fd37f7d5027f61366a1407262dc2a6a345d9e240c017c1833db1e6db6a46bd444b0c69520c856e7c6e9c366d150a7da3aeb160d1",
    	    ];

    	    Whirlpool h = new Whirlpool();

    	    foreach (int i, char[] s; strings)
    	            {
    	            h.update(cast(ubyte[]) s);
    	            char[] d = h.hexDigest;

    	            assert(d == results[i],":("~s~")("~d~")!=("~results[i]~")");
    	            }
    	    
    	    {
    	    	char[] s = new char[1000000];
    	    	for (auto i = 0; i < s.length; i++) s[i] = 'a';
    	    	char[] result = "0c99005beb57eff50a7cf005560ddf5d29057fd86b20bfd62deca0f1ccea4af51fc15490eddc47af32bb2b66c34ff9ad8c6008ad677f77126953b226e4ed8b01";
    	    	h.update(cast(ubyte[]) s);
    	    	char[] d = h.hexDigest;

    	    	assert(d == result,":(1 million times \"a\")("~d~")!=("~result~")");
    	    }    		
    	}
    	
    	void testNESSIE0bitVectors() {
    	    // Part of the NESSIE test vectors and results: strings of 0 bits
    	    ubyte[128] data = 0;
    	    static char[][] results =
    	    [
    		        "19fa61d75522a4669b44e39c1d2e1726c530232130d407f89afee0964997f7a73e83be698b288febcf88e3e03c4f0757ea8964e59b63d93708b138cc42a66eb3",
    	 	        "4d9444c212955963d425a410176fccfb74161e6839692b4c11fde2ed6eb559efe0560c39a7b61d5a8bcabd6817a3135af80f342a4942ccaae745abddfb6afed0",
    	 	        "8bdc9d4471d0dabd8812098b8cbdf5090beddb3d582917a61e176e3d22529d753fed9a37990ca18583855efbc4f26e88f62002f67722eb05f74c7ea5e07013f5",
    	 	        "86aabfd4a83c3551cc0a63185616acb41cdfa96118f1ffb28376b41067efa25fb6c889662435bfc11a4f0936be6bcc2c3e905c4686db06159c40e4dd67dd983f",
    	 	        "4ed6b52e915f09a803677c3131f7b34655d32817505d89a8cc07ed073ca3fedddd4a57cc53696027e824ab9822630087657c6fc6a28836cf1f252ed204bca576",
    	 	        "4a1d1d8380f38896b6fc5788c559f92727acfd4dfa7081c72302b17e1ed437b30a24cfd75a16fd71b6bf5aa7ae5c7084594e3003a0b71584dc993681f902df6f",
    	 	        "a2a379b0900a3c51809f4216aa3347fec486d50ec7376553349c5cf2a767049a87bf1ac4642185144924259ecf6b5c3b48b55a20565de289361e8ae5eafc5802",
    	 	        "23eb3e26a1543558672f29e196304cd778ea459cc8e38d199de0cc748bd32d58090fadb39e7c7b6954322de990556d001ba457061c4367c6fa5d6961f1046e2f",
    	 	        "7207be34fee657189af2748358f46c23175c1dcdd6741a9bdb139aeb255b618b711775fc258ac0fa53c350305862415ea121c65bf9e2fae06cbd81355d928ad7",
    	 	        "fef7d0be035d1860e95644864b199c3a94eb23ab7920134b73239a320eb7cae450092bc4ba8b9809e20c33937c37c52b52ca90241657ffd0816420c01f4fada8",
    	            "caafb557aef0fae9f20bcccda7f3dc769c478a70508f4f2d180303598276934c410bd3d17627159a9c55b5265b516ba7f3eef67fbb08d9f22e585bc45964c4d1",
    	            "8fe2b488ca099db8e421768e1e7e0193ffaa3000e8281403795575fe7d03bd87298c4f64b1c4311093e43de4d80049645782ee268c3653c7a5c13da3773d5564",
    	            "2e065509d50d6135e4c4bd238b0e4138391c98082a596e76bdf1fb925b5069adca9548f833296da968573b965f79cd806624fb8c7e06f728aa04f24185878933", 
    	            "51fdcea18fdb55a7497e06f33c74194daf0138411075749ed5f00eba59e7f850da6d36b7ee851beabcb449537910a73b8878c88a868a7168c677ac4601ac71d8",
    	            "2e19c1695b708d20f9b2e4e3cc2df40b9ca4dc8c709a148b1f0da5934116050d12c4ab44d0cc6cc82e86c0f140ddd4a7a1152b2c42d150965224fdd3fe2c5a94",
    	            "b501bc7a16fc812eda69ad0f8453ddc57278478cb1a704b1977e22090e82e7a280c1bf20bf130b98e7b3b2f4eda87ef03a4900dc7c0427da4e28e422186beecc",
    	            "a1f020a9b16c1318660d573d7e5203c14438196f247a55f09a6bf67211bcfdae71077cdf5b0e243a61ce10ffc561da4c0462600e11f176c2537da22bfc9386b9",
    	            "a4e587d1bdf7c7a9897dc2a002e0c3240aac073dabd4b04b2cec2ae8a5751d3059b12d78efcb1b9b7f18bb69ab7f7c9f89f6a1c5e59325bc5e827a1cdcf990f5",
    	            "9a7e6b5e34a4ff26b990a3bd844e059b39b7a58eefe37ef96c595cf6a52eb697f8263cdb34566ffa3387cc7895cab74196ef0e42162fde8e2b3aaf435aa1d07b",
    	            "a979eefd43c26fe934399d26e18714b59d53572f9dc265937c1a24b48480ccd7c409f8a0322156cc602afa2a3df8ba5e1c86fc00de2a420b9412aa48af7f10d0",
    	            "212d82efab4a9c99b4559ed090830db4e70f469d98927453d8ca2956afb4f92fa79174a552e7a17c1c87e640117d67b0ed377548ec64df6e534593c218369ef0",
    	            "fce5e68f1e2ae9aea93514e4cb2b29eddb4c6a3955244d554f99aea42c61db3e761f9bead5c05470c20cfbf605c1ea4b0f3877487cac9b8177bd3df3aa67cc3a",
    	            "c8f99a4fc37587d3cf36c65fc0249648866fe4c2a99ae47f0ee0f3d3270c3134a487182847afd3d056a15a9e35c5a2fd76947ac1627514a8d1f2ecc057cfcd5e",
    	            "98b42ee3e98849bf1f6d8589f129ab12f055883c520e6001f536e67ea6bd34db4bd610b415f455867b6ab1c31a9ddc37330a57fb6b0f1f20efba12ff560a2e16",
    	            "4b8e81860b0035eb2189eae73e23c9f34b3d993c3387ab276c2f47aada04391d609c70611b3420a72af1af12d4733f40457c0f75ac656c4e798e8b29556a8497",
    	            "3c0863d6bd033ff0ea6801d7c11a7f4f89c3f0678ebc1981d0e98ef9f435429dd25e1238aacfb47bfd6bdd4016981a2832c8b5687c654c12dba0c40c806d6a79",
    	            "1243171d17b72affa1beb5fa7a57cbd6432447476da2e785dd85946ba7d1836d81fa7048d7870d10072c56970d65ba2b759181f6115693deb2cf88ddf13878dd",
    	            "09b029e928fe1ebda47fd79e32e3201ded573890305fa7193c185d741c1dede443e55ee21c1ea5d608d03bf04e1f29832e70c22bc4962ff56d24b333f272dbc4",
    	            "97d10c6aa678886e186551df174bd9bfda299c1cfa0a926eda92290f99fc05d9e23ec4897686ea7eb576686a5c27b10c23c76e303410bbaed0e165e2a8d362dd",
    	            "799324597820515a94d854031e00312b91ac8f0aeb7ec79cc87503e2fbec48ddc1179f0f4d25326a59ec82a84057c04534b032b66924b925520a8ce0e4449649",
    	            "b5dadbdd162118d69ca21825c05d4fa4ee9195cbac15d1f649c235043c9bc1e3ff8920920b2843503d70deb087d9698dd6436698e1dc1f54216bdc6188f13acf",
    	            "3e3f188f8febbeb17a933feaf7fe53a4858d80c915ad6a1418f0318e68d49b4e459223cd414e0fbc8a57578fd755d86e827abef4070fc1503e25d99e382f72ba",
    	            "961b5f299f750f880fca004bdf2882e2fe1b491b0c0ee7e2b514c5dfdd53292dbdbee17e6d3bb5824cdec1867cc7090963be8fff0c1d8ed5864e07cacb50d68a",
    	            "54a9c8784dbf6c2618ca32057b76b9d6733c19f4a377cb7e892d057bf73e83fbaf6ac147a65fef0991dc296955440ad0b793f81c5cf71e29669ce3f19195aaf7",
    	            "7fd61031451a6635f4a8bf56f9e3b6c699c77d573bbe2f1f4938a0630b7d6fb64f202f5e8c4dafb23d5b4481089bc198d0324178a3c9a625e101744ac517b681",
    	            "bb00019c930b4f8b3c035861174b2e8cdb94cdfa4f5b08082944f86505cd6eb18f7c95f1e031ab42c510187927f9fedae3085a8e1918fb1acde7a0fc6cf3c62f",
    	            "9e74ee84973257cd3507387da7f4388ba46c55fa55ec0b991ec0ef01c8ed629eabd003f585c62d46d70a760b51ffba48f4aa1d10519ddb09ad95e536f5d9faac",
    	            "012cd71c3d938304bd65dcc705f1ea511ff1981aaf6b641f9e3714b1fec010ae8bedd39fb497cc1affcde1dd33569eb970c3dd7bb073ecd130028bb0f76b356c",
    	            "f7c0484614245f5cf54dc03d14e00ac30a86de8fe9e3de1741c75d3c7202570f2a5b70618fbb1c851285ec2c4cb961254ddade9ba7639993b12efa1d66e7c61d",
    	            "25f7cf0ceef2a61a657d852140daf9f19567534b34e1928c26fc6499a29e896c8c7a5c2bb2393fcca4c1ef811ef530954b758fbe2120303be63394fa340ae544",
    	            "4a9ce34ecdcfb9a5082ded6f56c24ae155ab3225dfd6ad39fa79df6dc9b58fbe346c40b1aa58af7b836e0e6346bcd0b0e686eba4ee86c4cc77eb7de1d16d43f0",
    	            "eeabdb9655fa9d652cce83fc697b8534a3d92cfa0c2cfabcf3393be1ad9d6589fd6eeebceac9bf3ef3251a7811b793fd03f17d2fa00edb17c423394dc6bedda1",
    	            "9ce6a4a20a719186c8946d824fb2be78260fed469a0a9946a44cfe3a43d5e11c642407b473fe72ef087c45edaeb2c137ff379a7f60747895ae03f6fc337cacd0",
    	            "af2d773502eef6e86448d5b465d056b9530877ffa1a97bbedc17c119e48bb3be0c79df54940d10163436c8841fbbd565ddb7a8de850445ef93ab5d2e8c19d4c1",
    	            "e67ab8b825252076477d3282e69b006f10d76a841318dcba82de290928f46ebbcc5a6e3ebfb8aefc8f997928f2adf351376c214eae23cd8e1632b7acae102959",
    	            "cf1476329ec7be76c0808249e39a9c0d05b0010eda8d9f21ce4a0794698d3016f28830fe1628061a9c2db8e0155092b3c5c606e2c3900f8112cf2f03cca96edd",
    	            "b0d5b4e4caa9f3360d016ed86fdb2b8ba579ae7df45c63231774cbd369aedb065cd32cac6914b629a76242d8bae5ddcec8decad1ccc3cf2fff044d1f84415975",
    	            "894e242d8ac2bffe0bb2f5be206796dd7666bf8c113547c064e6b7755e0b27af4126ec1301e8ddab0fd1ca202ffcd71287bb7fdbcba73f87fcfb90fea846129b",
    	            "2de58b719aaf6c07d8d4c3d2f61a3950ed877a8619328177b2a3f1deae6122797bfeb0ad190845805a129eacd11be90d63367d34f19734909a1af19a951e8359",
    	            "dc40979db11829c0e8513c23e92de628fd53d03a35c281393f35f5952c3f2e062100e1bf077f4ca0c24f26d250829daaaad9f33b5caa9c081ba2b4910648810a",
    	            "3cf2523477a56d6e8c8d2de34f574ce2ca1d13104bc209df6e0b2f587680ef038713216b4b869bb92e8d13293b8861b75a1fba392793f0b58dd21f65c06fb8b4",
    	            "4ce343439d4f8a3af5b2027fad0764d28ba561c7ff02ed53fe1d1de47a6b7e89f3f3f259c7c46968fe0c065e25ac91f0ad493b28f662792f1d561fce029bfb5b",
    	            "e1c06f1951ee384bea3a2ee974b81fd4674e65e89d09e42c0309d2df9e2aa004ccecd3dba2ae5e31d56f83cf7d84036b081713dd70e36881f2acf7312e6119b0",
    	            "586b6f60d778cd32d7fd4f6166d6c06e2f730477787f8f0259264d121b9aeb3963b38a9250adbcb6ac7029051acfaa141406efc19560b3ed33e86fab7ac1fea2",
    	            "edc165f52b5e493fa4b39ad06339be79b1afdb204bfc792011200ac951468eedfe6a73da03331ee95beccf01e2260a30d2d36679e184d6e6417fc01647731076",
    	            "d64ab30797e2d5e986b7b6fe99fc5aa79918d4423a5808d64f8042a9489485e6619f57d2865091b363a3b9b788b16690fba45e8be352fc9517b58a05937383e7",
    	            "0b3f5378ebed2bf4d7be3cfd818c1b03b6bb03d346948b04f4f40c726f0758702a0f1e225880e38dd5f6ed6de9b1e961e49fc1318d7cb74822f3d0e2e9a7e7b0",
    	            "8e2f3c36f961d08f0ee389412b434595914821829f21a213366f898516ec97bda3d13814fcd8c5fa9ab9ed92246c73824d54a36cb19a9c986f4b7e41ebe12ed3",
    	            "a516c5c831f4e56bf8ed0b7c193c967536f1063f23085fb3ff7098dd3070dc7fa6f9b26c5c26942cd4ff8189f777b66bff9385e6136df6a2b0d5bcd3c8ad2878",
    	            "30e347272c63d340ed0781cca5270894db39e7a97fb5f5e9ee2ad988a127eea4c7eee0f1c8a33dd5ed8d01f91e18cb433434a35d82bc4fde5f2fcd91a657d84e",
    	            "9417f6af2a8325268a0998adb4196189d7140797fb757d6f081defc5b5e4d37d8f45830eec029e2e45bb6b9e019fac97bd3ee79db9a8a4fdede69d66d63fa009",
    	            "d059f4847864c24d11511e5589005c1d5d550111e30c557be0b1f02ab7f2495e44799253724e19026bfa6b0e544d35747af75044237a25ee9a2315cc9e3e18d1",
    	            "6119c33347ad30067e75d33f5990b4ecdae1fc4e42aaf17888d2526ace673223a483875e1854f9f60aef51678be2d52b6b4880b7e07395431845b6736881ec45",
    	            "0fac8ef35b4f7be6e1ac1580eef1a8a90f81e3043cc45e918eda3543d9d4e9e3aa16c8e5e71bf792f81ca67f625ff98f9ca1eab8fa9b78c9d2e771cf249768dd",
    	            "15cfa7c1df8e0d6753d9a9aed0642867e26bb3cf11df7dac96f60c274e060fda941ec41eaff5f7375f3839632516ae9a831d9f2fbe2bd0ff02e9cf16e99ebd03",
    	            "85e124c4415bcf4319543e3a63ff571d09354ceebee1e325308c9069f43e2ae4d0e51d4eb1e8642870194e9530d8d8af6589d1bf6949ddf90a7f1208623795b9",
    	            "fce5e522e77e3f85b81565c43beb03b63231db8ea7940251bb5b33484353be66c8d6c314a661a89cfd9088b4e54762a21c246b7c662903ad2de5ab7c12a1e842",
    	            "75e11b045a3ea2af6b5cb47a602836ad1e3c8b72d242acca4be83cfc48826b4554c2f3f804b29181e71bed658ff366f66879f2fea08bda0c224f41db6b144871",
    	            "70b34639476a82db9086fad0810a0610cd778d1432e58a173f60d0480992ed1f04ae4dbffd8471efce2cc39576b6367c79863e844d198763c2e1a5f63a1747aa",
    	            "d2d92a49569825acec8a67abc1f71adaba3d1bef1137660e23e8950bc9f42ab571bdef48835e488c02d2a8a80bf94fd175f99e0b27db4bdf9596d64b39b1cf8d",
    	            "edf51be6cf0ae98337dd78747f7d0809585be1ee3c7b09dc439b513c7b2a0387d1950294ec73ba68308e30e2d4bdb679849a054a16b05333036da3cd59b76329",
    	            "e3ba063cb8efcdf32b0b38d929b88f28dd64a9a88df9958db2fd8074c24597c981b358882ff09a501b1b7037047fb96b716e5047f3630ddb4e5a39aa862c23f8",
    	            "a75bd3d61e3a0820fdb151a65fbe42bfb2e247fdba142a516374ce8305e4830049a69f37048b4c098a4415745e753bdd679b019b40cf93bbead892045bd6fa3d",
    	            "d45072f92f621082b1c5775f25e8cf8c93dd853219c7039355d308e9cc981794f2a593a71aea900487858e93860dea4898510e4ea48053404a9075b677e64531",
    	            "6a9d999c3cb074e3ea1605f68c01fa5b402b382f6953a5c973da9e888e0354c815d26fd72448aeb9aaab0a24685dbf8c096841f8d89606eabd58cd14ca6fbe74",
    	            "05b9648219a7e9d839084aaa64c4b2b74db42f8bd435a5b69079df2335ec24547b867ef9b567c43acd70fa0d010c48e4695833da57fe0c60ad9111958a3b4116",
    	            "6c243e768b3f7c4fc5c4dcb3bf6fa4fb44767615ebaeda0a572cd12624c29e508e4c814bea8f21f0f2eb69d0045297960e1ef5d3bee86858590f55d303cf1d87",
    	            "c98ebbaae2df1fcbb406b6dd831bb68ca317924a7563152a2ce61dd101bfef277a0f7be5fa040c9f41952052c58c3e32e2b07d7c0d39fbd20950a149595c82f8",
    	            "f739c74b2aea5c89781c5de3b9ee9cae1ccb71e6a8824b1c42e4ba0f35efb0a479ade849196af1a5ea74d3ba6ab1e05d330da469a96bfcbce7f9097fbfd3ed08",
    	            "cc5c707f98cfb307084d2202e8183f43014fc73b54d04592ae7ef6a3d95fc25ca04c8004f4d9fc4975cb5fb5921ee9e812c720474b5e4ba3dfdac77b9bd62872",
    	            "036b2bf483d53db440673faa192985c305b5f35c6c4f14d2bea22be44a843eaf81f620f47493d29d47a99292b26706e3d82db753ff1eff72e5912e904c652b01",
    	            "005f1d907c6be7a0063dc782426103d4bc64b12fa12addb2fe3129738349adb31dd92eb5fbf76538d1e16b4d7e0dea61863218231907e280d0ca9ab8b1264444",
    	            "b61902fc169b85a006698fbf8798ba6af1bf8ddbdb4300a70e5de4bfa35fb083e9a19387731c288215ce2bd7e4bd6548957c4e09a14afed1d5f91020fead2401",
    	            "6bc92cd769704e459dcee1b0a37a04ca79babfb5a6aa75ac16eb63ddb1f6207886942715c46daf871a6cb64357f19c7726ef7ed1fc36b15a95c05e4c4f221d43",
    	            "f61a9fc5ee251e87ea50a0031f132a2a8ec40755e0cb5b440e92c71869556b62fe91d6867dbc170a7a89b28c316c6930d7351f0269647205746a8ab0a65c79b8",
    	            "8fe667be1ae384ef73f1c12bb95b1e4378e54a70f8d0d329633d59ce6c24fe1c46807d94f44b3348d362f1b64ad811914fec30dbac19aeaa34468cc7e2888517",
    	            "95948460b4f1b29b463aa07b7be6e43af59e41dfd95b1bd93696c5094420715fcab72288a4d573d76349f0a97e67058828906db4ecf61d8210f5ff4a0a1df8f1",
    	            "a16153c7051b55e7926e75cbff374bec74f2e92d745b485809aa3d11b40cb367d1da142918bced6e015ca04293417116b688fda90b08a5c3170f9dd1fa9ac161",
    	            "c9b020a35ef82512f8adf2f3d839646c43e47cc385beefa1c553f4d153f1c9f98ff6658c19ff0ef30d02b255f0134701e1b8a9d5012047b1597182f2bcedd860",
    	            "a570fa22948572ea4e18a1c86550a5dab7c12259b65ee9fe5d32de799d7249e356fad35bd694435aef45a2483fe26ff15b83d5468ef34bcb932a26a3d801b658",
    	            "83c9bef94a2041a920df5b4665004202e4b3cc9373c881ee61ca402e765ebca6d9c6c4d9a521115ef427430a56ec4c1d007e56f170891ace1a672ed369a86a4f",
    	            "3cdb51acb9f8e1578de697cc0930450f31c20f92e719b6ec4a6ba9bc4b6ac8bb9d848f35d00a69090b3a7261a5314157596c737a88509a29e0881407ce5e3bcf",
    	            "dcc2cad3a902af3b3e6d5cf383b0181d9132f39a754961f3b3f670f4b196728de2fb4b595d18f74337911633ac1624a3aa3a33199e95e0cf7815f9b6d656c685",
    	            "e6ddf6e43856374276aaee041dbbe63655f52cd176363ee956c98202e0eb3fbee8f2fb9989a4a72cc494fa9f6f9759a3e4b0d33be24a9dc37e02ec1fbeebec04",
    	            "1c17c03ac300744c417447afe4fc8db2b814836dc7450c4b621cb147b5f96e2172b133ef62db323bdd8ad9b45d25932ad5feafbdb36ebd38c8d711b706bbf283",
    	            "3392d35211e8e70f2f10426e8b53e900a1d463d21970c0e82aff0d8e41b1abbd672d4d5650cb2c24d347f25f4a07242a2f2d5522f9215c09dfe01502f80e91e9",
    	            "84c17bd37a65ecac894c644bb7f1141d40352569dbd03b774a119d0d27e663db243108e1834225f1b37d573ee36dabdb9b015729800a2fcd6fdfcaa0e0ef519d",
    	            "ca3e0545fa1445d2c92f1efb0c7e3b094984b81a9f08f2c27b572531151dce8556caa8d532061794b3c58a2dde54ad43eaed1c2a04503cb985227a93f954ca57",
    	            "a7d14d84e4fdf8514f27a16a2d91af7044e2c1782971e9f790a06058659389ca333cd3f4e6de8845d3875ed478ae33b9392999ada377483e1adfaeebd4b33a58",
    	            "541e0b0add60b7af7fe95130f9e21a871397f13d206d76924b16e912afbd699243507e325c05431604514723bed8b9ca39491c6a9074a130f7629efc2b9056b3",
    	            "e4e08244a13e99f363fdcc35e6a90ff7dabc13be436b7fca2409fd1a8465e0607a9b160413a46ab7d8daade1b8527c3c15e66235dd868ec927fdc9c159203959",
    	            "3ebb7109dd79e67fef25bc0a21bb40a12eb37224f27319609c73dca5c3dad25879a97e5aabc6bdb55d9e521615de4d4a814185eb49e51133c59bb3dffc655162",
    	            "84cbd41d122c7df738ca360362d99152bf7239f74282fb7192c7d6d7e68f2560583163f7f68ea8fe8733607d4dd67934ffc5723b6525be04496498e82ba92221",
    	            "9a571a2982af27a6c11ca7015935bf58192e09cb15f725389709e8844083319e501bf32bff52dff8e5ec1e8c27bad05c143699eb19db1a514a5e127dc3c03379",
    	            "7af60a5d97c40d4f83eafa97c598c57964b4a25fa43784ffd0b4a8c76e76d97549facba985d1319987f3819fa7249f399eb2047844c9a5e008abfc292aead796",
    	            "311e6fe802eef5a6dc36a26f5cc831afa5a8874e9df044d5dd57ad721d00d8a3616a12f3767e5580a8653af82f14265d734d83e160be24b35d03d32eba14a902",
    	            "44f9aceeb3816d0e6c4f36a27a3989aa3e937e0d75720029832683999e87d8773f0fa8c29a71b62ddb744216c8e7e43993fedc32ce2902c0ac1f53f47a7dfa61",
    	            "a752bd35b98a170c04ae436354074b91c6f0ec0b42f20934bc3bc3c9ab117f8b1f7169b4c471a8c87a9236253645a25f2084ca002b584c7c12fb06633eb8c6db",
    	            "9c0c2d06c86ca051af80f32ee17ec2ea0756f83ec27660a38b85e1783422e07d4d331322fdbf7671849bda1b9b63c293c52bca566abe1e1d264bb8e48f76ca22",
    	            "7140bc6850b4ceda301e6fa9ece5a0f9e56cedc52720a3a80a6e8688abbc8fc633154672da0db25f95a252014f8a60cd1f3803a5d1d29686cf2d8d2cef096522",
    	            "84bbeb1d94f26cbcedfbc2e88b2db243f9c9609f541d2318af4d87c9045ac76e86743e000ac2aadbebf85d48a1135652e908c2bfa16cdbd1ef4caae26afe65a2",
    	            "58058d5811cb118fa2480b0e76c00cb217c6f7b40dc786f884194ad2ce2e4620862ebacf758afe3e304a16f6dd57503890d8c6609fb5b26df2f19a461ef2e4b1",
    	            "e7a337a2189f07031e4b68e6114df7e1f0979970e90a999390b6a374bbdeb37d5842c6c2a691d6d9f600626c57c73f4b70082ca6770daec1ede1dd28c984964e",
    	            "fab9052917bf1feaa1989d0bd79a6d4bbeae2bd4ecfdcd61607815d6c6857aaa38be4b27fc9ea6520cc0f0935ebb61012d1d580c5b04f6dca1ec973ebad2254e",
    	            "9855fe16ff0349c62e7d582ac20045f338d37fd70f62c860b83fcccea80b48c614e1ee443466f4e5ceffb7f27f40b749db5d6391d84981ed85c82c9c45b4cdf1",
    	            "c8079a4221f11ead657c380953ef46c87336604630266add73afb30f3010a1d7f3250fcb7e83f467bdbb1ddc11ea27f61364af0b7d334fc48525740cae208de7",
    	            "9e7d24de7b1f616085056466268d30cc306c618c775d4550570a0c51d6840627fd784c79141a9a4f77d9f938bfdcd8c69db2ce4f8733924e646113344407e1cf",
    	            "5e72bbed520a7b8ed4168157cd16d673742f2aef38be106a8a461b878af563d2f78fa6ca55ccbef568ff3cc735ab4d1c48d8169783910bf52488cd9ffd9f8cd4",
    	            "7aa748a47b62072cf213afce5f8212c7b2c2c7d90ac9a4ff3e964350a76eabe400379036c33474e1a2fa0d128f3cfb59249cf01fda102b5db8519f0b9a2b4f52",
    	            "4003434d77081d59ef43f826de060d4776d94ce8c9b7e0d11d3c7a159addcaaed691843793ff0a6482c1f9eb5740765bd47720cbd7660ea0e310a76e09bc38cd",
    	            "2adee7400ca8b2d5ee3cc614b7de1a94d70246ca6b29b7ecd7b01a59cea0592730aad9fee593c9d3d809d47d74b901f897e8c7901ddfc68ca5a1971c41853415",
    	            "b2bd8fb6dac755aee5cb7e834a697ef606b45652537961e587a11529995d588f11c28f7728999e19edcb07cedb18b98f843d240e537ed64b60c3f5497fe4d661",
    	            "d2cfec1e66e0ae55e01e17bdbc35e00c4f18588385433da3c9e823ea17fef5d17970dcbdb6108cc0dfea4dd39a30f23646473426d03566c622eb1da0b8b3a4dc",
    	            "065f8a62b8ec8d3f3172015e32a5714c30627b53533daae508ec85fb90048de708a2199d9ea4efa6949139f7526cbd8bab6402cbda92de82fad802409ca2e772",
    	            "a11ea682ea8b07efe7c1290988a8deffe453aaffae4f3dd6619d96e496f28f5fe3556c3999fe9fcdb5a19f0631580e9b1bcb74f5003f512b92bc068508cb279c",
    	            "4648c011888388ec85f19bd3a5247dc69ef1883d4472d6f7bfd5fdcaef3e35770c65ec0c32cadebb91471301225c21d4d64a520071b5ecc3bee8e0d21d6f6d3b",
    	            "f62d5b8a06576ede68c5d8454d448202ce0b1c85ce20a139739ce78e3cbb4810679ba2b4e211a2b0c03d064e3164b96ffb94f04bfd4d504657a9866b0eed1d5b",
    	            "cf73ab693e86e45fc33f9dc174443e7ea4e8acb131257f5ceac4503d9c7a1138342e2b80e6c4fddb3b47b00c990283903039cb5622ac905b3b9c1ed7c9982194",
    	    ];
    	    
    	    Whirlpool h = new Whirlpool();
    	    for (int i = 0; i < data.length; i++)
    	            {
    	            h.update(data[0..i]);
    	            char[] d = h.hexDigest;

    	            assert(d == results[i],":( 0-bytes)("~d~")!=("~results[i]~")");
    	            }         
    		
    	}

    	void testNESSIE1bitVectors() {
    	    // Part of the NESSIE test vectors and results: 512-bit strings containing a single 1-bit
    	    ubyte[64] data = 0;
    	    static char[][] results =
    	    [
	            "103e0055a9b090e11c8fddebba06c05ace8b64b896128f6eed3071fcf3dc16946778e07223233fd180fc40ccdb8430a640e37634271e655ca1674ebff507f8cb",
	            "a892e8125f792ee5997d175257633bf889f947759ad6f19dd233f467a3261643f815ded3eed7892a315402cb341fe713c109c0c217a9f4c53bb9920af88136e7",
	            "3732a6b86410ddbe043e4685c2ff369dc2c51b05a4331b427fa83e8020cab052bcb928c89d4777f91e3a131bdbba2698fd32ea80037e8eeb823e13fb3e81f9d3",
	            "4de6388058d0c7eafffaa2ca63c92a39763fea45ea4b200baa5c50b70338d66da002fac469dc6dc8a8494b0addc6d85071a038f0f2536b582d8b77ac02fd0057",
	            "6feb101327cd6781c8453433ff9e8b9dbe232ffcb127c4001a252226068b07cc4113e318fb7b67805284d519a2d516745f41b3ce876f4b2d6c1c059c025290d5",
	            "2d8b6102bb3379d8413a2b51306b3e54a3d3094a3c821b1dfd1540b6d3e6debad6df400f2be4cc6cd37a1d7607d4181cd2bb81afb549caf8660f9549be0f2338",
	            "866cffb7be9300f3978a3dfe4d7993995b7489262a817f851482edfea9f3705a851bdca07a60d02a523c418263cfd41c3961fd75748706c008ccde5be886d47b",
	            "11afb4234aa6d723be6a8270ffbd1800478fef76eee17cf62e645bd62fb1c702dd32b2d8e9c2fbdedf3be1efd4c8101f768127f0e7391f6600dbe27f0252a024",
	            "357b42ea79bc9786975a3c4470aab23e6229797badbd54365b5496e55d9dd79fe9624fb42266930a628ed4db08f9dd35ef1be10453fc18f42c7f5e1f9bae55e0",
	            "b9baf407866e278066c22bffa247a94f2507b03f4bccbd2a0138f96d6bea35d051c10f7f7c2f3d696087d1d2bcf3759acc32ae5cb52fb894b46d27a85e0095a4",
	            "5448d2a1d97128d1a558f6cea0c0874cf56aea7d41bdda8b2b6e78f97e4e22c69a70cd4a07e63f1e808d1c4dd9e8b8ceafef9b0898740c7db40e16da3b5bbbe1",
	            "1c6c58ce9882e23ba4362f31c643d4697546589fa95067cc791772bc90601992986529aeba8619d47290e8035febb5c064b23cd3c83bdeeba113e9b31385097d",
	            "c59bab69a6f2fe1d366ff10c4c22613c7b97c05209c165459528092e5856ff097d731403be8fd91454a1106eff25a3232c79c0622389b096820c2e2304beddad",
	            "964b17ed636c288ae69c2f7de33f1714a54e66d5b7c3775710f5371006f1a394e9152bb42e18e150fbc700f306848c2eeadc35a5158cc2fdab49ff79f35ce99e",
	            "c8156e3dee775762f07ef1b1a4405d032e1a7603847b7ad4c63ce77a4dabade65f528140ebd7839469558675cc5d14181c56a6b202e61e9ac81247f5dbce3648",
	            "6a4ab5336689e2732ae8f0eb7c823989d62aaf693e717d00e4f05aa0c86c796afe435e195fc285d96335c2804f68c2814ac0eb5fc04d10ef53ab152611b2e06e",
	            "14114eebda5d9704d3c881764e9aed5606fe7202e966390308335424c5ea93099b9c035c26a2673ce5c2f464407728ef630e48137529114ed0dc6c00bcec3ce7",
	            "d3697a1d13c7dc690423ec26eb57e81c28f00e7d7167e04aa927df7c0462ee2df53416183dfee9a2521262326c7b318388053bf857d0b324b44583790adb934d",
	            "b39d47be99521f88e6b2bae034df0f9f6b240b42b5f42809ba0304a3f411d2bdd54ab21db1124308a12b0b0cd6c895294038cf38bb3961f9a816f44743c9f645",
	            "6ed819463a6fd3392fa1842cde65d87e086d63282f6a6854907c143d7d8c63a7eaa8ac7d63133e094b039976969acd38544107bbe33fbc4a2d3463817e48ec65",
	            "e84dfc6ea219042c7c919e03869099bdb97ea1aa71c416a5a4f2049248396b3db4d85011bea66d0ece011cbd16a0b8ab6457b89398fed7ddb9a667ecfd23fb3e",
	            "934913b921ccfe13aceb83975bf6e28576d8e998154812068a486059f75250369e71bd9bdae1071724f3b92333f48fd284dcf61d38be62c84ac0b4450b165e22",
	            "fc6f10931f74c49fcfb8bac7b68a5c16378edecd9b7043bf80a6c2707451a30c8a1c12e29f773632552d5d214f8f2e11a53f6b475a2add7864d5b6cbc51074bd",
	            "c9c4d7c298875beb1374d822804227d49288b36dae94e70a58bc208d1a28cefe056310f678585a119869efe687da79ecc540bbd33e2ba30155560cc9a04c3e75",
	            "64f8095e01eb8f4b942adae314d42899379cbd23c381cf8d7aeb919df6c242d509c5c0d82d975ddfa356dba18cdf0f831c7eeb04a67bc04aa28a97d099e768c7",
	            "b7d9f7bded13f925442131d63742ebfb66b84ed67ce00b0e483cb25ef81dc8cec74d77eefeeb9232956139863b30afb6180dde52a7b3335bd15a5aa04d295ea7",
	            "8803a0fac2788f7c5068c1ac763b72dcb561fdf2459264307b7e95e07abc2cbd5f683978e305c194d72ead88e999fece82359575333db94c56aca09f8fe34efe",
	            "8debdbef830c747d09252e4e83cff63fc8b8f98a85a9b4a9ac8d9e3c525ff22dbfbd8dfc6d1784bf031586c51b316a2aae3e8785244c7c1122f36ee7a57dd733",
	            "eb1b743bd44c83119fe4423add4a8d5504a299f2ac6dbdfef18d90249e17401503d06b02d7535bcd18598f7b14a8427fcafbf0e8dcbab27535a69e8c1ac20610",
	            "d67a6c6743a69e27a0a95eeb36f6cc93643697d61adf191306e1476ed5e6872be7c35f5679ba7f96c76ec505ca32dd4c3905488a745474898402b7733bf6ccb5",
	            "bf1122db3e37bd96d87380f552f315a0dcaff494d8c31de7b91dec4a318f44afe5f74740d417f61baa1ee250c2def272c8e2ee7603e008ba2d5000bea8197ed5",
	            "d691960b1e93ca5d3811989ba6ae466001c5a13b8fc34d691b4321fa7e9ba6353828959183b11e0e6085261fe4459dca02225a8c9f1394ac01e2d481aca8ec49",
	            "403d2c5e40fbc43c2510d7680c1f7a4da75b201fae5640f0de53e3402240b8982cddaa10b018bde1f687e1a1d5eaf550b63e73579e9de212ed50f30d2b73f13e",
	            "1bbb4c044da78969e15d85233e816be172355ca39d22fb0a5afc644a38d7cfbe094bfcdde4548a957f7cba130132ce259af1840beee88391c03cd69e23f1537a",
	            "178791199c4ef1e59cc24e2664479715ad0ad8552dd08b9a5ec64653cd0498e00cd110927dfa64ed14f550a1aaaefa1848d27ef7da1b28cf39727f9ea367e369",
	            "abfc4d9c1eacacfb38eb0b0849706460613b58a5ee6b3e4d603c1fcd082db5df26cc3b7dbbe0bfa7fe8dbfb7e0b3f6218fb5b6da89cbcacc7ce5dacf2d369477",
	            "4fe2b665feed423dba73cdcb66b3107c35b3f7d7f66f70f1a468d23c855e4905f9c828101fd8d21a82ae98ea5cd05d1a453b27449b2b8d103a7db68d057ca9c9",
	            "224ae6eaf9d09f89161ad7eb8aa3c5ee599da2d7612ad6864ea11f899c745defd67df7476bf9b97a377558b743ebd5271733b0cdead59ef98c0a26f1ddad4e60",
	            "53ded6bd9ba59f2233c82e64a063c984c7ac6a40c1c4d235b7c8bc42ced76c37cab88be04ca1508acb47a6e37b128ff97e4ae58768aaad4ce7b5613a43c3107c",
	            "0b9e1f3b1a181c966cfdd6535703fa850ce903a8eec30b145956a807173856a2722514e1ce6d9e6a651a6805dbab0b1747a96c742c279d575aeebeaa9a89c45a",
	            "6595b49da5180d65d75ed9e0295bb4370d955211244d8eb693c486eb495e2df9aa5f6e9283b7bf3f28f573b592fef545ed9abb80699108b18c9c78fa0d362391",
	            "f9507807e8cae00ca6e45272ca59ae11149d2ae47e5e47796429fc55f7808865e319a745d0c8b7138d45238a9933eadc8aa99c67eca18fc63bd5b6a59e3dedbb",
	            "07e7fae6aced1af64aecda4a4a1af80ede9a3b8e0f414f28d33a9dc1145ef17abbfd87de2389e7f25573ee543c00535e800d0b3608389d88b74d11bbe417118b",
	            "98dcc619ebababc5c2cc80161ce41f268ac9c1bf740385e1e5553428d6030bdd1836333ce7b9001fa014693d08feeffdc242f490beff64ad7e2b1aed7afa73b3",
	            "d091be5664b8ae1920cb92524a372c527b9dba8d09ee038bd9f28f25afc672bb68eec5dc275b33fb39129f07963e53f93f6b5dc6ffaea1a1ebb03910232bb403",
	            "e101d7b6536bd4efbc8fd2d775dc35fefcfafb9172f7e0ee0ffb544f6e65ee3d6ba8a39042af2a7c331af415f85f925b7186bf0d9857eab14c6744cf9890de1f",
	            "1657fbedbaeece58d98ed8cfb436e0efcbb88f62d285496102af74736840d004e13307e7297037256a0325591e9614d04407796d6820028712ab9b472d6ad4c1",
	            "f87c810ca5e4ca9b9a7ca2a21cd86380aa806fd94736b7f884e2542fb1e523d9e9e50838a5fed03874c9b275e27f732e92a19fc4b0de3fa10ffe7621ab1a723a",
	            "1024c03e7b2f27b826131468e1893f9ff4710d65c1070a6e437f6661d715ac0e6677df09b074e28a4e8910b6b0cfc9a7769a22873e42e47c6da186464e9603d1",
	            "c1c7133d10616d74b80ec351192f188f224d1ee4e8fb3ab2bc8784b06aae0f117b704cf6186de6169ff36d164c9a7fb67cad483b0aa5218bd58cc0e705e37a7d",
	            "e6960d7b1709ea7aca83d25b0507cfa0ccf626619487bb6197ea1b514d7321bb5fb1ebb47821ab036ef927cf9c9c85f949245640f209c202d1f31a732544f3fa",
	            "e8529c7d4c1ccdefd6b442777465795d1af4f4c3e20e3acbd7107c9021a4b95b49c4f140a4a70461e90a290609d54defe372327f757e16d0b44afb09b527f79a",
	            "b009af954c683a3fa8710e4c4fbf8ec25c69a38dfb104c3b0874a588647154f37cfc02fa80caf5971a14f9dc5a1231d350952e9db4b0227b400d6e4ce05f9fc5",
	            "07cda6dc272b317e898d87a2e13cac99e06039b1635b48d5b0797d4af128ec391ffb2480bc80aebd94c3dbd5736d33a3f76e00e0fb900352b2e88f3e3c69fae6",
	            "e1443aa0d1a2f1a3094d898363bc5a1a5548fb4feb547dd828db53463fc0b48f2bb1bb03cbcb86898cf85f3384f9d9d46c34d337ff8d5e57f5c8679b0b96fa69",
	            "1bb3238689a0407d0041ad96ebcc12cf6c02c29cdf0a79479caba0507110df41f12f234bec0c2e25894ee3f1205d575803deb895dee791a65d341856af1c200d",
	            "97cf4e8b30162cb60314ba37c799a52a1eea7385b1f5530e9ea3a6c30e07c673e4898498e16246de9e48381b16ee990b3f14d83bbb607bad0b9728723d8073d7",
	            "b075f46cdf3c57b1c3f01e7dd6ad79edf2dc6354696271c4777805a45a1e5432f70954e500a1e2e6f8e5e9781474ddd82247a50c7893d7054ee1d511ceddcd67",
	            "5d2d5394f8b72861172bb232200c0f6e1a12c02c778555351d0f944a2796e5a4b69e9ae6336f9a3ba432965544c807208ff1221eaf17db338048745cde3bedbd",
	            "97f355899d4dd07b0f1ae4bfa4ad6f4819f68c4ca72f9ad8e266236f9141dc12c2af84863f34d3afcbc9991f94700f14a96f3f27c4e46b96944091d6a134ba29",
	            "2156b2f9689c4a271720a39829360ae88aa25d73eccc1292efca7efd690f3c53f026f591e84f0831b6c5313418912672480591668e4eb568bdc27cc838b0cc7b",
	            "71f0570ff6d21a636040946916608e626ac31f4c511a4faa4ba0cfe76fa01dcb9d1903f3de918797e1070132496b80d60227748393295a407f08f1d1f0141f34",
	            "2c80049fd1a19a87628c8da17a996a44351de966fbf6d84fdeb4d8e11891e058019e26bde1121397b32ca3b65c9a8d06e7b92526291403df9922b4e432330e9a",
	            "12321b915b6a89a62bf35ecfcdcfd947b6a1de3dcdc912b9196c9abe638fba7f1930f1040665d008fe8046a4c7a830253b941ec9b357335ff79090f2ce59440f",
	            "eff941d48b9eeb733dc91e0de2579fc664adb7882d66a60091435988c1590ecd92a372256b58f3a6848c7bc106c7c7e66870e47288f0f5233dacac69a55f7b31",
	            "d7d2eff2d8342f71902ac2abc6173126a2665921d1d81c2df4bfd84eb96839ac6e036a23c57f9b6f45777a68133316df6d759efc21ad66038d0f16b7b7c8b8f7",
	            "5d627ce22176699c9fbf21f5e3b219934830854a8022839d08624a0b8c2d890bce8f718413a9f3d203268ef43c6441c12480c994cff4862c5f210d0db42bf7a4",
	            "880bbc267653ed47306d41a702541d08f4d5aeae949dc1012e9b069c8fc81ffcea9c21031fb3a4fc39e86139c3b36294f2987d8232ee8e847a9135b3d444b849",
	            "0faf0e9b5781fbee09b1f4d3c5b8fe1e120749e84722a749052d5622b621add620d4f99019305a0e141bd72c9b29d708f54b4d53f92fc416ce87066d6a710ac4",
	            "331095fcae9e3153b65c52630d4baff016d6b34200e31e081bc8e85ec324e56e6be719976a442148a046e734fd657b28868b47f787fe7eb1eb49e3c82499e8b0",
	            "f92a406a1354623d0820683b0e14091e81c5f202feb314b2773de246d6994353e131fbdb07a953240eafba124fe9d970ff953613127f6768fcde59f6c5fe2b40",
	            "c81606ee059bdc3426b21c6285defdeb2ad8191e351cf33fe6b75b605cc6769cc9409b081ae31c3c4fc21e7c9ed1ef3f973e69de11005dff5b457cb3bdde1408",
	            "2198b86bd6fd3d0b3d5dcc42d58c7bad3c8eeba02ba25984ddebc92c7f1ea8d69b76dc8b4ff98ea3e084dfb54c1bd1660b5b292ecbaa9507b453a8e31dd35a3b",
	            "76d00bc2f0652db1296865e10a10de79fdf5a9a7b26ec86ba277cf2d26293780af1f58ba2eb39c0257b38464a4f39f6235e1159d64bbd640870cf3dac39b1999",
	            "6601997bf4a1d78a27de3080c4dc92180a2d648170617ff3d934441da3ad0ad3fb53041d0c6d6efea15ffb446e3f7ea0023ed2c423813b6df19e7b14f5060fe6",
	            "0b4bdf9210b0fd2f98378a0bf51ffe7a5dc0f7a36007ab463bc8d8875fdd54115f8335a87abfb8c3ed7a76efeeb3c50e74fdbc3c0b8c32847a3e63e957f8eb92",
	            "2b24c142aaa7f30dad11686dc5e6dc22a47c65ed6afef2e09b2bf51d48a0675fc940457065ba9b2df7b4b75163b74a2ceefa63e78f655c828d4a9e6bbf5b013f",
	            "75afe719227366682c8ba7693e0b4804c68d5e0554e9fb930f977d565ff2832a037d49cb8868825dbc7d4478de419122f2915cc572d7dc4397ed7059eef70f3e",
	            "f81f99006e902387f3ae9b738948b0b746224233ace8a4c08ec98e5580bd039343741b733820a79c377828663a162945977aeb8a296b450662e172ef191ef58f",
	            "b4b3b9b7f0c3500230b571bbc66be846c63a8df77c69db7993ec441470433c475fb4238c153db1a5a4bff08ef34f39ad34364df8e0743717765ba8bd004b352d",
	            "341e6104dd8c1fa56a39734ffe132628e20f1e793430f7e6fe130b2fdba198bf87246dcd3ad06ae487cce3b48085b20021b12aeadb91965b8d183a4e77692277",
	            "db59b6522835804d82ba273be37631e2e6d43aaf68153f3f17b267191c8e96547bed0e40b41c60f95d665a165574b0891dfbaf3161a23edd4d9f9bdf357e76af",
	            "f6d1821cb119c08388f1dffb8f2130e263cc8789a5d94534bebb8d2baa3e0d890e9d9e38df5dbd3d743f9da176f831731041693cdf4c24e8666f64994bf9242d",
	            "fedf882ca077bb513431fcbe6ed2de3a042519f049bfedc2ebda4499b6c7d398e2eec9357aeaa0644fbceb620cb48375dd53f821b38835f7cbc0588344d062c1",
	            "17f78508687ac4a08209daed389bfa781daf913304b0e42e0a8113ae1574f3d1a01ae87e7cfcf3ce774f5ef5d6405392f1d33699279b36e9e9ac29df4c92a556",
	            "19d8a87ee39e399f8d997e252534744cfcd7dabe9b95742b6c6859e52a30b12597fb5aea9043979e17e3ccf1b4560b7541d131558e3397abfb426d8caab791e3",
	            "075003a8742342a140b18c5030a9458bb57023c90f0cfcc1fcdc040c62aa350d0b3a9124f40a4539ead802909da114e121906b06b49b8857eea9b1061a787e28",
	            "e0caadd68471de9a76a8023b42b5b802dacbf3feba4ad5bf4ea3773e5c4f6ddaf225de3a770babae9e06006fab4063f8c2cef0e15943ebb9505c309073a2a27e",
	            "b7bf3b25c3c06212296ef3387fe09866aa9ba3b071383a7851ece5bb1efab81f22f1048b7ca26388e483c6adf63e25f984b9b644f78f5e0cd69f5baf9ab5abf6",
	            "492251fe48cda38f9109e0626e018b6b43aac7acf90415b32bafc66435fd05aae93fe15d7a08be29d6f8398eb9942918b72720f521845eec41bad7bedd86790d",
	            "26ff3c6d6c15a15cbbfa8ebb2b96f92518e4bb1690dcf73cb66eb6d89fdf14f148c52605f41008df7a9e3812645040b052be15374cc42f4cc15c05be7256a729",
	            "21ad90784e7b38f399347ea35d716c871ec81be2c9dac152015127da5a12d174103a8dff2e8454b4175fe17ca6970eb7427a7ee1b37acc94d00a84fac5510c19",
	            "d82fa12ae18aa4f8d8e1fb9afea749ac59f21723ba81735da6c4d585fd187a3045dc52e3b90f57b143c356199cf5361c7cf614a5ae1c871cfa61a176521f30a9",
	            "2179e4e9e7a6db1d5b365054846d31a9662ac4209f7c9902c806132ab4f113642fdb2d5f0e77b902dc743643d20bfc8fb045fbd7ec57f4d95dd80bb55f800343",
	            "c7a1fda64284ff73d2e88d49e6fca674916a47d885baa0e5b979674b703a0219b4f571a1a6df36345ec5b8335ec9ecea0953e1c8fe6e1375cca0833287af9fc7",
	            "b0600a74da6f95b5c118e976e927e4e1c5955576c97c4af7bd64385521c2c0eb14feaf0e9ffe5d476d57361f828fb4d001785de3b8fbfc8f095e49addcee7b8e",
	            "ab2e066882ba33a7522f4bd3430d2e64adab6479c32a4fd38778d53e0d25cabd077bda7542b20a58fc19ba90f723ebe8690cbc90513acf900d439bb602f8f404",
	            "c456dcb2c50a4c10dbf5be639da33e7ae7979726be9e3b44d3f051b989ea3e4094d0f450f97336a5a1eed74343defe52ce2fd61ca9da8ed3cfe553d837f001f4",
	            "9a7c7152fc7e9b639ec3b7475c676c22c2e03151d91349a3c2b2f88e6344f3d775138466a4cebdc43ee8f21c33f1fee7b3b57f828302276c61ae4f709fdcdc47",
	            "dde6a3cefa1f451ee013b44c573b7172f711afdc3e85277275f7aa13962132b7b902dfcb9b260fb089fdf006623c0dda6040cdd0279a30dc58ef7a59ec0f44d4",
	            "cb6c253bb4746fdce0c3c215f904ea1465ac5b25e101eb61aade30cc3dfdf00fbfeee970012dc932a96c76b7f420e393068e104d781d4edf833934911053cdda",
	            "191af857ff2685399a8f360ea07b5b0cb015bf45170ad8ce5c793cf9f3c1b48b5090f00e0b0124028254306c43e4d3e314feb5cce29fbeaaa9aa06a28b56ca12",
	            "700cc30f802a5542f338d7624dfc932931465314791f11f13b3ace81b5bd5794ae96281077b22536518cf7cf298334949d0f265221dffd049a25b1d5f37670a1",
	            "467345c507333de5bccd014652f06e7a838acaa67b3f77b96e2d8246c7aac7447cc806f317e8e92406b3c5be08cecd3dab95f71a8a7fe31899ad3ac5b598a3c8",
	            "99ecbac3f490f944a17dce59a63210a916f713b1a5e2c7e5c7b39b00d64dde8e588577f2a574a74ad782e1f9a3e4b14fabdb4730421864cea73288cc4b654a2a",
	            "16edffc937268e20d77425e0be980c7c9d94ee1144e5e46264adf0104685d10007297cdfd14ab4ce50b7e71043f41661e29553cab101b911836f6c41d9066bb4",
	            "d49ce7b8aefb3e5a96820a7a0ca6658a71289e96a1583b520c8058044621a4168810e177c8c55e6fdd92cb0090971414c7705a5ac58df66cd7860623c01ad709",
	            "750d86b2af430265645182c15edae95656f2ee8bf16a0b3bced54a30297134c6e7459f0a9f2d92d101337764e38fcfec8eb6f251d5518a138fdfaa895a59fe2b",
	            "9799ec3595efe5426a0fd98829c26227c791360664cd72775b5ec0a1e291a1f078d5735c865f2a72aea82980629e02e383734a10bfbabdd2658810026bed4659",
	            "f9ac0cd55d8fa16da7f22182a460691944375e3730420ed5731d52e2e715e87193864a8f2363b1fb8127aa711bfa82e63a184bbede099cdf91bf7d46c033d947",
	            "da040899e1980415defcb86908a04329d3fb295f9d83e47fa343e829af989751ef029e77e38ecb4d27194af6359a3b1fe39f1131022b4043db932775fe80a58b",
	            "edbc92235f59612838338e0fc5eff3c377476f5a51e4cdd58aaf00fa191f4c4df10ff9ae8970aa49ea032e55054b930b0f2149b673d8d9d85ffb0b5eb266633d",
	            "91f934dd273b2c5ca4f3155a10bb271857d563e0b503c954fe1df528fef44ab5932e1713b39119fc415768beafa3d0a3d3ba3a45912b0e0e9c26fd03b7ba48b9",
	            "7d606f4cf0aa403db41b5c522bb9cacb909d565a0cf8578974614a5e46f418556eae21dd1abbaeadc71f376d25982e16fe5371cacf2104c416a97befc028193b",
	            "d5f8e005cc6ac288123ccddab8ae4c7d3bb30dd56b0191d15d8d633acc42d485184ee42c93b50127f16341d337e2664282ff479e0f2b29d1c29b4367a46039b8",
	            "c03b95e073edabe35f3a5af4fbe3dd9a179e57572c8da147f03afdd7fe420fb155cf156eb5ed09ab85a23229ed9b13b1552c3212d7ca75adb1f7a1d0e0ac028e",
	            "bd29cb59e97e4274c5277e57b4f4fcc46ed64cb017afdf3267450e3ba633cb475b52325c2e114bedfb9f19ea2a919b0518f87b707ab5b01c45496eb0257a426b",
	            "b70531d56f611d32e2eb8a91c6274c2b554b2d3eca3ce5b211fcc86b77906da604943dad5f445faa563fea582a81a226f12cb10eb607207f9d9190b4667b707f",
	            "839e6b2fd4c5be65b0e9673dc7339508c29fac7d155ee9120dca5fa94768d8cb1ecd8ad11b023e02137850ea036e95ed26f985c4b350ca6b6c33af6c423bc34c",
	            "6402f05d0c2c09c346249d2f96cec8455af15804e599412b80c976d23caf49ce5246ae8693fd3416b0c7be9feb9e6351eaff67ad699a311d099523a90f8ee93c",
	            "2fd6309b424e415e30981986a7ef1c230145a422886697a7c09680591f1d9d6708c1722a1c94b9cb8651abc1cf13c7abff5a9c1485a4ee1223e3a00b6dbfe224",
	            "f956e9a34f103628b7a7289108e5c1967a5e65bf7797900f5a447394152722bc6d89402b8d07ce85ba9da1b7f05c2c25267fa72647924046ac2fd3906bfe5e2a",
	            "f8776a17eff7f015a6df41b0af68832802f66e0ce61d1bcd043249cba7f8f91b6b720695bd3dcc93970de2bc58215f22f123aba716d0c5f2215dd4fcab3d4b58",
	            "9d73687f851fd1864153f481caaef96a38da54e6c067d4af3e5335342291136205cb320f48389eea36799c2d064eee1eaf2ccf3a60b2ed8a70fb261ae497b231",
	            "9f58d7e9a4dc32b3265ed18894157b02c4cbb3ccdf4b23f03ca76f5fa16522f714542aa9265a1ea346cd130074b2966987f9c8934730b2216a17ae3c19883c24",
	            "5d478ea37ec162f06ac78117c20ba53311eb31d829e661f6ebad7fe08d274009e85474fdea902f9916379c0c5b7050d4c884cc7ae21cecb1326cf13062483e37",
	            "226b391899ebbb30eb5d051ab3e1ad0dd35fa7cfa5004cfc5f78f219df975f37f9b00a3c9585712ee5eaf3534f25969b6221065399778fe114c99e88655268da",
	            "9c8537ddf52599e884d52f85538f2b39d0c6e509323e271a675b98ebc81215a5ba3e90fae4ddb6981cbeed8a6c0ed0229948765aa00ba7823678823e19b469c6",
	            "1bf1701a9fe92dfbc335bab1db49a141ae908068c96d56511b2b8c4688acceb2ae1459f1bc726923e5fceda99b976fa473b4f1bc35d6e9a37f0a9b3915dba775",
	            "c5d30e94e5720491acbdabc43223805b093805f8a779b85ca5da3d57e9bc7010a0392006e6ccd1f5d172ee83f8820b7ce50830b1f6b0fa4f4255ea0e5ea99e30",
	            "daf2c7a5ca742dc687db97059525bda214a9717619ba63c8bdd4374b266ca9a8575f61c678378a41712b37a13aeb368b1c8b82cb735caf9ad12018825e657c02",
	            "a3ef1ab0bfa36895a53c8e9796341aab5b2bc71804e77c6942f417d4be98e52ca8bc9ae1c2129ba3467cf4345965495e0a4a7f2df7bac7c9f5dccde2e35cc0f1",
	            "a312050a4582f54f81b6026bd29ade0d15bea69abf709229f6f2e38b5f5915eb629a2b20369da9ebeb2c3ca8c1fd672810405e17c8e34e8f610482dfcbbb92e3",
	            "e82b4459973244a04566bd6e0378a6cd9b88d0bc824a2588931b04e3ba7b16370e588b27dba2afe2ca6d84991571fb4d9e9b51356ec2ecb5e3379e48d154f927",
	            "d996c62722bfcdf1393300c7d4d01704e5bb0e56272de75ad5d199bac02b211d334470500c836b1914ba1de06f1bffea4db8cef29597a702725c6a0e3fa80ab1",
	            "255c15b9227c07addce709bfeba38d660e90a69d43efa2998a9ddca681ff271382727173610f6e9ac98339288a3f8dee507845a98df7ed30d47ed088c0e020d8",
	            "32452860b1c8010c95c4b2d94369fd9925675d5edf6d7393d6a2d1ac93075fb22f13b215296fcb9eeefdebf23eddc6cd21c368e31fd7b70e03e25f416447b1bb",
	            "6b39619985f349bf9cdc0d0c39c50d1f98daf5fd33ea731ced81b2c75ef46879cfa4a93f16c43ef289cc9fafc2a0290f51dbe9e9ae9488c67828cc2b9be5b241",
	            "d3f2e49ec1e3d5100dffb95bad98e88ee8e147333216e0be408bb2bbf6cc5d8e94b062f32d3c9e190ea23caa4d556c03e8bc1254f147ca8cdec8ad7d37c42d78",
	            "a5a56b183085366dd216bfbedc790ea591824e84463e4b6125a9e19d722e884b2aa8c07dc66ffbd429d49689193012b0c5ac4e79f7d3e4ce7cc031f915aa7d4f",
	            "dd9dbbcbf1f2ae29dd19248321da914f8dbb41caaa2ffa2a6aee399c057735fdf7af46c21749d8b2edd46e18bd09bf8312016ad0f5ea2e2c989c2dd9d623baf5",
	            "dbd0e585ddb3fa6ce4a13216bc0f3f10242ab794a2cf19c8efe8f1a573a08c7d5d23be3b1a00c5a9fd8e8bdeeb20672e29470a26799c7238b89a4521310b7d8f",
	            "7050572b79aef7320307b1a6c29683f050fe46929226a4974cc7b4c382f230823ddb9cea8121364bdbb54d42e12e88334bffc9685b0ef9c3cef3c6f3b0e4a259",
	            "d84b177ee3f19af8e70719239704d331c400e9272324798ef7e23350151ff4b46a92dc4ffd69849377e21757b9a0249b25cfe5b28bf764581c983f78e8ce0418",
	            "b55c7dc96bdb9774fee73c8d057056abca001a19e89aace7af9d9d00bba50604cf40d789e36e4b76a2c599b94ef50c0a0d015598cf5559f17c98750711142ad0",
	            "3e27bfc768fbd93185a1cce8c7161e48ffc9001232eaeb4f5fd15f792755632bb2459f4eedef8cb8d9a65230b858848d7a9ad8c0dd8915e0dac651aa5faea849",
	            "82c88d6a5a5fe508a5712ecf8076fe9c7d7b583b2ed9f433ee0b959d9d81cdac46e7d10991ebd050d4e3989fac89b4fc6b2a1d1c92f3bff5a51456e9dec64a59",
	            "44e9b97a336e1ddd4aca21c956dc84c9082dbbdfdff86d9454c9a0e962152b481c27b95c7ef182b59477a580b86feefd8fa7c94311680a1e90c83ab6379952f9",
	            "eddbf847c6bb535a805cc08de8e519e97087c3cacdc272e9eec7add6dcd41cb2cf136e9eae003ca62b76697cdb38f1e44cbbc51b3653b99cd24aca97bfcd62ef",
	            "7c47e5477761f4323d9730b5af5af70906ca3cfc3b899a3a279ab4d1c3a38d372d9f0df89d826b8cfa0177dd715f11d45f02f7c0404f1c801157a58e6c056284",
	            "a87536663948f75cadea1bcd92b6ac3d189671180eb6164e59dcdd1b4de8a549c831a8391e58d28184768efc8db7a52b28c3402a0bb276b434449754a87d9609",
	            "73166a966407dd2df4fb6ebcc300b5f53c4a3af2d120c4bc5d10e7149b8eb020dadccc882fee71916ed8c0f9cfe4a777f26627eb159409ca7da17e0c9b09d335",
	            "1a9dc334c7ecff98053343a2fe9265e235b14bceb7a54f289fb7a2699446043e81824194ae170499b3bdd4a4af062b0f3b0d1c74ed9aca911dcc7ebf8f123d4f",
	            "82e1e65019c35d483f19f5c4ff3f8c4c23b85ab708b9186dc48198acb124b17c1543890eda5b7b1f90a70f9525925271a5551db56b04dfc3e7e0ad1bf4873a26",
	            "3f5b6e6b1cf707007b3de6e4dbc852cc127534d7b6ac932c46158bbbf76381e64e1ca8c8511730b542ab5a63a6e6350dbb511f5d0788c8c8294be5bf36c252a7",
	            "8f27c74ac3d616d76f828b5ec2ab4d12412c8e97cedbca5c3cfb750710c44334be5253a35e19dda6e42b516606c0d1dffb6787175a2015d4d4f8e260781f9d8b",
	            "b6bbc8af06fe3d7483aefe2debc7f0d87aae87ea1c8e5085651f4926759e5260dd42d450cd62725ac1b2e97143690e51194c1b47f2537b750bb956d476c17d73",
	            "8d33b1ab7de1fa265be9a270e40cf9a56e5ef230cba517d5138e03cbbf964fab41bf3e61096b5c2cdfa8d70675adb54269721be0ea50e0e897a55e726a76a6af",
	            "aaaf566554b615867e051dd0093a43fa53f5e301f690eddafbd9cfd5080fa7ab3136858fbd8e1b272d7f80cc4fc89c512b37c601fe47707e297cc3f6bb3ce4a8",
	            "8b5460c5124b72bd2b74f9b07a9f27e18b06fb84f1952f83c024cc685cddbaca40cbbdb520353faa8b7b826c3b5451deb2e9353beba61d3e8875a2754b86643b",
	            "e41b56e74fa0168f72f48faf65ddddfcac39c71c8b347d4a0842611ef636a6ea3e3ee32c2608fb40e18128638cc5365a072ae386c923e776bd29cbb5fac8eac5",
	            "a5fea58f916dc06e07dc4c17d0e99fea94b39bce0ae961b531c2b84bf08f7553f806cfcbd165b4eed1de314880d6975120fbf19ad237ded125c7909fd14a48a7",
	            "c77d064ca2fff6a75d8dfcf5a5e05301022c5eaa5b488e405189f120c13664998814eae9d646214fc782a0d7ebabee9c9e743177b0fa359301b3ade3dd8718da",
	            "1a3f04d23814f7a639bcaecffee688916a8e3015c6f8702e9ac0a04c44bdfc3fc5cade53ea304e8e316a2824d052db0e3bb962e051bfe761824d7c83c60d7b70",
	            "59e7c2f6abbcad4eaa0eafd77ddb76a5c88e35edba5c7cfe64619f9a2cf28380c366f2ec65b3b4f4a2644c0a17d05d870191b8ffbfe989cad4255aa7cff4a4a2",
	            "84b2629ca28513cbe404894eb9e0d38974bbc5897e17a90654c6adf5645cc32c96992f86a353ced5a28672ae588aa5ddd9c40c5310567e67eb6ae94e363de319",
	            "dcf01572bd9e858bcd3e541222bd944e1b18795fa0baa342374170f4fd082c4b506c33c307ee8e08e5e364ec74124110e0259db43357d840778528ff2a23b366",
	            "9bc5c5e2b91aa2e447462edec240781e553258299c709d319fe4e1f3f6ab909f94e53f4e61d840b5a05c5551ac3604c7005f6f3d18d511564dcc51b4da501868",
	            "4971d15d188ae2bb9d6b26a17cf834c62cf93f8b6b9687e3cadc94d8bcb1eb092fb9a6059af3fcee421cb889f770114ccdd4f42ea6e9dfec835251c4f2f8a6ac",
	            "cd0a6537663eb2588cbef1175d8f9c1adcd73c94b9ef4d0d5e2a9fcb462f706d52c6c05a0530fc2c5b008a634f1b617a0797a601b49127432d431b5265ed5f40",
	            "ee0df9fece977a92a579219681af597684008e0f4a01dbe1a99c746fa044610e213798dfdc33c1d4b140be66ec47668315117160ddc065c068980fdf071ff912",
	            "6078b1283620bfa58e83be2ed909e2db0d72e435872269bd42e4526f8a35c6c5cee9371656e2e3411cd5109959f3e2842b6c38f4038f3c84ea9246f230a4b021",
	            "fef5b389eabc7e5ca7efbf8a396ef5bddad9fcae35af432fd6a9edca18db855df4d37da0ed6dae4820d442c6a194d65818806cba5c285bc9ce7a5696de516d36",
	            "a1c292d99fae7f9567c0a91a46089ff1f33c8af773d25a98b22c884914fa24eeebc9b8ea2c028c81b3731d50756c0617021ab88ef96001b37a04adbb088dc7a4",
	            "9367bb62c840f9240c3284329b273eb368d882bff527b12f7b9983023eb3a1b200aa751fd464addc6f48595db402d9d006a9abcec6534949ce86510fff135501",
	            "95a08a421794580e9f7ab4949a4e4e5e722db17929c0823137d6642ceb13ef4d59019e05a7df6f16e2ea94bf0f785a989721e71717b29aa3c393e39304937476",
	            "09a7b1c781763ea340bed2fdf4b01b5ad672382c90f36a09c1edffbcc7b0bab5e0cbb7a07ed070b1533c070cdae7c652d70f6f694d62d3f9e41b4b097f7adc15",
	            "4f3df33cc7c9bbac58ef814c2fc8f817eae07caac61a80fc38b25fe45411940ce66c4c1efb6c4d2361a398e95a6625b24fecc2b748ab64fc431e4eee55a85a44",
	            "328efcba1f3465624efa493eb66db48d35c736a939d4478904b9d2d54999d5d2910947d8d00cd8696cb08fea8020e792fdc8ae67cd482243f582951f33af3c0b",
	            "f873b613171f37cf8d7b177d2593bac8d6e8d7ab1cd27f4abdaa187bb05201e4c3aacc49e01193919ac482bbb39bf76ce1b206311a2ead80ebcfda459ae49156",
	            "2b205223a289fb15c267fdf4fb1247d02bfca1ecb5ea4ac83a2f0e4c26034bcc77c7b28ebc10b77409f39ed9aed790df1689c6d39eaf019199ac36c6abb644d0",
	            "3f3b11dc0eb3f210ea2d5477cc026d1622aa45811d7e2fb4fdd0e48abd0503e431e6879a503af844074faf1824e31d224aea752c00343e4c264e8e634d277cde",
	            "eaa22359cb1fa05028f85fc36317d22465b80d265257513153cf85b2cd2599ba0741fca491246e32e52505fb2febf3482ce4132aa047f5256c4545233c82efd3",
	            "da194ce937a6f2a981f84280dc4c480e7f54957ca40aee792da5304de67d904f9b68cc22272e41255e5cbb763b3dacd106467d9c6053882d33bfebe309c33e34",
	            "157d9e657c3c824c117ba1f30d05b784457ca83725d620e9c6eb3192aa7f209b8aa445ef488b105cea1b2eebe38ec7f77d47d4e475434a15b7f40b84335e2f7a",
	            "6e588a470747fed57453e816735b98fcf54a1cba4ffc1eb78f23f4046deeec8f2a81a7fa16906c39aa74eea5416884b49e5cda46122c0569fc438bfc1ad3106c",
	            "c37c768c516eefce8f77f3026eac5f66c237fe0da822c212ca127cfaf0b50e83417d8aee04a08dd462f7e541942f5b41ad850ceea4193334fc9e4014c6837287",
	            "55c15edf6bbbc1c621ad1543b0119295e49c456965e9c9af2819b53a75614a03179c7462ee36c50c54adbf9d1c49b1fa8dd3edc11ca8cdbce5dcd27bfb96784e",
	            "37667341d3459e65d4b71f5038521c5f39ec2b35c1374cb20bf80997e3c25860204ec9c4a0331605751652b5f2458ce918b0ef5dc2fc4e00bd8f892e4bfa198d",
	            "2b36b15a9535238a4b7fb1d98aec0b0cc0646f1b2d8fccc2a2e86703d19855151f2c3a6ef3fe264346341908e83c85a6cb0035ac2a483f738e7d9f72925f9544",
	            "4b702247cf5813e83a63a701a82f9be4616c25ac3d18ca2201faa4be96627eb004249654eb324e9eddc11e11d227e32a00a3952aa188188237a35c0fc19877c3",
	            "cc55ed8a1ceea542b7497b514a603ed9aa166b7d3c4abdcde40360ac03f093924f75d095601e053f1d862b42886aff8005e46cc3bf40b0eba811d5595f2b6b38",
	            "fe6b940730302b87b5b998cced56048e0faecaf8abf64a3c97ddf63ddaabc13aa411819b84fd034aac68c28026fa8b3005432da78dea8bb491f03533dd9ac9dd",
	            "35f1dcf603cfa77610f9310a77b2f6b9a5005ca926c64148ae3cb8aea3ae6ecc2a3711b22944405be01814788b776a02e74b8cd1b81827f0cdb2f73dd9460311",
	            "38ef259316c0d7495901973274f4667abbf52d940adb67642becd659bb7741043d0d022cbc6cc54c2e9aaff6695edf0cf14bbbc306ab867cecfda28ac6752689",
	            "9f1c7808668002a6e265aeac130ffb18458e18cff1c83921d2d4f4325fae5c01df7599745047b6393260a1cef42f654337ca7b69f2a8f4d815cc1ce370637c5a",
	            "b84634703dba8385945a195ea1b36703dbff5eec16658d0197c00faf8c485f19c1dad0489beeac65eeb5434129604af3236b819d38e1e06832a944bfbbd2d6ce",
	            "8604705635c3411ce38fa517fbf0f6579d286316f551591e3821eca5c6f0f42779dbc9955a8a860511fbd2daa0a0c59dcec1bd0526c5d1d83d1806df3f56593c",
	            "ea6821c426e09e7a9dc337c566a9d219d643dd7d3b74641d78c6b662e84a10b678254f52bf5f8ee0586eceac431a232fc1d3e8174c89663d089a62ef4d5f5f3b",
	            "0e9ad78babeb26369f5b61bafc2be9ff4a12c72bcccf6ec9bfcc659563ee1f271330b6a0a637aa4a8d196e898995984d359bfb8feb992d898b45be29ae5d84a3",
	            "3af9574787cc8ca6a0d1707533d49455ae45b0c2231b3cd8e6d60d02af2703894df7c454c0a4728c46f63f8f7bec6b05a6b343cc6291817a011284aa78a490db",
	            "b51b4ea12d9daa6d44b31502c11c52f1ab3e21bf5c5bc25e7621980c676fd4a43115877d72c019b04121a667b226c140073a1430fb4d63b1b8792d9fbf32b979",
	            "67107eb372ad44bd2899ce1e26cd4ba7ea30c4711eabf0aafabb335c56d7a70c3568eb7582dbae201fb435c37f90409bfa450789315d75be3356ad849a6ecc4b",
	            "069cbaec93d08675082c4f5b9bc3d4f6fe10cb7a9bd4f5632f3ad2b9d8ccbe519455b94dab51ff3f06ee73a74b6e54c255c9eb8a990018e1b347ed4ba37da792",
	            "381cb8bb4782fa0058604f24e1134451ff559118233156a61869a295b2a7de0d86ca6af4d2e2904f2ec025431364f06f79d094ffda82fc89e960271ed5d732f9",
	            "f0a00756416c21cb770a6dbbe5be75e4dc348d0a3d53566c0b7c768350d171418799e675f10cd481a71366d4837e795af407f2ec2663477114752423f879f5e3",
	            "2f16287210f9146d1fc42084dd50321a210b4b3af9f4ec132e73c0d8905d0b0277be6fa0a7e52d9e95522969605cb7c24b70504d37677c637ad1ef6b8985030b",
	            "22dd95208f94837b0891c9249f38905244bcde39f2abe137683eb4db81866fd7385100ec09ef768d14beeae18d618dce4d0087e00f8a6a6c9c6990ecc3aa4d31",
	            "9d9d97907c0c18a28be229ed21e9d270c9673ce0d2742f2a4031575df9f3c9e5fd182e49cbadb95dfc5e27f3de498986aa495f6fba7df8148f0fb3249a6cc612",
	            "b8a289159edde1d5e75efd9ea2838564d19a814a39e4c304fa0ca0fa717550a9d71dda5633d8403fe275dda0ac44be67cd4611fe0633be90a575ec1c7c524805",
	            "94ea422e85365b87ca258756d51c2001762e6403570357f032f893b9e246231b3d8ad777385f9aa85040bf30c457bf4fce559baa174bf1e957385362b87161a5",
	            "77d3aed93d97ed9db206885f5c259ad3387f5b76252a90e8e92925034fb857b28d8a187bfb282005f929f0aeba0354a0f6b83386d51fb8538ed5cca83ce7c7d7",
	            "ca9f6735fed8fdfb39103684a2f52ce819a3a41757dd2e461338fed4884e59b9062d13157f659efb563352c31ab3808cdf22b32ce92acd471d25820383d81ba7",
	            "35d86b7b4a33ca2fa401e622fab7fb4a930f6399d2f1e7f94d88813f4147fdf7a1dab58d788e33a85a102da5dff47b8a435e0299f1a61cb50d46cf95be52ceed",
	            "35d5f4f3700156ac8b7261eca7be258f261aee8a99df44b76a34568ca4c4b98bd1d5805a74672836a6f9f4c7a454d07e02c763025dc957cae3c196455892e10e",
	            "471746c2b108a1a3d93a202cbb750f118d199a81fed18e71f65343855ce2b6afee9bd470e33386f8bac0fb9abc3e436cece2cf89ba91ab9c3a1e96c9d5ab1a92",
	            "72f8508410b5f64e8f29973dc121398e77389d25eed496b00d5e92c91d748345605027b190b9edd22a73d2f75bd6258a668337312f4c5fbf3c22e39fd392abff",
	            "619f56083177250c5fcde33f796f2f07a75b0fd0a2a41cfdf7382fa8bae07bbe4538c6f8f8ee6809cf7a00b79c07615280c9cb48cd2f0a05398996cfd04e488e",
	            "e8d10497f2bfa9e50f84648ab666669c37907a8fb444066c31346a2e561e283f83dc1883655bc7b29e09bc7c6a907dd2662751a1009685630ebaa6af9baf184a",
	            "6faf7b68b9256f58bfa535dd6cea7536a62accb91f9fa02f0fedbf937b4c109f4f1719d32b72fd00f0e7dda05f58eb402581ec4ef301aaf0836b823a2aaefec7",
	            "941e4e07d88ee6e67fabf340116d20cd9b394ded475a4f78b030c5c9a32d2d055a7b94f92cd30e99d6140deef4c0d65d45b35a4af20ee0d19cc610fd4128d11e",
	            "5a2ca65fc82a899d9b05ea6052a7107e4565c9d0385fe326227e19d8cb12c557d98ef9933e13fc615c32129a9f3e12ef269c19c5eee6bd942cc9b30991c1b357",
	            "de8727e21dafafd004dd5917b303a3b22cd32d57dfd97c681488b18daf8c9a51c1ad0541a725de795793464b41a9748a9ae7cf3e059db638962ba344e0a4b72e",
	            "816728cd234c1fa252981a51a0a08a0c5f0351acf462a0187b1e734517f60be816e9b775234f118ee1d194cd79cc7a0463150b7fe523c07305a20ab5b10378d2",
	            "9cbc588fbd6b2e2c7143c247f959a7b7e070b7f810b770ab94519bf18f3a7430d29ba647f47d7d715b91d86638f4c019627ca45d4f2d7579d7c08eb44dc05a2f",
	            "8b5f59fe443e95b4c8c21491448d44ff3a451433c5a25ff21bb3b9aab427901cca978288dd7a7ee455ecc5228854f8830808e1baed293674a08e422a5db4b4bb",
	            "4f7942ade9278b6a7548514d6bbe66505a75c62c5b26ded6374524b8371867f0cfb8b7fdef4b476413505ae45072712edece9db511dcde8f62b9c792baa2db4c",
	            "47988fb122829315816e0ff8d7b9cad6b73d738d4f2c564801f6ffa9d06190b4490d3854d7e49b396fe90931d98386a2fcf79f2616f1356a16ebf4c68c271dc5",
	            "dd909baff99233d5927af323b15a57ce0a2e9edc55ffc91e0a48621c081358e7a48e38b549671af49f571264feb55fb06c033dd564b31fa3a8e08b71f07e9f17",
	            "5b8f378c454b7bc05b84bff70d7948053e0d03bfd9e6d14d38fa3d1bc71074436ab3bf20d0e28502bfc4d5e213706cf527049763a943bc0d7e495a784811e2f3",
	            "1551116a356eda6db6563844fd192497d53cfb02bce10f9c4b0f7239fc2a7f14a5c8ede94a8b86d5dc7f4e994d9e2d89a025ab5068ccd1f05d629f0b3507d86f",
	            "2fdc1c73b09c7a103d2957e867beec1b53d27e216c7f7000819982b216a6b933f8921a159f208fc75b7acfa2e1b8d3fcc332e80341a3f9540b71005ab93bf88c",
	            "ee810fb3ed7d8cfcadd4580a1715591c2d6db836cf441cbbf25ff1933f4c4fb931a0e0ed61740568fa2c747156fe9e61d4112c6e70758afe3e7255118c0abdb4",
	            "cb527830b2d420f3df608bd00dc9b834b9e2cd927b2fd6a28dbd4379f5d35af13ada82f9cded5a17174159153d22b6dac0387ade029d86773f4026d96269a25a",
	            "122ba299f43c36147ac01bf0b0076627097727013dd7e7f7d9da6436926cb72b0326beb4cd005e98a992bfbd4f64ca27f0e7be6692acf6cec53dc16bd26cffba",
	            "9bc7e2518705723b905ca15aeab42c6a75733b865631b18670c9d4080e373975a1c3adafbaffcda80d6984313f5fe21d689c5f5fee76b89310236d575b5cfb2b",
	            "0c8f33005268f527bce9f23700112519f44abee554445672433e9c161d139a1c5bb82ed64f1dabe335b395102dd2c169e8c91f881ad0117e2968b168b059f0c7",
	            "2e90bbc24d0d0121b223793ba512f15d2b3c82925ce502be49af3ffb1619397476f63ddf2f6054d6179ffa17074719a3d5dab1bbdad98609563b98524da6f19a",
	            "c06b7a6ac732c746c1ea754c66cd17f5e7f86af5b8340bfaf8f613fb6352dfd534211b29fbccea737f82e2df01922656fcb02c06821b70ed1abcd56c6e8aee9f",
	            "bc0ac2b1be66ad421e72d8e883037ae42f433b8d83c66d8c5dfde9fce2302f697344f308d3060aca00042a9fa288f5abf33c146b2f97686eff4d84f73bdf290e",
	            "aebc7847b6edac9ece9980c4da03dd59083f276d771598ed65f5847d87eedf7621e6a95e71d15c21f95c0035dd05ccf8732464c9724c98190ee1ecb5d6981da6",
	            "3e17abcd1e9cd1add036aab092018c8d8771c27cc0f716cdf656e573ad2476fd6a0e011022446fd63d93cd161734ca89a59335feb65a0197abbde4482b900884",
	            "0c7892ecf2983f79967b7d439081e6501ddc1f18353e8bd71b3b9a85ec397d03410ec108db7c424c9e87b21004dde305da2829bce96939bbfb5820f6cbc6997b",
	            "f3dca2936d73b4e605739d59b598cd990ac8bc9b5f23b450d121969a433634eeae95e3655ca918a47fea82f38872b328cf23fccf441ec23a188aeb1559717b43",
	            "9ec7b6bf16f4e0f0cc38f91be8532a87f83bb8b6da9651851345f85b40555e6e7969af403fb0848a2e9265702a47f9b001f3f6834e42415604b73fc5f8552311",
	            "3ea1f7ae4e9409a5d59824fbb1808771d3183d9b112fd84a0c64de72fc736346e6740e66c170feaebf5702aff694dff02f575ca581241ec2fa9f409fe5f2b2c3",
	            "89939dc9f195f4247dab952d69d43d30dd982c0a3d4ce16a6e3926e6fca810b328d31980043ad7589e1cfe0ba827dfaa58c84826bdeef3f1b6fb8decde89fe00",
	            "d31f69cbd97c69dbb97c782a926f49333f1c3ec0b0c977c074367102887dfd733924fdc557756047610cf37e8e6f8f6e2db56ac3e421566c96d527a35c32bdfb",
	            "9c7a211b2d9efa709b7295aff37665eb75257b920522431917fa207c7fb0109b9503c49fe345f9c36a68b90ab2bcd6a40d26b49290c44af60fe72b3efb4afa8e",
	            "afcece9431670ca402b42c42db78c9f992e3128112fc4cc394f950d41bda5d751ecc7e39dc5c6b7535631f73590a547d69b418304602c310df1549123dc6e29e",
	            "5a6f4137a3cdd640e709978bab194f0ecc2dd0f4359b11c60ad02f3a3c0c37e546a47849fd13da45320b4e9142e1a641ecacba8a6077b73b273c755f6ee08cfe",
	            "df098818a55d9c43b0aa0d38900fc5e1586d135b045ce51819ba9ef35cc34d455fce68a75e5a193116d9c478ca22baba19b4623ca4cc5812905fb9fbee2bdfda",
	            "2f13af98de766a4f53c8fb7bd9d42b09b8fe99419868a72d6d65cff5dfe12688ce8858bd78937e53ea4768af09381d8e104c4053f99d258923f46db2610af6bb",
	            "c71d6e10d6d2b1047ac133ec57180332506456ec419e0542735e34d96eead9ca28e6994b0ff8cea908e68296a131d9ab95c4ac1c67c05b77e4d756cef29d9cef",
	            "7b50d9f44eb41a200deae1d00be67b4f711d1735c44c2c840afb0c90ba661ad9a7cefcfaa4c5fd0c951bbc6fe462a53600bb906d9ad089067742aa59e9c5ad0b",
	            "f3707937699425b7e5baf51d2d0ae2492bf0f4976e1433c136fedf2dacb9c66fd3beae0548868bd068a00b079e25430087a0c7e159ccd78c2abe8360681aee61",
	            "db4179db0ff8c0a9a4083c48625c5074e516aaff19a259e00ed6e0b042ef094adbe619514c1aab5f2bf5f0e5248a2450e491438b338854195dab9c10861896cd",
	            "9b920847bea6c8f618d5eb325ec0df83ebde16b3a4aca5d5dc2bb44e486669eb87bb6629c297c3d08284b3749ad6ccb755f00e0edff64abb25f1daa28b8f5305",
	            "bb3fd17e1a6ab86541ed7b5418cd8a21de89af92fdbd63c5196e0acb81968115e8300d8cae6dd43f61d637973c1fc6c4390e58847463e66e5cfa0f65fc373e41",
	            "8c8387809f03309c20974f2cc3e97151ccf77653f98760af04dff21fff9a472c73438253bef16888ec1d7e7c2dba841d96a265a54eefbdae48e861932aeb0e40",
	            "8c51bd91d04d05f1b89381c21967c4302bec21f9fe60d8eab74c0b885a08d6e88e829b446ac1d787c5865152f8e586423d5d8192b34ac533be23684db7bae7b0",
	            "e1ea22457c276a45d837f93a7ad6df4a6ce358dca04ae61fc90103a6b18aba8c553998464225706f8b8d755a25ff65dc6b79f770645d5c2ebbaa309a41bbc50f",
	            "994fd41fd14bc6a5ec89dc48e5732c6ecc39f74454f36a0c03aa171adf2e7a3eb01ae2959d9996d2ce416a86a1f6e3f1fda3a8234162f153d76bfc3a9f6a1e7c",
	            "fd03fa2750dd17603d137dc0c36870dee2ea0089301715fb1527839ce71b3f82bb09cf32c1ca9eaf5712cc4ed43c54aa92155a77811340b5042898212848b61b",
	            "c990afc80f67c757ebe25e25d91abc71b42ce5e20782195e3442a5ee46635ee35219af964a507bd701bae1893c91b0a0f1a28ad1c83d9f144004913b224bb59e",
	            "109a71c192ede3b7fcd9c65c1c8c98d5f8528bf30f383bfc679d869666ea756d98f591571c63c7dd431333e1e3a2df5850c998c5da1758f436da722ecfbb56ef",
	            "c6aa8714fe9e381753fbf0216f9192b62b5abe37e28d0293add06817f3c33090cf3ede043c8b06e5c02625d49835a64d43f23e8a7d46ebbf3d65fd87a2ad93b0",
	            "ebe6700e953432e82cd984f399c8614a5eaaec14908c5fc0b42473ebd71172bfa4171ee0e893671c8179832478daf5cc294487de535d9e8f8e48685d572cffd1",
	            "c16341cd61975170671937e1e1a94450866d785a40d225f67f64eefbb60a302e5980a9313aebc2b270634d628c3bb5a23811b42937cae41aa4bdbaa8a512e477",
	            "75a43905b285bc106b075affd4943ec2d5ce77209fdb94541973bd38a37b6ae536eaa8de3bad45beb72742cdfead7052f7a7f1ae92031eb16da8e6da79529f9b",
	            "f36a40ebee79d18e18b8aeef850f0f022165c25011d5c421729105c028934a7a66d944296f8d066f9a90eb4a6044e54712223595edfcec7f128fab2d920b0892",
	            "eb89314f6c3498da8d2c1d04bb8092cd691787d23a5fda2785830da40e4a599268109df8192f928b453ea65ab4b9e013e3972a18e63a9d48b82b6de4e2e92e04",
	            "71ffa7954ec0596019f33448ae5a1841e94bead7735ef8dd59e6f034cccbe3c6afc909ed9741fdb7bf6b34aea359d1cb29e5cc9a3467482125a20c65d39b1620",
	            "1b3e524a53c3628afa5ba9e8f5bb0602afc66c274f97944ac9b047ab4bbac05cb6b8622e3f4c356d916227dcaa062c6b2d7356d1b3a30a3751d024fe4c74e7aa",
	            "54c32c7a1fa25537f75985dd980f1de2508dee428992f28fe969f492ad95f227790ad0c5521151c4895a66b2638a8d61c143a990e0af458c4be2fef322d5d7d0",
	            "51232f76d5e4b56ba5d46c16f835bb65cc3827903289d52b784c163d597c0befaa398bd1ab6245b779a3d34057d1072b18388bffe36cb13ddff577b6f246a63b",
	            "9986709942110e94c1bf5857331f8a709095b8ab50492a79765cb29e1cfec7e13ca033af6b9eb8636caff7f9fb0787df63e78113802507986dd0c2e67b8a6a57",
	            "bd139d18c05f113a151b5406bfdfd5ad75be68cbd05baaabee495bbe60e50141c9bb5e89d77e8c28a0cbae29889b4ae2111fed3447fd7761be89eb62f35d540b",
	            "4659bd5e6cdeed74e368b7277d512f5787461c4d0c12d40b5c5a0d6e68f5e931eb1884d97ea56cac5bc581eebdbe967a0eed00df27a18bed73e9ba446d9f4796",
	            "8a1f483f43e83ba61ef26d267ab43b85a62c60baf8f6585ba9bc5e40809033839aa8731f8888aea3385027848ee5be601da96ee356fa8d511512b2d3d2b875b0",
	            "4f2c6da791cc87daeed228fae0c9ec64a4afba089e824e78fa491d6b3812a5b9f6feb4b3cb85bfa8ff8497f440dd04323fbe3ee12ae253654fb4d35c6eec5b97",
	            "e7172516bbfaba2a616a05ff476a4aee6db3e38d1884c3d15e478294b64146acb9935666b1e3428154a0884b4b1fb64d8d6e17997ddfa9e84d4c599253399e45",
	            "4cf90873d8774c8c38d2222ac298c2bed343152ebf863c2f8adca2a6b459e3280377e473a7458120f78e54804dbca0d430d75efad191aeb5242d03f397e86cd4",
	            "d3c6d3372d55d777ae8015b2251ba7d834a8d3bd7bd03ce5c2112491f7704631bcc9ac085ec7566aa95f76a18dc46b05563f66db751398e0d5471acc3c7dbdb4",
	            "c677f4c817b083fa0b2ad423dc87d4aba2e8c9b740c252f45e8d4472309a8602a3a8531d4fb402a18cbb8ae03ac8a36abc404e9496f992eeda8a9279d3fefd07",
	            "a5d0f7a525363853561d7588bc99bcc17d61c098c6396c879a6e5f36ff47c0138e96cd31ea6d1e18eb17d9023516b7fe9b1cc887b5431de41049e6f1615f4c73",
	            "2491d1316a0fa4a12c1902a6934d4fce70f82c7d05508a21362a7d12b6cb698479da37184fa73eaa1e6397ef5ed88a2ccbbe5a342e5484fd30577e14d766bc9c",
	            "7eacfc489a50242a317e13c161a21a2eef5f42fefbcbc122ce67c48c1057683f2a9915d9abe21e658d05d08f97bf180feb08b3d3527bd76897ece2f4caf5a697",
	            "3a6b9552f952a7cf5a34c6e155af3bbc2b46cdf51383cb0dbab22e1d8c4ecdecd00954eb2770ad551e7508ea8228e8ef70fabce7ed258a1b55c1ccd896b9d453",
	            "7f45a8d3451ac0ed423167ea49b546cf50204fa838e7746f454a01c145a1d335e1462d27a30232b5f8b9ba6d0b9bea043b9ac8c56a3b490272b121b6604fa107",
	            "68ac662a5a013a250d7e2f96618e11606cf675866ef683509039bcaf0b6d2d067d603e1239005368c4f34ab93131cfb9dc7199fcc3e72e93e933e57afbc41aa4",
	            "257ef7d74271ffdcd61db9de60a21b295484b7a01aa35e98a255ffcb046052a5bd70e2fc12fbcb93a5a0eb1a840f981012a7fe726f1dfafd87c7f4031c6e9964",
	            "6df89b0e95e03d6e6f17a1391e5c8f14336abae142c855101e1733e52007f837b5ca60971c3d04ce1be3ea947f40eb67622c7b61f0d7e96a4360fe3843b5198d",
	            "4dceb4214d2660f015929b36a6e1047b2f3b99534e6114bf0df15125d95bbfc0a17b715b14b5cdfa68cc9f07eb7af30d9eb89293b56a5660ff7eb87fbf90c987",
	            "d64fbb9ecb3053d39a148f22f085fe085eb713c4ed1c2ea6bda353c23a45a390aee9ce5c1226693baf2b88a58981b0ec69a681f39cb0bb6d83f05957364e4a5b",
	            "45d76897a558f2a451b1e2c98ed7815cb83bc0c694301a0ddaf2d945eb85cd2b6931bb399d12bdbeb4e2fdcd8fec6f88c20f51434f4dafe2da416e0e806830b0",
	            "562c949829c04fbe68574111f2b4316dc9691883f6ecdf2de25c467f85c4920da6b8dde9e3861ab39bf9855b644134f9d7c4d1d4d627f6bc43b95cc0aa622ffd",
	            "4276ab922866c3c7f84e1a2807f23bab757c4f78dd8594e023ba93a74f66b79bee54d0e836a7cd666dbf0265b0431a0901ed290770e2f29121a6db0053d80732",
	            "eafe662c7f81f4a6c019fff7a7ff565eab15a9941b89c550e87289406699e28a9240190602c49fb943dd01bff5606b6341bc60c9dcfbe0d3db3d676840613ce8",
	            "f16e7510bfb5f91d24df0e680b8c4e0395474c1133ad08928580dc70a99acbc569544a2b1565acbd2ecc9245bc692f5600bcd5446868a233ea4a9a919f8121dd",
	            "0df6f1017aee20affda45be25634028d9f13e75ed64af5f68f635919bdd34a8a7516fa7f44323d4391edc959176807f770cd77a9f73615dd08c65de729256401",
	            "491a90370bd082f309b49bff4586d6adeb7032c48f23117357e0be0be69a3c9918e3b1e29edd9b7bb43c047c3f73a065254aa0564b14da20d5f934da08c3e66d",
	            "e1be33ef90df6e7d36f33480990f5b0182246626cd300a77eb2786ed5888cb93eef4bcc1571517a03296b3ef8c41e02f98005d04155adb51d95e3241caabffbd",
	            "1a1f5600980bbc9bdfb45c45697a87e8ff41f4cbfa5534d743d566713c3cc264e636064693a3abe35ad7c98326774c3dc577f50517f66f5630f02ea0b87fb720",
	            "d3d413650c89866ce4717136d37b57f68ad7bf89ef8200605af26fc670213fd1fc2af1d5b6ac3c5389cdd16b461b1c12d311b6a64480721035be8ad50b10acec",
	            "449a31dca8801191a30282d1c044a32865ee511416a49f7b17dbdd101e8e0c0eb15748c9a01cc8cd4020aa17e8bd55f2958d905ee8b512a18fee248ee6790a55",
	            "a38075dbfb12b474ee2788074ca1769ee021bc97ef762cb2c9e263411731737d4f4bc89b27222563d76c8935775e8d1e1442a4f86c1678b06817fd0b93a4db02",
	            "db36a8ed0249f4071175091fc434c94f7327e2582844aea52fee88a48e057b32e2e1727f312f794567580ab85a79eb2fa5423d7939ca7aec77cc688a046d7a49",
	            "45bc46939a33daaee6b1e867172e699505bac99fb029f0405c74fb0905ccfe400fa023a1c38f288a379a76ba47d94a41968a507d57b055b4921533a3aceb1720",
	            "5187e33c831c3bdd683ad9573bf33c6a59e0be64e239091d46670a928ebf7704add048768db36fc7985c65669bc15d036d1f3ba06edcfc9ddf619d5dfd728473",
	            "7303b3f1e66f7bb8ee21b9087174eca1c932d1d951bfd2b8df1d28cd8553f7dc044513d598dec918ace3a97d89e3edc1c967181aa1a3e68bb40b83c154ed52f6",
	            "18d361971d3f350f65645a18e082d3fc7b1c3ab239263ccd222d123d2910369371589830a04ae1a446ac4829dc3c1cbc5ea2ad3d3f72d702bf391463adf1bf11",
	            "30a68e59733275fb286330c59483ddbf4b9e57b325968ec4f112fb1adf08ff407a7f65dc109af26ab74b9da2ff84335ee9dca41d6de30158fb703de1571184d7",
	            "14c97f85f260fded645b44d1a2bd1749c92d4732c64986f6887cbf7a5b4ba4112bf2e54aed4902010830f9603f31045223fb15353d7dd4698ef1a9ed47023be4",
	            "74d7d09289ea2c286e3bb171c57ce8d23da38814c1824550f6057c2859dff8f6a79d2fa695045248f8cd45747724c82dfe569e015832d99ecb8bb16b7cafda9f",
	            "d2729d12aec9b22d139b1ed0e7059c0a4065622d3a71ebc605fcec769c022f0c888c86fc34f6acaf50fa1e1ad4d3dd55dbc308a2f92b0cd004011edaa63ba3c8",
	            "d29aac419d056bc48399c34c1a5cc250659620e307a58cea354ced96375286b61f1d1b54cb69012e2a4472bcf0cc97b27feb321d335a250947e351d3d9837cab",
	            "8d6be94516e07d06a2ceebcc36eb619a55a54057477d1368af0bea6ef54324196c659952f1bd61417625003a42c42a8e8dd3101deb46121243e6c886b73b556c",
	            "4ae9d8bd788e850939e2d3bb4f76706370e0636cf05dbcef5f40382396e42947f6859590eff737e87e06826498472f9e198e3dc60ed1ca7280a353c303c5bc06",
	            "59132751634570d1334fdc68b1b2c11bd35fee2b4fa2d73666bba26a4d91c2fbb1c23d4436bfe372f077df5c993c880aad12d2faf5bb97853aa3f3be714763b4",
	            "63d8880e179c7724bfad5c85a29e05658baffc19d2f7a3a79c25c26e1a4126d03687d97b0fa610956acead9057052ed133346941a14510fc37849167c241b18b",
	            "00d7d6966a8ec87ab57a741f8e633cac8e2eb79d1cd070b5aadfdf08b5bf33891d2e9022d668c8fce556df1b5a34451e853c15a2388cf52c990a7026316c4c91",
	            "f5c6a0916db541ae2fe853f72a859e68e3939825dd920d98653c4b87b0a680ce1dab28a80abc31391984075ee1856ac0f31e8d735ce60b4552f61daa2cf4414d",
	            "14173d79590fe165b4c111b3724bcf992d1836a6931c800a0bbb48a7948467f1dacdca3bc4ca5b8309ad9069a0d2c62dcf0dd465e5cc6756773263696ac397a8",
	            "622b81bdfb1c2293ba946573baf56c43346b460d280cc751deea1a6329b7864503c1ae77e3fcb51d557872e5473607d470ad92d20956ec76ee66e5beefd9e2fe",
	            "62be23e8e6fcbe47e56587260a878c1f81627a13e5440d872548b982e43df2f5ada4cad7e70d5b19141a4db3f4398c59ef8c070adecdd296fef86e2f54da757e",
	            "1bb8d04ee9794abc82f6022a8f9e47c164c43d087152d67f88be98a7f7e0da12bce24e514fa13544f054e684cfac2eb793c2571d5cb7c446cecfe8bac9d27c65",
	            "74da70a534f835db20e7549386d30160c466219d34ccb9998b8b7aa7f7458beacbabffe11ff309ac803d2dc491f3f1b202b61978f65e8592f57f1c382b12a09a",
	            "4e808e725a13b437b20407e8908c2c67136d7cbe0697555059ecc3ddd5e95d97b219501f59c97da8971e4d49f70064b12529ce69a991a24d6ac7fe1e7dd30e4a",
	            "2bc681d4599bbd66d6c417474e38c800c2b08d62c4704d4b4c4c5d14d47a426ba5ab7985cfbbb270b10ef8e111fde2648a81ea74c6e34dcdd0ca5c288e76a6e6",
	            "1f950109542d031269a9a595310fb699c34b539b04b578dc7383a575ebac9c9fc9812e92cf64b1065199c2dc5bc7da9cb503d980bc14df216ff161cffd121123",
	            "58756b3e251ebaf3d3952378096f84c6266ea026c441095f9c1ffd8a5655b142923336613ea6205d1ca454591692e2215b4a6cf4d120431aca7f7de3042546de",
	            "79a2b18b8ead5bc15a64a8631e2ec760543a8725443da5e953df23a895ca359a72b441b6e3eca2d07636f7594fe8a607d3773e8b6827091c58761263f4848bab",
	            "0ce1253917e58b375366c41e84d0c2b282ff7bd5423fe7e0a70aa5a9e7b9d5778ae4c2fc33865b40a99781b561f0aa3015ff91c50541a62237706c1335a7fcb4",
	            "1a8d389c9c90acbfceceb6ca2f1c6abf8529744512c580267f10cf771a9f909d2ee7861c0999b613da63896c29c68eefb87984fee26a92ad1d41f2c2d546a4e8",
	            "301bf8c326e2b878c1156a94932b06e59320ae34c2adee3152b3be37c4d02e3aa47de048c47b3a6127db93d698ae5f73a7fa49cdd5f7b98612c639bda3b27297",
	            "05f4d991c066f3cfb790fa643aaf5cfd1393a6014281ae09b823e1a08380321a089195c925ff1c4e90cdebb75c93d3bca98a6b9dabd0f194bbe124798c070dfd",
	            "451992e069ede55e5caf27715a2c872c0509337cbe65fc4d9d73fa4ebd18dd79d720dd8d5ad84d8b74bbbf7aca713977ee6e86690f55cdd2412a4ddfd6fe48be",
	            "cec9a9b12d492cb612962a70bef6f771a94be9a08862454f03ab1d457da17ded265541aad6a72339b52648167bd510bf28d8eb520e051f194465a46ad6502354",
	            "73109fea56608e130853f32c1c320cb0b2829da5995ed4ec447d9d0639670f7273509d8d977a3f2c2ebc212dc670dddc5f89959d0ca463e7521c613873c08e3c",
	            "0ae9dc788436d29cecc3ea3198c161302e79217cb440ffed8f0b54e606f0eaa9d9fa84e3078308452768253939d1b1b6cee5fa0e0722dbaf0ffb17fec1f97bb4",
	            "eb46ba9cf79c9acca66aa49e6daa0736b90836a6c8b8d885b5d0bec735739296ec96a9409c8004c8e175fdbe70b176cf8e3075e4cae3e088a5e6541789e64e57",
	            "20949880098deb009d07aece4a87ecf88d87ffadf194af507f6442c979bdca771b93da2067051f73322601a7552f19e3734a9866430cdaa087e726d84cd6d58e",
	            "d5f631f449a93be4985c4807a91573b543dbbf5bc458134d0d1ae7da77721e8cae78ec02def478cea013ca90558a0f99a49c33559646702383d46bb6b5f2c5f7",
	            "3fc0502afa1c2dd4f6e19a69265f662f176142d4c8b804a318071e6dfb2836c9be631b875f342254d377e09e92f63f06c6a53d33a490b10b0f7025df97c7ef82",
	            "138f71318d182d519e6307c0a5711aac75e0cb8fea066ac621cb3e1002a61d9cebe3896b8650734d2848ac3075041dcbd8fa11faf8afe499cb9720cd586669d1",
	            "120fe051ec3630d0e64da4a5b29980d47f3aafcc18f819b7911b9b80552a85588a3b03668a9b9f777e26d060fee83f21d9ba6f4f33dbaae4bac0b34a896f9e99",
	            "2c780f8584e4202ed8a5ed881a1a11cb2f6d1d7068db08007ceca0376cf5a097728415d2ba216e4df419b8cca67b8b56acfffaaef1f987eafadec3882ea869d7",
	            "0c85e1500a3ee7c1d17c45edc60d20c72dd3b4e39e95182fda6743d0eb67e437194ada12895599d9294430744c7eb56fb5730b68191b00ba81419e75c9fd227f",
	            "1a0ce82c25ddfffaf6f7148f05d8ec65d9f19fadd01655b73ec7661f31c6f43075f87ff3e7c6cd217fa7d3c729645f95b11ed2cd945819b65a2a75df5374435f",
	            "e89b98613b49515adadc2e6f4e75cfd9e05ed6c0d7559c56d11c25d267d284df615c2949ac4a45a6472eb72bb9bad2a71d195adc5a29335bad103829b4f4454f",
	            "c1c6b4bb2e93aa0c20f67482b090688ff986343dda2639ed4618ea2b4dc7b0dab4f9133706b8feb39515362019a60e31b4f9fe4bb0ca7bb295319e240d6ee223",
	            "95264905515044c2161952b553ca5aa3fbeda6357d673fa4f7036cf63a74178a6bca6034de360c1d9a9ed00ae4ab5941ef9f27654e53651ecf7a3d79a0e4fe28",
	            "8f89f42047ebb2675f1b0ccec235abd8145618d09056fe37f31476c75c9478d2e3eb565177a32ac367d665f1d8c53ec23fc52dd592a227c2746c966179bcb15a",
	            "0af6f6673fcb2c6ee63a407813b87e66cc5a57af5c849d4fe07519514fcab2bc06d67649c31df02f02846ec063eed2dbf24587d9ca1c640596f58556ef278bed",
	            "08a42b31db93c28bd95962b976b8fc456b9787220c5e68972e549d26a5a53c81aa86e2504361136554b00d3f4a8e50560549073ad3505603ce32136473da663e",
	            "fe88724697c535e8438378818aad784628f4de466364aef0fedd18a0e264ddc68586ac0830ff3e3114abe71c19318129678d1bbd4f6338f2f52318c531ebc907",
	            "865b851a973da6267e94f859d7613a1fdf403d27e48ec9ebb6b7148caf04b9a5b51fa0afe26e1f15511e73c9b6749d9d55d724feb7485da1b0e9159d1a5a2317",
	            "215c89078472aae5b00c50febb2280a86c865834e52f38c77ecba81adf1ade7896b15dfc48165262c0689d3a9d954958e0117b78c4820348a53e3eddbcde4faa",
	            "e6c4151b04324760bb5e0ffe579b498afb0a45aa6ae7810ee3e260c880923aa5403aa3a6abb23c5c7ab00d74045f27a14503e5a1249b5532cd7871b67dc2897a",
	            "0b44f899590bf8aeb5d78e750c49d215a15e4f67f857d0db0516b7e483ce420a11b01344a90022463470a850cdcdf1b134718fc1a84d44a81c45996e352243a8",
	            "2d8c072f00d8287f1039ea58f0ed99efc26a589e10932a5f37053500b98c77dc422b11ebf73a806fc547fafa56447a5954f422fb1c3aeba9b500bcb66522337c",
	            "16798f87285a2a4f98cd3ca7904915dfb95b90a925c2d9edf508c11663a949a02aacba3021942fa44f0a5c14dad1d3d349498f6ea78abdbb790a6bb4d56ececc",
	            "5857c2957cd34e335a387132c26a56b3c07d0cf56b28598d63f25de881c97f7cdde4e0b312901cd94a9918ef7495aecf5e7505e0c3db6419b7490a0770478c51",
	            "04580332a8ee05136d425cbff0cee5dd877afe06ae58e18240bf45aba6bbddb280ae373b490487e2dce2fe6dc3aadd3139e01c9811f3088652eaae08aa57bcc0",
	            "db31bc9f12ff1d4384a3f797ab7263713174648c7a9ef373d6aecf8a242cb257b06a8eb5a412b46a239988e5667ab7f635b495065836e015407beac7ebb8fd5d",
	            "ad508a971146bffa11a14b3cc532647df21b958e25c1b8ce5c4a3ba0eff9b8cd27d59328574fe21affbf720f6eeee9f993ff607803abfe35a35b4ae5d830b805",
	            "075968327c0d3615df101b15c3739f88b65daa3fc8de65edb6ec323cc6acdb7b7f0a2e2633ad6fb205caec46beba7b5cf1cca0c0cc19ca47ba4048410ed8da86",
	            "3ca5c761e29e4ec25ecdf958fda0868dfab228e4ee169a874d8e71d692cb50266d5347ceaacb98ba7cac84f573654b21c26cbd47b45d52bc69a1ec25f9750843",
	            "cbf203294ced5a4c41816fa9fa7252197524edd2972dc3be0a1660cd67711e30345d07b0b9560a94401e9b95e9937f86102a79593683d67901a25f39aa425e59",
	            "696358ce0fc4ba722e6aeac8f3ec8d54cd61f2e7f5fe1485493cf9efd4ad7e302f5eb323b34ade62611bb7d6a1ee47fd4d2a308a37dc80c010849c40c81624cc",
	            "95adfd03e51d40efec2744a90051ff8fed936c21f799b54603162d2de96a5ff2116384e9db18f98966cf0f518329a79112add2bebd97da93e3dd9b25e05bc16a",
	            "c0cf0dde85ca3ca2fe63c66afc6157e9038db9233e72430b900267b9e309dbcb64e6e6995621b0b1d95963183a0933895230df62501588326d783dced2a0aabb",
	            "900e332a1b13ea34a4d8df1ca45487e46b420017ccb608adfa33dabc6e7fef26781deb3aee9c56544b1eebb02e4f92a3810a47f1614d4405c8fc2acd97aa9566",
	            "110737c31e68ec871d12ef97a592f992faa5f5207294cd57f88b9a8934a3fe1475d6724401625eb597d0aa1de65fc633ee7ba05f0cbd6f6944cf606eeb58c59c",
	            "95832c74e03e4a37f278a418a8f86b18c46840d338eb2879b558338ac3ba4890c5b5c606fdf79063bb26b30f7fc280f350fdcc01d9c7b8ecea7dc8ee6a940e44",
	            "3153a831b8e7588ad26c903f9d3ddd6b5f91ec396a73e71ec608c2c0a1f8a2ca6cdb019a91fb11b1057e074516c4b208d22ed35c607555ebb73a54c75b1f1d06",
	            "7eebb1aa5faf228b4215d1bc4d833e204d1ddd890b6be95eb0ddce5e94720ddcd7c89f5639ba04df96bc92ad47d9a69aa952c812fb784eae9945483a6a90af94",
	            "6e454aa33a61e01527123396013c26b5bce27534ee2d4f5389e834a57e3d8cfa6fbb23c11730dba1a238c8945e216dba1061c094685738cdf5c79dce09743dbc",
	            "24c5d02db5e888feca2f2c4d45470828e846a7f0d23486d9747c7cb5fbb03ff3aad38efd51519d473d0ec6510d888da82750cde1d943060b7191e5db40bc1b5f",
	            "a5b63292df363076a1e6ef6f9bf39ce6b0807fb8a90d43230ea27ea1fdb82686cba4a630a4713d0a7c1d4b204f5221fa5eaa424fa4d48692fec529c5414ebfd7",
	            "da6f385c25e47e24c7b13e64898cfefb2a77a448f5e2add7becce9f1e17fa7f5110a92f64954ecec923d9e73375cacdff7c41073269b36eabbd78805ab3375f1",
	            "e32e2b367d3e5b1e62b281f527c5504b607b2bee247ad5b40eeffc9712f0e6b489302c96fe9120a98053806780baafc31e1a11006c3483c644b6cde7640d7d06",
	            "e68a90199209b0611097fce65e7e92513c606d091be20e8cc8371dda59b8ead3952abdc9d09bff5395df703d2f8af18d689fbf8417a4e74182289366567f646d",
	            "fad6f95d18d79be8fa72ea23c08e42f9730f688b16960c370a00cdadd20dc69f89e473a32990499eeecca4ac16c68ed72ca806d0d35d18e38601f0f3d8f54227",
	            "22ed682862f93771c28c711d655139c8ae2f101f4f56e6d3a53c20d96a54f01248067c90453d3b37c861138d47abc82c2a5d7dcafb5a4b96b83d3e8a71404315",
	            "d2952d7c26e25d2b0b87a629ccce085575e0804fa89f1f5b0b86cd0f15c90dbc93406de1f706b50e5e8f4ba8906686d22e8bfec3d4758cffa5415c2fd2eb0896",
	            "c87681c9de903182ed2acd61a47a205673b410721be8ed5161f248316e54b88c54a7c98f04035e907abdcc1b05f62f5388997a943e7926ab1839be6485b93335",
	            "fcaeafe716b54dfcb21eea1129ea9d9de4f3d192d245a6d3022a2ea2fe74d665b8dc7749e0f16a5b7136025ac2e629c1711e8a452f287b370dbc146d51787752",
	            "8ce47a92be0fa4c1d02ffc9ad37a801bd8217d834f92b438b807b5945a160e40aeb2d6850bb7bd7270ee4906cd9c83f48cead8942b3d9eb960bc384f74b84a47",
	            "33a6c2a6b77f4006ea4cc421bea023a4a8e7066bb37b8ed7735c76228ed0ef94c469842970cca4691dc6dfe705b389b1bbf9483f314939a6ce98f8991245f7ff",
	            "a912b7f56b1e50e20c8e27f51bcf8247ebb00fa926fb000a31525329ed3782a3e3c39903dee4fe60ae52f79b7d3f7cbe2c1a65df2dcbdaf63a79029773cce617",
	            "239e6a108ec160bb9b937654375e435453a9b70e17ecd0d175e4ff9b7edbbf94422b5cf65344c0382bffe72377f8b83cbe5ccb16d8dcd74d3dfffe142b4c62d5",
	            "4d1a868e04ccf561ad6e7d8f603c73012b0706d60c9964ffe8cc2905d4eaf3e4273be861d2949de775105e9ead2cc4746ffb1381c681a384c5ee688e509b86a8",
	            "03d39c1bb98f017a36e3c9897bf90437ebd235ce00b5c48b914d0439de0093048f6f80e3bd10125df135730890207ad3291e1b5a69884127efebc26441a0fb71",
	            "7125f68441a38587fe679e43053512a6b2dbab286d915567e423839afec6b24ed251104c894158fe24a8e02ff663b1470d9e6d84431b639e034a4d84ba3c0b2b",
	            "9a4a8b20d5dfb3a8dae15406bf6602b6e6af726ec1abffe46981d21d3a4001b82be16c09075886571a33f2b81d682d1f6280be1c8265528e9ae9e5d24fe98182",
	            "ccafc172ae522c7e9a85dafcd49ecacce149386b170d95edc4bfb5f0247def14a7580aea60a1243c1f6ea78caa3cb31f87c6af31f7e12968af747ef4029c31b7",
	            "8c1004a049632b6889115092223724a662ba1e3a5c956bee69bb34b06eef18c84ac954e96085e66fb546a631558340786bdd5436fcd98cee396f6ec76a4a1ff9",
	            "4237cf42b759fce64ca87ec596890498b177b9e061fb5ca60adf86c5430f19bd5a47f423a7cc5c4d19621ec61882265306c6da9291917030b5a96819b8b3663a",
	            "44b566daa96e5df3cc08f8b93f2c4b6595ca410eab61e09fe3f2e05a41b9e2ca75e08d9731055e288fa71852e6f6a371ffc7dbf8af8445693ea6fc87636a701f",
	            "907f24858175652f38f087f28548120b93ba473a5ada900ae1f56701f5a85f849ca16cba266e8d3256fde52ca7781158f65b496ad94e5036ab4114c5c40a0cef",
	            "9b76a996f695a32f54bbc93e07c5b08704cdebaa34e0acb69af786338ce8e165de5d65ceaf3b49043b8652013537e49315ac4b7ce735e05169cd04f4efd337a4",
	            "64c40533154decff3f17fba6f3135ce24b51f371cbcc76d99b075333a4b868b89c3b904a1872bf84d529c1dd19d21af6861aa0782588333e3ff5d261cc41b883",
	            "b01bb404d0b218201fbb21b857e9654d6c509dcc33d67d1384e957fc6843d89086f968e1b9866bb8b33466702113e88756212204766b4935d0f60befebb2b958",
	            "6e3ebb62ed543a01dd660ed840ebc2743a09a4a4d570f63ec1403012e511e7bfb3da9d4df0a50f6e35beb71c99317fc10b825a5acfe2440f87457832574e5361",
	            "1f8fc3fceb1aae0dbe23b0e02f302161c60dec2ffec7bf093c169bf271adef43e69cca24f7f3b4c1a65b75ca9ebb37714ff4ea0fc917b18bbbfcb749b9f6cfa0",
	            "36f18d49f7428b0082010b989089e6c71547b93b5e144b632b1254683d494942e69a794fb384dcff8694ab0fc7377877983a5270ee90b381c2345af13d4791a7",
	            "04cb2455580e3a7358ee2e6ccc84f04f1eb6aa32dc5cee038cbdc5d8e2be0eecebbb3c3cfc0ca339240c149fbda6a5d02a3cb4163403556ffc68a21addfdc10f",
	            "d4b221abb87215f606b2fa5b70b7b4540c1adb9e88ad96b02754842533ce842b75b84b08db365147593e05b3ede8a9ec5e9f74a922932b2fdf53a6a1b4655786",
	            "977057ef8bb7685e4376eafd0f13e0f31200dc9119902348e1a7ad853cc97cb734b55ee045edfc1b988955310294c2cb66c91994442ae9cfbfec1420554d6031",
	            "3e24d97023cb3417df4b9715d2669503dc193706bfbc433ac584410d2104a833575f7caf583ff9553dfa09a22a4be03d8f61f1a8b92d08638fa20c73330c7668",
	            "baca624a02fa1dbfd4d351d4eeea68ac12c55cb43fea42ff581e7561a2cfe6f28f253339bb6cf5bcae7d79fd8629d57e6b4c1a6adfdb373e00664c4308a1cffd",
	            "dbf2baaa72b44696e461da464f107c8c15e11f456351f02f34d2fb2dd60f59c36537aad5ab8eb4eb6fdcb966cd8078cac81b96a51bb34f2ced7458ac60f082e3",
	            "ecfe64e524b4092418cff0d34acffd02ceccbbbef9cff4cd1082d6334429067789d944a52484616851f7f69c35549acc129877be0d315759c0a56066151bb0f1",
	            "3fb3954838c317443f46b08e3c4e57ca756063a665aaad4e0eca1b1ec1207699074e9ebce6edfb66bc9895add3e4c9c6c917e84627c210fa67ffc6d7136976ba",
	            "3f34fd1c5669b76a49d463ed40087ef073a3225017b7a3d5143e52de970155971344a11db89031b2bd57cbd17e8e5708b5ab9321db5da4270be91b57e5c2b554",
	            "9a582a7903f8d8ed37281e6ae457f5e4c7412f5f3221f56ec00acc407a1c62ef91ea1274fb65a3b31f2fbcad66b600f8a93f15880e739f7a54fab0872307abf9",
	            "2bfcedfd2ef7758e0fba77a3f97d52e54e9d043ac41bbb9c72df61b8aef8a9e9a2734f61f8434ef7445f760b2a21a6b4466aa56fb5b0fb6924118f0838eaed4f",
	            "f27e7447960903e7fb6667a267665a7a8ea9190311ab39fe3cf626456a599d52a0ed1e8040f564702bd40aab66f4440cde49c5c8cd4fa20ce3cf7d8e57bfe7f1",
	            "3ffe8ee27223f1caecc67fc4186c06eac940d9ed3e85020c88297a68627bc89fc9cf914d31f15122c69fe66e35933ce8d3ed4147f7ddf3b11c0e720d73f38b8f",
	            "5a0a87be3a2d567406a258cf094674956815ee99e0de0e9d32a8eba0429fc655217430ca26a4bd523b0197f83e7c4944cd47eda09fdf0d65b690584714fba241",
	            "68391f74e40f6c79f05ef126c56c46880702f84bd64498e03fe46c99e19a9c8f7619913b2db72d238b9cb1543a8b6feec26f5708695f22d5219890285efa9187",
	            "f9a840c2cd6ec0dc41684f9f2668e031e8d4acd549453a2da7c18d16dc0695681d82f600b4b5e4ab127f562b99390a88d69ddc09f246a3c7e93e195a43fe7c29",
	            "5d951688e3dfeb7d261ffbd15241fe6c5fbac6dec8f533ac6dbb0d4d3e9a6b0994964ad5d6a38d28af4ff675d6a13893783e2e77f2feea0ba6242af289f3d7b0",
	            "e4af4931de2116a5f0dacb06c0af13c2fd661e972034687f692471f0223e011849adb25303db96806d7fff331b2a37f7d5067181b8be04118aebb090d104f895",
	            "6961331e6409e3da3f34761a8a7760c71a72fa3341ab1a5d1ca2e53b1571ea31e6b08144c4c1637a8f1bd717cfc19062c002069320c244664e5ef84afe48dcec",
	            "315db7b6c1f368d5e2ea359102fb5b68f6ebbc907b9d88fedc1fa676fdd1abb91aee74aa4e011a8560ef52e0218d5dc08e7a49e564ac3029053c6b327cbd6cd5",
	            "c48e7f46c1580a0a04542d13f91b8a967463bce913c934541ab4a3f12890b29495e05a0d1429724b81817a4c823719c81ae3d5a81e8c3ef782ac6787f8697c19",
	            "bc63bc99cc101c36674279bec5bf3301dd5d084be31cef876ec0eb25f7c3a61c28560ac6966db432b721011f9b8f2a5ee0c714bb79552685092941be155803e2",
	            "0bc5fb814f57cba72018a14791d7c26a0c5151d70a633db67dea36630771b8f85d454474329537bf02b384c89e383bc53199f24d543eae537f5ce7df6dac8269",
	            "6bc2fcd93180bf2e29e840fe8dbe7087599ba5ef413049442566a01dc3ba7675197c02a7b294d7689d4eed9f7c513477419dc07778383dcb0a40c7653d9830e4",
	            "2d59ef4d4e55a11af520137a4bdd3cbf268b1fb00b3e9518d90113175984d61da1fbd66e036c33bc7c9d0dc930ba8bc22425f95e0207bee3986f14262f34d363",
	            "6fb4a6fcd74a02b3c0707880456a14d80c25f7097cc531b920612507f6e2ed522b7ae0226eae514dcbaf71b29aa3da8acc3b8cfe2f283cc02ba85fde9caf11eb",
	            "e06445ebb682677734bf1fbb798db6b0a2d9795ad6dfc77499c0544251ffaf740c2fd29e8e0e6f7ec9e5b78241921d58e5a4be54add33833700af1a97f09e69b",
	            "c993ae16f110958fe35ca96849231fbe6f22e6b5e9c2793e810511f37c1bcb76bb0f0f40709150a262f9a6cf35a2cfda3b7da7a7fc37cceddc3f228437466bc2",
	            "d430a45ebbcf77d7ddeb6a76fca9a9160eb5bde7977781956050580b5545cb74bb037a2bb8f55011cae9c4a3599c5b502c3bae1cf4e2dc25e624ddab12f85d47",
	            "384a9c49a73703209dc8cd4f6f570232dd31ed2092179f54e615ecf54fc16aaf1bfd5ac6a1332955dca97d00c1485e2049c7b62fd308c9ba77b0d279a334300e",
	            "f4db34ecc23e0e7fdd6da0a9c70e8f4a50782db9f0db60ac481adae06e16e671de959860161b1b55ef46c903ebee59bb00788c1c7fbd623db337dd7824c038b6",
	            "004e126c578a11c3a8e632162a43a9b5f2a8b0874a24767b2fabf1281eecad6a9ebc0abea2297c6bc584839849d20093bdb1298ce65546fb893d63b20958e9d9",
	            "9e652b0f982974e490e3cd81c2310f055fa661e5f9a5cc522fa917db04a25bfe0671ff39015f2045a4fdebd110777e36f46c13bb979f78a54aeae845457b6383",
	            "264b966b2188417b043fcd084cdf1369e5ea4a85ddb78f03401c897069e31a2e01e85efac6f2bb587f5df2a0d4880a961a3d68a3ad150314e2c5380b4a274c51",
	            "e959a598a135df3a8d822c002bae2813d0a3cbed41caaf564726f336415eff29190b93a4c3ad355f9900f7a6ff9d13a873bbd9825a5a3259491df6cdc014b1e8",
	            "0c3c430726b0ac9e9f8f1d5d8f798d48d9f445f91d8c6817818037319910bf1e615e64b9bc5905a843d3b16255112210a54b30f3c802066765e79ce216b88b2d",
	            "31c95de0b9eb882392d3272e2a7c2cbe4fc104706cc2aad5819bcf49667e2f985d99ac7284cc71bb0d5640f88610d2d5f24752aaa0391588d6fcd7f4e921e594",
	            "397b05bbb28d45240662a6e4cac2ace506208155eaf638167b67fe918ababdd817c589157905802e11e142f3146e3f80e0351aa0f240540c59290e63b384ac15",
	            "08e9eed87fde721a6d7ba35ae05201c26aa3ba391fb6a66cd89369d61ab36fe81c300cf80943997e247257137690d890f27b4f96e1ba011aaa06f607e516bc62",
	            "e2b3b839718f4ac05192ff24dc0474672a7c455d40f48240288e68a0ad341b5edc181dbd238a1a8f915a27e0213d6518b26212da639f60d9c9ab346d3b0c5698",
	            "d29236289a3a247f6dd7995f5fb4771bd99ddb87773efdedb34d5ad74eb32c407b9a2b3dc886931e82b31afb750405182b6e1c0b287ad2be8d075a9c23fd1673",
	            "da52484c18cda7127b5dd9c011488df8ef3577590e7e903f2ea2c98083ff007e941b892a4b9249477ddc5c4d7a450135375fee3f91416b5986d73252f8ba54f7",
	            "4a64654643d7ef34aca7a3b492b9eeb5e86d84b146ab21a19d488407d0cfd4aa0d4b01a6be6c205fff7898617e541f570e9cbf7c4f38a2639082d26236256d16",
	            "110318ddfe4ab6ab3abf65dae193ac3dab60d714336d49607b65a4dafa43ea68f812b9476a4e5fed473f1421ae184ce2995283678f08f27060421cf4e458ee21",
	            "e5a1b9fbc690ac63a527f54d67e73eb09ddbd0d7df180aeac35125b02cd28984bf05cc39d8952cf6593550c3e89d41b4d1b833ae3acd4c27cbea330fce39079e",
	            "61e2a1e37df9106f91a6e3bde652552b5a5de33d7febad957f04bb628df4b02e8ea952f6a7fa2f23a025a2a0813baa152486c0a998ec5385904cd80e5732da27",
	            "4c8c233304b069fc514ce0adf5bbcafe41fd7df64839ac309e5b7c99740b06b47cd8016bcd8a63c83b4c28b870c92d3ef2db6c9e065d65c57f0a641f12c250a2",
	            "3f21bba8f5912d816890b1ddb129aa344fce5a036de9107c19a227bac8779206796bf793c448ecb038b1b442bc3c3f2e4062737c9c323a5c6135ab76e217d3c2",
	            "9e3a2ae1a0a1ff02f61b9b89e3b47ec85922ef21556ad491c1b2c630a43d8ae84510a24a422404758e5aaf65bd090dba4004d81312288c2aede4277bcce24f23",
	            "fbd97322a23a2b48ae30eed4468e5b18456121c33e01a69c83ffbd4586dd3c5b3a5945d8e328884eb7d425b32bceddf941438340e382abf548c2763be8dd63ff",
	            "3bec10131f4e359d1fba2560570076442035346d89c5289195baffa2350c8aa9fc256e36dd5c89dceba08026727a0ddd6f09e919a7e04c0aef6675037e812809",
	            "452bd8157b9a02f3ec97c0c05edc19034e955a0155e2fb5b899ec48e78d954dfb38c618f46332d1ea42825b8c904882c10de96f8cd6d441a8b173f7b6c72ba63",
	            "d69195716ad0f72cdc26aef2400bec5e31cd4f93b9ac2673844cd2612f7ee4b55379ce51e00757df446f87988bc6be97fa95901c50a015aa28c2729d2ce81016",
	            "d54ff7e072154214a9a121d05be83dd4b2098eb6affcfd246cd49c68a437d5c025b477785842913df5e40608b88830fc2787307af1ec37cab5da069639690deb",
	            "c145ce9646dd65e0bc0459e29a3acad37522db2c20527284f321683b2354a525c44cc0e52892e8f0f6d62d2f499445fad2fcbe447b9a707320f640b8404ebc4b",
	            "65bee40d82dfd2ebda5f971fbe88343fd4ff359e00d6f8d5604d1de50980607ca079dea479ad093da681d2b92e2809a1f999ddfe160af52d4c66d94de7109f50",
	            "d1b17502c00d032b476721178b9199b101ac4d379fd410e21e8eef7df3ee8caba445d610657de3f487a34b4f9a00e4a983e15092c45ce7e0c1eb76f521797508",
	            "00e63723eb03706cd136529feb0d58d39ee950248e838bef82882b50222cf7d4dde10353494a3adb014b41459daa2d9501b02648a309697e4ab489ccc38d1673",
	            "6f1504a8b20708c33a61e48213888846f563186d05aa2ec7d7798fa1636176ddefe6db68ca48e1e47baa6b1dd6e5003f45dd4a38bb71b997228cc7d83020d0ed",
	            "6b308fb1bd0f40d4e4f392b5e2c2ba167583379fc88e157bf65aa6885f7e8d684229ff512c734a58cc90efa45c9166e5753896d3ff072252dd901d7410534220",
	            "3635eded777ee2dc7328a8bd8ec78812edef402417caa1ce4639b170a1cfaa0b4458139fe04defe883fc6c720b2205501a066ed4392a868ac7420b9f17a66574",
	            "1063c3c27fa416c1d2009b575680ac9033adfa57ef46fa5731df5c70977c106d2e75867ccb9895b07f0ed653c84cc42ec28998d5a4e20dc608d3fc4259d50475",
	            "4704bbd053ca0b94668032d7cf153800c8306441b068f8b6c72c8ef3d06bb74e0f2e52ac344de50c7bf954ec426406468c1a43b58d20ce44d41689751de284f7",
	            "a67b971df4997c1dd54e0a142da0ad079f791ae4a6355d0a1ea34a340c8da6ece6ff0b18afccecb86b0bdbb6d6f50fe682392fabf629d234498a9f9d6dbe954f",
	            "42c4d284c44eb9cf58dfad044364baba0e004b1115a66a1ef5d71f148ac9e95fc7c1d39c26f1e592bae4b0158848e69889eb6dda43d999a61ec1ffb5d2d8eded",
	            "4c8076ef44c6b4239d4eaaf97297434879382da4802944362c23b8891e867daeb3827a6f68d39d069015e027a5c27ac96d53ea5fb1063b8867615ee742754fea",
	            "19e72a28b5ac58055150e960b3a05d56fac57a9dec3247ae0638315591f0ecbe39496f9b470be60a1298e0c676343c043ba790edb6f456533c27384af5c7e390",
	            "b9d3092368b051ac96a2e8ac2ac0e3bf29f6a925ebb725c6d7c89c94c765564ec03043469611d8631f8d0b221ac8bf2a5d9fed0d979f2866a1a0bd4eecb54b4d",
	            "47cca8038e63244136301471aa15f9686aa393c022430cf1cf27f98e4bb7db2baa51d1e9e1ec2d52440a7b8cb3763cb9fded0429f393c8e2c2a876bba64a4d6f",
	            "41f686ba725dd5c6b98156a3755e1e9396100983b031c2b37e52c2948ace3767db010a9ac6d19f20f009f197393837f439e80e63ef4467494ec99b124e7caf5a",
	            "0944d919c719fc6893003adc304890ac4d1a992f02ac8be1c1cf08ccbc222097aed6d76e8347d51211890f33a7b92c78a639ae8fa7222384baf509947cf6bb5a",
	            "89e8969f6eba851801a1ee030fb8b6dacedd241dd0aa11f050c39aa0fc5b55c202fb573977c0979e208d00b22f1d9eece8bf0cd9a2e877d91bc391aaac914aef",
	            "e0ef7c285aa8a46e4d9bdac7a07ba66bdada14cac49df4b2b82c100818b916b0b60b30a23f0452e15a1505e04178a9b9d3b276c3306f89b921266c0460a17bf8",
	            "80e3b54a5254283df5813fe59450e4ba4aeca755cf8eaf6c26f5b462bed44c0d33299455f66dc9fa962661cbc839f4c91feb667b6fcf7a4abe3212e1a9de1b9e",
	            "d67fba4ecda130ed0fc68dd664246f4347f5c997dd3d12c1ee2836623fc958be04ae83ccbfab94e98fa007ec5b606efa87833aef2390de0b384214bd584ac018",
	            "ad10d5718f757a6498ffa6fef64ec43b4e2ee9d49527b6ac4095ea78e86173516a6fc0cb81bc6fa011b56b1652e3600a153d04bcd0572eb112cac27121970b35",
	            "c4d161bf69c0b572a6eacf593329ec35bafa23e3ac2a8ce0ea63868f7c3e33852c6f1d6aa6abed6d9bd0c20c8473ef50beda1e4e336a8e6ecaa9333161942bbf",
	            "122d3e93aeb2c9912c5f5111c03f77820c3ffb8d401168de01f4d2a6dd259250e320d6b92983ae7a7657ee8ca402f5f851ca879ddaed66e3799f951f4617e1c9",
	            "f7424d187ceec0fbe52a86f3242942d57671496dab77e958ac48b365865bd7137e85aa3a0fc8012cad52810e7bbd7e055321364c12d1f0fe16235d14cac8aee0",
	            "6995057c58f838493c23ea51a793ecbc401f04dac136bba15da02cd4c81f4c144fd3e101d3ee8a4a5cd71eee0dc35d97b89ee73fb0abc370dfb111845376b41f",
	            "8964f6f9596dbede0fd0ee20e8a24f66cb2a1cf191f37262fdd20115f6f61a2ccdc72b2956f5f4a72215dbe5721032053ce77cbc41c6c9bf5428f6e51c5dbde6",
	            "9ec45bffed4c124fd5c2bc3188fcf1998a262336b0720b5c39ac10803cd70fb1fc44b42120aefea09045e97d620003df61de6e915db1da9380d0976a0713da34",
	            "9b7902bfee5480813abac9c09f6128e0aabc91d42f7348aae8af7df92114d19a3b0b3287d8941c53d09eeb7621efa891acb93b8a4bde189beef1328d1e32f415",
	            "040a3de9e30669cb19a09345c4bd39945acb506b4a4c6f4c364ef3d248c0e97b1ec824d762843b8d8ae190bc5db5bd31ff02700a2ca01b077a7dfdf7b99ca9b4",
	            "993b4768519e758e9a4b547273f89755563ef93ad39f7c4800f8f29659e71cb62cb82bc31c7d1fcdb8b46126b0bc8be771ed8a38e56446ba3927e9a2b1139b57",
	            "a1620f1255d1560acd86910a3db08046bdf7adcf718e58ccedcb6644f7bdcbcf7fd3276d35580135c72a7033f60511c8db383f05c76e780f84e8081f73b9eddc",
	            "1d752fec1dbbf40a1b1fcc6c644b99abd28051df20577568f76998109da7c291ea108c7e66908c3309953d74c91b95bde18b7a01b52f91ef8eb034765288a134",
	            "c3ee32fa57f3c51bbaed046b62775ea310b29797033c5736492abb69c1eeac5f94424aa02047e533d671236482fb65fe98255a48f0f55d64b0612305854ce323",
	            "d5f23ce060d1366dd587f1b5873e0309507fb077584106adb4d463960ba70721016ca14f19b1be8c71742ce1d0f9dabc6899041b09105cf760f2c9222fce7a1c",
	            "f185f0ca1e99c36cf2f496151c6e2e3b36feba2b18da1dc7c7b8b6f66776cfeb51b2e8cd04297016d2dd5ed3908507378ad23d41fc99eec55c421e6c2059d6d6",
	            "09b6cb78b6b04690e8a8978abc1e092c0b630452c6d9481bbba1f2fca0a78d0123b8424ff8be44fa44ccf9efe75d5f2b2bea07568df67517bfd701fa6ed53e12",
	            "b3de3ead6a671cd99f9947774073b62c0ecdcba11fb021ed9c65b3c01ccde63386ee038eb2a5250666a316ae92b898760a14589156e3f3b0f408c6f6a3321b01",
	            "8ce517501cd06fe1c12c7317261c4014526aee9d0ee12255eae89f915ea5ac61e3fe22c2efc18cf28f904c93dd0634a51c86c2d28261d519dc35dadbd877106a",
	            "707d2bb46e220d858bce657e106e4ce5c276b325b3c65382a3af261f46cbb0e878cc0a31efd7c823ee102315cc75997a6c27d3ccfbbc77341f6a94f46bbab95d",
	            "3eb39d5aa4df97195a7c96caf2cf3b2b3d109273ab15c1a56b29ad1a98b4c24a080108cd14b86e509ba3f57c045c6953010c159b018d0469097d38b4fa0915d3",
	            "0cc4fb2c20a269d3e2000976a296ecdf8e832cb8a0146fe6c8f03d29a2c238ff887dce02935cc50354747cbcb3301624f27e4803e431a30cba1ce3a40e9af096",
	            "8b3904dd19814126fd0274ab49c597f6d7753352a2dd91fd8f9f54054c54bf0f06db4ff708a3a28bc37a921eee11ed7b6a537932cc5e94ee1ea657607e36c9f7",
	            "4e9567e12315edfa74db8ae24a59d5c77a71f3d6a49b6a7b377207047a6a6f800362fa38e8036148d45dfd518b284b9b3dc7f8b9d6d223fd2965b766c58a2ea4",
	            "e23385c4e0ae4e8f290dcbffe7fb7f8186b60e633e423d16d2c19fe7f3e5e6188b133e4cdd3ae23faeced99971bb716b10adf156834d275c6c60165af932d892",
	            "7c39f7ee42e94b5104f3a6dd9a3d426058ed9f36b92bfecabc83dca83351b1f6844c8d1f9899e4011654b1ac16ce207df3b4aac8f0997e59758dbd2b6f89c261",
	            "64a28857b2574fc5cc2e25b4f7d1c7c7855eadf7613c949cf4fe9485eac5307e18bbda3d5bd4f6124de74ad9a7964ba81f1d2487cedd4ec5d0c3789d47e74005",
	            "8065428949a42e59a8e6f2b9e17f11178dfac6449d78f3917ed6effb9d20e7e4159679e935f7ec49e3b04336eedf07200bdc4992936c3b2fa2c1b935392c448f",
	            "c9bd9c778669d411ec9f723fa0ec96fff5c0f82be52746037bdb9405eb33a8115c00a6bd5f5260730bc2437027609bf45f402fa8c1a26c0fe2c9d4e9255e6c8d",
	            "f1748c6ec048cb59fd271fa933c1cad6d00d86d66fcca9188f1b4239d50e34bbdbb6dfccae9bf8e4291a0ad5e76d48770d36824b850cfcbb4012d97f2ce5650f",
    	    ];
    	    
    	    Whirlpool h = new Whirlpool();
    	    for (int i = 0; i < data.length; i++)
    	    {
    	    	for (int j = 0; j < 8; j++) {
    	    		data[i] = 0x80 >>> j;

        	        h.update(data);
        	        char[] d = h.hexDigest;

        	        assert(d == results[i*8+j],":( 1-bytes)("~d~")!=("~results[i*8+j]~")");
    	    	}
    	    	data[i] = 0;
    	     }         
    		
    	}

    	testISOVectors();
    	testNESSIE0bitVectors();
    	testNESSIE1bitVectors();
    }
}
