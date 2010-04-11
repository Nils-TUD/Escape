/**
 * Cylindrical Bessel functions of integral order.
 *
 * Copyright: Based on the CEPHES math library, which is
 *            Copyright (C) 1994 Stephen L. Moshier (moshier@world.std.com).
 * License:   BSD style: $(LICENSE)
 * Authors:   Stephen L. Moshier (original C code). Conversion to D by Don Clugston
 */

module tango.math.Bessel;

import tango.math.Core;
private import tango.math.IEEE;

private {
/*
sqrt(j0^2(1/x^2) + y0^2(1/x^2)) = z P(z**2)/Q(z**2)
z(x) = 1/sqrt(x)
Relative error
n=7, d=7
Peak error =  1.80e-20
Relative error spread =  5.1e-2
*/
const real j0modulusn[] = [

   0x1.154700ea96e79656p-7, // 0.0084618334268988678395
   0x1.72244b6e998cd6fp-4,  // 0.090366644531602000525
   0x1.6ebccf42e9c19fd2p-1, // 0.71628425304232057209
   0x1.6bd844e89cbd639ap+1, // 2.8425375114252161456
   0x1.e812b377c75ebc96p+2, // 7.6261414212908496305
   0x1.46d69ca24ce76686p+3, // 10.213697735779743439
   0x1.b756f7234cc67146p+2, // 6.8646829457021346239
   0x1.943a7471eaa50ab2p-2  // 0.39475423760692244614
];

const real j0modulusd[] = [
   0x1.5b84007c37011506p-7, // 0.010605335461541217703
   0x1.cfe76758639bdab4p-4, // 0.11325779313322123049
   0x1.cbfa09bf71bafc7ep-1, // 0.8983920141407590632
   0x1.c8eafb3836f2eeb4p+1, // 3.5696710609899109018
   0x1.339db78060eb706ep+3, // 9.6130025393862137883
   0x1.a06530916be8bc7ap+3, // 13.012352260614782615
   0x1.23bfe7f67a54893p+3,  // 9.117176038171821116
   0x1p+0   // 1
];

/*
atan(y0(x)/j0(x)) = x - pi/4 + x P(x**2)/Q(x**2)
Absolute error
n=5, d=6
Peak error =  2.80e-21
Relative error spread =  5.5e-1
*/

const real j0phasen[] = [
   -0x1.ccbaf3865bb0985ep-22,   // -4.2908850907731129636e-07
   -0x1.3a6b175e64bdb82ep-14,   // -7.4963170368299641512e-05
   -0x1.06124b5310cdca28p-8,    // -0.0039988931558269906429
   -0x1.3cebb7ab09cf1b14p-4,    // -0.077373235181415168822
   -0x1.00156ed209b43c6p-1, // -0.50016351999224936946
   -0x1.78aa9ba4254ca20cp-1     // -0.73567663553935715194
];

const real j0phased[] = [
   0x1.ccbaf3865bb09918p-19,    // 3.4327080726184903899e-06
   0x1.3b5b0e12900d58b8p-11,    // 0.00060149323173421904041
   0x1.0897373ff9906f7ep-5, // 0.032298667821850250481
   0x1.450a5b8c552ade4ap-1, // 0.63484464729352451028
   0x1.123e263e7f0e96d2p+2, // 4.2850432977977361182
   0x1.d82ecca5654be7d2p+2, // 7.3778564086143760725
   0x1p+0   // 1
];

}

/***
 *  Bessel function of order zero
 *
 * Returns Bessel function of first kind, order zero of the argument.
 */

 /* The domain is divided into the intervals [0, 9] and
 * (9, infinity). In the first interval the rational approximation
 * is (x^2 - r^2) (x^2 - s^2) (x^2 - t^2) P7(x^2) / Q8(x^2),
 * where r, s, t are the first three zeros of the function.
 * In the second interval the expansion is in terms of the
 * modulus M0(x) = sqrt(J0(x)^2 + Y0(x)^2) and phase  P0(x)
 * = atan(Y0(x)/J0(x)).  M0 is approximated by sqrt(1/x)P7(1/x)/Q7(1/x).
 * The approximation to J0 is M0 * cos(x -  pi/4 + 1/x P5(1/x^2)/Q6(1/x^2)).
 */
