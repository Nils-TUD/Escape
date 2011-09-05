/**
 * $Id: decoder.c 174 2011-04-15 15:22:55Z nasmussen $
 */

#include <assert.h>

#include "core/cpu.h"
#include "core/decoder.h"
#include "core/register.h"
#include "core/instr/arith.h"
#include "core/instr/bit.h"
#include "core/instr/branch.h"
#include "core/instr/system.h"
#include "core/instr/compare.h"
#include "core/instr/intrpts.h"
#include "core/instr/load.h"
#include "core/instr/misc.h"
#include "core/instr/morebit.h"
#include "core/instr/store.h"
#include "core/instr/wyde.h"
#include "core/instr/float.h"
#include "exception.h"

static void dec_toDSS(sInstrArgs *iargs,tetra t,int fmt);
static void dec_toDS(sInstrArgs *iargs,tetra t,int fmt);
static void dec_toDA(sInstrArgs *iargs,tetra t,int fmt);
static void dec_toSA(sInstrArgs *iargs,tetra t,int fmt);
static void dec_toCT(sInstrArgs *iargs,tetra t,int fmt);
static void dec_toA(sInstrArgs *iargs,tetra t,int fmt);
static void dec_toIII(sInstrArgs *iargs,tetra t,int fmt);
static void dec_toLA(sInstrArgs *iargs,tetra t,int fmt);
static void dec_toMA(sInstrArgs *iargs,tetra t,int fmt);
static void dec_toSave(sInstrArgs *iargs,tetra t,int fmt);
static void dec_toUnsave(sInstrArgs *iargs,tetra t,int fmt);
static void dec_toS(sInstrArgs *iargs,tetra t,int fmt);

