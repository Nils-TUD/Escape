/**
 * $Id: set.c 199 2011-05-08 15:31:42Z nasmussen $
 */

#include <string.h>

#include "common.h"
#include "cli/cmds.h"
#include "cli/cmd/set.h"
#include "core/register.h"
#include "core/mmu.h"
#include "core/bus.h"
#include "core/cpu.h"
#include "mmix/io.h"

static void vmWriteOcta(octa addr,octa value);

void cli_cmd_set(size_t argc,const sASTNode **argv) {
	if(argc != 2)
		cmds_throwEx(NULL);

	for(octa offset = 0; ; offset++) {
		bool oFinished = false, vFinished = false;
		sCmdArg obj = cmds_evalArg(argv[0],ARGVAL_INT,offset,&oFinished);
		sCmdArg value = cmds_evalArg(argv[1],ARGVAL_INT | ARGVAL_FLOAT,offset,&vFinished);
		// we're finished if all objects have been assigned
		if(oFinished)
			break;

		switch(obj.origin) {
			case ORG_VMEM:
				vmWriteOcta(obj.location,value.d.integer);
				break;
			case ORG_VMEM1:
				mmu_writeByte(obj.location,value.d.integer,0);
				break;
			case ORG_VMEM2:
				mmu_writeWyde(obj.location,value.d.integer,0);
				break;
			case ORG_VMEM4:
				mmu_writeTetra(obj.location,value.d.integer,0);
				break;
			case ORG_VMEM8:
				mmu_writeOcta(obj.location,value.d.integer,0);
				break;
			case ORG_PMEM:
				bus_write(obj.location,value.d.integer,false);
				break;
			case ORG_LREG:
				reg_setLocal(obj.location,value.d.integer);
				break;
			case ORG_GREG:
				reg_setGlobal(obj.location,value.d.integer);
				break;
			case ORG_SREG:
				reg_setSpecial(obj.location,value.d.integer);
				break;
			case ORG_DREG:
				reg_set(obj.location,value.d.integer);
				break;
			case ORG_PC:
				if(value.d.integer & (sizeof(tetra) - 1))
					cmds_throwEx("Address is not tetra-aligned\n");
				cpu_setPC(value.d.integer);
				break;
			case ORG_EXPR:
				cmds_throwEx("The object to set can't be an arbitrary expression!\n");
				break;
		}
	}
}

static void vmWriteOcta(octa addr,octa value) {
	for(size_t i = 0; i < sizeof(octa); i++) {
		mmu_writeByte(addr,(value >> ((8 - i - 1) * 8)) & 0xFF,0);
		addr++;
	}
}
