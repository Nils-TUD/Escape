/**
 * $Id: idesc.c 227 2011-06-11 18:40:58Z nasmussen $
 */

#include "common.h"
#include "cli/lang/idesc.h"
#include "cli/lang/specreg.h"
#include "core/decoder.h"
#include "core/cpu.h"
#include "mmix/io.h"

static const sInstrDesc instrDescs[] = {
	/* 00 */ {"TRAP",	0,	5,	"%nx,%y,%z"},
	/* 01 */ {"FCMP",	0,	1,	"%l = %.y cmp %.z = %x"},
	/* 02 */ {"FUN",	0,	1,	"%l = [%.y <> %.z] = %x"},
	/* 03 */ {"FEQL",	0,	1,	"%l = [%.y == %.z] = %x"},
	/* 04 */ {"FADD",	0,	4,	"%l = %.y %(+%) %.z = %.x"},
	/* 05 */ {"FIX",	0,	4,	"%l = %e(fix%e) %.z = %x"},
	/* 06 */ {"FSUB",	0,	4,	"%l = %.y %(-%) %.z = %.x"},
	/* 07 */ {"FIXU",	0,	4,	"%l = %e(fix%e) %.z = %#x"},
	/* 08 */ {"FLOT",	0,	4,	"%l = %e(flot%e) %z = %.x"},
	/* 09 */ {"FLOTI",	0,	4,	"%l = %e(flot%e) %z = %.x"},
	/* 0A */ {"FLOTU",	0,	4,	"%l = %e(flot%e) %#z = %.x"},
	/* 0B */ {"FLOTUI",	0,	4,	"%l = %e(flot%e) %z = %.x"},
	/* 0C */ {"SFLOT",	0,	4,	"%l = %e(sflot%e) %z = %.x"},
	/* 0D */ {"SFLOTI",	0,	4,	"%l = %e(sflot%e) %z = %.x"},
	/* 0E */ {"SFLOTU",	0,	4,	"%l = %e(sflot%e) %#z = %.x"},
	/* 0F */ {"SFLOTUI",0,	4,	"%l = %e(sflot%e) %z = %.x"},
	/* 10 */ {"FMUL",	0,	4,	"%l = %.y %(*%) %.z = %.x"},
	/* 11 */ {"FCMPE",	0,	4,	"%l = %.y cmp %.z (%.E) = %x"},
	/* 12 */ {"FUNE",	0,	1,	"%l = [%.y <> %.z (%.E)] = %x"},
	/* 13 */ {"FEQLE",	0,	4,	"%l = [%.y == %.z (%.E)] = %x"},
	/* 14 */ {"FDIV",	0,	40,	"%l = %.y %(/%) %.z = %.x"},
	/* 15 */ {"FSQRT",	0,	40,	"%l = %e(sqrt%e) %.z = %.x"},
	/* 16 */ {"FREM",	0,	4,	"%l = %.y %(rem%) %.z = %.x"},
	/* 17 */ {"FINT",	0,	4,	"%l = %e(int%e) %.z = %.x"},
	/* 18 */ {"MUL",	0,	10,	"%l = %y * %z = %x"},
	/* 19 */ {"MULI",	0,	10,	"%l = %y * %z = %x"},
	/* 1A */ {"MULU",	0,	10,	"%l = %#y * %#z = %#x, rH=%#H"},
	/* 1B */ {"MULUI",	0,	10,	"%l = %#y * %z = %#x, rH=%#H"},
	/* 1C */ {"DIV",	0,	60,	"%l = %y / %z = %x, rR=%R"},
	/* 1D */ {"DIVI",	0,	60,	"%l = %y / %z = %x, rR=%R"},
	/* 1E */ {"DIVU",	0,	60,	"%l = %#D:%0hy / %#z = %#x, rR=%#R"},
	/* 1F */ {"DIVUI",	0,	60,	"%l = %#D:%0hy / %z = %#x, rR=%#R"},
	/* 20 */ {"ADD",	0,	1,	"%l = %y + %z = %x"},
	/* 21 */ {"ADDI",	0,	1,	"%l = %y + %z = %x"},
	/* 22 */ {"ADDU",	0,	1,	"%l = %#y + %#z = %#x"},
	/* 23 */ {"ADDUI",	0,	1,	"%l = %#y + %z = %#x"},
	/* 24 */ {"SUB",	0,	1,	"%l = %y - %z = %x"},
	/* 25 */ {"SUBI",	0,	1,	"%l = %y - %z = %x"},
	/* 26 */ {"SUBU",	0,	1,	"%l = %#y - %#z = %#x"},
	/* 27 */ {"SUBUI",	0,	1,	"%l = %#y - %z = %#x"},
	/* 28 */ {"2ADDU",	0,	1,	"%l = (%#y << 1) + %#z = %#x"},
	/* 29 */ {"2ADDUI",	0,	1,	"%l = (%#y << 1) + %z = %#x"},
	/* 2A */ {"4ADDU",	0,	1,	"%l = (%#y << 2) + %#z = %#x"},
	/* 2B */ {"4ADDUI",	0,	1,	"%l = (%#y << 2) + %z = %#x"},
	/* 2C */ {"8ADDU",	0,	1,	"%l = (%#y << 3) + %#z = %#x"},
	/* 2D */ {"8ADDUI",	0,	1,	"%l = (%#y << 3) + %z = %#x"},
	/* 2E */ {"16ADDU",	0,	1,	"%l = (%#y << 4) + %#z = %#x"},
	/* 2F */ {"16ADDUI",0,	1,	"%l = (%#y << 4) + %z = %#x"},
	/* 30 */ {"CMP",	0,	1,	"%l = %y cmp %z = %x"},
	/* 31 */ {"CMPI",	0,	1,	"%l = %y cmp %z = %x"},
	/* 32 */ {"CMPU",	0,	1,	"%l = %#y cmp %#z = %x"},
	/* 33 */ {"CMPUI",	0,	1,	"%l = %#y cmp %z = %x"},
	/* 34 */ {"NEG",	0,	1,	"%l = %y - %z = %x"},
	/* 35 */ {"NEGI",	0,	1,	"%l = %y - %z = %x"},
	/* 36 */ {"NEGU",	0,	1,	"%l = %y - %#z = %#x"},
	/* 37 */ {"NEGUI",	0,	1,	"%l = %y - %z = %#x"},
	/* 38 */ {"SL",		0,	1,	"%l = %y << %#z = %x"},
	/* 39 */ {"SLI",	0,	1,	"%l = %y << %z = %x"},
	/* 3A */ {"SLU",	0,	1,	"%l = %#y << %#z = %#x"},
	/* 3B */ {"SLUI",	0,	1,	"%l = %#y << %z = %#x"},
	/* 3C */ {"SR",		0,	1,	"%l = %y >> %#z = %x"},
	/* 3D */ {"SRI",	0,	1,	"%l = %y >> %z = %x"},
	/* 3E */ {"SRU",	0,	1,	"%l = %#y >>> %#z = %#x"},
	/* 3F */ {"SRUI",	0,	1,	"%l = %#y >>> %z = %#x"},
	/* 40 */ {"BN",		0,	1,	"%nx < 0? %b"},
	/* 41 */ {"BNB",	0,	1,	"%nx < 0? %b"},
	/* 42 */ {"BZ",		0,	1,	"%nx == 0? %b"},
	/* 43 */ {"BZB",	0,	1,	"%nx == 0? %b"},
	/* 44 */ {"BP",		0,	1,	"%nx > 0? %b"},
	/* 45 */ {"BPB",	0,	1,	"%nx > 0? %b"},
	/* 46 */ {"BOD",	0,	1,	"%nx odd? %b"},
	/* 47 */ {"BODB",	0,	1,	"%nx odd? %b"},
	/* 48 */ {"BNN",	0,	1,	"%nx >= 0? %b"},
	/* 49 */ {"BNNB",	0,	1,	"%nx >= 0? %b"},
	/* 4A */ {"BNZ",	0,	1,	"%nx != 0? %b"},
	/* 4B */ {"BNZB",	0,	1,	"%nx != 0? %b"},
	/* 4C */ {"BNP",	0,	1,	"%nx <= 0? %b"},
	/* 4D */ {"BNPB",	0,	1,	"%nx <= 0? %b"},
	/* 4E */ {"BEV",	0,	1,	"%nx even? %b"},
	/* 4F */ {"BEVB",	0,	1,	"%nx even? %b"},
	/* 50 */ {"PBN",	0,	1,	"%nx < 0? %b"},
	/* 51 */ {"PBNB",	0,	1,	"%nx < 0? %b"},
	/* 52 */ {"PBZ",	0,	1,	"%nx == 0? %b"},
	/* 53 */ {"PBZB",	0,	1,	"%nx == 0? %b"},
	/* 54 */ {"PBP",	0,	1,	"%nx > 0? %b"},
	/* 55 */ {"PBPB",	0,	1,	"%nx > 0? %b"},
	/* 56 */ {"PBOD",	0,	1,	"%nx odd? %b"},
	/* 57 */ {"PBODB",	0,	1,	"%nx odd? %b"},
	/* 58 */ {"PBNN",	0,	1,	"%nx >= 0? %b"},
	/* 59 */ {"PBNNB",	0,	1,	"%nx >= 0? %b"},
	/* 5A */ {"PBNZ",	0,	1,	"%nx != 0? %b"},
	/* 5B */ {"PBNZB",	0,	1,	"%nx != 0? %b"},
	/* 5C */ {"PBNP",	0,	1,	"%nx <= 0? %b"},
	/* 5D */ {"PBNPB",	0,	1,	"%nx <= 0? %b"},
	/* 5E */ {"PBEV",	0,	1,	"%nx even? %b"},
	/* 5F */ {"PBEVB",	0,	1,	"%nx even? %b"},
	/* 60 */ {"CSN",	0,	1,	"%l = %y < 0? %z = %x"},
	/* 61 */ {"CSNI",	0,	1,	"%l = %y < 0? %z = %x"},
	/* 62 */ {"CSZ",	0,	1,	"%l = %y == 0? %z = %x"},
	/* 63 */ {"CSZI",	0,	1,	"%l = %y == 0? %z = %x"},
	/* 64 */ {"CSP",	0,	1,	"%l = %y > 0? %z = %x"},
	/* 65 */ {"CSPI",	0,	1,	"%l = %y > 0? %z = %x"},
	/* 66 */ {"CSOD",	0,	1,	"%l = %y odd? %z = %x"},
	/* 67 */ {"CSODI",	0,	1,	"%l = %y odd? %z = %x"},
	/* 68 */ {"CSNN",	0,	1,	"%l = %y >= 0? %z = %x"},
	/* 69 */ {"CSNNI",	0,	1,	"%l = %y >= 0? %z = %x"},
	/* 6A */ {"CSNZ",	0,	1,	"%l = %y != 0? %z = %x"},
	/* 6B */ {"CSNZI",	0,	1,	"%l = %y != 0? %z = %x"},
	/* 6C */ {"CSNP",	0,	1,	"%l = %y <= 0? %z = %x"},
	/* 6D */ {"CSNPI",	0,	1,	"%l = %y <= 0? %z = %x"},
	/* 6E */ {"CSEV",	0,	1,	"%l = %y even? %z = %x"},
	/* 6F */ {"CSEVI",	0,	1,	"%l = %y even? %z = %x"},
	/* 70 */ {"ZSN",	0,	1,	"%l = %y < 0? %z: 0 = %x"},
	/* 71 */ {"ZSNI",	0,	1,	"%l = %y < 0? %z: 0 = %x"},
	/* 72 */ {"ZSZ",	0,	1,	"%l = %y == 0? %z: 0 = %x"},
	/* 73 */ {"ZSZI",	0,	1,	"%l = %y == 0? %z: 0 = %x"},
	/* 74 */ {"ZSP",	0,	1,	"%l = %y > 0? %z: 0 = %x"},
	/* 75 */ {"ZSPI",	0,	1,	"%l = %y > 0? %z: 0 = %x"},
	/* 76 */ {"ZSOD",	0,	1,	"%l = %y odd? %z: 0 = %x"},
	/* 77 */ {"ZSODI",	0,	1,	"%l = %y odd? %z: 0 = %x"},
	/* 78 */ {"ZSNN",	0,	1,	"%l = %y >= 0? %z: 0 = %x"},
	/* 79 */ {"ZSNNI",	0,	1,	"%l = %y >= 0? %z: 0 = %x"},
	/* 7A */ {"ZSNZ",	0,	1,	"%l = %y != 0? %z: 0 = %x"},
	/* 7B */ {"ZSNZI",	0,	1,	"%l = %y != 0? %z: 0 = %x"},
	/* 7C */ {"ZSNP",	0,	1,	"%l = %y <= 0? %z: 0 = %x"},
	/* 7D */ {"ZSNPI",	0,	1,	"%l = %y <= 0? %z: 0 = %x"},
	/* 7E */ {"ZSEV",	0,	1,	"%l = %y even? %z: 0 = %x"},
	/* 7F */ {"ZSEVI",	0,	1,	"%l = %y even? %z: 0 = %x"},
	/* 80 */ {"LDB",	1,	1,	"%l = M1[%#y] = %x"},
	/* 81 */ {"LDBI",	1,	1,	"%l = M1[%#y] = %x"},
	/* 82 */ {"LDBU",	1,	1,	"%l = M1[%#y] = %#x"},
	/* 83 */ {"LDBUI",	1,	1,	"%l = M1[%#y] = %#x"},
	/* 84 */ {"LDW",	1,	1,	"%l = M2[%#y] = %x"},
	/* 85 */ {"LDWI",	1,	1,	"%l = M2[%#y] = %x"},
	/* 86 */ {"LDWU",	1,	1,	"%l = M2[%#y] = %#x"},
	/* 87 */ {"LDWUI",	1,	1,	"%l = M2[%#y] = %#x"},
	/* 88 */ {"LDT",	1,	1,	"%l = M4[%#y] = %x"},
	/* 89 */ {"LDTI",	1,	1,	"%l = M4[%#y] = %x"},
	/* 8A */ {"LDTU",	1,	1,	"%l = M4[%#y] = %#x"},
	/* 8B */ {"LDTUI",	1,	1,	"%l = M4[%#y] = %#x"},
	/* 8C */ {"LDO",	1,	1,	"%l = M8[%#y] = %x"},
	/* 8D */ {"LDOI",	1,	1,	"%l = M8[%#y] = %x"},
	/* 8E */ {"LDOU",	1,	1,	"%l = M8[%#y] = %#x"},
	/* 8F */ {"LDOUI",	1,	1,	"%l = M8[%#y] = %#x"},
	/* 90 */ {"LDSF",	1,	1,	"%l = M4[%#y] = %.x"},
	/* 91 */ {"LDSFI",	1,	1,	"%l = M4[%#y] = %.x"},
	/* 92 */ {"LDHT",	1,	1,	"%l = M4[%#y] << 32 = %#x"},
	/* 93 */ {"LDHTI",	1,	1,	"%l = M4[%#y] << 32 = %#x"},
	/* 94 */ {"CSWAP",	2,	2,	"%l = [M8[%#y + %#z] == %#P] = %x, %p"},
	/* 95 */ {"CSWAPI",	2,	2,	"%l = [M8[%#y + %#z] == %#P] = %x, %p"},
	/* 96 */ {"LDUNC",	1,	1,	"%l = M8[%#y] = %#x"},
	/* 97 */ {"LDUNCI",	1,	1,	"%l = M8[%#y] = %#x"},
	/* 98 */ {"LDVTS",	0,	1,	"%l = %#x, TC[%#y+%#z]"},
	/* 99 */ {"LDVTSI",	0,	1,	"%l = %#x, TC[%#y+%#z]"},
	// mems for PRE* are different to MMIX-SIM because we actually do something
	/* 9A */ {"PRELD",	1,	1,	"[%#z : %#nx]"},
	/* 9B */ {"PRELDI",	1,	1,	"[%#z : %#nx]"},
	/* 9C */ {"PREGO",	1,	1,	"[%#z : %#nx]"},
	/* 9D */ {"PREGOI",	1,	1,	"[%#z : %#nx]"},
	/* 9E */ {"GO",		0,	3,	"%l = %#x, -> %#y+%#z"},
	/* 9F */ {"GOI",	0,	3,	"%l = %#x, -> %#y+%#z"},
	/* A0 */ {"STB",	1,	1,	"M1[%#y] = %nx"},
	/* A1 */ {"STBI",	1,	1,	"M1[%#y] = %nx"},
	/* A2 */ {"STBU",	1,	1,	"M1[%#y] = %#nx"},
	/* A3 */ {"STBUI",	1,	1,	"M1[%#y] = %#nx"},
	/* A4 */ {"STW",	1,	1,	"M2[%#y] = %nx"},
	/* A5 */ {"STWI",	1,	1,	"M2[%#y] = %nx"},
	/* A6 */ {"STWU",	1,	1,	"M2[%#y] = %#nx"},
	/* A7 */ {"STWUI",	1,	1,	"M2[%#y] = %#nx"},
	/* A8 */ {"STT",	1,	1,	"M4[%#y] = %nx"},
	/* A9 */ {"STTI",	1,	1,	"M4[%#y] = %nx"},
	/* AA */ {"STTU",	1,	1,	"M4[%#y] = %#nx"},
	/* AB */ {"STTUI",	1,	1,	"M4[%#y] = %#nx"},
	/* AC */ {"STO",	1,	1,	"M8[%#y] = %nx"},
	/* AD */ {"STOI",	1,	1,	"M8[%#y] = %nx"},
	/* AE */ {"STOU",	1,	1,	"M8[%#y] = %#nx"},
	/* AF */ {"STOUI",	1,	1,	"M8[%#y] = %#nx"},
	/* B0 */ {"STSF",	1,	1,	"M4[%#y] = %n.x"},
	/* B1 */ {"STSFI",	1,	1,	"M4[%#y] = %n.x"},
	/* B2 */ {"STHT",	1,	1,	"M4[%#y] = %#nx>>32"},
	/* B3 */ {"STHTI",	1,	1,	"M4[%#y] = %#nx>>32"},
	/* B4 */ {"STCO",	1,	1,	"M8[%#y] = %nx"},
	/* B5 */ {"STCOI",	1,	1,	"M8[%#y] = %nx"},
	/* B6 */ {"STUNC",	1,	1,	"M8[%#y] = %#nx"},
	/* B7 */ {"STUNCI",	1,	1,	"M8[%#y] = %#nx"},
	// mems for SYNCD*, PREST* and SYNCID* are different to MMIX-SIM because we actually do something
	/* B8 */ {"SYNCD",	1,	1,	"[%#z : %#nx]"},
	/* B9 */ {"SYNCDI",	1,	1,	"[%#z : %#nx]"},
	/* BA */ {"PREST",	1,	1,	"[%#z : %#nx]"},
	/* BB */ {"PRESTI",	1,	1,	"[%#z : %#nx]"},
	/* BC */ {"SYNCID",	1,	1,	"[%#z : %#nx]"},
	/* BD */ {"SYNCIDI",1,	1,	"[%#z : %#nx]"},
	/* BE */ {"PUSHGO",	0,	3,	"(%nx) rO=%#O, rL=%L, rJ=%#J -> %#z"},
	/* BF */ {"PUSHGOI",0,	3,	"(%nx) rO=%#O, rL=%L, rJ=%#J -> %#z"},
	/* C0 */ {"OR",		0,	1,	"%l = %#y | %#z = %#x"},
	/* C1 */ {"ORI",	0,	1,	"%l = %#y | %z = %#x"},
	/* C2 */ {"ORN",	0,	1,	"%l = %#y | ~%#z = %#x"},
	/* C3 */ {"ORNI",	0,	1,	"%l = %#y | ~%z = %#x"},
	/* C4 */ {"NOR",	0,	1,	"%l = ~(%#y | %#z) = %#x"},
	/* C5 */ {"NORI",	0,	1,	"%l = ~(%#y | %z) = %#x"},
	/* C6 */ {"XOR",	0,	1,	"%l = %#y ^ %#z = %#x"},
	/* C7 */ {"XORI",	0,	1,	"%l = %#y ^ %z = %#x"},
	/* C8 */ {"AND",	0,	1,	"%l = %#y & %#z = %#x"},
	/* C9 */ {"ANDI",	0,	1,	"%l = %#y & %z = %#x"},
	/* CA */ {"ANDN",	0,	1,	"%l = %#y \\ %#z = %#x"},
	/* CB */ {"ANDNI",	0,	1,	"%l = %#y \\ %z = %#x"},
	/* CC */ {"NAND",	0,	1,	"%l = ~(%#y & %#z) = %#x"},
	/* CD */ {"NANDI",	0,	1,	"%l = ~(%#y & %z) = %#x"},
	/* CE */ {"NXOR",	0,	1,	"%l = ~(%#y ^ %#z) = %#x"},
	/* CF */ {"NXORI",	0,	1,	"%l = ~(%#y ^ %z) = %#x"},
	/* D0 */ {"BDIF",	0,	1,	"%l = %#y bdif %#z = %#x"},
	/* D1 */ {"BDIFI",	0,	1,	"%l = %#y bdif %z = %#x"},
	/* D2 */ {"WDIF",	0,	1,	"%l = %#y wdif %#z = %#x"},
	/* D3 */ {"WDIFI",	0,	1,	"%l = %#y wdif %z = %#x"},
	/* D4 */ {"TDIF",	0,	1,	"%l = %#y tdif %#z = %#x"},
	/* D5 */ {"TDIFI",	0,	1,	"%l = %#y tdif %z = %#x"},
	/* D6 */ {"ODIF",	0,	1,	"%l = %#y odif %#z = %#x"},
	/* D7 */ {"ODIFI",	0,	1,	"%l = %#y odif %z = %#x"},
	/* D8 */ {"MUX",	0,	1,	"%l = %#M ? %#y : %#z = %#x"},
	/* D9 */ {"MUXI",	0,	1,	"%l = %#M ? %#y : %z = %#x"},
	/* DA */ {"SADD",	0,	1,	"%l = bits(%#y \\ %#z) = %x"},
	/* DB */ {"SADDI",	0,	1,	"%l = bits(%#y \\ %z) = %x"},
	/* DC */ {"MOR",	0,	1,	"%l = %#y mor %#z = %#x"},
	/* DD */ {"MORI",	0,	1,	"%l = %#y mor %z = %#x"},
	/* DE */ {"MXOR",	0,	1,	"%l = %#y mxor %#z = %#x"},
	/* DF */ {"MXORI",	0,	1,	"%l = %#y mxor %z = %#x"},
	/* E0 */ {"SETH",	0,	1,	"%l = #%0hwz000000000000"},
	/* E1 */ {"SETMH",	0,	1,	"%l = #0000%0hwz00000000"},
	/* E2 */ {"SETML",	0,	1,	"%l = #00000000%0hwz0000"},
	/* E3 */ {"SETL",	0,	1,	"%l = #000000000000%0hwz"},
	/* E4 */ {"INCH",	0,	1,	"%l = %#y + #%0hwz000000000000 = %#x"},
	/* E5 */ {"INCMH",	0,	1,	"%l = %#y + #0000%0hwz00000000 = %#x"},
	/* E6 */ {"INCML",	0,	1,	"%l = %#y + #00000000%0hwz0000 = %#x"},
	/* E7 */ {"INCL",	0,	1,	"%l = %#y + #000000000000%0hwz = %#x"},
	/* E8 */ {"ORH",	0,	1,	"%l = %#y | #%0hwz000000000000 = %#x"},
	/* E9 */ {"ORMH",	0,	1,	"%l = %#y | #0000%0hwz00000000 = %#x"},
	/* EA */ {"ORML",	0,	1,	"%l = %#y | #00000000%0hwz0000 = %#x"},
	/* EB */ {"ORL",	0,	1,	"%l = %#y | #000000000000%0hwz = %#x"},
	/* EC */ {"ANDNH",	0,	1,	"%l = %#y \\ #%0hwz000000000000 = %#x"},
	/* ED */ {"ANDNMH",	0,	1,	"%l = %#y \\ #0000%0hwz00000000 = %#x"},
	/* EE */ {"ANDNML",	0,	1,	"%l = %#y \\ #00000000%0hwz0000 = %#x"},
	/* EF */ {"ANDNL",	0,	1,	"%l = %#y \\ #000000000000%0hwz = %#x"},
	/* F0 */ {"JMP",	0,	1,	"-> %#z"},
	/* F1 */ {"JMPB",	0,	1,	"-> %#z"},
	/* F2 */ {"PUSHJ",	0,	1,	"(%nx) rO=%#O, rL=%L, rJ=%#J -> %#z"},
	/* F3 */ {"PUSHJB",	0,	1,	"(%nx) rO=%#O, rL=%L, rJ=%#J -> %#z"},
	/* F4 */ {"GETA",	0,	1,	"%l = %#y"},
	/* F5 */ {"GETAB",	0,	1,	"%l = %#y"},
	/* F6 */ {"PUT",	0,	1,	"%sx = %#*"},
	/* F7 */ {"PUTI",	0,	1,	"%sx = %#*"},
	/* F8 */ {"POP",	0,	3,	"(%nx) rO=%#O, rL=%L -> %#z"},
	/* F9 */ {"RESUME",	0,	5,	"(%y)"},
	/* FA */ {"SAVE",	20,	1,	"(%z) %l = %#x"},
	/* FB */ {"UNSAVE",	20,	1,	"(%nx) %#z, rS=%#S, rO=%#O, rG=%G, rL=%L"},
	/* FC */ {"SYNC",	0,	1,	"%y"},
	/* FD */ {"SWYM",	0,	1,	"NOP"},
	/* FE */ {"GET",	0,	1,	"%l = %sz = %#x"},
	/* FF */ {"TRIP",	0,	5,	"%#nx %#y %#z"},
};