// Note that, since we're not using pipelining, branches and probable branches are the same
static const sInstr instructions[] = {
	/* 00: TRAP */		{cpu_instr_trap,	I_I8I8I8,	A_III},
	/* 01: FCMP */		{cpu_instr_fcmp,	I_RRR,		A_DSS},
	/* 02: FUN */		{cpu_instr_fun,		I_RRR,		A_DSS},
	/* 03: FEQL */		{cpu_instr_feql,	I_RRR,		A_DSS},
	/* 04: FADD */		{cpu_instr_fadd,	I_RRR,		A_DSS},
	/* 05: FIX */		{cpu_instr_fix,		I_RI8R,		A_DS},
	/* 06: FSUB */		{cpu_instr_fsub,	I_RRR,		A_DSS},
	/* 07: FIXU */		{cpu_instr_fixu,	I_RI8R,		A_DS},
	/* 08: FLOT */		{cpu_instr_flot,	I_RI8R,		A_DS},
	/* 09: FLOTI */		{cpu_instr_flot,	I_RI8I8,	A_DS},
	/* 0A: FLOTU */		{cpu_instr_flotu,	I_RI8R,		A_DS},
	/* 0B: FLOTUI */	{cpu_instr_flotu,	I_RI8I8,	A_DS},
	/* 0C: SFLOT */		{cpu_instr_sflot,	I_RI8R,		A_DS},
	/* 0D: SFLOTI */	{cpu_instr_sflot,	I_RI8I8,	A_DS},
	/* 0E: SFLOTU */	{cpu_instr_sflotu,	I_RI8R,		A_DS},
	/* 0F: SFLOTUI */	{cpu_instr_sflotu,	I_RI8I8,	A_DS},
	/* 10: FMUL */		{cpu_instr_fmul,	I_RRR,		A_DSS},
	/* 11: FCMPE */		{cpu_instr_fcmpe,	I_RRR,		A_DSS},
	/* 12: FUNE */		{cpu_instr_fune,	I_RRR,		A_DSS},
	/* 13: FEQLE */		{cpu_instr_feqle,	I_RRR,		A_DSS},
	/* 14: FDIV */		{cpu_instr_fdiv,	I_RRR,		A_DSS},
	/* 15: FSQRT */		{cpu_instr_fsqrt,	I_RI8R,		A_DS},
	/* 16: FREM */		{cpu_instr_frem,	I_RRR,		A_DSS},
	/* 17: FINT */		{cpu_instr_fint,	I_RI8R,		A_DS},
	/* 18: MUL */		{cpu_instr_mul,		I_RRR,		A_DSS},
	/* 19: MULI */		{cpu_instr_mul,		I_RRI8,		A_DSS},
	/* 1A: MULU */		{cpu_instr_mulu,	I_RRR,		A_DSS},
	/* 1B: MULUI */		{cpu_instr_mulu,	I_RRI8,		A_DSS},
	/* 1C: DIV */		{cpu_instr_div,		I_RRR,		A_DSS},
	/* 1D: DIVI */		{cpu_instr_div,		I_RRI8,		A_DSS},
	/* 1E: DIVU */		{cpu_instr_divu,	I_RRR,		A_DSS},
	/* 1F: DIVUI */		{cpu_instr_divu,	I_RRI8,		A_DSS},
	/* 20: ADD */		{cpu_instr_add,		I_RRR,		A_DSS},
	/* 21: ADDI */		{cpu_instr_add,		I_RRI8,		A_DSS},
	/* 22: ADDU */		{cpu_instr_addu,	I_RRR,		A_DSS},
	/* 23: ADDUI */		{cpu_instr_addu,	I_RRI8,		A_DSS},
	/* 24: SUB */		{cpu_instr_sub,		I_RRR,		A_DSS},
	/* 25: SUBI */		{cpu_instr_sub,		I_RRI8,		A_DSS},
	/* 26: SUBU */		{cpu_instr_subu,	I_RRR,		A_DSS},
	/* 27: SUBUI */		{cpu_instr_subu,	I_RRI8,		A_DSS},
	/* 28: 2ADDU */		{cpu_instr_2addu,	I_RRR,		A_DSS},
	/* 29: 2ADDUI */	{cpu_instr_2addu,	I_RRI8,		A_DSS},
	/* 2A: 4ADDU */		{cpu_instr_4addu,	I_RRR,		A_DSS},
	/* 2B: 4ADDUI */	{cpu_instr_4addu,	I_RRI8,		A_DSS},
	/* 2C: 8ADDU */		{cpu_instr_8addu,	I_RRR,		A_DSS},
	/* 2D: 8ADDUI */	{cpu_instr_8addu,	I_RRI8,		A_DSS},
	/* 2E: 16ADDU */	{cpu_instr_16addu,	I_RRR,		A_DSS},
	/* 2F: 16ADDUI */	{cpu_instr_16addu,	I_RRI8,		A_DSS},
	/* 30: CMP */		{cpu_instr_cmp,		I_RRR,		A_DSS},
	/* 31: CMPI */		{cpu_instr_cmp,		I_RRI8,		A_DSS},
	/* 32: CMPU */		{cpu_instr_cmpu,	I_RRR,		A_DSS},
	/* 33: CMPUI */		{cpu_instr_cmpu,	I_RRI8,		A_DSS},
	/* 34: NEG */		{cpu_instr_neg,		I_RI8R,		A_DSS},
	/* 35: NEGI */		{cpu_instr_neg,		I_RI8I8,	A_DSS},
	/* 36: NEGU */		{cpu_instr_negu,	I_RI8R,		A_DSS},
	/* 37: NEGUI */		{cpu_instr_negu,	I_RI8I8,	A_DSS},
	/* 38: SL */		{cpu_instr_sl,		I_RRR,		A_DSS},
	/* 39: SLI */		{cpu_instr_sl,		I_RRI8,		A_DSS},
	/* 3A: SLU */		{cpu_instr_slu,		I_RRR,		A_DSS},
	/* 3B: SLUI */		{cpu_instr_slu,		I_RRI8,		A_DSS},
	/* 3C: SR */		{cpu_instr_sr,		I_RRR,		A_DSS},
	/* 3D: SRI */		{cpu_instr_sr,		I_RRI8,		A_DSS},
	/* 3E: SRU */		{cpu_instr_sru,		I_RRR,		A_DSS},
	/* 3F: SRUI */		{cpu_instr_sru,		I_RRI8,		A_DSS},
	/* 40: BN */		{cpu_instr_bn,		I_RF16,		A_CT},
	/* 41: BNB */		{cpu_instr_bn,		I_RB16,		A_CT},
	/* 42: BZ */		{cpu_instr_bz,		I_RF16,		A_CT},
	/* 43: BZB */		{cpu_instr_bz,		I_RB16,		A_CT},
	/* 44: BP */		{cpu_instr_bp,		I_RF16,		A_CT},
	/* 45: BPB */		{cpu_instr_bp,		I_RB16,		A_CT},
	/* 46: BOD */		{cpu_instr_bod,		I_RF16,		A_CT},
	/* 47: BODB */		{cpu_instr_bod,		I_RB16,		A_CT},
	/* 48: BNN */		{cpu_instr_bnn,		I_RF16,		A_CT},
	/* 49: BNNB */		{cpu_instr_bnn,		I_RB16,		A_CT},
	/* 4A: BNZ */		{cpu_instr_bnz,		I_RF16,		A_CT},
	/* 4B: BNZB */		{cpu_instr_bnz,		I_RB16,		A_CT},
	/* 4C: BNP */		{cpu_instr_bnp,		I_RF16,		A_CT},
	/* 4D: BNPB */		{cpu_instr_bnp,		I_RB16,		A_CT},
	/* 4E: BEV */		{cpu_instr_bev,		I_RF16,		A_CT},
	/* 4F: BEVB */		{cpu_instr_bev,		I_RB16,		A_CT},
	/* 50: PBN */		{cpu_instr_bn,		I_RF16,		A_CT},
	/* 51: PBNB */		{cpu_instr_bn,		I_RB16,		A_CT},
	/* 52: PBZ */		{cpu_instr_bz,		I_RF16,		A_CT},
	/* 53: PBZB */		{cpu_instr_bz,		I_RB16,		A_CT},
	/* 54: PBP */		{cpu_instr_bp,		I_RF16,		A_CT},
	/* 55: PBPB */		{cpu_instr_bp,		I_RB16,		A_CT},
	/* 56: PBOD */		{cpu_instr_bod,		I_RF16,		A_CT},
	/* 57: PBODB */		{cpu_instr_bod,		I_RB16,		A_CT},
	/* 58: PBNN */		{cpu_instr_bnn,		I_RF16,		A_CT},
	/* 59: PBNNB */		{cpu_instr_bnn,		I_RB16,		A_CT},
	/* 5A: PBNZ */		{cpu_instr_bnz,		I_RF16,		A_CT},
	/* 5B: PBNZB */		{cpu_instr_bnz,		I_RB16,		A_CT},
	/* 5C: PBNP */		{cpu_instr_bnp,		I_RF16,		A_CT},
	/* 5D: PBNPB */		{cpu_instr_bnp,		I_RB16,		A_CT},
	/* 5E: PBEV */		{cpu_instr_bev,		I_RF16,		A_CT},
	/* 5F: PBEVB */		{cpu_instr_bev,		I_RB16,		A_CT},
	/* 60: CSN */		{cpu_instr_csn,		I_RRR,		A_DSS},
	/* 61: CSNI */		{cpu_instr_csn,		I_RRI8,		A_DSS},
	/* 62: CSZ */		{cpu_instr_csz,		I_RRR,		A_DSS},
	/* 63: CSZI */		{cpu_instr_csz,		I_RRI8,		A_DSS},
	/* 64: CSP */		{cpu_instr_csp,		I_RRR,		A_DSS},
	/* 65: CSPI */		{cpu_instr_csp,		I_RRI8,		A_DSS},
	/* 66: CSOD */		{cpu_instr_csod,	I_RRR,		A_DSS},
	/* 67: CSODI */		{cpu_instr_csod,	I_RRI8,		A_DSS},
	/* 68: CSNN */		{cpu_instr_csnn,	I_RRR,		A_DSS},
	/* 69: CSNNI */		{cpu_instr_csnn,	I_RRI8,		A_DSS},
	/* 6A: CSNZ */		{cpu_instr_csnz,	I_RRR,		A_DSS},
	/* 6B: CSNZI */		{cpu_instr_csnz,	I_RRI8,		A_DSS},
	/* 6C: CSNP */		{cpu_instr_csnp,	I_RRR,		A_DSS},
	/* 6D: CSNPI */		{cpu_instr_csnp,	I_RRI8,		A_DSS},
	/* 6E: CSEV */		{cpu_instr_csev,	I_RRR,		A_DSS},
	/* 6F: CSEVI */		{cpu_instr_csev,	I_RRI8,		A_DSS},
	/* 70: ZSN */		{cpu_instr_zsn,		I_RRR,		A_DSS},
	/* 71: ZSNI */		{cpu_instr_zsn,		I_RRI8,		A_DSS},
	/* 72: ZSZ */		{cpu_instr_zsz,		I_RRR,		A_DSS},
	/* 73: ZSZI */		{cpu_instr_zsz,		I_RRI8,		A_DSS},
	/* 74: ZSP */		{cpu_instr_zsp,		I_RRR,		A_DSS},
	/* 75: ZSPI */		{cpu_instr_zsp,		I_RRI8,		A_DSS},
	/* 76: ZSOD */		{cpu_instr_zsod,	I_RRR,		A_DSS},
	/* 77: ZSODI */		{cpu_instr_zsod,	I_RRI8,		A_DSS},
	/* 78: ZSNN */		{cpu_instr_zsnn,	I_RRR,		A_DSS},
	/* 79: ZSNNI */		{cpu_instr_zsnn,	I_RRI8,		A_DSS},
	/* 7A: ZSNZ */		{cpu_instr_zsnz,	I_RRR,		A_DSS},
	/* 7B: ZSNZI */		{cpu_instr_zsnz,	I_RRI8,		A_DSS},
	/* 7C: ZSNP */		{cpu_instr_zsnp,	I_RRR,		A_DSS},
	/* 7D: ZSNPI */		{cpu_instr_zsnp,	I_RRI8,		A_DSS},
	/* 7E: ZSEV */		{cpu_instr_zsev,	I_RRR,		A_DSS},
	/* 7F: ZSEVI */		{cpu_instr_zsev,	I_RRI8,		A_DSS},
	/* 80: LDB */		{cpu_instr_ldb,		I_RRR,		A_DA},
	/* 81: LDBI */		{cpu_instr_ldb,		I_RRI8,		A_DA},
	/* 82: LDBU */		{cpu_instr_ldbu,	I_RRR,		A_DA},
	/* 83: LDBUI */		{cpu_instr_ldbu,	I_RRI8,		A_DA},
	/* 84: LDW */		{cpu_instr_ldw,		I_RRR,		A_DA},
	/* 85: LDWI */		{cpu_instr_ldw,		I_RRI8,		A_DA},
	/* 86: LDWU */		{cpu_instr_ldwu,	I_RRR,		A_DA},
	/* 87: LDWUI */		{cpu_instr_ldwu,	I_RRI8,		A_DA},
	/* 88: LDT */		{cpu_instr_ldt,		I_RRR,		A_DA},
	/* 89: LDTI */		{cpu_instr_ldt,		I_RRI8,		A_DA},
	/* 8A: LDTU */		{cpu_instr_ldtu,	I_RRR,		A_DA},
	/* 8B: LDTUI */		{cpu_instr_ldtu,	I_RRI8,		A_DA},
	/* 8C: LDO */		{cpu_instr_ldo,		I_RRR,		A_DA},
	/* 8D: LDOI */		{cpu_instr_ldo,		I_RRI8,		A_DA},
	/* 8E: LDOU */		{cpu_instr_ldou,	I_RRR,		A_DA},
	/* 8F: LDOUI */		{cpu_instr_ldou,	I_RRI8,		A_DA},
	/* 90: LDSF */		{cpu_instr_ldsf,	I_RRR,		A_DA},
	/* 91: LDSFI */		{cpu_instr_ldsf,	I_RRI8,		A_DA},
	/* 92: LDHT */		{cpu_instr_ldht,	I_RRR,		A_DA},
	/* 93: LDHTI */		{cpu_instr_ldht,	I_RRI8,		A_DA},
	/* 94: CSWAP */		{cpu_instr_cswap,	I_RRR,		A_DSS},
	/* 95: CSWAPI */	{cpu_instr_cswap,	I_RRI8,		A_DSS},
	/* 96: LDUNC */		{cpu_instr_ldunc,	I_RRR,		A_DA},
	/* 97: LDUNCI */	{cpu_instr_ldunc,	I_RRI8,		A_DA},
	/* 98: LDVTS */		{cpu_instr_ldvts,	I_RRR,		A_DSS},
	/* 99: LDVTSI */	{cpu_instr_ldvts,	I_RRI8,		A_DSS},
	/* 9A: PRELD */		{cpu_instr_preld,	I_I8RR,		A_LA},
	/* 9B: PRELDI */	{cpu_instr_preld,	I_I8RI8,	A_LA},
	/* 9C: PREGO */		{cpu_instr_prego,	I_I8RR,		A_LA},
	/* 9D: PREGOI */	{cpu_instr_prego,	I_I8RI8,	A_LA},
	/* 9E: GO */		{cpu_instr_go,		I_RRR,		A_DSS},
	/* 9F: GOI */		{cpu_instr_go,		I_RRI8,		A_DSS},
	/* A0: STB */		{cpu_instr_stb,		I_RRR,		A_SA},
	/* A1: STBI */		{cpu_instr_stb,		I_RRI8,		A_SA},
	/* A2: STBU */		{cpu_instr_stbu,	I_RRR,		A_SA},
	/* A3: STBUI */		{cpu_instr_stbu,	I_RRI8,		A_SA},
	/* A4: STW */		{cpu_instr_stw,		I_RRR,		A_SA},
	/* A5: STWI */		{cpu_instr_stw,		I_RRI8,		A_SA},
	/* A6: STWU */		{cpu_instr_stwu,	I_RRR,		A_SA},
	/* A7: STWUI */		{cpu_instr_stwu,	I_RRI8,		A_SA},
	/* A8: STT */		{cpu_instr_stt,		I_RRR,		A_SA},
	/* A9: STTI */		{cpu_instr_stt,		I_RRI8,		A_SA},
	/* AA: STTU */		{cpu_instr_sttu,	I_RRR,		A_SA},
	/* AB: STTUI */		{cpu_instr_sttu,	I_RRI8,		A_SA},
	/* AC: STO */		{cpu_instr_sto,		I_RRR,		A_SA},
	/* AD: STOI */		{cpu_instr_sto,		I_RRI8,		A_SA},
	/* AE: STOU */		{cpu_instr_stou,	I_RRR,		A_SA},
	/* AF: STOUI */		{cpu_instr_stou,	I_RRI8,		A_SA},
	/* B0: STSF */		{cpu_instr_stsf,	I_RRR,		A_SA},
	/* B1: STSFI */		{cpu_instr_stsf,	I_RRI8,		A_SA},
	/* B2: STHT */		{cpu_instr_stht,	I_RRR,		A_SA},
	/* B3: STHTI */		{cpu_instr_stht,	I_RRI8,		A_SA},
	/* B4: STCO */		{cpu_instr_stco,	I_I8RR,		A_SA},
	/* B5: STCOI */		{cpu_instr_stco,	I_I8RI8,	A_SA},
	/* B6: STUNC */		{cpu_instr_stunc,	I_RRR,		A_SA},
	/* B7: STUNCI */	{cpu_instr_stunc,	I_RRI8,		A_SA},
	/* B8: SYNCD */		{cpu_instr_syncd,	I_I8RR,		A_LA},
	/* B9: SYNCDI */	{cpu_instr_syncd,	I_I8RI8,	A_LA},
	/* BA: PREST */		{cpu_instr_prest,	I_I8RR,		A_LA},
	/* BB: PRESTI */	{cpu_instr_prest,	I_I8RI8,	A_LA},
	/* BC: SYNCID */	{cpu_instr_syncid,	I_I8RR,		A_LA},
	/* BD: SYNCIDI */	{cpu_instr_syncid,	I_I8RI8,	A_LA},
	/* BE: PUSHGO */	{cpu_instr_push,	I_RRR,		A_MA},
	/* BF: PUSHGOI */	{cpu_instr_push,	I_RRI8,		A_MA},
	/* C0: OR */		{cpu_instr_or,		I_RRR,		A_DSS},
	/* C1: ORI */		{cpu_instr_or,		I_RRI8,		A_DSS},
	/* C2: ORN */		{cpu_instr_orn,		I_RRR,		A_DSS},
	/* C3: ORNI */		{cpu_instr_orn,		I_RRI8,		A_DSS},
	/* C4: NOR */		{cpu_instr_nor,		I_RRR,		A_DSS},
	/* C5: NORI */		{cpu_instr_nor,		I_RRI8,		A_DSS},
	/* C6: XOR */		{cpu_instr_xor,		I_RRR,		A_DSS},
	/* C7: XORI */		{cpu_instr_xor,		I_RRI8,		A_DSS},
	/* C8: AND */		{cpu_instr_and,		I_RRR,		A_DSS},
	/* C9: ANDI */		{cpu_instr_and,		I_RRI8,		A_DSS},
	/* CA: ANDN */		{cpu_instr_andn,	I_RRR,		A_DSS},
	/* CB: ANDNI */		{cpu_instr_andn,	I_RRI8,		A_DSS},
	/* CC: NAND */		{cpu_instr_nand,	I_RRR,		A_DSS},
	/* CD: NANDI */		{cpu_instr_nand,	I_RRI8,		A_DSS},
	/* CE: NXOR */		{cpu_instr_nxor,	I_RRR,		A_DSS},
	/* CF: NXORI */		{cpu_instr_nxor,	I_RRI8,		A_DSS},
	/* D0: BDIF */		{cpu_instr_bdif,	I_RRR,		A_DSS},
	/* D1: BDIFI */		{cpu_instr_bdif,	I_RRI8,		A_DSS},
	/* D2: WDIF */		{cpu_instr_wdif,	I_RRR,		A_DSS},
	/* D3: WDIFI */		{cpu_instr_wdif,	I_RRI8,		A_DSS},
	/* D4: TDIF */		{cpu_instr_tdif,	I_RRR,		A_DSS},
	/* D5: TDIFI */		{cpu_instr_tdif,	I_RRI8,		A_DSS},
	/* D6: ODIF */		{cpu_instr_odif,	I_RRR,		A_DSS},
	/* D7: ODIFI */		{cpu_instr_odif,	I_RRI8,		A_DSS},
	/* D8: MUX */		{cpu_instr_mux,		I_RRR,		A_DSS},
	/* D9: MUXI */		{cpu_instr_mux,		I_RRI8,		A_DSS},
	/* DA: SADD */		{cpu_instr_sadd,	I_RRR,		A_DSS},
	/* DB: SADDI */		{cpu_instr_sadd,	I_RRI8,		A_DSS},
	/* DC: MOR */		{cpu_instr_mor,		I_RRR,		A_DSS},
	/* DD: MORI */		{cpu_instr_mor,		I_RRI8,		A_DSS},
	/* DE: MXOR */		{cpu_instr_mxor,	I_RRR,		A_DSS},
	/* DF: MXORI */		{cpu_instr_mxor,	I_RRI8,		A_DSS},
	/* E0: SETH */		{cpu_instr_seth,	I_RI16,		A_DS},
	/* E1: SETMH */		{cpu_instr_setmh,	I_RI16,		A_DS},
	/* E2: SETML */		{cpu_instr_setml,	I_RI16,		A_DS},
	/* E3: SETL */		{cpu_instr_setl,	I_RI16,		A_DS},
	/* E4: INCH */		{cpu_instr_inch,	I_RI16,		A_DS},
	/* E5: INCMH */		{cpu_instr_incmh,	I_RI16,		A_DS},
	/* E6: INCML */		{cpu_instr_incml,	I_RI16,		A_DS},
	/* E7: INCL */		{cpu_instr_incl,	I_RI16,		A_DS},
	/* E8: ORH */		{cpu_instr_orh,		I_RI16,		A_DS},
	/* E9: ORMH */		{cpu_instr_ormh,	I_RI16,		A_DS},
	/* EA: ORML */		{cpu_instr_orml,	I_RI16,		A_DS},
	/* EB: ORL */		{cpu_instr_orl,		I_RI16,		A_DS},
	/* EC: ANDNH */		{cpu_instr_andnh,	I_RI16,		A_DS},
	/* ED: ANDNMH */	{cpu_instr_andnmh,	I_RI16,		A_DS},
	/* EE: ANDNML */	{cpu_instr_andnml,	I_RI16,		A_DS},
	/* EF: ANDNL */		{cpu_instr_andnl,	I_RI16,		A_DS},
	/* F0: JMP */		{cpu_instr_jmp,		I_F24,		A_A},
	/* F1: JMPB */		{cpu_instr_jmp,		I_B24,		A_A},
	/* F2: PUSHJ */		{cpu_instr_push,	I_RF16,		A_MA},
	/* F3: PUSHJB */	{cpu_instr_push,	I_RB16,		A_MA},
	/* F4: GETA */		{cpu_instr_geta,	I_RF16,		A_DA},
	/* F5: GETAB */		{cpu_instr_geta,	I_RB16,		A_DA},
	/* F6: PUT */		{cpu_instr_put,		I_S80R,		A_DS},
	/* F7: PUTI */		{cpu_instr_put,		I_S80I8,	A_DS},
	/* F8: POP */		{cpu_instr_pop,		I_I8I16,	A_MA},
	/* F9: RESUME */	{cpu_instr_resume,	I_00I8,		A_S},
	/* FA: SAVE */		{cpu_instr_save,	I_R0I8,		A_SAVE},
	/* FB: UNSAVE */	{cpu_instr_unsave,	I_I80R,		A_UNSAVE},
	/* FC: SYNC */		{cpu_instr_sync,	I_I24,		A_S},
	/* FD: SWYM */		{cpu_instr_swym,	I_I8I8I8,	A_III},
	/* FE: GET */		{cpu_instr_get,		I_R0S8,		A_DS},
	/* FF: TRIP */		{cpu_instr_trip,	I_I8I8I8,	A_III},
};

