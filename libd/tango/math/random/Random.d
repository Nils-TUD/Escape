/*******************************************************************************
    Random number generators
    
    This is an attempt at having a good flexible and easy to use random number
    generator.
    ease of use:
    $(UL
      $(LI  shared generator for quick usage available through the "rand" object
            ---
            int i=rand.uniformR(10); // a random number from [0;10)
            ---
      )
      $(LI  simple Random (non threadsafe) and RandomSync (threadsafe) types to 
            create new generators (for heavy use a good idea is one Random object per thread)
      )
      $(LI  several distributions can be requested like this
            ---
            rand.distributionD!(type)(paramForDistribution)
            ---
            the type can often be avoided if the parameters make it clear.
            From it single numbers can be generated with .getRandom(), and variables
            initialized either with call style (var) or with .randomize(var).
            Utility functions to generate numbers directly are also available.
            The choice to put all the distribution in a single object that caches them
            has made (for example) the gamma distribution very easy to implement.
      )
      $(LI  sample usage:
            ---
            auto r=new Random();
            int i; float f; real rv; real[100] ar0; real[] ar=ar0[];
            // initialize with uniform distribution
            i=r.uniform!(int);
            f=r.uniform!(float);
            rv=r.uniform!(real);
            foreach (ref el;ar)
              el=r.uniform!(real);
            // another way to do all the previous in one go:
            r(i)(f)(rv)(ar);
            // unfortunetely one cannot use directly ar0...
            // uniform distribution 0..10
            i=r.uniformR(10);
            f=r.uniformR(10.0f);
            rv=r.uniformR(10.0L);
            foreach (ref el;ar)
              el=r.uniformR(10.0L);
            // another way to do all the previous in one go:
            r.uniformRD(10)(i)(f)(r)(ar);
            // uniform numbers in [5;10)
            i=r.uniformR2(5,10);
            // uniform numbers in (5;10)
            f=r.uniformR2(5.0f,10.0f);
            rv=r.uniformR2(5.0L,10.0L);
            foreach (ref el;ar)
              el=r.uniformR2(5.0L,10.0L);
            // another way to do all the previous in one go:
            r.uniformR2D(5.0L,10.0L)(i)(f)(r)(ar);
            // uniform distribution -10..10
            i=r.uniformRSymm(10);
            // well you get it...
            r.uniformRSymmD(10)(i)(f)(r)(ar);
            // any distribution can be stored
            auto r2=r.uniformRSymmD(10);
            // and used later
            r2(ar);
            // complex distributions (normal,exp,gamma) are produced for the requested type
            r.normalSource!(float)()(f);
            // with sigma=2
            r.normalD(2.0f)(f);
            // and can be used also to initialize other types
            r.normalSource!(float)()(r)(ar);
            r.normalD(2.0f)(r)(ar);
            // but this is different from
            r.normalSource!(real)()(i)(r)(ar);
            r.normalD(2.0L)(i)(r)(ar);
            // as the source generates numbers of its type that then are simply cast to
            // the type needed.
            // Uniform distribution (as its creation for different types has no overhead)
            // is never cast, so that (for example) bounds exclusion for floats is really
            // guaranteed.
            // For the other distribution using a distribution of different type than
            // the variable should be done with care, as underflow/overflow might ensue.
            //
            // Some utility functions are also available
            int i2=r.uniform!(int)();
            int i2=r.randomize(i); // both i and i2 are initialized to the same value
            float f2=r.normalSigma(3.0f);
            ---
      )
    )
    flexibility:
    $(UL
      $(LI  easily swappable basic source
            ---
            // a random generator that uses the system provided random generator:
            auto r=RandomG!(Urandom)();
            ---
            One could also build an engine that can be changed at runtime (that calls
            a delegate for example), but this adds a little overhead, and changing
            engine is not something done often, so this is not part of the library.
      )
      $(LI  ziggurat generator can be easily adapted to any decreasing derivable
            distribution, the hard parametrization (to find xLast) can be done
            automatically
      )
      $(LI  several distributions available "out of the box"
      )
      )
      Quality:
      $(UL
      $(LI  the default Source combines two surces that pass all statistical tests 
            (KISS+CMWC)
            (P. L'Ecuyer and R. Simard, ACM Transactions on Mathematical Software (2007),
            33, 4, Article 22, for KISS, see CMWC enigine for the other)
      )
      $(LI  floating point uniform generator always initializes the full mantissa, the
            only flaw is a (*very* small) predilection of 0 as least important bit 
            (IEEE rounds to 0 in case of tie).
            Using a method that initializes the full mantissa was shown to improve the
            quality of subsequntly derived normal distribued numbers
            (Thomas et al. Gaussian random number generators. Acm Comput Surv (2007)
            vol. 39 (4) pp. 11))
      )
      $(LI  Ziggurat method, a very fast and accurate method was used for both Normal and
            exp distributed numbers.
      )
      $(LI  gamma distribued numbers uses a method recently proposed by Marsaglia and
            Tsang. The method is very fast, and should be good.
            My (Fawzi's) feeling is that the transformation h(x)=(1+d*x)^3 might lose
            a couple of bits of precision in some cases, but it is unclear if this
            might become visible in (*very* extensive) tests or not.
      )
       the basic source can be easily be changed with something else
      Efficiency:
      $(LI  very fast methods have been used, and some effort has been put into
            optimizing some of them, but not all, but the interface has been choosen
            so that close to optimal implementation can be provided through the same
            interface.
      )
      $(LI  Normal and Exp sources allocated only upon request: no memory waste, but
            a (*very* small) speed hit, that can be avoided by storing the source in
            a variable and using it (not going through the RandomG)
      )
    )
    Annoyances:
    $(UL
      $(LI  I have added two "next" methods to RandomG for backward compatibility
            reasons, and the .instance from Random has been
            replaced by the "rand" object. The idea behind this is that RandomG is
            a template and rand it should be shared across all templates.
            If the name rand is considered bad one could change it. 
            I kept .instance static method that returns rand, so this remain a dropin
            replacement of the old random.
      )
      $(LI You cannot initialize a static array directly, this because randomize is
          declared like this:
            ---
            U randomize(U)(ref U a) { }
            ---
            and a static array cannot be passed by reference. Removing the ref would
            make arrays initialized, and scalar not, which is much worse.
      )
    )

        copyright:      Copyright (c) 2008. Fawzi Mohamed
        license:        BSD style: $(LICENSE)
        version:        Initial release: July 2008
        author:         Fawzi Mohamed

*******************************************************************************/
module tango.math.random.Random;
import tango.math.random.engines.URandom;
import tango.math.random.engines.KissCmwc;
import tango.math.random.engines.ArraySource;
// TODO import tango.math.random.engines.Sync;
import tango.math.random.engines.Twister;
import tango.math.random.NormalSource;
import tango.math.random.ExpSource;
import tango.math.Math;
import tango.core.Traits;

// ----- templateFu begin --------
/// compile time integer power
private T ctfe_powI(T)(T x,int p){
    T xx=cast(T)1;
    if (p<0){
        p=-p;
        x=1/x;
    }
    for (int i=0;i<p;++i)
        xx*=x;
    return xx;
}
// ----- templateFu end --------

version (Win32) {
         private extern(Windows) int QueryPerformanceCounter (ulong *);
}
version (Posix) {
    private import tango.stdc.posix.sys.time;
}
version (Escape) {
	extern (C) ulong cpu_rdtsc();
}

version(darwin) { version=has_urandom; }
version(linux)  { version=has_urandom; }
version(solaris){ version=has_urandom; }

/// if T is a float
template isFloat(T){
    static if(is(T==float)||is(T==double)||is(T==real)){
        const bool isFloat=true;
    } else {
        const bool isFloat=false;
    }
}

/// The default engine, a reasonably collision free, with good statistical properties
/// not easy to invert, and with a relatively small key (but not too small)
alias KissCmwc_32_1 DefaultEngine;

