/**
 * $Id: decoder.h 227 2011-06-11 18:40:58Z nasmussen $
 */

#ifndef DECODE_H_
#define DECODE_H_

#include "common.h"

#define OPCODE(x)	(((x) >> 24) & MASK(8))
#define DST(x)		(((x) >> 16) & MASK(8))
#define SRC1(x)		(((x) >>  8) & MASK(8))
#define SRC2(x)		(((x) >>  0) & MASK(8))
#define LIT16(x)	(((x) >>  0) & MASK(16))
#define LIT24(x)	(((x) >>  0) & MASK(24))

#define I_I8I8I8	0	// three 8 bit immeds; op1,op2,op3
#define I_RRR		1	// three regs
#define I_RRI8		2	// two regs, one 8 bit immed
#define I_RI8R		3	// one reg, one 8 bit immed, one reg
#define I_RI8I8		4	// one reg, two 8 bit immeds
#define I_I8RR		5	// one 8 bit immed, two regs
#define I_I8RI8		6	// one 8 bit immed, one reg, one 8 bit immed
#define I_RI16		7	// one reg, one 16 bit immed
#define I_I8I16		8	// one 8 bit immed, one 16 bit immed
#define I_RF16		9	// one reg, one 16 bit forward offset
#define I_RB16		10	// one reg, one 16 bit backward offset
#define I_F24		11	// one 24 bit forward offset
#define I_B24		12	// one 24 bit backward offset
#define I_R0S8		13	// one reg, 8 bit zero, 8 bit special
#define I_S80R		14	// 8 bit special, 8 bit zero, one reg
#define I_S80I8		15	// 8 bit special, 8 bit zero, 8 bit immed
#define I_R0I8		16	// one reg (X) and an 8-bit immed (Z)
#define I_00I8		17	// one 8-bit immed (Z)
#define I_I80R		18	// X = 8-bit immed, Y = 0, Z = reg
#define I_I24		19	// one 24-bit immed

#define A_DSS		0	// dest-reg, src1, src2
#define A_DS		1	// dest-reg, src
#define A_S			2	// source
#define A_A			3	// address
#define A_CT		4	// cmp-reg, target
#define A_DA		5	// dest-reg, address
#define A_SA		6	// src-reg, address
#define A_MA		7	// margin-reg, address
#define A_III		8	// imm1, imm2, imm3
#define A_LA		9	// length, address
#define A_SAVE		10	// special format for save
#define A_UNSAVE	11	// special format for unsave

/**
 * All instruction-numbers
 */
typedef enum {
	TRAP, FCMP, FUN, FEQL, FADD, FIX, FSUB, FIXU,
	FLOT, FLOTI, FLOTU, FLOTUI, SFLOT, SFLOTI, SFLOTU, SFLOTUI,
	FMUL, FCMPE, FUNE, FEQLE, FDIV, FSQRT, FREM, FINT,
	MUL, MULI, MULU, MULUI, DIV, DIVI, DIVU, DIVUI,
	ADD, ADDI, ADDU, ADDUI, SUB, SUBI, SUBU, SUBUI,
	IIADDU, IIADDUI, IVADDU, IVADDUI, VIIIADDU, VIIIADDUI, XVIADDU, XVIADDUI,
	CMP, CMPI, CMPU, CMPUI, NEG, NEGI, NEGU, NEGUI,
	SL, SLI, SLU, SLUI, SR, SRI, SRU, SRUI,
	BN, BNB, BZ, BZB, BP, BPB, BOD, BODB,
	BNN, BNNB, BNZ, BNZB, BNP, BNPB, BEV, BEVB,
	PBN, PBNB, PBZ, PBZB, PBP, PBPB, PBOD, PBODB,
	PBNN, PBNNB, PBNZ, PBNZB, PBNP, PBNPB, PBEV, PBEVB,
	CSN, CSNI, CSZ, CSZI, CSP, CSPI, CSOD, CSODI,
	CSNN, CSNNI, CSNZ, CSNZI, CSNP, CSNPI, CSEV, CSEVI,
	ZSN, ZSNI, ZSZ, ZSZI, ZSP, ZSPI, ZSOD, ZSODI,
	ZSNN, ZSNNI, ZSNZ, ZSNZI, ZSNP, ZSNPI, ZSEV, ZSEVI,
	LDB, LDBI, LDBU, LDBUI, LDW, LDWI, LDWU, LDWUI,
	LDT, LDTI, LDTU, LDTUI, LDO, LDOI, LDOU, LDOUI,
	LDSF, LDSFI, LDHT, LDHTI, CSWAP, CSWAPI, LDUNC, LDUNCI,
	LDVTS, LDVTSI, PRELD, PRELDI, PREGO, PREGOI, GO, GOI,
	STB, STBI, STBU, STBUI, STW, STWI, STWU, STWUI,
	STT, STTI, STTU, STTUI, STO, STOI, STOU, STOUI,
	STSF, STSFI, STHT, STHTI, STCO, STCOI, STUNC, STUNCI,
	SYNCD, SYNCDI, PREST, PRESTI, SYNCID, SYNCIDI, PUSHGO, PUSHGOI,
	OR, ORI, ORN, ORNI, NOR, NORI, XOR, XORI,
	AND, ANDI, ANDN, ANDNI, NAND, NANDI, NXOR, NXORI,
	BDIF, BDIFI, WDIF, WDIFI, TDIF, TDIFI, ODIF, ODIFI,
	MUX, MUXI, SADD, SADDI, MOR, MORI, MXOR, MXORI,
	SETH, SETMH, SETML, SETL, INCH, INCMH, INCML, INCL,
	ORH, ORMH, ORML, ORL, ANDNH, ANDNMH, ANDNML, ANDNL,
	JMP, JMPB, PUSHJ, PUSHJB, GETA, GETAB, PUT, PUTI,
	POP, RESUME, SAVE, UNSAVE, SYNC, SWYM, GET, TRIP
} eMMIXOpcode;

/**
 * The arguments of an instruction
 */
typedef struct {
	octa x;
	octa y;
	octa z;
} sInstrArgs;

/**
 * The execution-function
 */
typedef void (*fInstr)(const sInstrArgs *iargs);

/**
 * An instruction
 */
typedef struct {
	fInstr execute;
	int format;
	int args;
} sInstr;

/**
 * Returns the instruction of instruction-opcode <no>
 *
 * @param no the number
 * @return the instruction
 */
const sInstr *dec_getInstr(int no);

/**
 * Decodes the raw-instruction <t> into the corresponding arguments in <instr>.
 *
 * @param t the raw-instruction
 * @param instr the instruction to write the arguments to
 * @throws TRAP_BREAKS_RULES if the instruction <t> breaks the instruction-format-rules (e.g. if a
 *  byte should be zero, but isn't)
 */
void dec_decode(tetra t,sInstrArgs *iargs);

#endif /* DECODE_H_ */