const sInstr *dec_getInstr(int no) {
	return instructions + no;
}

void dec_decode(tetra t,sInstrArgs *iargs) {
	const sInstr *desc = dec_getInstr(OPCODE(t));
	int format = desc->format;
	int args = desc->args;
	iargs->x = iargs->y = iargs->z = 0;
	switch(args) {
		case A_DSS:
			dec_toDSS(iargs,t,format);
			break;
		case A_DS:
			dec_toDS(iargs,t,format);
			break;
		case A_DA:
			dec_toDA(iargs,t,format);
			break;
		case A_SA:
			dec_toSA(iargs,t,format);
			break;
		case A_CT:
			dec_toCT(iargs,t,format);
			break;
		case A_A:
			dec_toA(iargs,t,format);
			break;
		case A_III:
			dec_toIII(iargs,t,format);
			break;
		case A_LA:
			dec_toLA(iargs,t,format);
			break;
		case A_MA:
			dec_toMA(iargs,t,format);
			break;
		case A_SAVE:
			dec_toSave(iargs,t,format);
			break;
		case A_UNSAVE:
			dec_toUnsave(iargs,t,format);
			break;
		case A_S:
			dec_toS(iargs,t,format);
			break;
		default:
			assert(false);
			break;
	}
}

static void dec_toDSS(sInstrArgs *iargs,tetra t,int fmt) {
	assert(fmt == I_RI8I8 || fmt == I_RI8R || fmt == I_RRR || fmt == I_RRI8);
	iargs->x = DST(t);
	if(fmt == I_RI8I8 || fmt == I_RI8R)
		iargs->y = ZEXT(SRC1(t),8);
	else
		iargs->y = reg_get(SRC1(t));
	if(fmt == I_RRI8 || fmt == I_RI8I8)
		iargs->z = ZEXT(SRC2(t),8);
	else
		iargs->z = reg_get(SRC2(t));
}

