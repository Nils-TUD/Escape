/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <esc/common.h>
#include <esc/proc.h>
#include <esc/fileio.h>
#include <esc/heap.h>
#include <stdlib.h>
#include <string.h>

#if 0
int main(void) {
	/*if(exec((const char*)0x12345678,NULL) < 0) {
		printe("Exec failed");
		return EXIT_FAILURE;
	}*/

	/*u32 i;
	for(i = 0; i < 250; i++) {
		if(fork() == 0) {
			while(1)
				wait(EV_NOEVENT);
		}
	}*/

	return EXIT_SUCCESS;
}
#endif

#define SINGLE_MANTISSE_BITS	23
#define DOUBLE_MANTISSE_BITS	52
#define SINGLE_BIAS				127
#define DOUBLE_BIAS				1023
#define SINGLE_EXP_MASK			0xFF
#define DOUBLE_EXP_MASK			0x7FF
#define SINGLE_MANT_MASK		0x7FFFFF
#define DOUBLE_MANT_MASK		0xFFFFFFFFFFFFFLL
#define SINGLE_SIGN_BIT			0x80000000L
#define DOUBLE_SIGN_BIT			0x8000000000000000LL
#define MAX_LEN					52

typedef struct {
	unsigned int start;
	char data[MAX_LEN + 1];
} sBigInteger;
typedef struct {
	unsigned int start;
	char data[MAX_LEN * 2 + 1];
} sExtBigInteger;

static void incBigInt(unsigned int *astart,char *adata,unsigned int bstart,char *bdata) {
	unsigned int val,carry;
	int start = MIN(*astart,bstart);
	int len = MAX_LEN;
	char aval,bval;

	carry = 0;
	adata += MAX_LEN - 1;
	bdata += MAX_LEN - 1;
	while(len >= start) {
		aval = *adata - '0';
		bval = *bdata - '0';
		val = aval + bval + carry;
		*adata = (val % 10) + '0';
		carry = val / 10;
		len--;
		adata--;
		bdata--;
	}
	*adata += carry;
	if(*adata > '0')
		(*astart)--;
}

static sBigInteger floatBitTbl[] = {
	/* 2^-01 */ {51, "0000000000000000000000000000000000000000000000000005"},
	/* 2^-02 */ {51, "0000000000000000000000000000000000000000000000000025"},
	/* 2^-03 */ {50, "0000000000000000000000000000000000000000000000000125"},
	/* 2^-04 */ {50, "0000000000000000000000000000000000000000000000000625"},
	/* 2^-05 */ {49, "0000000000000000000000000000000000000000000000003125"},
	/* 2^-06 */ {48, "0000000000000000000000000000000000000000000000015625"},
	/* 2^-07 */ {48, "0000000000000000000000000000000000000000000000078125"},
	/* 2^-08 */ {47, "0000000000000000000000000000000000000000000000390625"},
	/* 2^-09 */ {46, "0000000000000000000000000000000000000000000001953125"},
	/* 2^-10 */ {46, "0000000000000000000000000000000000000000000009765625"},
	/* 2^-11 */ {45, "0000000000000000000000000000000000000000000048828125"},
	/* 2^-12 */ {44, "0000000000000000000000000000000000000000000244140625"},
	/* 2^-13 */ {43, "0000000000000000000000000000000000000000001220703125"},
	/* 2^-14 */ {43, "0000000000000000000000000000000000000000006103515625"},
	/* 2^-15 */ {42, "0000000000000000000000000000000000000000030517578125"},
	/* 2^-16 */ {41, "0000000000000000000000000000000000000000152587890625"},
	/* 2^-17 */ {41, "0000000000000000000000000000000000000000762939453125"},
	/* 2^-18 */ {40, "0000000000000000000000000000000000000003814697265625"},
	/* 2^-19 */ {39, "0000000000000000000000000000000000000019073486328125"},
	/* 2^-20 */ {39, "0000000000000000000000000000000000000095367431640625"},
	/* 2^-21 */ {38, "0000000000000000000000000000000000000476837158203125"},
	/* 2^-22 */ {37, "0000000000000000000000000000000000002384185791015625"},
	/* 2^-23 */ {36, "0000000000000000000000000000000000011920928955078125"},
	/* 2^-24 */ {36, "0000000000000000000000000000000000059604644775390625"},
	/* 2^-25 */ {35, "0000000000000000000000000000000000298023223876953125"},
	/* 2^-26 */ {34, "0000000000000000000000000000000001490116119384765625"},
	/* 2^-27 */ {34, "0000000000000000000000000000000007450580596923828125"},
	/* 2^-28 */ {33, "0000000000000000000000000000000037252902984619140625"},
	/* 2^-29 */ {32, "0000000000000000000000000000000186264514923095703125"},
	/* 2^-30 */ {32, "0000000000000000000000000000000931322574615478515625"},
	/* 2^-31 */ {31, "0000000000000000000000000000004656612873077392578125"},
	/* 2^-32 */ {30, "0000000000000000000000000000023283064365386962890625"},
	/* 2^-33 */ {29, "0000000000000000000000000000116415321826934814453125"},
	/* 2^-34 */ {29, "0000000000000000000000000000582076609134674072265625"},
	/* 2^-35 */ {28, "0000000000000000000000000002910383045673370361328125"},
	/* 2^-36 */ {27, "0000000000000000000000000014551915228366851806640625"},
	/* 2^-37 */ {27, "0000000000000000000000000072759576141834259033203125"},
	/* 2^-38 */ {26, "0000000000000000000000000363797880709171295166015625"},
	/* 2^-39 */ {25, "0000000000000000000000001818989403545856475830078125"},
	/* 2^-40 */ {25, "0000000000000000000000009094947017729282379150390625"},
	/* 2^-41 */ {24, "0000000000000000000000045474735088646411895751953125"},
	/* 2^-42 */ {23, "0000000000000000000000227373675443232059478759765625"},
	/* 2^-43 */ {22, "0000000000000000000001136868377216160297393798828125"},
	/* 2^-44 */ {22, "0000000000000000000005684341886080801486968994140625"},
	/* 2^-45 */ {21, "0000000000000000000028421709430404007434844970703125"},
	/* 2^-46 */ {20, "0000000000000000000142108547152020037174224853515625"},
	/* 2^-47 */ {20, "0000000000000000000710542735760100185871124267578125"},
	/* 2^-48 */ {19, "0000000000000000003552713678800500929355621337890625"},
	/* 2^-49 */ {18, "0000000000000000017763568394002504646778106689453125"},
	/* 2^-50 */ {18, "0000000000000000088817841970012523233890533447265625"},
	/* 2^-51 */ {17, "0000000000000000444089209850062616169452667236328125"},
	/* 2^-52 */ {16, "0000000000000002220446049250313080847263336181640625"},
};