real cylBessel_j0(real x)
{

/*
j0(x) = (x^2-JZ1)(x^2-JZ2)(x^2-JZ3)P(x**2)/Q(x**2)
0 <= x <= 9
Relative error
n=7, d=8
Peak error =  8.49e-22
Relative error spread =  2.2e-3
*/

const real j0n[] = [
   -0x1.3e8ff72b890d72d8p+59,   // -717338674752678806.75
   0x1.cc86e3755a4c803p+53, // 16203350096431504.023
   -0x1.0ea6f5bac6623616p+47,   // -148792613364529.10563
   0x1.532c6d94d36f2874p+39,    // 728369646185.71710555
   -0x1.ef25a232f6c00118p+30,   // -2076797068.7409668133
   0x1.aa0690536c11fc2p+21, // 3490002.0407334713999
   -0x1.94e67651cc57535p+11,    // -3239.201943301299801
   0x1.4bfe47ac8411eeb2p+0  // 1.2968487545186417705
];

const real j0d[] = [
   0x1.0096dec5f6560158p+73,    // 9466475654163919450000
   0x1.11705db14995fb9cp+66,    // 78813405543084322411
   0x1.220a41c3daaa7a58p+58,    // 326556083284519401.36
   0x1.93c6b48d196c1082p+49,    // 887913237328600.12894
   0x1.9814684a10dbfda2p+40,    // 1752689035792.8593389
   0x1.36f20ec527fccda4p+31,    // 2608400226.5781006111
   0x1.634596b9247fc34p+21, // 2910386.8404016477069
   0x1.1d3eb73f90657bfcp+11,    // 2281.9598691768877638
   0x1p+0   // 1
];



    real xx, y, z, modulus, phase;

    xx = x * x;
    if ( xx < 81.0L ) {
        const real [] JZ = [5.783185962946784521176L, 30.47126234366208639908L, 7.488700679069518344489e1L];
        y = (xx - JZ[0]) * (xx - JZ[1]) * (xx - JZ[2]);
        y *= poly( xx, j0n) / poly( xx, j0d);
        return y;
    }

    y = fabs(x);
    xx = 1.0/xx;
    phase = poly( xx, j0phasen) / poly( xx, j0phased);

    z = 1.0/y;
    modulus = poly( z, j0modulusn) / poly( z, j0modulusd);

    y = modulus * cos( y -  PI_4 + z*phase) / sqrt(y);
    return y;
}

/**
 * Bessel function of the second kind, order zero
 * Also known as the cylindrical Neumann function, order zero.
 *
 * Returns Bessel function of the second kind, of order
 * zero, of the argument.
 */

/* The domain is divided into the intervals [0, 5>, [5,9> and
 * [9, infinity). In the first interval a rational approximation
 * R(x) is employed to compute y0(x)  = R(x) + 2/pi * log(x) * j0(x).
 *
 * In the second interval, the approximation is
 *     (x - p)(x - q)(x - r)(x - s)P7(x)/Q7(x)
 * where p, q, r, s are zeros of y0(x).
 *
 * The third interval uses the same approximations to modulus
 * and phase as j0(x), whence y0(x) = modulus * sin(phase).
 */
