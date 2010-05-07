/*******************************************************************************
        copyright:      Copyright (c) 2008. Fawzi Mohamed
        license:        BSD style: $(LICENSE)
        version:        Initial release: July 2008
        author:         Fawzi Mohamed
*******************************************************************************/
module tango.math.random.engines.KissCmwc;
private import Integer = tango.text.convert.Integer;

/+ CMWC and KISS random number generators combined, for extra security wrt. plain CMWC and
+ Marisaglia, Journal of Modern Applied Statistical Methods (2003), vol.2,No.1,p 2-13
+ a simple safe and fast RNG that passes all statistical tests, has a large seed, and is reasonably fast
+ This is the engine, *never* use it directly, always use it though a RandomG class
+/
struct KissCmwc(uint cmwc_r=1024U,ulong cmwc_a=987769338UL){
    uint[cmwc_r] cmwc_q=void;
    uint cmwc_i=cmwc_r-1u;
    uint cmwc_c=123;
    private uint kiss_x = 123456789;
    private uint kiss_y = 362436000;
    private uint kiss_z = 521288629;
    private uint kiss_c = 7654321;
    uint nBytes = 0;
    uint restB  = 0;

    const int canCheckpoint=true;
    const int canSeed=true;
    
    void skip(uint n){
        for (int i=n;i!=n;--i){
            next;
        }
    }
    ubyte nextB(){
        if (nBytes>0) {
            ubyte res=cast(ubyte)(restB & 0xFF);
            restB >>= 8;
            --nBytes;
            return res;
        } else {
            restB=next;
            ubyte res=cast(ubyte)(restB & 0xFF);
            restB >>= 8;
            nBytes=3;
            return res;
        }
    }
    uint next(){
        const uint rMask=cmwc_r-1u;
        static assert((rMask&cmwc_r)==0,"cmwc_r is supposed to be a power of 2"); // put a more stringent test?
        const uint m=0xFFFF_FFFE;
        cmwc_i=(cmwc_i+1)&rMask;
        ulong t=cmwc_a*cmwc_q[cmwc_i]+cmwc_c;
        cmwc_c=cast(uint)(t>>32);
        uint x=cast(uint)t+cmwc_c;
        if (x<cmwc_c) {
            ++x; ++cmwc_c;
        }
        const ulong a = 698769069UL;
        ulong k_t;
        kiss_x = 69069*kiss_x+12345;
        kiss_y ^= (kiss_y<<13); kiss_y ^= (kiss_y>>17); kiss_y ^= (kiss_y<<5);
        k_t = a*kiss_z+kiss_c; kiss_c = cast(uint)(k_t>>32);
        kiss_z=cast(uint)k_t;
        return (cmwc_q[cmwc_i]=m-x)+kiss_x+kiss_y+kiss_z; // xor to avoid overflow?
    }
    
    ulong nextL(){
        return ((cast(ulong)next)<<32)+cast(ulong)next;
    }
    
