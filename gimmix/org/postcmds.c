/**
 * $Id: postcmds.c 212 2011-05-15 19:24:23Z nasmussen $
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define rO				10	/* register stack offset */
#define rS				11
#define rG				19	/* global threshold register */
#define rL				20	/* local threshold register */
typedef unsigned int tetra;
typedef struct {tetra h,l;} octa;
typedef unsigned char byte;
typedef struct {
  tetra tet; /* the tetrabyte of simulated memory */
  tetra freq; /* the number of times it was obeyed as an instruction */
  unsigned char bkpt; /* breakpoint information for this tetrabyte */
  unsigned char file_no; /* source file number, if known */
  unsigned short line_no; /* source line number, if known */
} mem_tetra;
extern octa oplus(octa x,octa y);
extern octa shift_left(octa y,int s);
extern octa shift_right(octa y,int s,int u);
extern void print_hex(octa o);
extern int lring_size;
extern int lring_mask;
extern octa zero_octa;

#ifdef MMMIX
/* for MMMIX */
typedef enum {false, true, wow} bool; /* slightly extended booleans */
typedef struct {
  octa o;
  struct specnode_struct *p;
} spec;
typedef struct specnode_struct {
  octa o;
  bool known;
  octa addr;
  struct specnode_struct *up,*down;
} specnode;
typedef struct coroutine_struct {
 char *name; /* symbolic identification of a coroutine */
 int stage; /* its rank */
 struct coroutine_struct *next; /* its successor */
 struct coroutine_struct **lockloc; /* what it might be locking */
 struct control_struct *ctl; /* its data */
} coroutine;
typedef struct control_struct {
 octa loc; /* virtual address where an instruction originated */
 int op;
 unsigned char xx,yy,zz; /* the original instruction bytes */
 spec y,z,b,ra; /* inputs */
 specnode x,a,go,rl; /* outputs */
 coroutine *owner; /* a coroutine whose |ctl| this is */
 int i; /* internal opcode */
 int state; /* internal mindset */
 bool usage; /* should rU be increased? */
 bool need_b; /* should we stall until |b.p==NULL|? */
 bool need_ra; /* should we stall until |ra.p==NULL|? */
 bool ren_x; /* does |x| correspond to a rename register? */
 bool mem_x; /* does |x| correspond to a memory write? */
 bool ren_a; /* does |a| correspond to a rename register? */
 bool set_l; /* does |rl| correspond to a new value of rL? */
 bool interim; /* does this instruction need to be reissued on interrupt? */
 unsigned int arith_exc; /* arithmetic exceptions for event bits of rA */
 unsigned int hist; /* history bits for use in branch prediction */
 int denin,denout; /* execution time penalties for subnormal handling */
 octa cur_O,cur_S; /* speculative rO and rS before this instruction */
 unsigned int interrupt; /* does this instruction generate an interrupt? */
 void *ptr_a, *ptr_b, *ptr_c; /* generic pointers for miscellaneous use */
} control;
extern octa mem_read(octa addr);
extern specnode g[256];
extern specnode *l; /* local registers */
extern control *hot, *cool; /* front and rear of the reorder buffer */
extern octa cool_O,cool_S; /* values of rO, rS before the |cool| instruction */
#define LREG(x)		(l[(x)].o)
static octa GREG(int x) {
	if(x == rO || x == rS) {
		if(hot == cool) {
			/* pipeline empty */
			g[rO].o = shift_left(cool_O,3);
			g[rS].o = shift_left(cool_S,3);
		}
		else {
			g[rO].o = shift_left(hot->cur_O,3);
			g[rS].o = shift_left(hot->cur_S,3);
		}
	}
	return g[x].o;
}
static octa readMem(octa addr) {
	return mem_read(addr);
}
#else
/* for MMIX */
extern mem_tetra* mem_find(octa addr);
extern octa g[256]; /* global registers */
extern octa *l; /* local registers */
#define LREG(x)		(l[(x)])
#define	GREG(x)		(g[(x)])
static octa readMem(octa addr) {
	mem_tetra *ll = mem_find(addr);
	octa o;
	o.h = ll->tet;
	o.l = (ll+1)->tet;
	return o;
}
#endif

#define MAX_POST_CMDS	16

#define TYPE_NULL		0
#define TYPE_DREGS		1
#define TYPE_MEM		2

typedef struct {
	int type;
	union {
		struct {
			int from;
			int to;
		} regs;
		struct {
			octa from;
			octa to;
		} mem;
	} data;
} sPostCmd;