real cylBessel_y0(real x)
{


/*
y0(x) = 2/pi * log(x) * j0(x) + P(z**2)/Q(z**2)
0 <= x <= 5
Absolute error
n=7, d=7
Peak error =  8.55e-22
Relative error spread =  2.7e-1
*/


const real y0n[] = [
   -0x1.068026b402e2bf7ap+54,   // -18471836903848111.869
   0x1.3a2f7be8c4c8a03ep+55,    // 44217675959919696.118
   -0x1.89928488d6524792p+51,   // -3461898868011666.2367
   0x1.3e3ea2846f756432p+46,    // 87478428048349.347852
   -0x1.c8be8d9366867c78p+39,   // -980851018163.26266841
   0x1.43879530964e5fbap+32,    // 5427926320.5871333911
   -0x1.bee052fef72a5d8p+23,    // -14643241.497979473031
   0x1.e688c8fe417c24d8p+13     // 15569.098141204453537
];

const real y0d[] = [
   0x1.bc96c5351e564834p+57,    // 250281326806871184.4
   0x1.6821ac3b4c5209a6p+51,    // 3167750475899536.3015
   0x1.27098b571836ce64p+44,    // 20274807665027.425387
   0x1.41870d2a9b90aa76p+36,    // 86309393065.722818817
   0x1.00394fd321f52f48p+28,    // 268670205.19579046695
   0x1.317ce3b16d65b27p+19, // 625639.1154086099882
   0x1.0432b36efe4b20aep+10,    // 1040.7922017558416978
   0x1p+0   // 1
];


/*
y0(x) = (x-Y0Z1)(x-Y0Z2)(x-Y0Z3)(x-Y0Z4)P(x)/Q(x)
4.5 <= x <= 9
Absolute error
n=9, d=9
Peak error =  2.35e-20
Relative error spread =  7.8e-13
*/

const real y059n[] = [
   -0x1.0fce17d26a21f218p+19,   // -556656.74443537359254
   -0x1.c6fc144765fdfaa8p+16,   // -116476.07921445322667
   0x1.3e20237c53c7180ep+19,    // 651521.10892666707554
   0x1.7d14055ff6a493c4p+17,    // 195112.04199107204434
   0x1.b8b694729689d1f4p+12,    // 7051.4112420921711619
   -0x1.1e24596784b6c5cp+12,    // -4578.2718272384775987
   0x1.35189cb3ece7ab46p+6, // 77.274035273870974615
   0x1.9428b3f406b4aa08p+4, // 25.259937241771050606
   -0x1.791187b68dd4240ep+0,    // -1.4729237385452767513
   0x1.8417216d568b325ep-6  // 0.023687155383733848699
];

const real y059d[] = [
   0x1.17af71a3d4167676p+30,    // 1173085288.9571169385
   0x1.a36abbb668c79d6cp+31,    // 3518324187.2046479411
   -0x1.4ff64a14ed73c4d6p+29,   // -704563522.61594346786
   0x1.9d427af195244ffep+26,    // 108333547.77472789584
   -0x1.4e85bbbc8d2fd914p+23,   // -10961629.86826467061
   0x1.ac59b523ae0bd16cp+19,    // 877261.66060545261587
   -0x1.8ebda33eaac74518p+15,   // -51038.818837487053814
   0x1.16194a051cd55a12p+11,    // 2224.7902856410171942
   -0x1.f2d714ab48d1bd7ep+5,    // -62.355019891891258817
   0x1p+0   // 1
];


    real xx, y, z, modulus, phase;

    if ( x < 0.0 ) return -real.max;
    xx = x * x;
    if ( xx < 81.0L ) {
        if ( xx < 20.25L ) {
            y = M_2_PI * log(x) * cylBessel_j0(x);
            y += poly( xx, y0n) / poly( xx, y0d);
        } else {
            const real [] Y0Z = [3.957678419314857868376e0L, 7.086051060301772697624e0L,
                1.022234504349641701900e1L, 1.336109747387276347827e1L];
            y = (x - Y0Z[0])*(x - Y0Z[1])*(x - Y0Z[2])*(x - Y0Z[3]);
            y *= poly( x, y059n) / poly( x, y059d);
        }
        return y;
    }

    y = fabs(x);
    xx = 1.0/xx;
    phase = poly( xx, j0phasen) / poly( xx, j0phased);

    z = 1.0/y;
    modulus = poly( z, j0modulusn) / poly( z, j0modulusd);

    y = modulus * sin( y -  PI_4 + z*phase) / sqrt(y);
    return y;
}

