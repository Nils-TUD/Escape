/**
 * $Id: morebit.c 210 2011-05-15 13:38:03Z nasmussen $
 */

#include "common.h"
#include "core/cpu.h"
#include "core/decoder.h"
#include "core/instr/morebit.h"
#include "core/register.h"

static tetra cpu_instr_twydediff(tetra op1,tetra op2);
static octa cpu_instr_countBits(octa x);
static octa cpu_instr_boolMult(octa x,octa y,bool xor);

void cpu_instr_bdif(const sInstrArgs *iargs) {
	octa op1 = iargs->y;
	octa op2 = iargs->z;
	// the following describes it bytewise (the algorithm is taken from knuth's implementation,
	// but extended to 64bit):
	// at first we sub (#100 + (op1 & #FF)) by (op2 & #FF). if op2 is greater than op1, the 1
	// "disappears". otherwise its still there
	octa d = (op1 & 0x00FF00FF00FF00FF) + 0x0100010001000100 - (op2 & 0x00FF00FF00FF00FF);
	// now, we mask that maybe present 1
	octa m = d & 0x0100010001000100;
	// if its present (m - (m >> 8)) leads to #FF, so that we simply remove the 1.
	// if not, it leads to #00, so that we set the byte to 0
	octa x = d & (m - (m >> 8));
	// now we simply do that again, but for the other bytes
	d = ((op1 >> 8) & 0x00FF00FF00FF00FF) + 0x0100010001000100 - ((op2 >> 8) & 0x00FF00FF00FF00FF);
	m = d & 0x0100010001000100;
	// finally add them together to form the result
	octa res = x + ((d & (m - (m >> 8))) << 8);
	reg_set(iargs->x,res);
}

void cpu_instr_wdif(const sInstrArgs *iargs) {
	octa op1 = iargs->y;
	octa op2 = iargs->z;
	// calculate wyde-diff for the 2 tetras separatly.
	// note that I haven't translated the algo to 64bit (like above), because at least for the
	// "substraction-trick" I don't know a way to do that on an octa at once. Because we would
	// have 16 cases there.
	octa res = (octa)cpu_instr_twydediff(op1 >> 32,op2 >> 32) << 32;
	res |= cpu_instr_twydediff(op1 & 0xFFFFFFFF,op2 & 0xFFFFFFFF);
	reg_set(iargs->x,res);
}
static tetra cpu_instr_twydediff(tetra op1,tetra op2) {
	// the basic idea for the following is to substract op1 by op2, but prepare op2, so that
	// the wyde is either the one to substract (if its less) or the one in op1 (if its not less).
	// this way, its either the difference or 0.

	// a and b are #10000 if op2 > op1
	tetra a = ((op1 >> 16) - (op2 >> 16)) & 0x10000;
	tetra b = ((op1 & 0xFFFF) - (op2 & 0xFFFF)) & 0x10000;

	// the mask m has 4 cases (for a,b):
	// 0,0: (#00000 - #00000 - (#00000 >> 16)) = #00000000
	// 0,1: (#10000 - #00000 - (#10000 >> 16)) = #0000FFFF
	// 1,0: (#00000 - #10000 - (#00000 >> 16)) = #FFFF0000
	// 1,1: (#10000 - #10000 - (#10000 >> 16)) = #FFFFFFFF
	// so, basically we're selecting which of those 2 wydes we want to have (but the other way
	// around, i.e. 0 means use difference and #FFFF means use 0; see below)
	tetra m = b - a - (b >> 16);

	// first: (x ^ (y ^ x)) = y
	// when we mask (y ^ x) by the wydes we want to select, we get either the part of y or 0.
	// xor with 0 leads to the original bits, and xoring with itself leads to 0.
	// example: op1=#8F0024F1, op2=#480142F0
	// xormask = (op1 ^ op2) & m = #C7016601 & #0000FFFF = #00006601
	// subtrahend = op2 ^ xormask = #480124F1
	// op1 - subtrahend = #46FF0000
	tetra xormask = (op1 ^ op2) & m;
	tetra subtrahend = op2 ^ xormask;
	return op1 - subtrahend;
}

void cpu_instr_tdif(const sInstrArgs *iargs) {
	octa op1 = iargs->y;
	octa op2 = iargs->z;
	tetra op1t1 = op1 >> 32,op1t2 = op1 & 0xFFFFFFFF;
	tetra op2t1 = op2 >> 32,op2t2 = op2 & 0xFFFFFFFF;
	octa res = 0;
	if(op2t1 < op1t1)
		res |= (octa)(op1t1 - op2t1) << 32;
	if(op2t2 < op1t2)
		res |= op1t2 - op2t2;
	reg_set(iargs->x,res);
}

void cpu_instr_odif(const sInstrArgs *iargs) {
	octa op1 = iargs->y;
	octa op2 = iargs->z;
	octa res = op2 > op1 ? 0 : op1 - op2;
	reg_set(iargs->x,res);
}

void cpu_instr_sadd(const sInstrArgs *iargs) {
	octa res = cpu_instr_countBits(iargs->y & ~iargs->z);
	reg_set(iargs->x,res);
}
static octa cpu_instr_countBits(octa x) {
	// this is the algorithm used by knuth, but translated to 64bit
	// patterns:	0x55..55 = 01010101..01010101
	// 				0x33..33 = 00110011..00110011
	// 				0x0F..0F = 00001111..00001111
	// put count of each 2 bits into those 2 bits
	x = x - ((x >> 1) & 0x5555555555555555);
	// put count of each 4 bits into those 4 bits
	x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
	// put count of each 8 bits into those 8 bits
	x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0F;
	// put count of each 16 bits into their lowest 8 bits
	x = x + (x >> 8);
	// put count of each 32 bits into their lowest 8 bits
	x = x + (x >> 16);
	// put count of each 64 bits into their lowest 8 bits
	x = x + (x >> 32);
	return x & 0xFF;
}

void cpu_instr_mor(const sInstrArgs *iargs) {
	octa res = cpu_instr_boolMult(iargs->y,iargs->z,false);
	reg_set(iargs->x,res);
}

void cpu_instr_mxor(const sInstrArgs *iargs) {
	octa res = cpu_instr_boolMult(iargs->y,iargs->z,true);
	reg_set(iargs->x,res);
}

static octa cpu_instr_boolMult(octa x,octa y,bool xor) {
	// bit: 63  62      57  56  55      49      07  06      00
	// x = x00 x01 ... x07 x10 x11 ... x17 ... x70 x71 ... x77
	// y = y00 y01 ... y07 y10 y11 ... y17 ... y70 y71 ... y77

	// the following is knuth's algorithm, but translated to 64bit
	// the algorithm does always take one bit in every byte of y and combines that with one
	// byte of x. that means, we take 8 bits at once in each step.
	int k;
	octa res = 0;
	for(k = 0; x != 0; k++, x >>= 8) {
		if(x & 0xFF) {
			// first grab the k'th bit of every byte in y
			// by multiplying with 0xFF we get 0xFF for those bytes where the bit is set, else 0x00
			octa a = ((y >> k) & 0x0101010101010101) * 0xFF;
			// now we get the current byte from x and copy that into all bytes. i.e. if the byte
			// is 0x23, we get 0x2323232323232323
			octa c = (x & 0xFF) * 0x0101010101010101;
			// AND them, so that we have the byte from c where the bit of a is set, else 0
			// finally, OR/XOR them to the result
			if(xor)
				res ^= a & c;
			else
				res |= a & c;
		}
	}
	return res;
}