/// Class that represents a random number generator.
/// Normally you should get random numbers either with call-like interface:
///   auto r=new Random(); r(i)(j)(k);
/// or with randomize
///   r.randomize(i); r.randomize(j); r.randomize(k);
/// if you use this you should be able to easily switch distribution later,
/// as all distributions support this interface, and can be built on the top of RandomG
///   auto r2=r.NormalSource!(float)(); r2(i)(j)(k);
/// there are utility methods within random for the cases in which you do not
/// want to build a special distribution for just a few numbers
final class RandomG(SourceT=DefaultEngine)
{
    // uniform random source
    SourceT source;
    // normal distributed sources
    NormalSource!(RandomG,float)  normalFloat;
    NormalSource!(RandomG,double) normalDouble;
    NormalSource!(RandomG,real)   normalReal;
    // exp distributed sources
    ExpSource!(RandomG,float)  expFloat;
    ExpSource!(RandomG,double) expDouble;
    ExpSource!(RandomG,real)   expReal;

    /// Creates and seeds a new generator
    this (bool randomInit=true)
    {
        if (randomInit)
            this.seed;
    }
    
    /// if source.canSeed seeds the generator using the shared rand generator
    /// (use urandom directly if available?)
    RandomG seed ()
    {
        static if(source.canSeed){
            source.seed(&rand.uniform!(uint));
        }
        return this;
    }
    /// if source.canSeed seeds the generator using the given source of uints
    RandomG seed (uint delegate() seedSource)
    {
        static if(source.canSeed){
            source.seed(seedSource);
        }
        return this;
    }
    
    /// compatibility with old Random, deprecate??
    uint next(){
        return uniform!(uint)();
    }
    /// ditto
    uint next(uint to){
        return uniformR!(uint)(to);
    }
    /// ditto
    uint next(uint from,uint to){
        return uniformR2!(uint)(from,to);
    }
    /// ditto
    // TODO 
    /*static RandomG!(Sync!(DefaultEngine)) instance(){
        return rand;
    }*/
    //-------- Utility functions to quickly get a uniformly distributed random number -----------

    /// uniform distribution on the whole range of integer types, and on
    /// the (0;1) range for floating point types. Floating point guarantees the initialization
    /// of the full mantissa, but due to rounding effects it might have *very* small
    /// dependence due to rounding effects on the least significant bit (in case of tie 0 is favored).
    /// if boundCheck is false in the floating point case bounds might be included (but with a
    /// lower propability than other numbers)
    T uniform(T,bool boundCheck=true)(){
        static if(is(T==uint)) {
            return source.next;
        } else static if (is(T==int) || is(T==char) || is(T==byte) || is(T==ubyte)){
            union Uint2A{
                T t;
                uint u;
            }
            Uint2A a;
            a.u=source.next;
            return a.t;
        } else static if (is(T==long) || is (T==ulong)){
            return cast(T)source.nextL;
        } else static if (is(T==bool)){
            return cast(bool)(source.next & 1u); // check lowest bit
        } else static if (is(T==float)||is(T==double)||is(T==real)){
            static if (T.mant_dig<30) {
                const T halfT=(cast(T)1)/(cast(T)2);
                const T fact32=ctfe_powI(halfT,32);
                const uint minV=1u<<(T.mant_dig-1);
                uint nV=source.next;
                if (nV>=minV) {
                    T res=nV*fact32;
                    static if (boundCheck){
                        if (res!=cast(T)1) return res;
                        // 1 due to rounding (<3.e-8), 0 impossible
                        return uniform!(T,boundCheck)();
                    } else {
                        return res;
                    }
                } else { // probability 0.00390625 for 24 bit mantissa
                    T scale=fact32;
                    while (nV==0){ // probability 2.3283064365386963e-10
                        nV=source.next;
                        scale*=fact32;
                    }
                    T res=nV*scale+source.next*scale*fact32;
                    static if (boundCheck){
                        if (res!=cast(T)0) return res;
                        return uniform!(T,boundCheck)(); // 0 due to underflow (<1.e-38), 1 impossible
                    } else {
                        return res;
                    }
                }
            } else static if (T.mant_dig<62) {
                const T halfT=(cast(T)1)/(cast(T)2);
                const T fact64=ctfe_powI(halfT,64);
                const ulong minV=1UL<<(T.mant_dig-1);
                ulong nV=source.nextL;
                if (nV>=minV) {
                    T res=nV*fact64;
                    static if (boundCheck){
                        if (res!=cast(T)1) return res;
                        // 1 due to rounding (<1.e-16), 0 impossible
                        return uniform!(T,boundCheck)();
                    } else {
                        return res;
                    }
                } else { // probability 0.00048828125 for 53 bit mantissa
                    const T fact32=ctfe_powI(halfT,32);
                    const ulong minV2=1UL<<(T.mant_dig-33);
                    if (nV>=minV2){
                        return ((cast(T)nV)+(cast(T)source.next)*fact32)*fact64;
                    } else { // probability 1.1368683772161603e-13 for 53 bit mantissa
                        T scale=fact64;
                        while (nV==0){
                            nV=source.nextL;
                            scale*=fact64;
                        }
                        T res=scale*((cast(T)nV)+(cast(T)source.nextL)*fact64);
                        static if (boundCheck){
                            if (res!=cast(T)0) return res;
                            // 0 due to underflow (<1.e-307)
                            return uniform!(T,boundCheck)();
                        } else {
                            return res;
                        }
                    }
                }
            } else static if (T.mant_dig<=64){
                const T halfT=(cast(T)1)/(cast(T)2);
                const T fact8=ctfe_powI(halfT,8);
                const T fact72=ctfe_powI(halfT,72);
                ubyte nB=source.nextB;
                if (nB!=0){
                    T res=nB*fact8+source.nextL*fact72;
                    static if (boundCheck){
                        if (res!=cast(T)1) return res;
                        // 1 due to rounding (<1.e-16), 0 impossible
                        return uniform!(T,boundCheck)();
                    } else {
                        return res;
                    }
                } else { // probability 0.00390625
                    const T fact64=ctfe_powI(halfT,64);
                    T scale=fact8;
                    while (nB==0){
                        nB=source.nextB;
                        scale*=fact8;
                    }
                    T res=((cast(T)nB)+(cast(T)source.nextL)*fact64)*scale;
                    static if (boundCheck){
                        if (res!=cast(T)0) return res;
                        // 0 due to underflow (<1.e-4932), 1 impossible
                        return uniform!(T,boundCheck)();
                    } else {
                        return res;
                    }
                }
            } else {
                // (T.mant_dig > 64 bits), not so optimized, but works for any size
                const T halfT=(cast(T)1)/(cast(T)2);
                const T fact32=ctfe_powI(halfT,32);
                uint nL=source.next;
                T fact=fact32;
                while (nL==0){
                    fact*=fact32;
                    nL=source.next;
                }
                T res=nL*fact;
                for (int rBits=T.mant_dig-1;rBits>0;rBits-=32) {
                    fact*=fact32;
                    res+=source.next()*fact;
                }
                static if (boundCheck){
                    if (res!=cast(T)0 && res !=cast(T)1) return res;
                    return uniform!(T,boundCheck)(); // really unlikely...
                } else {
                    return res;
                }
            }
        } else static if (is(T==cfloat)||is(T==cdouble)||is(T==creal)){
            return cast(T)(uniform!(realType!(T))()+1i*uniform!(realType!(T))());
        } else static if (is(T==ifloat)||is(T==idouble)||is(T==ireal)){
            return cast(T)(1i*uniform!(realType!(T))());
        } else static assert(0,T.stringof~" unsupported type for uniform distribution");
    }
    