debug(UnitTest) {

unittest {
  // argument, result1, result2, derivative. Correct result is result1+result2.
const real [4][] j0_test_points = [
    [8.0L, 1.71646118164062500000E-1L, 4.68897349140609086941E-6L, -2.34636346853914624381E-1L],
    [4.54541015625L, -3.09783935546875000000E-1L, 7.07472668157686463367E-6L, 2.42993657373627558460E-1L],
    [2.85711669921875L, -2.07901000976562500000E-1L, 1.15237285263902751582E-5L, -3.90402225324501311651E-1L],
    [2.0L, 2.23876953125000000000E-1L, 1.38260162356680518275E-5L, -5.76724807756873387202E-1L],
    [1.16415321826934814453125e-10L, 9.99984741210937500000E-1L, 1.52587890624999966119E-5L,
        9.99999999999999999997E-1L],
    [-2.0L, 2.23876953125000000000E-1L,
        1.38260162356680518275E-5L, 5.76724807756873387202E-1L]
];

const real [4][] y0_test_points = [
    [ 8.0L, 2.23510742187500000000E-1L, 1.07472000662205273234E-5L, 1.58060461731247494256E-1L],
    [4.54541015625L, -2.08114624023437500000E-1L, 1.45018823856668874574E-5L, -2.88887645307401250876E-1L],
    [2.85711669921875L, 4.20303344726562500000E-1L, 1.32781607563122276008E-5L, -2.82488638474982469213E-1],
    [2.0L, 5.10360717773437500000E-1L, 1.49548763076195966066E-5L, 1.07032431540937546888E-1L],
    [1.16415321826934814453125e-10L, -1.46357574462890625000E1L, 3.54110537011061127637E-6L,
        5.46852220461145271913E9L]
];

const real [4][] j1_test_points = [
  [ 8.0L, 2.34634399414062500000E-1L, 1.94743985212438127665E-6L,1.42321263780814578043E-1],
  [4.54541015625L, -2.42996215820312500000E-1L, 2.55844668494153980076E-6L, -2.56317734136211337012E-1],
  [2.85711669921875L, 3.90396118164062500000E-1L, 6.10716043881165077013E-6L, -3.44531507106757980441E-1L],
  [2.0L, 5.76721191406250000000E-1L, 3.61635062338720244824E-6L,  -6.44716247372010255494E-2L],
  [1.16415321826934814453125e-10L, 5.820677273504770710133016109466552734375e-11L,
   8.881784197001251337312921818461805735896e-16L, 4.99999999999999999997E-1L],
  [-2.0L, -5.76721191406250000000E-1L, -3.61635062338720244824E-6L, -6.44716247372010255494E-2L]
];

const real [4][] y1_test_points = [
    [8.0L, -1.58065795898437500000E-1L,
        5.33416719000574444473E-6L, 2.43279047103972157309E-1L],
    [4.54541015625L, 2.88879394531250000000E-1L,
        8.25077615125087585195E-6L, -2.71656024771791736625E-1L],
    [2.85711669921875L, 2.82485961914062500000E-1,
        2.67656091996921314433E-6L, 3.21444694221532719737E-1],
    [2.0L, -1.07040405273437500000E-1L,
        7.97373249995311162923E-6L, 5.63891888420213893041E-1],
    [1.16415321826934814453125e-10L, -5.46852220500000000000E9L,
        3.88547280871200700671E-1L, 4.69742480525120196168E19L]
];

    foreach(real [4] t; j0_test_points) {
        assert(feqrel(cylBessel_j0(t[0]), t[1]+t[2]) >=real.mant_dig-3);
    }

    foreach(real [4] t; y0_test_points) {
        assert(feqrel(cylBessel_y0(t[0]), t[1]+t[2]) >=real.mant_dig-4);
    }
    foreach(real [4] t; j1_test_points) {
        assert(feqrel(cylBessel_j1(t[0]), t[1]+t[2]) >=real.mant_dig-3);
    }

    foreach(real [4] t; y1_test_points) {
        assert(feqrel(cylBessel_y1(t[0]), t[1]+t[2]) >=real.mant_dig-4);
    }

    // Values from MS Excel, of doubtful accuracy.
    assert(fabs(-0.060_409_940_421_649 - cylBessel_j0(173.5)) < 0.000_000_000_1);
    assert(fabs(-0.044_733_447_576_5866 - cylBessel_y0(313.25)) < 0.000_000_000_1);
    assert(fabs(0.00391280088318945 - cylBessel_j1(123.25)) < 0.000_000_000_1);
    assert(fabs(-0.0648628570878951 - cylBessel_j1(-91)) < 0.000_000_000_1);
    assert(fabs(-0.0759578537652805 - cylBessel_y1(107.75)) < 0.000_000_000_1);
}
}

