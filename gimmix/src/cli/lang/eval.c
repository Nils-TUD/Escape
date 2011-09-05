/**
 * $Id: eval.c 207 2011-05-14 11:55:06Z nasmussen $
 */

#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "common.h"
#include "cli/lang/eval.h"
#include "cli/cmds.h"
#include "core/register.h"
#include "core/mmu.h"
#include "core/bus.h"
#include "core/arith/int.h"
#include "core/arith/float.h"
#include "core/cpu.h"
#include "mmix/mem.h"
#include "mmix/io.h"
#include "exception.h"

typedef struct {
	int align;
	int width;
} sFetchOp;

static void eval_stmtList(const sASTNode *node,octa offset,bool *finished);
static void eval_cmdStmt(const sASTNode *node,octa offset,bool *finished);
static sCmdArg eval_intExp(const sASTNode *node,octa offset,bool *finished);
static sCmdArg eval_floatExp(const sASTNode *node,octa offset,bool *finished);
static sCmdArg eval_strExp(const sASTNode *node,octa offset,bool *finished);
static sCmdArg eval_pcExp(const sASTNode *node,octa offset,bool *finished);
static sCmdArg eval_binOpExp(const sASTNode *node,octa offset,bool *finished);
static sCmdArg eval_unOpExp(const sASTNode *node,octa offset,bool *finished);
static sCmdArg eval_fetchExp(const sASTNode *node,octa offset,bool *finished);
static sCmdArg eval_doFetchExp(int op,sASTNode *expr,octa offset,bool *finished);
static octa eval_vmReadOcta(octa addr);
static sCmdArg eval_getRangeExp(const sASTNode *node,octa offset,bool *finished);

static sFetchOp fetchOps[] = {
	/* FETCH_VMEM */	{1,	8},
	/* FETCH_VMEM1 */	{1,	1},
	/* FETCH_VMEM2 */	{2,	2},
	/* FETCH_VMEM4 */	{4,	4},
	/* FETCH_VMEM8 */	{8,	8},
	/* FETCH_PMEM */	{1,	8},
	/* FETCH_LREG */	{1,	1},
	/* FETCH_GREG */	{1,	1},
	/* FETCH_SREG */	{1,	1},
	/* FETCH_DREG */	{1,	1},
};

sCmdArg eval_get(const sASTNode *node,octa offset,bool *finished) {
	sCmdArg res;
	switch(node->type) {
		case AST_STMTLIST:
			eval_stmtList(node,offset,finished);
			break;
		case AST_CMDSTMT:
			eval_cmdStmt(node,offset,finished);
			break;
		case AST_INTEXP:
			res = eval_intExp(node,offset,finished);
			break;
		case AST_FLOATEXP:
			res = eval_floatExp(node,offset,finished);
			break;
		case AST_STREXP:
			res = eval_strExp(node,offset,finished);
			break;
		case AST_PCEXP:
			res = eval_pcExp(node,offset,finished);
			break;
		case AST_BINOPEXP:
			res = eval_binOpExp(node,offset,finished);
			break;
		case AST_UNOPEXP:
			res = eval_unOpExp(node,offset,finished);
			break;
		case AST_FETCHEXP:
			res = eval_fetchExp(node,offset,finished);
			break;
		case AST_RANGEEXP:
			res = eval_getRangeExp(node,offset,finished);
			break;
		default:
			assert(false);
			break;
	}
	return res;
}

static void eval_stmtList(const sASTNode *node,octa offset,bool *finished) {
	if(!node->d.stmtList.isEmpty) {
		eval_stmtList(node->d.stmtList.tail,offset,finished);
		eval_get(node->d.stmtList.head,offset,finished);
	}
}

static void eval_cmdStmt(const sASTNode *node,octa offset,bool *finished) {
	UNUSED(offset);
	UNUSED(finished);
	jmp_buf env;
	int ex = setjmp(env);
	if(ex == EX_NONE) {
		ex_push(&env);
		cmds_exec(node);
	}
	ex_pop();
}

static sCmdArg eval_intExp(const sASTNode *node,octa offset,bool *finished) {
	sCmdArg res = {ARGVAL_INT,ORG_EXPR,0,{0}};
	res.d.integer = node->d.intExp.value;
	if(offset == 0)
		*finished = false;
	return res;
}

static sCmdArg eval_floatExp(const sASTNode *node,octa offset,bool *finished) {
	sCmdArg res = {ARGVAL_FLOAT,ORG_EXPR,0,{0}};
	res.d.floating = node->d.floatExp.value;
	if(offset == 0)
		*finished = false;
	return res;
}

static sCmdArg eval_strExp(const sASTNode *node,octa offset,bool *finished) {
	sCmdArg res = {ARGVAL_STR,ORG_EXPR,0,{0}};
	res.d.str = node->d.strExp.value;
	if(offset == 0)
		*finished = false;
	return res;
}