static void genTable(void) {
	int i;
	char test[MAX_LEN + 1];
	sBigInteger n;
	sBigInteger incr;
	snprintf(n.data,sizeof(n.data),"%0*Ld",MAX_LEN,5LL);
	snprintf(test,sizeof(test),"%Ld",5LL);
	n.start = MAX_LEN - strlen(test);

	for(i = 0; i < 52; i++) {
		printf("/* 2^-%02d */ {%2d, \"%s\"},\n",i + 1,n.start,n.data);
		memcpy(&incr,&n,sizeof(sBigInteger));
		incBigInt(&n.start,n.data,incr.start,incr.data);
		incBigInt(&n.start,n.data,incr.start,incr.data);
		incBigInt(&n.start,n.data,incr.start,incr.data);
		incBigInt(&n.start,n.data,incr.start,incr.data);
	}
}

static void printIEEE754(long long pre,unsigned int mantBits,int exp,unsigned long long mantisse) {
	static sExtBigInteger post = {
		.start = MAX_LEN,
		.data = "0000000000000000000000000000000000000000000000000000"
				"0000000000000000000000000000000000000000000000000000"
	};
	char *postData = post.data;
	unsigned int index;
	unsigned long long pos = 1LL << mantBits;
	mantisse &= pos - 1;
	/* calc digits behind the comma */
	if(mantisse) {
		index = 0;
		pos >>= 1;
		while(pos != 0) {
			/* multiply with 10 */
			postData++;
			post.start--;
			if(mantisse & pos) {
				/* add number in float-bit-table */
				incBigInt(&post.start,postData,floatBitTbl[index].start,floatBitTbl[index].data);
			}
			index++;
			pos >>= 1;
		}
	}

	postData += post.start;
	postData[MAX_LEN - post.start] = '\0';
	printf("%ld.%s\n",(unsigned long)pre,postData);
}

static void printDouble2(double d) {
	union doubleInt {
		double d;
		unsigned long long i;
	};
	union doubleInt fBits = { .d = d };
	long long pre;
	int exp;
	unsigned long long mantisse;

	/* extract exponent and mantisse */
	exp = (fBits.i >> DOUBLE_MANTISSE_BITS) & DOUBLE_EXP_MASK;
	mantisse = (fBits.i & DOUBLE_MANT_MASK);

	/* null */
	if(exp == 0 && mantisse == 0) {
		printf("0.0\n");
		return;
	}
	/* denormalized */
	else if(exp == 0)
		exp = 1 - DOUBLE_BIAS;
	/* normalized */
	else {
		exp -= DOUBLE_BIAS;
		mantisse |= 1LL << DOUBLE_MANTISSE_BITS;
	}

	/* calc digits in front of the comma */
	pre = (mantisse >> (DOUBLE_MANTISSE_BITS - exp));
	if(fBits.i & DOUBLE_SIGN_BIT)
		pre = -pre;
	printIEEE754(pre,DOUBLE_MANTISSE_BITS,exp,mantisse);
}