private {

/*
j1(x) = (x^2-r0^2)(x^2-r1^2)(x^2-r2^2) x P(x**2)/Q(x**2)
0 <= x <= 9
Relative error
n=8, d=8
Peak error =  2e-21
*/
const real j1n[] = [
   -0x1.2f494fa4e623b1bp+58,    // -341470097444474566.73
   0x1.8289f0a5f1e1a784p+52,    // 6800062979972634.4695
   -0x1.9d773ee29a52c6d8p+45,   // -56826307302218.347096
   0x1.e9394ff57a46071cp+37,    // 262650068655.28419323
   -0x1.616c7939904a359p+29,    // -741183271.19545404286
   0x1.424414b9ee5671eap+20,    // 1320001.2953933121449
   -0x1.6db34a9892d653e6p+10,   // -1462.801427977939339
   0x1.dcd7412d90a0db86p-1, // 0.93132976227963279129
   -0x1.1444a1643199ee5ep-12    // -0.0002634697796221277629
];

const real j1d[] = [
   0x1.5a1e0a45eb67bacep+75,    // 51077904551614157846000
   0x1.35ee485d62f0ccbap+68,    // 357325874689695599490
   0x1.11ee7aad4e4bcd8p+60, // 1233678068848311511.9
   0x1.3adde5dead800244p+51,    // 2769597563759616.0707
   0x1.041c413dfbab693p+42, // 4468662138862.6782953
   0x1.4066d12193fcc082p+32,    // 5375447329.5780754392
   0x1.24309d0dc2c4d42ep+22,    // 4787239.2634382967477
   0x1.7115bea028dd75f2p+11,    // 2952.6795197294374573
   0x1p+0   // 1
];

/*
sqrt(j1^2(1/x^2) + y1^2(1/x^2)) = z P(z**2)/Q(z**2)
z(x) = 1/sqrt(x)
Relative error
n=7, d=8
Peak error =  1.35e=20
Relative error spread =  9.9e0
*/
const real [] j1modulusn = [
   0x1.059262020bf7520ap-6, // 0.015965076170857146501
   0x1.012ffc4d1f5cdbc8p-3, // 0.1255798064266130869
   0x1.03c17ce18cae596p+0,  // 1.0146711397798581413
   0x1.6e0414a7114ae3ccp+1, // 2.8594995322951809401
   0x1.cb047410d229cbc4p+2, // 7.1721468128459064808
   0x1.4385d04bb718faaap+1, // 2.527521168680500659
   0x1.914074c30c746222p-2, // 0.39184744301302421771
   -0x1.42abe77f6b307aa6p+2     // -5.0417422050784420988
];

const real [] j1modulusd = [
   0x1.47d4e6ad98d8246ep-6, // 0.020009255668254074661
   0x1.42562f48058ff904p-3, // 0.15739094675581809421
   0x1.44985e2af35c6f9cp+0, // 1.2679499487743315312
   0x1.c6f4a03469c4ef6cp+1, // 3.5543403869556082614
   0x1.1829a060e8d604cp+3,  // 8.7550813572658517658
   0x1.44111c892f9cc84p+1,  // 2.5317722005704352897
   -0x1.d7c36d7f1e5aef6ap-1,    // -0.9214128701852838347
   -0x1.8eeafb1ac81c4c06p+2,    // -6.2330920945682393176
   0x1p+0   // 1
];
/*
atan(y1(x)/j1(x))  =  x - 3pi/4 + z P(z**2)/Q(z**2)
z(x) = 1/x
Absolute error
n=5, d=6
Peak error =  4.83e-21
Relative error spread =  1.9e0
*/
const real [] j1phasen = [
   0x1.ca9f612d83aaa818p-20,    // 1.7085022351347062848e-06
   0x1.2e82fcfb7d0fee9ep-12,    // 0.00028849761267159262584
   0x1.e28858c1e947506p-7,  // 0.01472572645054468815
   0x1.12b8f96e5173d20ep-2, // 0.26828374610737510555
   0x1.965e6a013154c0ap+0,  // 1.5873781445419181766
   0x1.0156a25eaa0dd78p+1   // 2.0104563677051447839
];

const real [] j1phased = [
   0x1.31bf961e57c71ae4p-18,    // 4.5560059603592167679e-06
   0x1.9464d8f2abf750a6p-11,    // 0.00077132021973190404398
   0x1.446a786bac2131fp-5,  // 0.039601550289607123098
   0x1.76caa8513919873cp-1, // 0.73201490394108064711
   0x1.2130b56bc1a563e4p+2, // 4.5185979416188131125
   0x1.b3cc1a865259dfc6p+2, // 6.8093324958548730892
   0x1p+0   // 1
];

}