static sCmdArg eval_pcExp(const sASTNode *node,octa offset,bool *finished) {
	UNUSED(node);
	sCmdArg res = {ARGVAL_INT,ORG_PC,0,{0}};
	res.d.integer = cpu_getPC();
	if(offset == 0)
		*finished = false;
	return res;
}

static sCmdArg eval_binOpExp(const sASTNode *node,octa offset,bool *finished) {
	sCmdArg res = {ARGVAL_INT,ORG_EXPR,0,{0}};
	sCmdArg left = eval_get(node->d.binOpExp.left,offset,finished);
	sCmdArg right = eval_get(node->d.binOpExp.right,offset,finished);
	if(left.type == ARGVAL_STR || right.type == ARGVAL_STR)
		cmds_throwEx("Unsupported operation!\n");

	if(left.type == ARGVAL_FLOAT || right.type == ARGVAL_FLOAT) {
		unsigned ex;
		if(left.type != ARGVAL_FLOAT) {
			left.d.integer = fl_floatit(left.d.integer,0,true,false,&ex);
			left.type = ARGVAL_FLOAT;
		}
		if(right.type != ARGVAL_FLOAT) {
			right.d.integer = fl_floatit(right.d.integer,0,true,false,&ex);
			right.type = ARGVAL_FLOAT;
		}

		res.type = ARGVAL_FLOAT;
		switch(node->d.binOpExp.op) {
			case BINOP_ADD:
				res.d.integer = fl_add(left.d.integer,right.d.integer,&ex);
				break;
			case BINOP_SUB:
				res.d.integer = fl_sub(left.d.integer,right.d.integer,&ex);
				break;
			case BINOP_MULU:
			case BINOP_MUL:
				res.d.integer = fl_mult(left.d.integer,right.d.integer,&ex);
				break;
			case BINOP_DIVU:
			case BINOP_DIV:
				res.d.integer = fl_divide(left.d.integer,right.d.integer,&ex);
				break;
			case BINOP_MODU:
			case BINOP_MOD:
				// 2500 makes sure that the remainder can be calculated in one step
				res.d.integer = fl_remstep(left.d.integer,right.d.integer,2500,&ex);
				break;

			case BINOP_AND:
			case BINOP_OR:
			case BINOP_XOR:
			case BINOP_SL:
			case BINOP_SAR:
			case BINOP_SR:
				cmds_throwEx("Unsupported operation!\n");
				break;

			default:
				assert(false);
				break;
		}
	}
	else {
		// note: some of the operations are the same on x86 and mmix. but some of them aren't.
		// therefore, we use the mmix-integer-arithmetic-functions if they are different.
		octa aux;
		switch(node->d.binOpExp.op) {
			case BINOP_ADD:
				res.d.integer = left.d.integer + right.d.integer;
				break;
			case BINOP_SUB:
				res.d.integer = left.d.integer - right.d.integer;
				break;
			case BINOP_MUL:
				res.d.integer = int_smult(left.d.integer,right.d.integer,&aux);
				break;
			case BINOP_MULU:
				res.d.integer = int_umult(left.d.integer,right.d.integer,&aux);
				break;
			case BINOP_DIV:
				res.d.integer = int_sdiv(left.d.integer,right.d.integer,&aux);
				break;
			case BINOP_DIVU:
				res.d.integer = int_udiv(0,left.d.integer,right.d.integer,&aux);
				break;
			case BINOP_MOD:
				int_sdiv(left.d.integer,right.d.integer,&aux);
				res.d.integer = aux;
				break;
			case BINOP_MODU:
				int_udiv(0,left.d.integer,right.d.integer,&aux);
				res.d.integer = aux;
				break;
			case BINOP_AND:
				res.d.integer = left.d.integer & right.d.integer;
				break;
			case BINOP_OR:
				res.d.integer = left.d.integer | right.d.integer;
				break;
			case BINOP_XOR:
				res.d.integer = left.d.integer ^ right.d.integer;
				break;
			case BINOP_SL:
				res.d.integer = int_shiftLeft(left.d.integer,right.d.integer);
				break;
			case BINOP_SAR:
				res.d.integer = int_shiftRightArith(left.d.integer,right.d.integer);
				break;
			case BINOP_SR:
				res.d.integer = int_shiftRightLog(left.d.integer,right.d.integer);
				break;
			default:
				assert(false);
				break;
		}
	}
	if(offset == 0)
		*finished = false;
	return res;
}

static sCmdArg eval_unOpExp(const sASTNode *node,octa offset,bool *finished) {
	sCmdArg res = {ARGVAL_INT,ORG_EXPR,0,{0}};
	sCmdArg expr = eval_get(node->d.unOpExp.expr,offset,finished);
	if(expr.type == ARGVAL_STR)
		cmds_throwEx("Unsupported operation!\n");

	switch(node->d.unOpExp.op) {
		case UNOP_NEG:
			if(expr.type == ARGVAL_FLOAT) {
				res.d.integer = expr.d.integer ^ MSB(64);
				res.type = ARGVAL_FLOAT;
			}
			else
				res.d.integer = -expr.d.integer;
			break;
		case UNOP_NOT:
			if(expr.type == ARGVAL_FLOAT)
				cmds_throwEx("Unsupported operation!\n");
			res.d.integer = ~expr.d.integer;
			break;
		default:
			assert(false);
			break;
	}
	if(offset == 0)
		*finished = false;
	return res;
}

