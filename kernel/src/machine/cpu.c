/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <common.h>
#include <machine/cpu.h>
#include <printf.h>
#include <string.h>

/* based on http://forum.osdev.org/viewtopic.php?t=11998 */

/* the cpu-vendors; index in vendors-array */
#define CPUID_VENDOR_OLDAMD			0
#define CPUID_VENDOR_AMD			1
#define CPUID_VENDOR_INTEL			2
#define CPUID_VENDOR_VIA			3
#define CPUID_VENDOR_OLDTRANSMETA	4
#define CPUID_VENDOR_TRANSMETA		5
#define CPUID_VENDOR_CYRIX			6
#define CPUID_VENDOR_CENTAUR		7
#define CPUID_VENDOR_NEXGEN			8
#define CPUID_VENDOR_UMC			9
#define CPUID_VENDOR_SIS			10
#define CPUID_VENDOR_NSC			11
#define CPUID_VENDOR_RISE			12
#define CPUID_VENDOR_UNKNOWN		13

#define VENDOR_STRLEN				12

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
static sCPU cpu;

void cpu_detect(void) {
	u32 i;
	/* get vendor-string */
	char vendor[VENDOR_STRLEN + 1];
	cpu_getStrInfo(CPUID_GETVENDORSTRING,vendor);
	vendor[VENDOR_STRLEN] = '\0';

	/* check which one it is */
	cpu.vendor = CPUID_VENDOR_UNKNOWN;
	for(i = 0; i < ARRAY_SIZE(vendors) - 1; i++) {
		if(strcmp(vendors[i],vendor) == 0) {
			cpu.vendor = i;
			break;
		}
	}

	/* set default values */
	cpu.model = 0;
	cpu.family = 0;
	cpu.type = 0;
	cpu.brand = 0;
	cpu.stepping = 0;
	cpu.signature = 0;

	/* fetch some additional infos for known cpus */
	switch(cpu.vendor) {
		case CPUID_VENDOR_INTEL: {
			u32 eax,ebx,unused;
			cpu_getInfo(CPUID_GETFEATURES,&eax,&ebx,&unused,&unused);
			cpu.model = (eax >> 4) & 0xf;
			cpu.family = (eax >> 8) & 0xf;
			cpu.type = (eax >> 12) & 0x3;
			cpu.brand = ebx & 0xff;
			cpu.stepping = eax & 0xf;
			cpu.signature = eax;
		}
		break;

		case CPUID_VENDOR_AMD: {
			u32 eax,unused;
			cpu_getInfo(CPUID_GETFEATURES,&eax,&unused,&unused,&unused);
			cpu.model = (eax >> 4) & 0xf;
			cpu.family = (eax >> 8) & 0xf;
			cpu.stepping = eax & 0xf;
		}
		break;
	}
}

void cpu_sprintf(sStringBuffer *buf) {
	u32 size;
	prf_sprintf(buf,"%-12s%s\n","Vendor:",vendors[cpu.vendor]);
	switch(cpu.vendor) {
		case CPUID_VENDOR_INTEL: {
			const char **models = NULL;
			if(cpu.type < ARRAY_SIZE(intelTypes))
				prf_sprintf(buf,"%-12s%s\n","Type:",intelTypes[cpu.type]);
			else
				prf_sprintf(buf,"%-12s%d\n","Type:",cpu.type);

			if(cpu.family < ARRAY_SIZE(intelFamilies))
				prf_sprintf(buf,"%-12s%s\n","Family:",intelFamilies[cpu.family]);
			else
				prf_sprintf(buf,"%-12s%d\n","Family:",cpu.family);

			switch(cpu.family) {
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
			if(models != NULL && cpu.model < size)
				prf_sprintf(buf,"%-12s%s\n","Model:",models[cpu.model]);
			else
				prf_sprintf(buf,"%-12s%d\n","Model:",cpu.model);
			prf_sprintf(buf,"%-12s%d\n","Brand:",cpu.brand);
			prf_sprintf(buf,"%-12s%d\n","Stepping:",cpu.stepping);
			prf_sprintf(buf,"%-12s%08x\n","Signature:",cpu.signature);
		}
		break;

		case CPUID_VENDOR_AMD:
			prf_sprintf(buf,"%-12s%d\n","Family:",cpu.family);
			prf_sprintf(buf,"%-12s","Model:");
			switch(cpu.family) {
				case 4:
					prf_sprintf(buf,"%s%d\n","486 Model",cpu.model);
					break;
				case 5:
				switch(cpu.model) {
					case 0:
					case 1:
					case 2:
					case 3:
					case 6:
					case 7:
						prf_sprintf(buf,"%s%d\n","K6 Model",cpu.model);
						break;
					case 8:
						prf_sprintf(buf,"%s\n","K6-2 Model 8");
						break;
					case 9:
						prf_sprintf(buf,"%s\n","K6-III Model 9");
						break;
					default:
						prf_sprintf(buf,"%s%d\n","K5/K6 Model",cpu.model);
						break;
				}
				break;
				case 6:
				switch(cpu.model) {
					case 1:
					case 2:
					case 4:
						prf_sprintf(buf,"%s%d\n","Athlon Model ",cpu.model);
						break;
					case 3:
						prf_sprintf(buf,"%s\n","Duron Model 3");
						break;
					case 6:
						prf_sprintf(buf,"%s\n","Athlon MP/Mobile Athlon Model 6");
						break;
					case 7:
						prf_sprintf(buf,"%s\n","Mobile Duron Model 7");
						break;
					default:
						prf_sprintf(buf,"%s%d\n","Duron/Athlon Model ",cpu.model);
						break;
				}
				break;
			}
			prf_sprintf(buf,"%-12s%d\n","Stepping:",cpu.stepping);
			break;

		default:
			prf_sprintf(buf,"%-12s%d\n","Model:",cpu.model);
			prf_sprintf(buf,"%-12s%d\n","Type:",cpu.type);
			prf_sprintf(buf,"%-12s%d\n","Family:",cpu.family);
			prf_sprintf(buf,"%-12s%d\n","Brand:",cpu.brand);
			prf_sprintf(buf,"%-12s%d\n","Stepping:",cpu.stepping);
			prf_sprintf(buf,"%-12s%08x\n","Signature:",cpu.signature);
			break;
	}
}