    /// uniform distribution on the range [0;to) for integer types, and on
    /// the (0;to) range for floating point types. Same caveat as uniform(T) apply
    T uniformR(T,bool boundCheck=true)(T to)
    in { assert(to>0,"empty range");}
    body {
        static if (is(T==uint) || is(T==int) || is(T==char) || is(T==byte) || is(T==ubyte)){
            uint d=uint.max/cast(uint)to,dTo=to*d;
            uint nV=source.next;
            if (nV>=dTo){
                for (int i=0;i<1000;++i) {
                    nV=source.next;
                    if (nV<dTo) break;
                }
                assert(nV<dTo,"this is less probable than 1.e-301, something is wrong with the random number generator");
            }
            return cast(T)(nV%to);
        } else static if (is(T==ulong) || is(T==long)){
            ulong d=ulong.max/cast(ulong)to,dTo=to*d;
            ulong nV=source.nextL;
            if (nV>=dTo){
                for (int i=0;i<1000;++i) {
                    nV=source.nextL;
                    if (nV<dTo) break;
                }
                assert(nV<dTo,"this is less probable than 1.e-301, something is wrong with the random number generator");
            }
            return cast(T)(nV%to);
        } else static if (is(T==float)|| is(T==double)||is(T==real)){
            T res=uniform!(T,false)*to;
            static if (boundCheck){
                if (res!=cast(T)0 && res!=to) return res;
                return uniformR(to);
            } else {
                return res;
            }
        } else static assert(0,T.stringof~" unsupported type for uniformR distribution");
    }
    /// uniform distribution on the range (-to;to) for integer types, and on
    /// the (-to;0)(0;to) range for floating point types if boundCheck is true.
    /// If boundCheck=false the range changes to [-to;0)u(0;to] with a slightly
    /// lower propability at the bounds for floating point numbers.
    /// excludeZero controls if 0 is excluded or not (by default float exclude it,
    /// ints no). Please note that the probability of 0 in floats is very small due
    //  to the high density of floats close to 0.
    /// Cannot be used on unsigned types.
    ///
    /// In here there is probably one of the few cases where c handling of modulo of negative
    /// numbers is handy
    T uniformRSymm(T,bool boundCheck=true, bool excludeZero=isFloat!(T))(T to,int iter=2000)
    in { assert(to>0,"empty range");}
    body {
        static if (is(T==int)|| is(T==byte)){
            int d=int.max/to,dTo=to*d;
            int nV=cast(int)source.next;
            static if (excludeZero){
                int isIn=nV<dTo&&nV>-dTo&&nV!=0;
            } else {
                int isIn=nV<dTo&&nV>-dTo;
            }
            if (isIn){
                return cast(T)(nV%to);
            } else {
                for (int i=0;i<1000;++i) {
                    nV=cast(int)source.next;
                    if (nV<dTo && nV>-dTo) break;
                }
                assert(nV<dTo && nV>-dTo,"this is less probable than 1.e-301, something is wrong with the random number generator");
                return cast(T)(nV%to);
            }
        } else static if (is(T==long)){
            long d=long.max/to,dTo=to*d;
            long nV=cast(long)source.nextL;
            static if (excludeZero){
                int isIn=nV<dTo&&nV>-dTo&&nV!=0;
            } else {
                int isIn=nV<dTo&&nV>-dTo;
            }
            if (isIn){
                return nV%to;
            } else {
                for (int i=0;i<1000;++i) {
                    nV=source.nextL;
                    if (nV<dTo && nV>-dTo) break;
                }
                assert(nV<dTo && nV>-dTo,"this is less probable than 1.e-301, something is wrong with the random number generator");
                return nV%to;
            }
        } else static if (is(T==float)||is(T==double)||is(T==real)){
            static if (T.mant_dig<30){
                const T halfT=(cast(T)1)/(cast(T)2);
                const T fact32=ctfe_powI(halfT,32);
                const uint minV=1u<<T.mant_dig;
                uint nV=source.next;
                if (nV>=minV) {
                    T res=nV*fact32*to;
                    static if (boundCheck){
                        if (res!=to) return (1-2*cast(int)(nV&1u))*res;
                        // to due to rounding (~3.e-8), 0 impossible with normal to values
                        assert(iter>0,"error with the generator, probability < 10^(-8*2000)");
                        return uniformRSymm(to,iter-1);
                    } else {
                        return (1-2*cast(int)(nV&1u))*res;
                    }
                } else { // probability 0.008 for 24 bit mantissa
                    T scale=fact32;
                    while (nV==0){ // probability 2.3283064365386963e-10
                        nV=source.next;
                        scale*=fact32;
                    }
                    uint nV2=source.next;
                    T res=(cast(T)nV+cast(T)nV2*fact32)*scale*to;
                    static if (excludeZero){
                        if (res!=cast(T)0) return (1-2*cast(int)(nV&1u))*res;
                        assert(iter>0,"error with the generator, probability < 10^(-8*2000)");
                        return uniformRSymm(to,iter-1); // 0 due to underflow (<1.e-38), 1 impossible
                    } else {
                        return (1-2*cast(int)(nV&1u))*res;
                    }
                }
            } else static if (T.mant_dig<62) {
                const T halfT=(cast(T)1)/(cast(T)2);
                const T fact64=ctfe_powI(halfT,64);
                const ulong minV=1UL<<(T.mant_dig);
                ulong nV=source.nextL;
                if (nV>=minV) {
                    T res=nV*fact64*to;
                    static if (boundCheck){
                        if (res!=to) return (1-2*cast(int)(nV&1UL))*res;
                        // to due to rounding (<1.e-16), 0 impossible with normal to values
                        assert(iter>0,"error with the generator, probability < 10^(-16*2000)");
                        return uniformRSymm(to,iter-1);
                    } else {
                        return (1-2*cast(int)(nV&1UL))*res;
                    }
                } else { // probability 0.00048828125 for 53 bit mantissa
                    const T fact32=ctfe_powI(halfT,32);
                    const ulong minV2=1UL<<(T.mant_dig-32);
                    if (nV>=minV2){
                        uint nV2=source.next;
                        T res=((cast(T)nV)+(cast(T)nV2)*fact32)*fact64*to;
                        return (1-2*cast(int)(nV2&1UL))*res; // cannot be 0 or to with normal to values
                    } else { // probability 1.1368683772161603e-13 for 53 bit mantissa
                        T scale=fact64;
                        while (nV==0){
                            nV=source.nextL;
                            scale*=fact64;
                        }
                        ulong nV2=source.nextL;
                        T res=to*scale*((cast(T)nV)+(cast(T)nV2)*fact64);
                        static if (excludeZero){
                            if (res!=cast(T)0) return (1-2*cast(int)(nV2&1UL))*res;
                            // 0 due to underflow (<1.e-307)
                            assert(iter>0,"error with the generator, probability < 10^(-16*2000)");
                            return uniformRSymm(to,iter-1);
                        } else {
                            return (1-2*cast(int)(nV2&1UL))*res;
                        }
                    }
                }
            } else static if (T.mant_dig<=64) {
                const T halfT=(cast(T)1)/(cast(T)2);
                const T fact8=ctfe_powI(halfT,8);
                const T fact72=ctfe_powI(halfT,72);
                ubyte nB=source.nextB;
                if (nB!=0){
                    ulong nL=source.nextL;
                    T res=to*(nB*fact8+nL*fact72);
                    static if (boundCheck){
                        if (res!=to) return (1-2*cast(int)(nL&1UL))*res;
                        // 1 due to rounding (<1.e-16), 0 impossible with normal to values
                        assert(iter>0,"error with the generator, probability < 10^(-16*2000)");
                        return uniformRSymm(to,iter-1);
                    } else {
                        return (1-2*cast(int)(nL&1UL))*res;
                    }
                } else { // probability 0.00390625
                    const T fact64=ctfe_powI(halfT,64);
                    T scale=fact8;
                    while (nB==0){
                        nB=source.nextB;
                        scale*=fact8;
                    }
                    ulong nL=source.nextL;
                    T res=((cast(T)nB)+(cast(T)nL)*fact64)*scale*to;
                    static if (excludeZero){
                        if (res!=cast(T)0) return ((nL&1UL)?res:-res);
                        // 0 due to underflow (<1.e-4932), 1 impossible
                        assert(iter>0,"error with the generator, probability < 10^(-16*2000)");
                        return uniformRSymm(to,iter-1);
                    } else {
                        return ((nL&1UL)?res:-res);
                    }
                }
            } else {
                // (T.mant_dig > 64 bits), not so optimized, but works for any size
                const T halfT=(cast(T)1)/(cast(T)2);
                const T fact32=ctfe_powI(halfT,32);
                uint nL=source.next;
                T fact=fact32;
                while (nL==0){
                    fact*=fact32;
                    nL=source.next;
                }
                T res=nL*fact;
                for (int rBits=T.mant_dig;rBits>0;rBits-=32) {
                    fact*=fact32;
                    nL=source.next();
                    res+=nL*fact;
                }
                static if (boundCheck){
                    if (res!=to && res!=cast(T)0) return ((nL&1UL)?res:-res);
                    // 1 due to rounding (<1.e-16), 0 impossible with normal to values
                    assert(iter>0,"error with the generator, probability < 10^(-16*2000)");
                    return uniformRSymm(to,iter-1);
                } else {
                    return ((nL&1UL)?res:-res);
                }
            }
        } else static assert(0,T.stringof~" unsupported type for uniformRSymm distribution");
    }
    /// uniform distribution [from;to) for integers, and (from;to) for floating point numbers.
    /// if boundCheck is false the bounds are included in the floating point number distribution.
    /// the range for int and long is limited to only half the possible range
    /// (it could be worked around using long aritmethic for int, and doing a carry by hand for long,
    /// but I think it is seldomly needed, for int you are better off using long when needed)
    T uniformR2(T,bool boundCheck=true)(T from,T to)
    in {
        assert(to>from,"empy range in uniformR2");
        static if (is(T==int) || is(T==long)){
            assert(from>T.min/2&&to<T.max/2," from..to range too big");
        }
    }
    body {
        static if (is(T==int)||is(T==long)||is(T==uint)||is(T==ulong)){
            return from+uniformR(to-from);
        } else if (is(T==char) || is(T==byte) || is(T==ubyte)){
            int d=cast(int)to-cast(int)from;
            int nV=uniformR(d);
            return cast(T)(nV+cast(int)from);
        } else static if (is(T==float) || is(T==double) || is(T==real)){
            T res=from+(to-from)*uniform!(T,false);
            static if (boundCheck){
                if (res!=from && res!=to) return res;
                return uniformR2(from,to);
            } else {
                return res;
            }
        } else static assert(0,T.stringof~" unsupported type for uniformR2 distribution");
    }
    /// returns a random element of the given array (which must be non empty)
    T uniformEl(T)(T[] arr){
        assert(arr.length>0,"array has to be non empty");
        return arr[uniformR(arr.length)];
    }
    /// randomizes the given array and returns it (for some types this is potentially
    /// more efficient, both from the use of random numbers and speedwise)
    U randomizeUniform(U,bool boundCheck)(ref U a){
        static if (is(U S:S[])){
            alias BaseTypeOfArrays!(U) T;
            static if (is(T==byte)||is(T==ubyte)||is(T==char)){
                uint val=source.next; /// begin without value?
                int rest=4;
                for (size_t i=0;i<a.length;++i) {
                    if (rest!=0){
                        a[i]=cast(T)(0xFFu&val);
                        val>>=8;
                        --rest;
                    } else {
                        val=source.next;
                        a[i]=cast(T)(0xFFu&val);
                        val>>=8;
                        rest=3;
                    }
                }
            } else static if (is(T==uint)||is(T==int)){
                T* aEnd=a.ptr+a.length;
                for (T* aPtr=a.ptr;aPtr!=aEnd;++aPtr)
                    *aPtr=cast(T)(source.next);
            } else static if (is(T==ulong)||is(T==long)){
                T* aEnd=a.ptr+a.length;
                for (T* aPtr=a.ptr;aPtr!=aEnd;++aPtr)
                    *aPtr=cast(T)(source.nextL);
            } else static if (is(T==float)|| is(T==double)|| is(T==real)) {
                // optimize more? not so easy with guaranteed full mantissa initialization
                T* aEnd=a.ptr+a.length;
                for (T* aPtr=a.ptr;aPtr!=aEnd;++aPtr){
                    *aPtr=uniform!(T,boundCheck)();
                }
            } else static assert(T.stringof~" type not supported by randomizeUniform");
        } else {
            a=uniform!(U,boundCheck)();
        }
        return a;
    }
    