static void dec_toDS(sInstrArgs *iargs,tetra t,int fmt) {
	assert(fmt == I_RI16 || fmt == I_RI8R || fmt == I_RI8I8 ||
			fmt == I_R0S8 || fmt == I_S80I8 || fmt == I_S80R);
	iargs->x = DST(t);
	if((fmt == I_R0S8 || fmt == I_S80I8 || fmt == I_S80R) && SRC1(t) != 0)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	switch(fmt) {
		case I_RI16:
			iargs->z = ZEXT(LIT16(t),16);
			break;
		case I_RI8R:
			iargs->y = ZEXT(SRC1(t),8);
			iargs->z = reg_get(SRC2(t));
			break;
		case I_RI8I8:
			iargs->y = ZEXT(SRC1(t),8);
			iargs->z = ZEXT(SRC2(t),8);
			break;
		case I_S80I8:
		case I_R0S8:
			iargs->z = ZEXT(SRC2(t),8);
			break;
		case I_S80R:
			iargs->z = reg_get(SRC2(t));
			break;
	}
}

static void dec_toDA(sInstrArgs *iargs,tetra t,int fmt) {
	assert(fmt == I_RRR || fmt == I_RRI8 || I_RF16 || I_RB16);
	iargs->x = DST(t);
	if(fmt == I_RRR)
		iargs->y = reg_get(SRC1(t)) + reg_get(SRC2(t));
	else if(fmt == I_RRI8)
		iargs->y = reg_get(SRC1(t)) + ZEXT(SRC2(t),8);
	else if(fmt == I_RF16)
		iargs->y = cpu_getPC() + LIT16(t) * sizeof(tetra);
	else
		iargs->y = cpu_getPC() + ((socta)LIT16(t) - 0x10000) * sizeof(tetra);
}