/**
 *  Bessel function of order one
 *
 * Returns Bessel function of order one of the argument.
 */
/* The domain is divided into the intervals [0, 9] and
 * (9, infinity). In the first interval the rational approximation
 * is (x^2 - r^2) (x^2 - s^2) (x^2 - t^2) x P8(x^2) / Q8(x^2),
 * where r, s, t are the first three zeros of the function.
 * In the second interval the expansion is in terms of the
 * modulus M1(x) = sqrt(J1(x)^2 + Y1(x)^2) and phase  P1(x)
 * = atan(Y1(x)/J1(x)).  M1 is approximated by sqrt(1/x)P7(1/x)/Q8(1/x).
 * The approximation to j1 is M1 * cos(x -  3 pi/4 + 1/x P5(1/x^2)/Q6(1/x^2)).
 */
real cylBessel_j1(real x)
{
    real xx, y, z, modulus, phase;

    xx = x * x;
    if ( xx < 81.0L ) {
        const real [] JZ = [1.46819706421238932572e1L,
            4.92184563216946036703e1L, 1.03499453895136580332e2L];
        y = (xx - JZ[0]) * (xx - JZ[1]) * (xx - JZ[2]);
        y *= x * poly( xx, j1n) / poly( xx, j1d);
        return y;
    }
    y = fabs(x);
    xx = 1.0/xx;
    phase = poly( xx, j1phasen) / poly( xx, j1phased);
    z = 1.0/y;
    modulus = poly( z, j1modulusn) / poly( z, j1modulusd);

    const real M_3PI_4 = 3 * PI_4;

    y = modulus * cos( y -  M_3PI_4 + z*phase) / sqrt(y);
    if( x < 0 )
        y = -y;
    return y;
}


private {
/*
y1(x) = 2/pi * (log(x) * j1(x) - 1/x) + R(x^2) z P(z**2)/Q(z**2)
0 <= x <= 4.5
z(x) = x
Absolute error
n=6, d=7
Peak error =  7.25e-22
Relative error spread =  4.5e-2
*/
const real [] y1n = [
   -0x1.32cab2601090742p+54,    // -21588552584537117.03
   0x1.432ceb7a8eaeff16p+52,    // 5685362960165615.9426
   -0x1.bcebec5a2484d3fap+47,   // -244598222688834.41402
   0x1.cc58f3cb54d6ac66p+41,    // 3954354656937.6771362
   -0x1.b1255e154d0eec0ep+34,   // -29067933781.204035773
   0x1.7a337df43298a7c8p+26,    // 99143159.815588153696
   -0x1.f77a1afdeff0b62cp+16    // -128890.10543727518796
];

const real [] y1d = [
   0x1.8733bcfd7236e604p+56,    // 110113602692855526.02
   0x1.5af412c672fd18d4p+50,    // 1525917240904692.388
   0x1.394ba130685755ep+43, // 10764748948290.729233
   0x1.7b3321523b24afcp+35, // 50895325841.848228333
   0x1.52946dac22f61d0cp+27,    // 177513325.37926774666
   0x1.c9040c6053de5318p+18,    // 467984.19337937079798
   0x1.be5156e6771dba34p+9, // 892.63546448532311355
   0x1p+0   // 1
];

/*
y1(x) = (x-YZ1)(x-YZ2)(x-YZ3)(x-YZ4)R(x) P(z)/Q(z)
z(x) = x
4.5 <= x <= 9
Absolute error
n=9, d=10
Peak error =  3.27e-22
Relative error spread =  4.5e-2
*/
const real y159n[] = [
   0x1.2fed87b1e60aa736p+18,    // 311222.12023306885099
   -0x1.1a2b18cdb2d1ec5ep+20,   // -1155761.5502193641786
   -0x1.b848827f47b47022p+20,   // -1803400.1560742424354
   -0x1.b2e422305ea19a86p+20,   // -1781314.136808997406
   -0x1.e3f82ac304534676p+16,   // -123896.16703822167479
   0x1.47a2cb5e852d657ep+14,    // 20968.69860275353983
   0x1.81b2fc6e44d7be8p+12, // 6171.1866285981340353
   -0x1.cd861d7b090dd22ep+9,    // -923.04777467672433155
   0x1.588897d683cbfbe2p+5, // 43.066695857903594508
   -0x1.5c7feccf76856bcap-1     // -0.68066349060542105505
];

const real y159d[] = [
   0x1.9b64f2a4d5614462p+26,    // 107844554.57552367857
   -0x1.17501e0e38db675ap+30,   // -1171523459.5555244587
   0x1.fe88b567c2911c1cp+31,    // 4282669747.8800133502
   -0x1.86b1781e04e748d4p+29,   // -819343107.75239426512
   0x1.ccd7d4396f2edbcap+26,    // 120807248.89741107429
   -0x1.694110c682e5cbcap+23,   // -11837576.387717419745
   0x1.c20f7005b88c789ep+19,    // 921723.50069835124759
   -0x1.983a5b4275ab7da8p+15,   // -52253.178241421874943
   0x1.17c60380490fa1fcp+11,    // 2238.187927382180589
   -0x1.ee84c254392634d8p+5,    // -61.81482377814679767
   0x1p+0   // 1
];

}