    /// randomizes the given array and returns it (for some types this is potentially
    /// more efficient, both from the use of random numbers and speedwise)
    U randomizeUniformR(U,V,bool boundCheck=true)(ref U a,V to)
    in { assert((cast(BaseTypeOfArrays!(U))to)>0,"empty range");}
    body {
        alias BaseTypeOfArrays!(U) T;
        static assert(is(V:T),"incompatible a and to type "~U.stringof~" "~V.stringof);
        static if (is(U S:S[])){
            static if (is(T==uint) || is(T==int) || is(T==char) || is(T==byte) || is(T==ubyte)){
                uint d=uint.max/cast(uint)to,dTo=(cast(uint)to)*d;
                T* aEnd=a.ptr+a.length;
                for (T* aPtr=a.ptr;aPtr!=aEnd;++aPtr){
                    uint nV=source.next;
                    if (nV<dTo){
                        *aPtr=cast(T)(nV % cast(uint)to);
                    } else {
                        for (int i=0;i<1000;++i) {
                            nV=source.next;
                            if (nV<dTo) break;
                        }
                        assert(nV<dTo,"this is less probable than 1.e-301, something is wrong with the random number generator");
                        *aPtr=cast(T)(nV % cast(uint)to);
                    }
                }
            } else static if (is(T==ulong) || is(T==long)){
                ulong d=ulong.max/cast(ulong)to,dTo=(cast(ulong)to)*d;
                T* aEnd=a.ptr+a.length;
                for (T* aPtr=a.ptr;aPtr!=aEnd;++aPtr){
                    ulong nV=source.nextL;
                    if (nV<dTo){
                        el=cast(T)(nV % cast(ulong)to);
                    } else {
                        for (int i=0;i<1000;++i) {
                            nV=source.nextL;
                            if (nV<dTo) break;
                        }
                        assert(nV<dTo,"this is less probable than 1.e-301, something is wrong with the random number generator");
                        el=cast(T)(nV% cast(ulong)to);
                    }
                }
            } else static if (is(T==float) || is(T==double) || is(T==real)){
                T* aEnd=a.ptr+a.length;
                for (T* aPtr=a.ptr;aPtr!=aEnd;++aPtr){
                    *aPtr=uniformR!(T,boundCheck)(cast(T)to);
                }
            } else static assert(0,T.stringof~" unsupported type for uniformR distribution");
        } else {
            a=uniformR!(T,boundCheck)(cast(T)to);
        }
        return a;
    }
    /// randomizes the given variable and returns it (for some types this is potentially
    /// more efficient, both from the use of random numbers and speedwise)
    U randomizeUniformR2(U,V,W,bool boundCheck=true)(ref U a,V from, W to)
    in {
        alias BaseTypeOfArrays!(U) T;
        assert((cast(T)to)>(cast(T)from),"empy range in uniformR2");
        static if (is(T==int) || is(T==long)){
            assert(from>T.min/2&&to<T.max/2," from..to range too big");
        }
    }
    body {
        alias BaseTypeOfArrays!(U) T;
        static assert(is(V:T),"incompatible a and from type "~U.stringof~" "~V.stringof);
        static assert(is(W:T),"incompatible a and to type "~U.stringof~" "~W.stringof);
        static if (is(U S:S[])){
            static if (is(T==uint)||is(T==ulong)){
                T d=cast(T)to-cast(T)from;
                T* aEnd=a.ptr+a.length;
                for (T* aPtr=a.ptr;aPtr!=aEnd;++aPtr){
                    *aPtr=from+uniformR!(d)();
                }
            } else if (is(T==char) || is(T==byte) || is(T==ubyte)){
                int d=cast(int)to-cast(int)from;
                T* aEnd=a.ptr+a.length;
                for (T* aPtr=a.ptr;aPtr!=aEnd;++aPtr){
                    *aPtr=cast(T)(uniformR!(d)+cast(int)from);
                }
            } else static if (is(T==float) || is(T==double) || is(T==real)){
                T* aEnd=a.ptr+a.length;
                for (T* aPtr=a.ptr;aPtr!=aEnd;++aPtr){
                    T res=cast(T)from+(cast(T)to-cast(T)from)*uniform!(T,false);
                    static if (boundCheck){
                        if (res!=cast(T)from && res!=cast(T)to){
                            *aPtr=res;
                        } else {
                            *aPtr=uniformR2!(T,boundCheck)(cast(T)from,cast(T)to);
                        }
                    } else {
                        *aPtr=res;
                    }
                }
            } else static assert(0,T.stringof~" unsupported type for uniformR2 distribution");
        } else {
            a=uniformR2!(T,boundCheck)(from,to);
        }
        return a;
    }
    /// randomizes the given variable like uniformRSymm and returns it
    /// (for some types this is potentially more efficient, both from the use of
    /// random numbers and speedwise)
    U randomizeUniformRSymm(U,V,bool boundCheck=true, bool excludeZero=isFloat!(BaseTypeOfArrays!(U)))
        (ref U a,V to)
    in { assert((cast(BaseTypeOfArrays!(U))to)>0,"empty range");}
    body {
        alias BaseTypeOfArrays!(U) T;
        static assert(is(V:T),"incompatible a and to type "~U.stringof~" "~V.stringof);
        static if (is(U S:S[])){
            static if (is(T==int)|| is(T==byte)){
                int d=int.max/cast(int)to,dTo=(cast(int)to)*d;
                T* aEnd=a.ptr+a.length;
                for (T* aPtr=a.ptr;aPtr!=aEnd;++aPtr){
                    int nV=cast(int)source.next;
                    static if (excludeZero){
                        int isIn=nV<dTo&&nV>-dTo&&nV!=0;
                    } else {
                        int isIn=nV<dTo&&nV>-dTo;
                    }
                    if (isIn){
                        *aPtr=cast(T)(nV% cast(int)to);
                    } else {
                        for (int i=0;i<1000;++i) {
                            nV=cast(int)source.next;
                            if (nV<dTo&&nV>-dTo) break;
                        }
                        assert(nV<dTo && nV>-dTo,"this is less probable than 1.e-301, something is wrong with the random number generator");
                        *aPtr=cast(T)(nV% cast(int)to);
                    }
                }
            } else static if (is(T==long)){
                long d=long.max/cast(T)to,dTo=(cast(T)to)*d;
                long nV=cast(long)source.nextL;
                T* aEnd=a.ptr+a.length;
                for (T* aPtr=a.ptr;aPtr!=aEnd;++aPtr){
                    static if (excludeZero){
                        int isIn=nV<dTo&&nV>-dTo&&nV!=0;
                    } else {
                        int isIn=nV<dTo&&nV>-dTo;
                    }
                    if (isIn){
                        *aPtr=nV% cast(T)to;
                    } else {
                        for (int i=0;i<1000;++i) {
                            nV=source.nextL;
                            if (nV<dTo && nV>-dTo) break;
                        }
                        assert(nV<dTo && nV>-dTo,"this is less probable than 1.e-301, something is wrong with the random number generator");
                        *aPtr=nV% cast(T)to;
                    }
                }
            } else static if (is(T==float)||is(T==double)||is(T==real)){
                T* aEnd=a.ptr+a.length;
                for (T* aPtr=a.ptr;aPtr!=aEnd;++aPtr){
                    *aPtr=uniformRSymm!(T,boundCheck,excludeZero)(cast(T)to);
                }
            } else static assert(0,T.stringof~" unsupported type for uniformRSymm distribution");
        } else {
            a=uniformRSymm!(T,boundCheck,excludeZero)(cast(T)to);
        }
        return a;
    }
    