static sCmdArg eval_fetchExp(const sASTNode *node,octa offset,bool *finished) {
	return eval_doFetchExp(node->d.fetchExp.op,node->d.fetchExp.expr,offset,finished);
}

static sCmdArg eval_doFetchExp(int op,sASTNode *expr,octa offset,bool *finished) {
	sCmdArg e = eval_get(expr,offset * fetchOps[op].width,finished);
	if(e.type != ARGVAL_INT)
		cmds_throwEx("Unsupported operation!\n");

	sCmdArg res;
	res.type = ARGVAL_INT;
	res.origin = ORG_EXPR;
	res.location = e.d.integer & ~(octa)(fetchOps[op].align - 1);
	res.d.integer = 0;
	// don't do anything when we're done because it may cause an error if we've crossed a "border"
	// e.g. sp[31..32]. the last call would be for sp[33]
	if(*finished)
		return res;

	switch(op) {
		case FETCH_VMEM:
			// be carefull with aligning here, therefore byte-access
			res.d.integer = eval_vmReadOcta(res.location);
			res.origin = ORG_VMEM;
			break;
		case FETCH_VMEM1:
			// use the uncached version here because we don't want to change the machine-state (TC)
			res.d.integer = mmu_readByte(res.location,0);
			res.origin = ORG_VMEM1;
			break;
		case FETCH_VMEM2:
			res.d.integer = mmu_readWyde(res.location,0);
			res.origin = ORG_VMEM2;
			break;
		case FETCH_VMEM4:
			res.d.integer = mmu_readTetra(res.location,0);
			res.origin = ORG_VMEM4;
			break;
		case FETCH_VMEM8:
			res.d.integer = mmu_readOcta(res.location,0);
			res.origin = ORG_VMEM8;
			break;
		case FETCH_PMEM:
			res.d.integer = bus_read(res.location,false);
			res.origin = ORG_PMEM;
			break;
		case FETCH_LREG:
			res.d.integer = reg_getLocal(res.location);
			res.origin = ORG_LREG;
			break;
		case FETCH_GREG:
			if(res.location < MAX(256 - GREG_NUM,MIN_GLOBAL))
				cmds_throwEx("The global register %Od does not exist\n",res.location);
			res.d.integer = reg_getGlobal(res.location);
			res.origin = ORG_GREG;
			break;
		case FETCH_SREG:
			if(res.location >= SPECIAL_NUM)
				cmds_throwEx("The special register %Od does not exist\n",res.location);
			res.d.integer = reg_getSpecial(res.location);
			res.origin = ORG_SREG;
			break;
		case FETCH_DREG:
			res.d.integer = reg_get(res.location);
			res.origin = ORG_DREG;
			break;
		default:
			assert(false);
			break;
	}
	if(offset == 0)
		*finished = false;
	return res;
}

static octa eval_vmReadOcta(octa addr) {
	octa res = 0;
	for(size_t i = 0; i < sizeof(octa); i++) {
		res |= (octa)mmu_readByte(addr,0) << ((8 - i - 1) * 8);
		addr++;
	}
	return res;
}

static sCmdArg eval_getRangeExp(const sASTNode *node,octa offset,bool *finished) {
	sCmdArg res = {ARGVAL_INT,ORG_EXPR,0,{0}};
	bool lFinished = false, rFinished = false;
	sCmdArg left = eval_get(node->d.rangeExp.left,offset,&lFinished);
	sCmdArg right = eval_get(node->d.rangeExp.right,offset,&rFinished);
	if(left.type != ARGVAL_INT || right.type != ARGVAL_INT)
		cmds_throwEx("Unsupported operation!\n");
	if(node->d.rangeExp.left->type == AST_RANGEEXP || node->d.rangeExp.right->type == AST_RANGEEXP)
		cmds_throwEx("Sorry, you can't nest ranges!\n");

	bool rangeFinished;
	switch(node->d.rangeExp.op) {
		case RANGE_FROMTO:
			// backwards
			if(left.d.integer > right.d.integer) {
				res.d.integer = left.d.integer - offset;
				rangeFinished = res.d.integer < right.d.integer;
			}
			// forward
			else {
				res.d.integer = left.d.integer + offset;
				// take care of overflow
				rangeFinished = offset > right.d.integer ||
						left.d.integer > right.d.integer - offset;
			}
			break;
		case RANGE_FROMCNT:
			res.d.integer = left.d.integer + offset;
			rangeFinished = offset > right.d.integer - 1;
			break;
		default:
			assert(false);
			break;
	}
	if(!rangeFinished)
		*finished = false;
	return res;
}