static void dec_toSA(sInstrArgs *iargs,tetra t,int fmt) {
	assert(fmt == I_RRR || fmt == I_RRI8 || fmt == I_I8RR || fmt == I_I8RI8);
	if(fmt == I_RRR || fmt == I_RRI8)
		iargs->x = reg_get(DST(t));
	else
		iargs->x = ZEXT(DST(t),8);
	if(fmt == I_RRR || fmt == I_I8RR)
		iargs->y = reg_get(SRC1(t)) + reg_get(SRC2(t));
	else
		iargs->y = reg_get(SRC1(t)) + ZEXT(SRC2(t),8);
}

static void dec_toCT(sInstrArgs *iargs,tetra t,int fmt) {
	assert(fmt == I_RF16 || fmt == I_RB16);
	iargs->x = reg_get(DST(t));
	if(fmt == I_RF16)
		iargs->z = cpu_getPC() + LIT16(t) * sizeof(tetra);
	else
		iargs->z = cpu_getPC() + ((socta)LIT16(t) - 0x10000) * sizeof(tetra);
}

static void dec_toA(sInstrArgs *iargs,tetra t,int fmt) {
	assert(fmt == I_F24 || fmt == I_B24);
	if(fmt == I_F24)
		iargs->z = cpu_getPC() + LIT24(t) * sizeof(tetra);
	else
		iargs->z = cpu_getPC() + ((socta)LIT24(t) - 0x1000000) * sizeof(tetra);
}