    /// returns another (mostly indipendent, depending on seed size) random generator
    RandG spawn(RandG=RandomG)(){
        RandG res=new RandG(false);
        synchronized(this){
            res.seed(&uniform!(uint));
        }
        return res;
    }
    
    // ------- structs for uniform distributions -----
    /// uniform distribution on the whole range for integers, and on (0;1) for floats
    /// with boundCheck=true this is equivalent to r itself, here just for completness
    struct UniformDistribution(T,bool boundCheck){
        RandomG r;
        static UniformDistribution create(RandomG r){
            UniformDistribution res;
            res.r=r;
            return res;
        }
        /// chainable call style initialization of variables (thorugh a call to randomize)
        UniformDistribution opCall(U,S...)(ref U a,S args){
            randomize(a,args);
            return *this;
        }
        /// returns a random number
        T getRandom(){
            return r.uniform!(T,boundCheck)();
        }
        /// initialize el
        U randomize(U)(ref U a){
            return r.randomizeUniform!(U,boundCheck)(a);
        }    
    }

    /// uniform distribution on the subrange [0;to) for integers, (0;to) for floats
    struct UniformRDistribution(T,bool boundCheck){
        T to;
        RandomG r;
        /// initializes the probability distribution
        static UniformRDistribution create(RandomG r,T to){
            UniformRDistribution res;
            res.r=r;
            res.to=to;
            return res;
        }
        /// chainable call style initialization of variables (thorugh a call to randomize)
        UniformRDistribution opCall(U)(ref U a){
            randomize(a);
            return *this;
        }
        /// returns a random number
        T getRandom(){
            return r.uniformR!(T,boundCheck)(to);
        }
        /// initialize el
        U randomize(U)(ref U a){
            return r.randomizeUniformR!(U,T,boundCheck)(a,to);
        }
    }

    /// uniform distribution on the subrange (-to;to) for integers, (-to;0)u(0;to) for floats
    /// excludeZero controls if the zero should be excluded, boundCheck if the boundary should
    /// be excluded for floats
    struct UniformRSymmDistribution(T,bool boundCheck=true,bool excludeZero=isFloat!(T)){
        T to;
        RandomG r;
        /// initializes the probability distribution
        static UniformRSymmDistribution create(RandomG r,T to){
            UniformRSymmDistribution res;
            res.r=r;
            res.to=to;
            return res;
        }
        /// chainable call style initialization of variables (thorugh a call to randomize)
        UniformRSymmDistribution opCall(U)(ref U a){
            randomize(a);
            return *this;
        }
        /// returns a random number
        T getRandom(){
            return r.uniformRSymm!(T,boundCheck,excludeZero)(to);
        }
        /// initialize el
        U randomize(U)(ref U a){
            return r.randomizeUniformRSymm!(U,T,boundCheck)(a,to);
        }
    }

    /// uniform distribution on the subrange (-to;to) for integers, (0;to) for floats
    struct UniformR2Distribution(T,bool boundCheck){
        T from,to;
        RandomG r;
        /// initializes the probability distribution
        static UniformRDistribution create(RandomG r,T from, T to){
            UniformRDistribution res;
            res.r=r;
            res.from=from;
            res.to=to;
            return res;
        }
        /// chainable call style initialization of variables (thorugh a call to randomize)
        UniformRDistribution opCall(U,S...)(ref U a,S args){
            randomize(a,args);
            return *this;
        }
        /// returns a random number
        T getRandom(){
            return r.uniformR2!(T,boundCheck)(from,to);
        }
        /// initialize a
        U randomize(ref U a){
            return r.randomizeUniformR2!(U,T,T,boundCheck)(a,from,to);
        }
    }

