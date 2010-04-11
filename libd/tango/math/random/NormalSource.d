/*******************************************************************************
        copyright:      Copyright (c) 2008. Fawzi Mohamed
        license:        BSD style: $(LICENSE)
        version:        Initial release: July 2008
        author:         Fawzi Mohamed
*******************************************************************************/
module tango.math.random.NormalSource;
private import Integer = tango.text.convert.Integer;
import tango.math.Math:exp,sqrt,log,PI;
import tango.math.ErrorFunction:erfc;
import tango.math.random.Ziggurat;
import tango.core.Traits: isRealType;

/// class that returns gaussian (normal) distributed numbers (f=exp(-0.5*x*x)/sqrt(2*pi))
class NormalSource(RandG,T){
    static assert(isRealType!(T),T.stringof~" not acceptable, only floating point variables supported");
    /// probability distribution (non normalized, should be divided by sqrt(2*PI))
    static real probDensityF(real x){ return exp(-0.5L*x*x); }
    /// inverse probability distribution
    static real invProbDensityF(real x){ return sqrt(-2.0L*log(x)); }
    /// complement of the cumulative density distribution (integral x..infinity probDensityF)
    static real cumProbDensityFCompl(real x){ return sqrt(0.5L*PI)*erfc(x/sqrt(2.0L));/*normalDistributionCompl(x);*/ }
    /// tail for normal distribution
    static T tailGenerator(RandG r, T dMin, int iNegative) 
    { 
        T x, y; 
        do 
        {
            x = -log(r.uniform!(T)) / dMin;
            y = -log(r.uniform!(T)); 
        } while (y+y < x * x); 
        return (iNegative ?(-x - dMin):(dMin + x)); 
    }
    alias Ziggurat!(RandG,T,probDensityF,tailGenerator,true) SourceT;
    /// internal source of normal numbers
    SourceT source;
    /// initializes the probability distribution
    this(RandG r){
        source=SourceT.create!(invProbDensityF,cumProbDensityFCompl)(r,0xe.9dda4104d699791p-2L);
    }
    /// chainable call style initialization of variables (thorugh a call to randomize)
    NormalSource opCall(U,S...)(ref U a,S args){
        randomize(a,args);
        return this;
    }
    /// returns a normal distribued number
    final T getRandom(){
        return source.getRandom();
    }
    /// returns a normal distribued number with the given sigma (standard deviation)
    final T getRandom(T sigma){
        return sigma*source.getRandom();
    }
    /// returns a normal distribued number with the given sigma (standard deviation) and mu (average)
    final T getRandom(T sigma, T mu){
        return mu+sigma*source.getRandom();
    }
    /// initializes a variable with normal distribued number and returns it
    U randomize(U)(ref U a){
        return source.randomize(a);
    }
    /// initializes a variable with normal distribued number with the given sigma and returns it
    U randomize(U,V)(ref U a,V sigma){
        return source.randomizeOp((T x){ return x*cast(T)sigma; },a);
    }
    /// initializes a variable with normal distribued numbers with the given sigma and mu and returns it
    U randomize(U,V,S)(ref U el,V sigma, S mu){
        return source.randomizeOp((T x){ return x*cast(T)sigma+cast(T)mu; },a);
    }
    /// initializes the variable with the result of mapping op on the random numbers (of type T)
    U randomizeOp(U,S)(S delegate(T) op,ref U a){
        return source.randomizeOp(op,a);
    }
    /// normal distribution with different default sigma and mu
    /// f=exp(-x*x/(2*sigma^2))/(sqrt(2 pi)*sigma)
    struct NormalDistribution{
        T sigma,mu;
        NormalSource source;
        /// constructor
        static NormalDistribution create(NormalSource source,T sigma,T mu){
            NormalDistribution res;
            res.source=source;
            res.sigma=sigma;
            res.mu=mu;
            return res;
        }
        /// chainable call style initialization of variables (thorugh a call to randomize)
        NormalDistribution opCall(U,S...)(ref U a,S args){
            randomize(a,args);
            return *this;
        }
        /// returns a single number
        T getRandom(){
            return mu+sigma*source.getRandom();
        }
        /// initialize a
        U randomize(U)(ref U a){
            T op(T x){return mu+sigma*x; }
            return source.randomizeOp(&op,a);
        }
        /// initialize a (let s and m have different types??)
        U randomize(U,V)(ref U a,V s){
            T op(T x){return mu+(cast(T)s)*x; }
            return source.randomizeOp(&op,a);
        }
        /// initialize a (let s and m have different types??)
        U randomize(U,V,S)(ref U a,V s, S m){
            T op(T x){return (cast(T)m)+(cast(T)s)*x; }
            return source.randomizeOp(&op,a);
        }
    }
    /// returns a normal distribution with a non-default sigma/mu
    NormalDistribution normalD(T sigma=cast(T)1,T mu=cast(T)0){
        return NormalDistribution.create(this,sigma,mu);
    }
}