static void printFloat(float f) {
	union floatInt {
		float f;
		unsigned long i;
	};
	union floatInt fBits = { .f = f };
	long long pre;
	unsigned int exp;
	unsigned long long mantisse;

	/* extract exponent and mantisse */
	exp = ((fBits.i >> SINGLE_MANTISSE_BITS) & SINGLE_EXP_MASK) - SINGLE_BIAS;
	mantisse = (fBits.i & SINGLE_MANT_MASK) | (1 << SINGLE_MANTISSE_BITS);
	/* calc digits in front of the comma */
	pre = (mantisse >> (SINGLE_MANTISSE_BITS - exp));
	if(fBits.i & SINGLE_SIGN_BIT)
		pre = -pre;
	printIEEE754(pre,SINGLE_MANTISSE_BITS,exp,mantisse);
}

void printLong(long long l) {
	if(l < 0) {
		printc('-');
		l = -l;
	}
	if(l >= 10)
		printLong(l / 10);
	printc("0123456789"[l % 10]);
}

void printDouble(double d) {
	char buf[100];
	long long val = 0;
	unsigned int pos = 0;
	int ndigits = 10;

	val = (long long)d;
	printLong(val);
	d -= val;
	if(d < 0)
		d = -d;
	while(ndigits-- > 0) {
		d *= 10;
		val = (long long)d;
		buf[pos++] = (val % 10) + '0';
		d -= val;
	}
	printf(".%s\n",buf);
}

int main(int argc,char *argv[]) {
	/*printf("1=");
	printDouble(1);
	printf("-1=");
	printDouble(-1);
	printf("0=");
	printDouble(0);
	printf("0.4=");
	printDouble(0.4);
	printf("-1.231=");
	printDouble(-1.231);
	printf("999.999=");
	printDouble(999.999);
	printf("1234.5678=");
	printDouble(1234.5678);
	printf("1189378123.78167213123=");
	printDouble(1189378123.78167213123);
	printf("-0.4=");
	printDouble(-0.4);
	printf("0.3333333333333333333=");
	printDouble(0.3333333333333333333);
	printf("812917263123223.81672813231=");
	printDouble(812917263123223.81672813231);*/

	u32 i;
	printf("%Ld, %Ld, %Ld\n",1LL,8167127123123123LL,-81273123LL);
	printf("%Lu, %Lx, %016LX\n",1ULL,0x7179bafed2122ULL,0x1234ULL);

	for(i = 0; i < 3; i++) {
		if(fork() == 0)
			break;
	}

	printf("[%d] 1 %f, %f, %f, %f\n",getpid(),1.f,-1.f,0.f,0.4f,18.4f);
	printf("[%d] 2 %f, %f, %f, %f\n",getpid(),-1.231f,999.999f,1234.5678f,1189378123.78167213123f);

	printf("[%d] 3 %lf, %lf, %lf, %lf\n",getpid(),1.,-1.,0.,0.4,18.4);
	printf("[%d] 4 %lf, %lf, %lf, %lf\n",getpid(),-1.231,999.999,1234.5678,1189378123.78167213123);

	printf("[%d] 5 %8.4lf, %8.1lf, %8.10lf, %8.20lf\n",getpid(),1.,-1.,0.,0.4,18.4);
	printf("[%d] 6 %3.0lf, %4.1lf, %2.4lf, %10.10lf\n",getpid(),-1.231,999.999,1234.5678,1189378123.78167213123);

	/*scanf("%f",&f);
	start = cycles();
	printFloat(f);
	end = cycles();
	printf("cycles=%Ld\n",end - start);

	start = cycles();
	printf("%.25f\n",f);
	end = cycles();
	printf("cycles=%Ld\n",end - start);
	*/
	/*scanf("%lf",&d);*/
	/*printf("%.25lf\n",d);*/

	/*
	unsigned long long incr = 5;
	unsigned long long opost = 0;
	index = 0;
	pos = 1 << (23 - exp);
	mantisse &= pos - 1;
	pos >>= 1;
	while(pos != 0) {
		opost *= 10;
		if(mantisse & pos) {
			printf("post=	%Ld, incr=%Ld\n",opost,incr);
			opost += incr;
		}
		incr *= 5;
		index++;
		pos >>= 1;
	}

	postData[MAX_LEN] = '\0';
	printf("p=%#x, exp=%d, mant=%#0*x, pre=%Ld, post=%s\n",p,exp,(23 + 3) / 4,mantisse,pre,postData);
	printf("opost=%Ld\n",opost);*/
	return 0;
}