static void postcmds_exec(void);
static void postcmds_dregs(sPostCmd *cmd);
static void postcmds_mem(sPostCmd *cmd);
static void postcmds_parse(const char *arg);
static octa postcmds_scano(const char *str,int *total);

static sPostCmd cmds[MAX_POST_CMDS + 1];

void postcmds_perform(int argc,char **argv) {
	int i;
	for(i = 1; i < argc; i++) {
		if(strncmp(argv[i],"--postcmds=",sizeof("--postcmds=") - 1) == 0) {
			postcmds_parse(argv[i] + sizeof("--postcmds=") - 1);
			postcmds_exec();
			break;
		}
	}
}

static void postcmds_exec(void) {
	int i;
	for(i = 0; cmds[i].type != TYPE_NULL; i++) {
		switch(cmds[i].type) {
			case TYPE_DREGS:
				postcmds_dregs(cmds + i);
				break;
			case TYPE_MEM:
				postcmds_mem(cmds + i);
				break;
		}
	}
}

static void postcmds_dregs(sPostCmd *cmd) {
	octa regO = shift_right(GREG(rO),3,0);
	octa regG = GREG(rG);
	int j,O = regO.l;
	for(j = cmd->data.regs.from; j <= cmd->data.regs.to; j++) {
		octa val;
		if(j >= regG.l)
			val = GREG(j);
		else
			val = j < GREG(rL).l ? LREG((O + j) & lring_mask) : zero_octa;
		printf("$%d: %08X%08X\n",j,val.h,val.l);
	}
}

static void postcmds_mem(sPostCmd *cmd) {
	octa addr;
	octa eight = {0,8};
	for(addr = cmd->data.mem.from;
		!(addr.h > cmd->data.mem.to.h || (addr.h == cmd->data.mem.to.h && addr.l > cmd->data.mem.to.l));
		addr = oplus(addr,eight)) {
		octa val = readMem(addr);
		printf("m[%08X%08X]: %08X%08X\n",addr.h,addr.l,val.h,val.l);
	}
}

static void postcmds_parse(const char *arg) {
	int i = 0,total;
	while(i < MAX_POST_CMDS && *arg) {
		switch(*arg) {
			case 'r':
				// skip "r:"
				arg += 2;
				cmds[i].type = TYPE_DREGS;
				sscanf(arg,"$%d..$%d",&cmds[i].data.regs.from,&cmds[i].data.regs.to);
				break;
			case 'm':
				// skip "m:"
				arg += 2;
				cmds[i].type = TYPE_MEM;
				cmds[i].data.mem.from.h = 0;
				cmds[i].data.mem.from.l = 0;
				cmds[i].data.mem.to.h = 0;
				cmds[i].data.mem.to.l = 0;
				cmds[i].data.mem.from = postcmds_scano(arg,&total);
				arg += total + 2;	/* skip ".." */
				cmds[i].data.mem.to = postcmds_scano(arg,&total);
				arg += total;
				break;
			default:
				fprintf(stderr,"Unknown post-op '%c'",*arg);
				exit(EXIT_FAILURE);
				break;
		}

		// walk to separator or end
		while(*arg && *arg != ',')
			arg++;
		if(*arg == ',')
			arg++;
		i++;
	}
	// terminate
	cmds[i].type = TYPE_NULL;
}

static octa postcmds_scano(const char *str,int *total) {
	octa res;
	res.l = res.h = 0;
	int j;
	*total = 0;
	for(j = 0; isxdigit(*str) && j < 2; j++) {
		int x = 0;
		int i;
		for(i = 0; isxdigit(*str) && i < 8; i++) {
			if(isdigit(*str))
				x = x * 16 + (*str - '0');
			else
				x = x * 16 + (10 + tolower(*str) - 'a');
			str++;
			(*total)++;
		}
		if(j == 0)
			res.h = x;
		else
			res.l = x;
	}
	/*
	 * 0x123456789A (total = 10)
	 * +--------+--------+
	 * |12345678|0000009A|
	 * +--------+--------+
	 * -->
	 * +--------+--------+
	 * |00000012|3456789A|
	 * +--------+--------+
	 */
	if(*total > 8 && *total != 16) {
		int shift = 8 - (*total - 8);
		int lowHigh = res.h & ((1 << (shift * 4)) - 1);
		res.h >>= shift * 4;
		res.l |= lowHigh << ((*total - 8) * 4);
	}
	else if(*total <= 8) {
		res.l = res.h;
		res.h = 0;
	}
	return res;
}

