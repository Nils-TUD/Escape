/**
 * $Id: devices.c 219 2011-06-08 07:35:56Z nasmussen $
 */

#include <string.h>

#include "common.h"
#include "cli/cmds.h"
#include "cli/cmd/devices.h"
#include "cli/console.h"
#include "core/bus.h"
#include "dev/disk.h"
#include "dev/term.h"
#include "dev/timer.h"
#include "dev/ram.h"
#include "mmix/io.h"
#include "config.h"

static void printDevice(const sDevice *dev);
static void printWriteOnlyReg(int rno,const char *name);
static void printReg(const sDevice *dev,int rno,const char *name);
static void printRegFlags(const sDevice *dev,int rno,const char *name,const char **msgs,size_t size);
static void printFlags(octa reg,const char **msgs,size_t size);

static const char *diskCtrl[] = {
	"STRT","IE","WRT","ERR","DONE","RDY"
};
static const char *timerCtrl[] = {
	"EXP","IE"
};
static const char *termCtrl[] = {
	"RDY","IE"
};
static const char *kbCtrl[] = {
	"RDY","IE"
};

void cli_cmd_devices(size_t argc,const sASTNode **argv) {
	UNUSED(argv);
	if(argc != 0)
		cmds_throwEx(NULL);

	for(int i = 0; ; i++) {
		const sDevice *dev = bus_getDevByIndex(i);
		if(!dev)
			break;
		if(i > 0)
			mprintf("\n");
		printDevice(dev);
	}
}

static void printDevice(const sDevice *dev) {
	mprintf("Device '%s':\n",dev->name);
	mprintf("%4sMemory:           #%016OX .. #%016OX\n","",dev->startAddr,dev->endAddr);
	mprintf("%4sIRQ-Mask:         #%016OX\n","",dev->irqMask);
	if(strcmp(dev->name,"Disk") == 0) {
		mprintf("%4sBuffer:           #%016OX .. #%016OX\n",
				"",dev->startAddr + DISK_BUFFER,dev->startAddr + DISK_BUFFER + DISK_BUF_SIZE - 1);
		printRegFlags(dev,0,"Ctrl",diskCtrl,ARRAY_SIZE(diskCtrl));
		printReg(dev,1,"Count");
		printReg(dev,2,"Sector");
		printReg(dev,3,"Capacity");
	}
	else if(strcmp(dev->name,"Timer") == 0) {
		printRegFlags(dev,0,"Ctrl",timerCtrl,ARRAY_SIZE(timerCtrl));
		printReg(dev,1,"Divisor");
	}
	else if(strncmp(dev->name,"Terminal",8) == 0) {
		printRegFlags(dev,0,"RecvCtrl",termCtrl,ARRAY_SIZE(termCtrl));
		printReg(dev,1,"RecvData");
		printRegFlags(dev,2,"XmtrCtrl",termCtrl,ARRAY_SIZE(termCtrl));
		printWriteOnlyReg(3,"XmtrData");
	}
	else if(strcmp(dev->name,"Keyboard") == 0) {
		printRegFlags(dev,0,"Ctrl",kbCtrl,ARRAY_SIZE(kbCtrl));
		printReg(dev,1,"Data");
	}
}

static void printWriteOnlyReg(int rno,const char *name) {
	mprintf("%4sr[#%02X:%s]:%*s-\n","",rno * sizeof(octa),name,10 - strlen(name),"");
}

static void printReg(const sDevice *dev,int rno,const char *name) {
	octa data = bus_read(dev->startAddr + rno * sizeof(octa),false);
	mprintf("%4sr[#%02X:%s]:%*s#%016OX\n","",rno * sizeof(octa),name,10 - strlen(name),"",data);
}

static void printRegFlags(const sDevice *dev,int rno,const char *name,const char **msgs,size_t size) {
	octa data = bus_read(dev->startAddr + rno * sizeof(octa),false);
	mprintf("%4sr[#%02X:%s]:%*s#%016OX (","",rno * sizeof(octa),name,10 - strlen(name),"",data);
	printFlags(data,msgs,size);
	mprintf(")\n");
}

static void printFlags(octa reg,const char **msgs,size_t size) {
	if(reg == 0)
		mprintf("-");
	for(size_t i = 0; i < 64; i++) {
		if(reg & ((octa)1 << i)) {
			mprintf("%s",i < size ? msgs[i] : "??");
			reg &= ~((octa)1 << i);
			if(reg)
				mprintf("+");
		}
	}
}