/**
 *  Bessel function of the second kind, order zero
 *
 * Returns Bessel function of the second kind, of order
 * zero, of the argument.
 */
/* The domain is divided into the intervals [0, 4.5>, [4.5,9> and
 * [9, infinity). In the first interval a rational approximation
 * R(x) is employed to compute y0(x)  = R(x) + 2/pi * log(x) * j0(x).
 *
 * In the second interval, the approximation is
 *     (x - p)(x - q)(x - r)(x - s)P9(x)/Q10(x)
 * where p, q, r, s are zeros of y1(x).
 *
 * The third interval uses the same approximations to modulus
 * and phase as j1(x), whence y1(x) = modulus * sin(phase).
 *
 * ACCURACY:
 *
 *  Absolute error, when y0(x) < 1; else relative error:
 *
 * arithmetic   domain     # trials      peak         rms
 *    IEEE      0, 30       36000       2.7e-19     5.3e-20
 *
 */
real cylBessel_y1(real x)
in {
    assert(x>=0.0);
    // TODO: should it return -infinity for x<0 ?
}
body {
    real xx, y, z, modulus, phase;

    z = 1.0/x;
    xx = x * x;
    if ( xx < 81.0L ) {
        if ( xx < 20.25L ) {
            y = M_2_PI * (log(x) * cylBessel_j1(x) - z);
            y += x * poly( xx, y1n) / poly( xx, y1d);
        } else {
            const real [] Y1Z =
            [   2.19714132603101703515e0L, 5.42968104079413513277e0L,
                8.59600586833116892643e0L, 1.17491548308398812434e1L];
            y = (x - Y1Z[0])*(x - Y1Z[1])*(x - Y1Z[2])*(x - Y1Z[3]);
            y *= poly( x, y159n) / poly( x, y159d);
        }
        return y;
    }
    xx = 1.0/xx;
    phase = poly( xx, j1phasen) / poly( xx, j1phased);
    modulus = poly( z, j1modulusn) / poly( z, j1modulusd);

    const real M_3PI_4 = 3 * PI_4;

    z = modulus * sin( x -  M_3PI_4 + z*phase) / sqrt(x);
    return z;
}

/**
 *  Bessel function of integer order
 *
 * Returns Bessel function of order n, where n is a
 * (possibly negative) integer.
 *
 * The ratio of jn(x) to j0(x) is computed by backward
 * recurrence.  First the ratio jn/jn-1 is found by a
 * continued fraction expansion.  Then the recurrence
 * relating successive orders is applied until j0 or j1 is
 * reached.
 *
 * If n = 0 or 1 the routine for j0 or j1 is called
 * directly.
 *
 * BUGS: Not suitable for large n or x.
 *
 */
