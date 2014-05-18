/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <sys/common.h>
#include <sys/mem/kheap.h>
#include <sys/mem/cache.h>
#include <sys/task/smp.h>
#include <sys/task/timer.h>
#include <sys/cpu.h>
#include <sys/ostream.h>
#include <sys/video.h>
#include <sys/log.h>
#include <sys/util.h>
#include <string.h>

/* based on http://forum.osdev.org/viewtopic.php?t=11998 */

/* the cpu-vendors; index in vendors-array */
enum Vendors {
	VENDOR_OLDAMD,
	VENDOR_AMD,
	VENDOR_INTEL,
	VENDOR_VIA,
	VENDOR_OLDTRANSMETA,
	VENDOR_TRANSMETA,
	VENDOR_CYRIX,
	VENDOR_CENTAUR,
	VENDOR_NEXGEN,
	VENDOR_UMC,
	VENDOR_SIS,
	VENDOR_NSC,
	VENDOR_RISE,
	VENDOR_UNKNOWN
};

static void doPrint(OStream &os,CPU::Info *cpu,const SMP::CPU *smpcpu);

static const char *vendors[] = {
	"AMDisbetter!",	/* early engineering samples of AMD K5 processor */
	"AuthenticAMD",
	"GenuineIntel",
	"CentaurHauls",
	"TransmetaCPU",
	"GenuineTMx86",
	"CyrixInstead",
	"CentaurHauls",
	"NexGenDriven",
	"UMC UMC UMC ",
	"SiS SiS SiS ",
	"Geode by NSC",
	"RiseRiseRise",
	"Unknown     ",
};

static const char *intelTypes[] = {
	"Original OEM",
	"Overdrive",
	"Dual-capable",
	"Reserved"
};

static const char *intelFamilies[] = {
	"Unknown",
	"Unknown",
	"Unknown",
	"i386",
	"i486",
	"Pentium",
	"Pentium Pro",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Pentium 4"
};

/* intel models for family 4 */
static const char *intel4Models[] = {
	"DX",
	"DX",
	"SX",
	"487/DX2",
	"SL",
	"SX2",
	"Unknown",
	"Write-back enhanced DX2",
	"DX4"
};

/* intel models for family 5 */
static const char *intel5Models[] = {
	"Unknown",
	"60/66",
	"75-200",
	"for 486 system",
	"MMX"
};

/* intel models for family 6 */
static const char *intel6Models[] = {
	"Unknown",
	"Pentium Pro",
	"Unknown",
	"Pentium II Model 3",
	"Unknown",
	"Pentium II Model 5/Xeon/Celeron",
	"Celeron",
	"Pentium III/Pentium III Xeon - external L2 cache",
	"Pentium III/Pentium III Xeon - internal L2 cache"
};

/* the information about our cpu */
CPU::Info *CPU::cpus;
uint64_t CPU::cpuHz;
uint64_t CPU::busHz;

void CPU::detect() {
	uint32_t eax,ebx,edx,unused;
	char vendor[VENDOR_STRLEN + 1];
	cpuid_t id = SMP::getCurId();
	if(cpus == NULL) {
		cpus = (Info*)Cache::alloc(sizeof(Info) * SMP::getCPUCount());
		if(!cpus)
			Util::panic("Not enough mem for CPU-infos");

		/* detect the speed just once */
		cpuHz = Timer::detectCPUSpeed(&busHz);
		Log::get().writef("Detected %u %Lu Mhz CPU(s) on a %Lu Mhz bus\n",
			SMP::getCPUCount(),cpuHz / 1000000,busHz / 1000000);
	}

	/* get vendor-string */
	cpuid(CPUID_GETVENDORSTRING,vendor);
	vendor[VENDOR_STRLEN] = '\0';

	/* check which one it is */
	cpus[id].vendor = VENDOR_UNKNOWN;
	for(size_t i = 0; i < ARRAY_SIZE(vendors) - 1; i++) {
		if(strcmp(vendors[i],vendor) == 0) {
			cpus[id].vendor = i;
			break;
		}
	}

	/* read brand string */
	cpus[id].name[0] = 0;
	cpuid(CPUID_INTELEXTENDED,&eax,&unused,&unused,&unused);
	if(eax & 0x80000000) {
		switch((uint8_t)eax) {
			case 0x4 ... 0x9:
				cpuid(CPUID_INTELBRANDSTRINGEND,
					&cpus[id].name[8],&cpus[id].name[9],&cpus[id].name[10],&cpus[id].name[11]);
			case 0x3:
				cpuid(CPUID_INTELBRANDSTRINGMORE,
					&cpus[id].name[4],&cpus[id].name[5],&cpus[id].name[6],&cpus[id].name[7]);
			case 0x2:
				cpuid(CPUID_INTELBRANDSTRING,
					&cpus[id].name[0],&cpus[id].name[1],&cpus[id].name[2],&cpus[id].name[3]);
				break;
		}
	}

	/* set default values */
	cpus[id].model = 0;
	cpus[id].family = 0;
	cpus[id].type = 0;
	cpus[id].brand = 0;
	cpus[id].stepping = 0;
	cpus[id].signature = 0;

	/* fetch some additional infos for known cpus */
	switch(cpus[id].vendor) {
		case VENDOR_INTEL:
			cpuid(CPUID_GETFEATURES,&eax,&ebx,&unused,&edx);
			cpus[id].model = (eax >> 4) & 0xf;
			cpus[id].family = (eax >> 8) & 0xf;
			cpus[id].type = (eax >> 12) & 0x3;
			cpus[id].brand = ebx & 0xff;
			cpus[id].stepping = eax & 0xf;
			cpus[id].signature = eax;
			cpus[id].features = edx;
			break;

		case VENDOR_AMD:
			cpuid(CPUID_GETFEATURES,&eax,&unused,&unused,&edx);
			cpus[id].model = (eax >> 4) & 0xf;
			cpus[id].family = (eax >> 8) & 0xf;
			cpus[id].stepping = eax & 0xf;
			cpus[id].features = edx;
			break;
	}
}