    // ---------- gamma distribution, move to a separate module? --------
    /// gamma distribution f=x^(alpha-1)*exp(-x/theta)/(gamma(alpha)*theta^alpha)
    /// alpha has to be bigger than 1, for alpha<1 use gammaD(alpha)=gammaD(alpha+1)*pow(r.uniform!(T),1/alpha)
    /// from Marsaglia and Tsang, ACM Transaction on Mathematical Software, Vol. 26, N. 3
    /// 2000, p 363-372
    struct GammaDistribution(T){
        RandomG r;
        T alpha;
        T theta;
        static GammaDistribution create(RandomG r,T alpha=cast(T)1,T theta=cast(T)1){
            GammaDistribution res;
            res.r=r;
            res.alpha=alpha;
            res.theta=theta;
            assert(alpha>=cast(T)1,"implemented only for alpha>=1");
            return res;
        }
        /// chainable call style initialization of variables (thorugh a call to randomize)
        GammaDistribution opCall(U,S...)(ref U a,S args){
            randomize(a,args);
            return *this;
        }
        /// returns a single random number
        T getRandom(T a=alpha,T t=theta)
        in { assert(a>=cast(T)1,"implemented only for alpha>=1"); }
        body {
            T d=a-(cast(T)1)/(cast(T)3);
            T c=(cast(T)1)/sqrt(d*cast(T)9);
            auto n=r.normalSource!(T)();
            for (;;) {
                do {
                    T x=n.getRandom();
                    T v=c*x+cast(T)1;
                    v=v*v*v; // might underflow (in extreme situations) so it is in the loop
                } while (v<=0)
                T u=r.uniform!(T)();
                if (u<1-(cast(T)0.331)*(x*x)*(x*x)) return t*d*v;
                if (log(u)< x*x/2+d*(1-v+log(v))) return t*d*v;
            }
        }
        /// initializes b with gamma distribued random numbers
        U randomize(U)(ref U b,T a=alpha,T t=theta){
            static if (is(U S:S[])) {
                alias BaseTypeOfArrays!(U) T;
                T* bEnd=b.ptr+b.length;
                for (T* bPtr=b.ptr;bPtr!=bEnd;++bPtr){
                    *bPtr=cast(BaseTypeOfArrays!(U)) getRandom(a,t);
                }
            } else {
                b=cast(U) getRandom(a,t);
            }
            return b;
        }
        /// maps op on random numbers (of type T) and initializes b with it
        U randomizeOp(U,S)(S delegate(T)op, ref U b,T a=alpha, T t=theta){
            static if(is(U S:S[])){
                alias BaseTypeOfArrays!(U) T;
                T* bEnd=b.ptr+b.length;
                for (T* bPtr=b.ptr;bPtr!=bEnd;++bPtr){
                    *bPtr=cast(BaseTypeOfArrays!(U))op(getRandom(a,t));
                }
            } else {
                b=cast(U)op(getRandom(a));
            }
            return b;
        }
    }
    
    //-------- various distributions available -----------
    
    /// generators of normal numbers (sigma=1,mu=0) of the given type
    /// f=exp(-x*x/(2*sigma^2))/(sqrt(2 pi)*sigma)
    NormalSource!(RandomG,T) normalSource(T)(){
        static if(is(T==float)){
            if (!normalFloat) normalFloat=new NormalSource!(RandomG,T)(this);
            return normalFloat;
        } else static if (is(T==double)){
            if (!normalDouble) normalDouble=new NormalSource!(RandomG,T)(this);
            return normalDouble;
        } else static if (is(T==real)){
            if (!normalReal) normalReal=new NormalSource!(RandomG,T)(this);
            return normalReal;
        } else static assert(0,T.stringof~" no normal source implemented");
    }

    /// generators of exp distribued numbers (beta=1) of the given type
    /// f=1/beta*exp(-x/beta)
    ExpSource!(RandomG,T) expSource(T)(){
        static if(is(T==float)){
            if (!expFloat) expFloat=new ExpSource!(RandomG,T)(this);
            return expFloat;
        } else static if (is(T==double)){
            if (!expDouble) expDouble=new ExpSource!(RandomG,T)(this);
            return expDouble;
        } else static if (is(T==real)){
            if (!expReal) expReal=new ExpSource!(RandomG,T)(this);
            return expReal;
        } else static assert(0,T.stringof~" no exp source implemented");
    }
    
    /// generators of normal numbers with a different default sigma/mu
    /// f=exp(-x*x/(2*sigma^2))/(sqrt(2 pi)*sigma)
    NormalSource!(RandomG,T).NormalDistribution normalD(T)(T sigma=cast(T)1,T mu=cast(T)0){
        return normalSource!(T).normalD(sigma,mu);
    }
    /// exponential distribued numbers with a different default beta
    /// f=1/beta*exp(-x/beta)
    ExpSource!(RandomG,T).ExpDistribution expD(T)(T beta){
        return expSource!(T).expD(beta);
    }
    /// gamma distribued numbers with the given default alpha
    GammaDistribution!(T) gammaD(T)(T alpha=cast(T)1,T theta=cast(T)1){
        return GammaDistribution!(T).create(this,alpha,theta);
    }

    /// uniform distribution on the whole integer range, and on (0;1) for floats
    /// should return simply this??
    UniformDistribution!(T,true) uniformD(T)(){
        return UniformDistribution!(T,true).create(this);
    }
    /// uniform distribution on the whole integer range, and on [0;1] for floats
    UniformDistribution!(T,false) uniformBoundsD(T)(){
        return UniformDistribution!(T,false).create(this);
    }
    /// uniform distribution [0;to) for ints, (0:to) for reals
    UniformRDistribution!(T,true) uniformRD(T)(T to){
        return UniformRDistribution!(T,true).create(this,to);
    }
    /// uniform distribution [0;to) for ints, [0:to] for reals
    UniformRDistribution!(T,false) uniformRBoundsD(T)(T to){
        return UniformRDistribution!(T,false).create(this,to);
    }
    /// uniform distribution (-to;to) for ints and (-to;0)u(0;to) for reals
    UniformRSymmDistribution!(T,true,isFloat!(T)) uniformRSymmD(T)(T to){
        return UniformRSymmDistribution!(T,true,isFloat!(T)).create(this,to);
    }
    /// uniform distribution (-to;to) for ints and [-to;0)u(0;to] for reals
    UniformRSymmDistribution!(T,false,isFloat!(T)) uniformRSymmBoundsD(T)(T to){
        return UniformRSymmDistribution!(T,false,isFloat!(T)).create(this,to);
    }
    /// uniform distribution [from;to) fro ints and (from;to) for reals
    UniformR2Distribution!(T,true) uniformR2D(T)(T from, T to){
        return UniformR2Distribution!(T,true).create(this,from,to);
    }
    /// uniform distribution [from;to) for ints and [from;to] for reals
    UniformR2Distribution!(T,false) uniformR2BoundsD(T)(T from, T to){
        return UniformR2Distribution!(T,false).create(this,from,to);
    }
    
    // -------- Utility functions for other distributions -------
    // add also the corresponding randomize functions?
    
    /// returns a normal distribued number
    T normal(T)(){
        return normalSource!(T).getRandom();
    }
    /// returns a normal distribued number with the given sigma
    T normalSigma(T)(T sigma){
        return normalSource!(T).getRandom(sigma);
    }
    /// returns a normal distribued number with the given sigma and mu
    T normalSigmaMu(T)(T sigma,T mu){
        return normalSource!(T).getRandom(sigma,mu);
    }
    
