/*******************************************************************************
        copyright:      Copyright (c) 2008. Fawzi Mohamed
        license:        BSD style: $(LICENSE)
        version:        Initial release: July 2008
        author:         Fawzi Mohamed
*******************************************************************************/
module tango.math.random.engines.CMWC;
private import Integer=tango.text.convert.Integer;

/+ CMWC a random number generator,
+ Marisaglia, Journal of Modern Applied Statistical Methods (2003), vol.2,No.1,p 2-13
+ a simple and fast RNG that passes all statistical tests, has a large seed, and is very fast
+ By default ComplimentaryMultiplyWithCarry with r=1024, a=987769338, b=2^32-1, period a*b^r>10^9873
+ This is the engine, *never* use it directly, always use it though a RandomG class
+/
struct CMWC(uint cmwc_r=1024U,ulong cmwc_a=987769338UL){
    uint[cmwc_r] cmwc_q=void;
    uint cmwc_i=cmwc_r-1u;
    uint cmwc_c=123;
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
        return (cmwc_q[cmwc_i]=m-x);
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
    }
    /// writes the current status in a string
    char[] toString(){
        char[] res=new char[4+16+(cmwc_r+5)*9];
        int i=0;
        res[i..i+4]="CMWC";
        i+=4;
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
        foreach (val;[cmwc_i,cmwc_c,nBytes,restB]){
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
        assert(s[i..i+4]=="CMWC","unexpected kind, expected CMWC");
        i+=4;
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
        foreach (val;[&cmwc_i,&cmwc_c,&nBytes,&restB]){
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

/// some variations, the first has a period of ~10^39461, the first number (r) is basically the size of the seed
/// (and all bit patterns of that size are guarenteed to crop up in the period), the period is 2^(32*r)*a
alias CMWC!(4096U,18782UL)     CMWC_4096_1;
alias CMWC!(2048U,1030770UL)   CMWC_2048_1;
alias CMWC!(2048U,1047570UL)   CMWC_2048_2;
alias CMWC!(1024U,5555698UL)   CMWC_1024_1;
alias CMWC!(1024U,987769338UL) CMWC_1024_2;
alias CMWC!(512U,123462658UL)  CMWC_512_1;
alias CMWC!(512U,123484214UL)  CMWC_512_2;
alias CMWC!(256U,987662290UL)  CMWC_256_1;
alias CMWC!(256U,987665442UL)  CMWC_256_2;
alias CMWC!(128U,987688302UL)  CMWC_128_1;
alias CMWC!(128U,987689614UL)  CMWC_128_2;
alias CMWC!(64U ,987651206UL)  CMWC_64_1;
alias CMWC!(64U ,987657110UL)  CMWC_64_2;
alias CMWC!(32U ,987655670UL)  CMWC_32_1;
alias CMWC!(32U ,987655878UL)  CMWC_32_2;
alias CMWC!(16U ,987651178UL)  CMWC_16_1;
alias CMWC!(16U ,987651182UL)  CMWC_16_2;
alias CMWC!(8U  ,987651386UL)  CMWC_8_1;
alias CMWC!(8U  ,987651670UL)  CMWC_8_2;
alias CMWC!(4U  ,987654366UL)  CMWC_4_1;
alias CMWC!(4U  ,987654978UL)  CMWC_4_2;
alias CMWC_1024_2 CMWC_default;