real cylBessel_jn(int n, real x )
{
    real pkm2, pkm1, pk, xk, r, ans;
    int k, sign;

    if ( n < 0 ) {
        n = -n;
        if ( (n & 1) == 0 )  /* -1**n */
            sign = 1;
        else
            sign = -1;
    } else
        sign = 1;

    if ( x < 0.0L ) {
        if ( n & 1 )
            sign = -sign;
        x = -x;
    }

    if ( n == 0 )
        return sign * cylBessel_j0(x);
    if ( n == 1 )
        return sign * cylBessel_j1(x);
    if ( n == 2 )
        return sign * (2.0L * cylBessel_j1(x) / x  -  cylBessel_j0(x));

    if ( x < real.epsilon )
        return 0;

    /* continued fraction */
    k = 53;
    pk = 2 * (n + k);
    ans = pk;
    xk = x * x;

    do {
        pk -= 2.0L;
        ans = pk - (xk/ans);
    } while( --k > 0 );
    ans = x/ans;

    /* backward recurrence */

    pk = 1.0L;
    pkm1 = 1.0L/ans;
    k = n-1;
    r = 2 * k;

    do  {
        pkm2 = (pkm1 * r  -  pk * x) / x;
        pk = pkm1;
        pkm1 = pkm2;
        r -= 2.0L;
    } while( --k > 0 );

    if ( fabs(pk) > fabs(pkm1) )
        ans = cylBessel_j1(x)/pk;
    else
        ans = cylBessel_j0(x)/pkm1;
    return sign * ans;
}

/**
 *  Bessel function of second kind of integer order
 *
 * Returns Bessel function of order n, where n is a
 * (possibly negative) integer.
 *
 * The function is evaluated by forward recurrence on
 * n, starting with values computed by the routines
 * cylBessel_y0() and cylBessel_y1().
 *
 * If n = 0 or 1 the routine for cylBessel_y0 or cylBessel_y1 is called
 * directly.
 */
real cylBessel_yn(int n, real x)
in {
    assert(x>0); // TODO: should it return -infinity for x<=0 ?
}
body {
    real an, r;
    int k, sign;

    if ( n < 0 ) {
        n = -n;
        if ( (n & 1) == 0 ) /* -1**n */
            sign = 1;
        else
            sign = -1;
    } else
        sign = 1;

    if ( n == 0 )
        return sign * cylBessel_y0(x);
    if ( n == 1 )
        return sign * cylBessel_y1(x);

    /* forward recurrence on n */
    real anm2 = cylBessel_y0(x);
    real anm1 = cylBessel_y1(x);
    k = 1;
    r = 2 * k;
    do {
        an = r * anm1 / x  -  anm2;
        anm2 = anm1;
        anm1 = an;
        r += 2.0L;
        ++k;
    } while( k < n );
    return sign * an;
}

debug(UnitTest) {

unittest {

    // Wronksian test for Bessel functions
    void testWronksian(int n, real x)
    {
      real Jnp1 = cylBessel_jn(n + 1, x);
      real Jmn = cylBessel_jn(-n, x);
      real Jn = cylBessel_jn(n, x);
      real Jmnp1 = cylBessel_jn(-(n + 1), x);
      /* This should be trivially zero.  */
      assert( fabs(Jnp1 * Jmn + Jn * Jmnp1) == 0);
      if (x < 0.0) {
          x = -x;
          Jn = cylBessel_jn(n, x);
          Jnp1 = cylBessel_jn(n + 1, x);
      }
      real Yn = cylBessel_yn(n, x);
      real Ynp1 = cylBessel_yn(n + 1, x);
      /* The Wronksian.  */
      real w1 = Jnp1 * Yn - Jn * Ynp1;
      /* What the Wronksian should be. */
      real w2 = 2.0 / (PI * x);

      real reldif = feqrel(w1, w2);
      if (n==2 || n==-3)  {
        // I'm not sure why the n=2 case performs so badly.
        assert(reldif >= real.mant_dig-14);
      } else assert(reldif >= real.mant_dig-6);
    }

  real delta;
  int n, i, j;

  delta = 0.6 / PI;
  for (n = -30; n <= 30; n++) {
    real x = -30.0;
    while (x < 30.0)    {
        testWronksian (n, x);
        x += delta;
    }
    delta += .00123456;
  }
  assert(cylBessel_jn(20, 1e-80)==0);
}

}