    /// returns an exp distribued number
    T exp(T)(){
        return expSource!(T).getRandom();
    }
    /// returns an exp distribued number with the given scale beta
    T expBeta(T)(T beta){
        return expSource!(T).getRandom(beta);
    }
        
    /// returns a gamma distribued number
    /// from Marsaglia and Tsang, ACM Transaction on Mathematical Software, Vol. 26, N. 3
    /// 2000, p 363-372
    T gamma(T)(T alpha=cast(T)1,T sigma=cast(T)1)
    in { assert(alpha>=cast(T)1,"implemented only for alpha>=1"); }
    body {
        auto n=normalSource!(T);
        T d=alpha-(cast(T)1)/(cast(T)3);
        T c=(cast(T)1)/sqrt(d*cast(T)9);
        for (;;) {
            T x,v;
            do {
                x=n.getRandom();
                v=c*x+cast(T)1;
                v=v*v*v; // might underflow (in extreme situations) so it is in the loop
            } while (v<=0)
            T u=uniform!(T)();
            if (u<1-(cast(T)0.331)*(x*x)*(x*x)) return sigma*d*v;
            if (log(u)< x*x/2+d*(1-v+log(v))) return sigma*d*v;
        }
    }
    // ---------------
    
    /// writes the current status in a string
    char[] toString(){
        return source.toString();
    }
    /// reads the current status from a string (that should have been trimmed)
    /// returns the number of chars read
    size_t fromString(char[] s){
        return source.fromString(s);
    }
    
    // make this by default a uniformRandom number generator
    /// chainable call style initialization of variables (thorugh a call to randomize)
    RandomG opCall(U)(ref U a){
        randomize(a);
        return this;
    }
    /// returns a random number
    T getRandom(T)(){
        return uniform!(T,true)();
    }
    /// initialize el
    U randomize(U)(ref U a){
        return randomizeUniform!(U,true)(a);
    }
    
} // end class RandomG

/// make the default random number generator type
/// (a non threadsafe random number generator) easily available
/// you can safely expect a new instance of this to be indipendent from all the others
alias RandomG!() Random;
/// default threadsafe random number generator type
// TODO
//alias RandomG!(Sync!(DefaultEngine)) RandomSync;

/// shared locked (threadsafe) random number generator
/// initialized with urandom if available, with time otherwise
//static RandomSync rand;
static Random rand;
static this ()
{
    //rand = new RandomSync(false);
	rand = new Random();
    version(has_urandom){
        URandom r;
        rand.seed(&r.next);
    } else {
        ulong s;
        version (Posix){
            timeval tv;
            gettimeofday (&tv, null);
            s = tv.tv_usec;
        } else version (Win32) {
             QueryPerformanceCounter (&s);
         }
        else version(Escape) {
        	s = cpu_rdtsc();
        }
        uint[2] a;
        a[0]= cast(uint)(s & 0xFFFF_FFFFUL);
        a[1]= cast(uint)(s>>32);
        rand.seed(&(ArraySource(a).next));
    }
}