const sInstrDesc *idesc_get(int no) {
	return instrDescs + no;
}

const char *idesc_disasm(octa loc,tetra instr,bool absoluteJumps) {
	static char buf[32];
	const sSpecialReg *sreg = NULL;
	const sInstr *ip = dec_getInstr(OPCODE(instr));
	const sInstrDesc *desc = idesc_get(OPCODE(instr));
	const size_t n = sizeof(buf);
	byte dst = DST(instr);
	byte src1 = SRC1(instr);
	byte src2 = SRC2(instr);
	wyde lit16 = LIT16(instr);
	tetra lit24 = LIT24(instr);
	switch(ip->format) {
		case I_I8I8I8:
			msnprintf(buf,n,"%-7s #%02BX,#%02BX,#%02BX",desc->name,dst,src1,src2);
			break;
		case I_RRR:
			msnprintf(buf,n,"%-7s $%Bd,$%Bd,$%Bd",desc->name,dst,src1,src2);
			break;
		case I_RRI8:
			msnprintf(buf,n,"%-7s $%Bd,$%Bd,#%02BX",desc->name,dst,src1,src2);
			break;
		case I_RI8R:
			msnprintf(buf,n,"%-7s $%Bd,#%02BX,$%Bd",desc->name,dst,src1,src2);
			break;
		case I_RI8I8:
			msnprintf(buf,n,"%-7s $%Bd,#%02BX,#%02BX",desc->name,dst,src1,src2);
			break;
		case I_I8RR:
			msnprintf(buf,n,"%-7s #%02BX,$%Bd,$%Bd",desc->name,dst,src1,src2);
			break;
		case I_I8RI8:
			msnprintf(buf,n,"%-7s #%02BX,$%Bd,#%02BX",desc->name,dst,src1,src2);
			break;
		case I_RI16:
			msnprintf(buf,n,"%-7s $%Td,#%04TX",desc->name,dst,lit16);
			break;
		case I_I8I16:
			msnprintf(buf,n,"%-7s #%02BX,#%04WX",desc->name,dst,lit16);
			break;
		case I_RF16:
			if(absoluteJumps) {
				loc += LIT16(instr) << 2;
				msnprintf(buf,n,"%-7s $%Bd,#%016OX",desc->name,dst,loc);
			}
			else
				msnprintf(buf,n,"%-7s $%Bd,+#%05TX",desc->name,dst,(tetra)lit16 << 2);
			break;
		case I_RB16:
			if(absoluteJumps) {
				loc += ((signed)LIT16(instr) - 0x10000) << 2;
				msnprintf(buf,n,"%-7s $%Bd,#%016OX",desc->name,dst,loc);
			}
			else {
				tetra off = ((signed)LIT16(instr) - 0x10000) << 2;
				msnprintf(buf,n,"%-7s $%Bd,-#%05TX",desc->name,dst,-off);
			}
			break;
		case I_F24:
			if(absoluteJumps) {
				loc += LIT24(instr) << 2;
				msnprintf(buf,n,"%-7s #%016OX",desc->name,loc);
			}
			else
				msnprintf(buf,n,"%-7s +#%07TX",desc->name,lit24 << 2);
			break;
		case I_B24:
			if(absoluteJumps) {
				loc += ((signed)LIT24(instr) - 0x1000000) << 2;
				msnprintf(buf,n,"%-7s #%016OX",desc->name,loc);
			}
			else {
				tetra off = ((signed)LIT24(instr) - 0x1000000) << 2;
				msnprintf(buf,n,"%-7s -#%07TX",desc->name,-off);
			}
			break;
		case I_R0S8:
			sreg = sreg_get(SRC2(instr));
			if(sreg)
				msnprintf(buf,n,"%-7s $%Bd,%s",desc->name,dst,sreg->name);
			else
				msnprintf(buf,n,"%-7s $%Bd,#%02BX",desc->name,dst,src2);
			break;
		case I_S80R:
			sreg = sreg_get(DST(instr));
			if(sreg)
				msnprintf(buf,n,"%-7s %s,$%Bd",desc->name,sreg->name,src2);
			else
				msnprintf(buf,n,"%-7s #%02BX,$%Bd",desc->name,dst,src2);
			break;
		case I_S80I8:
			sreg = sreg_get(DST(instr));
			if(sreg)
				msnprintf(buf,n,"%-7s %s,#%02BX",desc->name,sreg->name,src2);
			else
				msnprintf(buf,n,"%-7s #%02BX,#%02BX",desc->name,dst,src2);
			break;
		case I_R0I8:
			msnprintf(buf,n,"%-7s $%d,#%02BX",desc->name,dst,src2);
			break;
		case I_00I8:
			msnprintf(buf,n,"%-7s #%02BX",desc->name,src2);
			break;
		case I_I80R:
			msnprintf(buf,n,"%-7s #%02BX,$%d",desc->name,dst,src2);
			break;
		case I_I24:
			msnprintf(buf,n,"%-7s #%06TX",desc->name,lit24);
			break;
		default:
			msnprintf(buf,n,"???");
			break;
	}
	return buf;
}