    void seed(uint delegate() rSeed){
        cmwc_i=cmwc_r-1u; // randomize also this?
        for (int ii=0;ii<10;++ii){
            for (uint i=0;i<cmwc_r;++i){
                cmwc_q[i]=rSeed();
            }
            uint nV=rSeed();
            uint to=(uint.max/cmwc_a)*cmwc_a;
            for (int i=0;i<10;++i){
                if (nV<to) break;
                nV=rSeed();
            }
            cmwc_c=cast(uint)(nV%cmwc_a);
            nBytes = 0;
            restB=0;
            if (cmwc_c==0){
                for (uint i=0;i<cmwc_r;++i)
                    if (cmwc_q[i]!=0) return;
            } else if (cmwc_c==cmwc_a-1){
                for (uint i=0;i<cmwc_r;++i)
                    if (cmwc_q[i]!=0xFFFF_FFFF) return;
            } else return;
        }
        cmwc_c=1;
        kiss_x = rSeed();
        for (int i=0;i<100;++i){
            kiss_y=rSeed();
            if (kiss_y!=0) break;
        }
        if (kiss_y==0) kiss_y=362436000;
        kiss_z=rSeed();
        /* Don’t really need to seed c as well (is reset after a next),
           but doing it allows to completely restore a given internal state */
        kiss_c = rSeed() % 698769069; /* Should be less than 698769069 */
    }
    /// writes the current status in a string
    char[] toString(){
        char[] res=new char[11+16+(cmwc_r+9)*9];
        int i=0;
        res[i..i+11]="CMWC+KISS99";
        i+=11;
        Integer.format(res[i..i+16],cmwc_a,"x16");
        i+=16;
        res[i]='_';
        ++i;
        Integer.format(res[i..i+8],cmwc_r,"x8");
        i+=8;
        foreach (val;cmwc_q){
            res[i]='_';
            ++i;
            Integer.format(res[i..i+8],val,"x8");
            i+=8;
        }
        foreach (val;[cmwc_i,cmwc_c,nBytes,restB,kiss_x,kiss_y,kiss_z,kiss_c]){
            res[i]='_';
            ++i;
            Integer.format(res[i..i+8],val,"x8");
            i+=8;
        }
        assert(i==res.length,"unexpected size");
        return res;
    }
    /// reads the current status from a string (that should have been trimmed)
    /// returns the number of chars read
    size_t fromString(char[] s){
        char[16] tmpC;
        size_t i=0;
        assert(s[i..i+11]=="CMWC+KISS99","unexpected kind, expected CMWC+KISS99");
        i+=11;
        assert(s[i..i+16]==Integer.format(tmpC,cmwc_a,"x16"),"unexpected cmwc_a");
        i+=16;
        assert(s[i]=='_',"unexpected format, expected _");
        i++;
        assert(s[i..i+8]==Integer.format(tmpC,cmwc_r,"x8"),"unexpected cmwc_r");
        i+=8;
        foreach (ref val;cmwc_q){
            assert(s[i]=='_',"no separator _ found");
            ++i;
            uint ate;
            val=cast(uint)Integer.convert(s[i..i+8],16,&ate);
            assert(ate==8,"unexpected read size");
            i+=8;
        }
        foreach (val;[&cmwc_i,&cmwc_c,&nBytes,&restB,&kiss_x,&kiss_y,&kiss_z,&kiss_c]){
            assert(s[i]=='_',"no separator _ found");
            ++i;
            uint ate;
            *val=cast(uint)Integer.convert(s[i..i+8],16,&ate);
            assert(ate==8,"unexpected read size");
            i+=8;
        }
        return i;
    }
}

/// some variations of the CMWC part, the first has a period of ~10^39461
/// the first number (r) is basically the size of the seed and storage (and all bit patterns
/// of that size are guarenteed to crop up in the period), the period is (2^32-1)^r*a
alias KissCmwc!(4096U,18782UL)     KissCmwc_4096_1;
alias KissCmwc!(2048U,1030770UL)   KissCmwc_2048_1;
alias KissCmwc!(2048U,1047570UL)   KissCmwc_2048_2;
alias KissCmwc!(1024U,5555698UL)   KissCmwc_1024_1;
alias KissCmwc!(1024U,987769338UL) KissCmwc_1024_2;
alias KissCmwc!(512U,123462658UL)  KissCmwc_512_1;
alias KissCmwc!(512U,123484214UL)  KissCmwc_512_2;
alias KissCmwc!(256U,987662290UL)  KissCmwc_256_1;
alias KissCmwc!(256U,987665442UL)  KissCmwc_256_2;
alias KissCmwc!(128U,987688302UL)  KissCmwc_128_1;
alias KissCmwc!(128U,987689614UL)  KissCmwc_128_2;
alias KissCmwc!(64U ,987651206UL)  KissCmwc_64_1;
alias KissCmwc!(64U ,987657110UL)  KissCmwc_64_2;
alias KissCmwc!(32U ,987655670UL)  KissCmwc_32_1;
alias KissCmwc!(32U ,987655878UL)  KissCmwc_32_2;
alias KissCmwc!(16U ,987651178UL)  KissCmwc_16_1;
alias KissCmwc!(16U ,987651182UL)  KissCmwc_16_2;
alias KissCmwc!(8U  ,987651386UL)  KissCmwc_8_1;
alias KissCmwc!(8U  ,987651670UL)  KissCmwc_8_2;
alias KissCmwc!(4U  ,987654366UL)  KissCmwc_4_1;
alias KissCmwc!(4U  ,987654978UL)  KissCmwc_4_2;
alias KissCmwc_1024_2 KissCmwc_default;