bool CPU::hasFeature(uint64_t feat) {
	/* don't use the cpus-array here, since it is called before CPU::detect() */
	uint32_t unused,ecx,edx;
	cpuid(CPUID_GETFEATURES,&unused,&unused,&ecx,&edx);
	if(feat >> 32)
		return !!(ecx & (feat >> 32));
	return !!(edx & feat);
}

void CPUBase::print(OStream &os) {
	for(auto cpu = SMP::begin(); cpu != SMP::end(); ++cpu) {
		os.writef("CPU %d:\n",cpu->id);
		doPrint(os,CPU::cpus + cpu->id,&*cpu);
	}
}

static void doPrint(OStream &os,CPU::Info *cpu,const SMP::CPU *smpcpu) {
	size_t size;
	os.writef("\t%-12s%lu Cycles\n","Total:",smpcpu->lastTotal);
	os.writef("\t%-12s%Lu Cycles\n","Non-Idle:",smpcpu->lastCycles);
	os.writef("\t%-12s%Lu Hz\n","Speed:",CPU::getSpeed());
	os.writef("\t%-12s%s\n","Vendor:",vendors[cpu->vendor]);
	os.writef("\t%-12s%s\n","Name:",(char*)cpu->name);
	switch(cpu->vendor) {
		case VENDOR_INTEL: {
			const char **models = NULL;
			if(cpu->type < ARRAY_SIZE(intelTypes))
				os.writef("\t%-12s%s\n","Type:",intelTypes[cpu->type]);
			else
				os.writef("\t%-12s%d\n","Type:",cpu->type);

			if(cpu->family < ARRAY_SIZE(intelFamilies))
				os.writef("\t%-12s%s\n","Family:",intelFamilies[cpu->family]);
			else
				os.writef("\t%-12s%d\n","Family:",cpu->family);

			switch(cpu->family) {
				case 4:
					models = intel4Models;
					size = ARRAY_SIZE(intel4Models);
					break;
				case 5:
					models = intel5Models;
					size = ARRAY_SIZE(intel5Models);
					break;
				case 6:
					models = intel6Models;
					size = ARRAY_SIZE(intel6Models);
					break;
			}
			if(models != NULL && cpu->model < size)
				os.writef("\t%-12s%s\n","Model:",models[cpu->model]);
			else
				os.writef("\t%-12s%d\n","Model:",cpu->model);
			os.writef("\t%-12s%d\n","Brand:",cpu->brand);
			os.writef("\t%-12s%d\n","Stepping:",cpu->stepping);
			os.writef("\t%-12s%08x\n","Signature:",cpu->signature);
		}
		break;

		case VENDOR_AMD:
			os.writef("\t%-12s%d\n","Family:",cpu->family);
			os.writef("\t%-12s","Model:");
			switch(cpu->family) {
				case 4:
					os.writef("%s%d\n","486 Model",cpu->model);
					break;
				case 5:
				switch(cpu->model) {
					case 0:
					case 1:
					case 2:
					case 3:
					case 6:
					case 7:
						os.writef("%s%d\n","K6 Model",cpu->model);
						break;
					case 8:
						os.writef("%s\n","K6-2 Model 8");
						break;
					case 9:
						os.writef("%s\n","K6-III Model 9");
						break;
					default:
						os.writef("%s%d\n","K5/K6 Model",cpu->model);
						break;
				}
				break;
				case 6:
				switch(cpu->model) {
					case 1:
					case 2:
					case 4:
						os.writef("%s%d\n","Athlon Model ",cpu->model);
						break;
					case 3:
						os.writef("%s\n","Duron Model 3");
						break;
					case 6:
						os.writef("%s\n","Athlon MP/Mobile Athlon Model 6");
						break;
					case 7:
						os.writef("%s\n","Mobile Duron Model 7");
						break;
					default:
						os.writef("%s%d\n","Duron/Athlon Model ",cpu->model);
						break;
				}
				break;
			}
			os.writef("\t%-12s%d\n","Stepping:",cpu->stepping);
			break;

		default:
			os.writef("\t%-12s%d\n","Model:",cpu->model);
			os.writef("\t%-12s%d\n","Type:",cpu->type);
			os.writef("\t%-12s%d\n","Family:",cpu->family);
			os.writef("\t%-12s%d\n","Brand:",cpu->brand);
			os.writef("\t%-12s%d\n","Stepping:",cpu->stepping);
			os.writef("\t%-12s%08x\n","Signature:",cpu->signature);
			break;
	}
}