debug(UnitTest){
    import tango.math.random.engines.KISS;
    import tango.math.random.engines.CMWC;
    import tango.stdc.stdio:printf;
    import tango.io.Stdout;

    /// very simple statistal test, mean within maxOffset, and maximum/minimum at least minmax/maxmin
    bool checkMean(T)(T[] a, real maxmin, real minmax, real expectedMean, real maxOffset,bool alwaysPrint=false,bool checkB=false){
        T minV,maxV;
        real meanV=0.0L;
        if (a.length>0){
            minV=a[0];
            maxV=a[0];
            foreach (el;a){
                assert(!checkB || (cast(T)0<el && el<cast(T)1),"el out of bounds");
                if (el<minV) minV=el;
                if (el>maxV) maxV=el;
                meanV+=cast(real)el;
            }
            meanV/=cast(real)a.length;
            bool printM=false;
            if (cast(real)minV>maxmin) printM=true;
            if (cast(real)maxV<minmax) printM=true;
            if (expectedMean-maxOffset>meanV || meanV>expectedMean+maxOffset) printM=true;
            if (printM){
                version (GNU){
                    printf("WARNING tango.math.Random statistic is strange: %.*s[%d] %Lg %Lg %Lg\n\0",cast(int)T.stringof.length,T.stringof.ptr,a.length,cast(real)minV,meanV,cast(real)maxV);
                } else {
                    printf("WARNING tango.math.Random statistic is strange: %.*s[%d] %Lg %Lg %Lg\n\0",cast(int)T.stringof.length,T.stringof.ptr,a.length,cast(real)minV,meanV,cast(real)maxV);
                }
            } else if (alwaysPrint) {
                version (GNU){
                    printf("tango.math.Random statistic: %.*s[%d] %Lg %Lg %Lg\n\0",cast(int)T.stringof.length,T.stringof.ptr,a.length,cast(real)minV,meanV,cast(real)maxV);
                } else {
                    printf("tango.math.Random statistic: %.*s[%d] %Lg %Lg %Lg\n\0",cast(int)T.stringof.length,T.stringof.ptr,a.length,cast(real)minV,meanV,cast(real)maxV);
                }
            }
            return printM;
        }
    }
    
    /// check a given generator both on the whole array, and on each element separately
    bool doTests(RandG,Arrays...)(RandG r,real maxmin, real minmax, real expectedMean, real maxOffset,bool alwaysPrint,bool checkB, Arrays arrs){
        bool gFail=false;
        foreach (i,TA;Arrays){
            alias BaseTypeOfArrays!(TA) T;
            // all together
            r(arrs[i]);
            bool fail=checkMean!(T)(arrs[i],maxmin,minmax,expectedMean,maxOffset,alwaysPrint,checkB);
            // one by one
            foreach (ref el;arrs[i]){
                r(el);
            }
            fail |= checkMean!(T)(arrs[i],maxmin,minmax,expectedMean,maxOffset,alwaysPrint,checkB);
            gFail |= fail;
        }
        return gFail;
    }
    
    void testRandSource(RandS)(){
        auto r=new RandomG!(RandS)();
        // r.fromString("KISS99_b66dda10_49340130_8f3bf553_224b7afa_00000000_00000000"); // to reproduce a given test...
        char[] initialState=r.toString(); // so that you can reproduce things...
        bool allStats=false; // set this to true to show all statistics (helpful to track an error)
        try{
            r.uniform!(uint);
            r.uniform!(ubyte);
            r.uniform!(ulong);
            int count=10_000;
            for (int i=count;i!=0;--i){
                float f=r.uniform!(float);
                assert(0<f && f<1,"float out of bounds");
                double d=r.uniform!(double);
                assert(0<d && d<1,"double out of bounds");
                real rr=r.uniform!(real);
                assert(0<rr && rr<1,"double out of bounds");
            }
            // checkpoint status (str)
            char[] status=r.toString();
            uint tVal=r.uniform!(uint);
            ubyte t2Val=r.uniform!(ubyte);
            ulong t3Val=r.uniform!(ulong);

            byte[1000]  barr;
            ubyte[1000] ubarr;
            uint[1000]  uiarr;
            int[1000]   iarr;
            float[1000] farr;
            double[1000]darr;
            real[1000]  rarr;
            byte[]  barr2=barr[];
            ubyte[] ubarr2=ubarr[];
            uint[]  uiarr2=uiarr[];
            int[]   iarr2=iarr[];
            float[] farr2=farr[];
            double[]darr2=darr[];
            real[]  rarr2=rarr[];
            
            bool fail=false,gFail=false;
            if (allStats) Stdout("Uniform").newline;
            fail =doTests(r,-100.0L,100.0L,0.0L,20.0L,allStats,false,barr2);
            fail|=doTests(r,100.0L,155.0L,127.5L,20.0L,allStats,false,ubarr2);
            fail|=doTests(r,0.25L*cast(real)(uint.max),0.75L*cast(real)uint.max,
                0.5L*uint.max,0.2L*uint.max,allStats,false,uiarr2);
            fail|=doTests(r,0.5L*cast(real)int.min,0.5L*cast(real)int.max,
                0.0L,0.2L*uint.max,allStats,false,iarr2);
            fail|=doTests(r,0.2L,0.8L,0.5L,0.2L,allStats,true,farr2,darr2,rarr2);
            gFail|=fail;
            if (fail) Stdout("... with Uniform distribution");

            if (allStats) Stdout("UniformD").newline;
            fail =doTests(r.uniformD!(int)(),-100.0L,100.0L,0.0L,20.0L,allStats,false,barr2);
            fail|=doTests(r.uniformD!(int)(),100.0L,155.0L,127.5L,20.0L,allStats,false,ubarr2);
            fail|=doTests(r.uniformD!(int)(),0.25L*cast(real)(uint.max),0.75L*cast(real)uint.max,
                0.5L*uint.max,0.2L*uint.max,allStats,false,uiarr2);
            fail|=doTests(r.uniformD!(real)(),0.5L*cast(real)int.min,0.5L*cast(real)int.max,
                0.0L,0.2L*uint.max,allStats,false,iarr2);
            fail|=doTests(r.uniformD!(real)(),0.2L,0.8L,0.5L,0.2L,allStats,true,farr2,darr2,rarr2);
            gFail|=fail;
            if (fail) Stdout("... with UniformD distribution");

            if (allStats) Stdout("UniformBoundsD").newline;
            fail =doTests(r.uniformBoundsD!(int)(),-100.0L,100.0L,0.0L,20.0L,allStats,false,barr2);
            fail|=doTests(r.uniformBoundsD!(int)(),100.0L,155.0L,127.5L,20.0L,allStats,false,ubarr2);
            fail|=doTests(r.uniformBoundsD!(int)(),0.25L*cast(real)(uint.max),0.75L*cast(real)uint.max,
                0.5L*uint.max,0.2L*uint.max,allStats,false,uiarr2);
            fail|=doTests(r.uniformBoundsD!(int)(),0.5L*cast(real)int.min,0.5L*cast(real)int.max,
                0.0L,0.2L*uint.max,allStats,false,iarr2);
            fail|=doTests(r.uniformBoundsD!(int)(),0.2L,0.8L,0.5L,0.2L,allStats,false,farr2,darr2,rarr2);
            gFail|=fail;
            if (fail) Stdout("... with UniformBoundsD distribution");

            if (allStats) Stdout("UniformRD").newline;
            fail =doTests(r.uniformRD(cast(byte)101),25.0L,75.0L,50.0L,15.0L,allStats,false,barr2);
            fail|=doTests(r.uniformRD(cast(ubyte)201),50.0L,150.0L,100.0L,20.0L,allStats,false,ubarr2);
            fail|=doTests(r.uniformRD(1001u),250.0L,750.0L,500.0L,100.0L,allStats,false,uiarr2);
            fail|=doTests(r.uniformRD(1001 ),250.0L,750.0L,500.0L,100.0L,allStats,false,iarr2);
            fail|=doTests(r.uniformRD(1000.0L),250.0L,750.0L,500.0L,100.0L,
                allStats,false,farr2,darr2,rarr2);
            fail|=doTests(r.uniformRD(1.0L),0.2L,0.8L,0.5L,0.2L,allStats,true,farr2,darr2,rarr2);
            gFail|=fail;
            if (fail) Stdout("... with uniformRD distribution");
        
            if (allStats) Stdout("UniformRBoundsD").newline;
            fail =doTests(r.uniformRBoundsD(cast(byte)101),25.0L,75.0L,50.0L,15.0L,allStats,false,barr2);
            fail|=doTests(r.uniformRBoundsD(cast(ubyte)201),50.0L,150.0L,100.0L,20.0L,allStats,false,ubarr2);
            fail|=doTests(r.uniformRBoundsD(1001u),250.0L,750.0L,500.0L,100.0L,allStats,false,uiarr2);
            fail|=doTests(r.uniformRBoundsD(1001 ),250.0L,750.0L,500.0L,100.0L,allStats,false,iarr2);
            fail|=doTests(r.uniformRBoundsD(1000.0L),250.0L,750.0L,500.0L,100.0L,
                allStats,false,farr2,darr2,rarr2);
            gFail|=fail;
            if (fail) Stdout("... with uniformRBoundsD distribution");
        
            if (allStats) Stdout("Rsymm").newline;
            fail =doTests(r.uniformRSymmD!(byte)(cast(byte)100),
                -40.0L,40.0L,0.0L,30.0L,allStats,false,barr2);
            fail|=doTests(r.uniformRSymmD!(int)(1000),
                -300.0L,300.0L,0.0L,200.0L,allStats,false,iarr2);
            fail|=doTests(r.uniformRSymmD!(real)(1.0L),
                -0.3L,0.3L,0.0L,0.3L,allStats,false,farr2,darr2,rarr2);
            gFail|=fail;
            if (fail) Stdout("... with Rsymm distribution");

            if (allStats) Stdout("RsymmBounds").newline;
            fail =doTests(r.uniformRSymmBoundsD!(byte)(cast(byte)100),
                -40.0L,40.0L,0.0L,30.0L,allStats,false,barr2);
            fail|=doTests(r.uniformRSymmBoundsD!(int)(1000),
                -300.0L,300.0L,0.0L,200.0L,allStats,false,iarr2);
            fail|=doTests(r.uniformRSymmBoundsD!(real)(1.0L),
                -0.3L,0.3L,0.0L,0.3L,allStats,false,farr2,darr2,rarr2);
            gFail|=fail;
            if (fail) Stdout("... with RsymmBounds distribution");
        
            if (allStats) Stdout("Norm").newline;
            fail =doTests(r.normalSource!(float)(),-0.5L,0.5L,0.0L,1.0L,
                allStats,false,farr2,darr2,rarr2);
            fail|=doTests(r.normalSource!(double)(),-0.5L,0.5L,0.0L,1.0L,
                allStats,false,farr2,darr2,rarr2);
            fail|=doTests(r.normalSource!(real)(),-0.5L,0.5L,0.0L,1.0L,
                allStats,false,farr2,darr2,rarr2);
            fail|=doTests(r.normalD!(real)(0.5L,5.0L),4.5L,5.5L,5.0L,0.5L,
                allStats,false,farr2,darr2,rarr2);
            gFail|=fail;
            if (fail) Stdout("... with Normal distribution");
        
            if (allStats) Stdout("Exp").newline;
            fail =doTests(r.expSource!(float)(),0.8L,2.0L,1.0L,1.0L,
                allStats,false,farr2,darr2,rarr2);
            fail|=doTests(r.expSource!(double)(),0.8L,2.0L,1.0L,1.0L,
                allStats,false,farr2,darr2,rarr2);
            fail|=doTests(r.expSource!(real)(),0.8L,2.0L,1.0L,1.0L,
                allStats,false,farr2,darr2,rarr2);
            fail|=doTests(r.expD!(real)(5.0L),1.0L,7.0L,5.0L,1.0L,
                allStats,false,farr2,darr2,rarr2);
            gFail|=fail;
            if (fail) Stdout("... with Exp distribution");
        
            r.fromString(status);
            assert(r.uniform!(uint)==tVal,"restoring of status from str failed");
            assert(r.uniform!(ubyte)==t2Val,"restoring of status from str failed");
            assert(r.uniform!(ulong)==t3Val,"restoring of status from str failed");
            assert(!gFail,"Random.d failure");
        } catch(Exception e) {
            Stdout(initialState).newline;
            throw e;
        }
    }

    unittest {
        testRandSource!(Kiss99)();
        testRandSource!(CMWC_default)();
        testRandSource!(KissCmwc_default)();
        testRandSource!(Twister)();
        testRandSource!(DefaultEngine)();
        testRandSource!(Sync!(DefaultEngine))();
    }

}