static void dec_toIII(sInstrArgs *iargs,tetra t,int fmt) {
	assert(fmt == I_I8I8I8);
	iargs->x = DST(t);
	iargs->y = SRC1(t);
	iargs->z = SRC2(t);
}

static void dec_toLA(sInstrArgs *iargs,tetra t,int fmt) {
	assert(fmt == I_I8RI8 || fmt == I_I8RR);
	iargs->x = DST(t);
	if(fmt == I_I8RI8)
		iargs->z = reg_get(SRC1(t)) + ZEXT(SRC2(t),8);
	else
		iargs->z = reg_get(SRC1(t)) + reg_get(SRC2(t));
}

static void dec_toMA(sInstrArgs *iargs,tetra t,int fmt) {
	assert(fmt == I_RRR || fmt == I_RRI8 || fmt == I_RF16 || fmt == I_RB16 || fmt == I_I8I16);
	iargs->x = DST(t);
	switch(fmt) {
		case I_RRI8:
			iargs->z = reg_get(SRC1(t)) + ZEXT(SRC2(t),8);
			break;
		case I_RRR:
			iargs->z = reg_get(SRC1(t)) + reg_get(SRC2(t));
			break;
		case I_RF16:
			iargs->z = cpu_getPC() + LIT16(t) * sizeof(tetra);
			break;
		case I_RB16:
			iargs->z = cpu_getPC() + ((socta)LIT16(t) - 0x10000) * sizeof(tetra);
			break;
		case I_I8I16:
			iargs->z = LIT16(t) * sizeof(tetra) + reg_getSpecial(rJ);
			break;
	}
}

static void dec_toSave(sInstrArgs *iargs,tetra t,int fmt) {
	assert(fmt == I_R0I8);
	if(SRC1(t) != 0)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	iargs->x = DST(t);
	iargs->z = ZEXT(SRC2(t),8);
}

static void dec_toUnsave(sInstrArgs *iargs,tetra t,int fmt) {
	assert(fmt == I_I80R);
	if(SRC1(t) != 0)
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	iargs->x = ZEXT(DST(t),8);
	// mmix puts it in y, so we do so as well
	iargs->y = reg_get(SRC2(t));
}

static void dec_toS(sInstrArgs *iargs,tetra t,int fmt) {
	assert(fmt == I_00I8 || fmt == I_I24);
	if(fmt == I_00I8 && (DST(t) || SRC1(t)))
		ex_throw(EX_DYNAMIC_TRAP,TRAP_BREAKS_RULES);
	if(fmt == I_00I8)
		iargs->y = SRC2(t);
	else
		iargs->y = LIT24(t);
}
