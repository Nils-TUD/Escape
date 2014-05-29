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

#include <esc/common.h>

#include "../../CPUInfo.h"

/* based on http://forum.osdev.org/viewtopic.php?t=11998 */

enum CPUIdRequests {
	CPUID_GETVENDORSTRING,
	CPUID_GETFEATURES,
	CPUID_GETTLB,
	CPUID_GETSERIAL,

	CPUID_INTELEXTENDED = (int)0x80000000,
	CPUID_INTELFEATURES,
	CPUID_INTELBRANDSTRING,
	CPUID_INTELBRANDSTRINGMORE,
	CPUID_INTELBRANDSTRINGEND
};

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

void X86CPUInfo::cpuid(unsigned code,uint32_t *eax,uint32_t *ebx,uint32_t *ecx,uint32_t *edx) const {
	asm volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(code));
}

void X86CPUInfo::cpuid(unsigned code,char *str) const {
	uint32_t *buf = reinterpret_cast<uint32_t*>(str);
	uint32_t eax,ebx,ecx,edx;
	cpuid(code,&eax,&ebx,&ecx,&edx);
	buf[0] = ebx;
	buf[1] = edx;
	buf[2] = ecx;
}

X86CPUInfo::Info X86CPUInfo::getInfo() const {
	Info info;
	uint32_t eax,ebx,edx,unused;
	char vendor[VENDOR_STRLEN + 1];

	/* get vendor-string */
	cpuid(CPUID_GETVENDORSTRING,vendor);
	vendor[VENDOR_STRLEN] = '\0';

	/* check which one it is */
	info.vendor = VENDOR_UNKNOWN;
	for(size_t i = 0; i < ARRAY_SIZE(vendors) - 1; i++) {
		if(strcmp(vendors[i],vendor) == 0) {
			info.vendor = i;
			break;
		}
	}

	/* read brand string */
	info.name[0] = 0;
	cpuid(CPUID_INTELEXTENDED,&eax,&unused,&unused,&unused);
	if(eax & 0x80000000) {
		switch((uint8_t)eax) {
			case 0x4 ... 0x9:
				cpuid(CPUID_INTELBRANDSTRINGEND,
					&info.name[8],&info.name[9],&info.name[10],&info.name[11]);
			case 0x3:
				cpuid(CPUID_INTELBRANDSTRINGMORE,
					&info.name[4],&info.name[5],&info.name[6],&info.name[7]);
			case 0x2:
				cpuid(CPUID_INTELBRANDSTRING,
					&info.name[0],&info.name[1],&info.name[2],&info.name[3]);
				break;
		}
	}

	/* set default values */
	info.model = 0;
	info.family = 0;
	info.type = 0;
	info.brand = 0;
	info.stepping = 0;
	info.signature = 0;

	/* fetch some additional infos for known cpus */
	switch(info.vendor) {
		case VENDOR_INTEL:
			cpuid(CPUID_GETFEATURES,&eax,&ebx,&unused,&edx);
			info.model = (eax >> 4) & 0xf;
			info.family = (eax >> 8) & 0xf;
			info.type = (eax >> 12) & 0x3;
			info.brand = ebx & 0xff;
			info.stepping = eax & 0xf;
			info.signature = eax;
			info.features = edx;
			break;

		case VENDOR_AMD:
			cpuid(CPUID_GETFEATURES,&eax,&unused,&unused,&edx);
			info.model = (eax >> 4) & 0xf;
			info.family = (eax >> 8) & 0xf;
			info.stepping = eax & 0xf;
			info.features = edx;
			break;
	}

	return info;
}

void X86CPUInfo::print(FILE *f,info::cpu &cpu) {
	size_t size;
	Info info = getInfo();

	fprintf(f,"\t%-12s%Lu Hz\n","Speed:",cpu.speed());
	fprintf(f,"\t%-12s%s\n","Vendor:",vendors[info.vendor]);
	fprintf(f,"\t%-12s%s\n","Name:",(char*)info.name);
	switch(info.vendor) {
		case VENDOR_INTEL: {
			const char **models = NULL;
			if(info.type < ARRAY_SIZE(intelTypes))
				fprintf(f,"\t%-12s%s\n","Type:",intelTypes[info.type]);
			else
				fprintf(f,"\t%-12s%d\n","Type:",info.type);

			if(info.family < ARRAY_SIZE(intelFamilies))
				fprintf(f,"\t%-12s%s\n","Family:",intelFamilies[info.family]);
			else
				fprintf(f,"\t%-12s%d\n","Family:",info.family);

			switch(info.family) {
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
			if(models != NULL && info.model < size)
				fprintf(f,"\t%-12s%s\n","Model:",models[info.model]);
			else
				fprintf(f,"\t%-12s%d\n","Model:",info.model);
			fprintf(f,"\t%-12s%d\n","Brand:",info.brand);
			fprintf(f,"\t%-12s%d\n","Stepping:",info.stepping);
			fprintf(f,"\t%-12s%08x\n","Signature:",info.signature);
		}
		break;

		case VENDOR_AMD:
			fprintf(f,"\t%-12s%d\n","Family:",info.family);
			fprintf(f,"\t%-12s","Model:");
			switch(info.family) {
				case 4:
					fprintf(f,"%s%d\n","486 Model",info.model);
					break;
				case 5:
				switch(info.model) {
					case 0:
					case 1:
					case 2:
					case 3:
					case 6:
					case 7:
						fprintf(f,"%s%d\n","K6 Model",info.model);
						break;
					case 8:
						fprintf(f,"%s\n","K6-2 Model 8");
						break;
					case 9:
						fprintf(f,"%s\n","K6-III Model 9");
						break;
					default:
						fprintf(f,"%s%d\n","K5/K6 Model",info.model);
						break;
				}
				break;
				case 6:
				switch(info.model) {
					case 1:
					case 2:
					case 4:
						fprintf(f,"%s%d\n","Athlon Model ",info.model);
						break;
					case 3:
						fprintf(f,"%s\n","Duron Model 3");
						break;
					case 6:
						fprintf(f,"%s\n","Athlon MP/Mobile Athlon Model 6");
						break;
					case 7:
						fprintf(f,"%s\n","Mobile Duron Model 7");
						break;
					default:
						fprintf(f,"%s%d\n","Duron/Athlon Model ",info.model);
						break;
				}
				break;
			}
			fprintf(f,"\t%-12s%d\n","Stepping:",info.stepping);
			break;

		default:
			fprintf(f,"\t%-12s%d\n","Model:",info.model);
			fprintf(f,"\t%-12s%d\n","Type:",info.type);
			fprintf(f,"\t%-12s%d\n","Family:",info.family);
			fprintf(f,"\t%-12s%d\n","Brand:",info.brand);
			fprintf(f,"\t%-12s%d\n","Stepping:",info.stepping);
			fprintf(f,"\t%-12s%08x\n","Signature:",info.signature);
			break;
	}
}
