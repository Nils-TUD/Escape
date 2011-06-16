/**
 * $Id: cpu.h 900 2011-06-02 20:18:17Z nasmussen $
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

#ifndef MMIX_CPU_H_
#define MMIX_CPU_H_

#include <esc/common.h>

#define SPECIAL_NUM		33

#define rA				21				// arithmetic status register
#define rB				0				// bootstrap register (trip)
#define rC				8				// cycle counter
#define rD				1				// dividend register
#define rE				2				// epsilon register
#define rF				22				// failure location register
#define rG				19				// global threshold register
#define rH				3				// himult register
#define rI				12				// interval register
#define rJ				4				// return-jump register
#define rK				15				// interrupt mask register
#define rL				20				// local threshold register
#define rM				5				// multiplex mask register
#define rN				9				// serial number
#define rO				10				// register stack offset
#define rP				23				// prediction register
#define rQ				16				// interrupt request register
#define rR				6				// remainder register
#define rS				11				// register stack pointer
#define rT				13				// trap address register
#define rU				17				// usage counter
#define rV				18				// virtual translation register
#define rW				24				// where-interrupted register (trip)
#define rX				25				// execution register (trip)
#define rY				26				// Y operand (trip)
#define rZ				27				// Z operand (trip)
#define rBB				7				// bootstrap register (trap)
#define rTT				14				// dynamic trap address register
#define rWW				28				// where interrupted register (trap)
#define rXX				29				// execution register (trap)
#define rYY				30				// Y operand (trap)
#define rZZ				31				// Z operand (trap)
#define rSS				32				// kernel-stack location (GIMMIX only)

/**
 * @param rno the special-number
 * @return the name of the given special-register
 */
const char *cpu_getSpecialName(int rno);

/**
 * Sets rbb, rww, rxx, ryy and rzz to the current values of rBB, rWW, rXX, rYY and rZZ,
 * respectively.
 *
 * @param rbb the pointer to rBB
 * @param rww the pointer to rWW
 * @param rxx the pointer to rXX
 * @param ryy the pointer to rYY
 * @param rzz the pointer to rZZ
 */
void cpu_getKSpecials(uint64_t *rbb,uint64_t *rww,uint64_t *rxx,uint64_t *ryy,uint64_t *rzz);

/**
 * Sets rBB, rWW, rXX, rYY and rZZ to the current values of rbb, rww, rxx, ryy and rzz,
 * respectively.
 *
 * @param rbb the new value of rBB
 * @param rww the new value of rWW
 * @param rxx the new value of rXX
 * @param ryy the new value of rYY
 * @param rzz the new value of rZZ
 */
void cpu_setKSpecials(uint64_t rbb,uint64_t rww,uint64_t rxx,uint64_t ryy,uint64_t rzz);

/**
 * Performs a SYNCD and SYNCID for the given region to ensure that the IC agrees with the DC
 *
 * @param start the start-address
 * @param end the end-address
 */
extern void cpu_syncid(uintptr_t start,uintptr_t end);

/**
 * Retrieves the value of the global-register with given number
 *
 * @param rno the global-number
 * @return the value
 */
extern uint64_t cpu_getGlobal(int rno);

/**
 * Retrieves the value of the special-register with given number
 *
 * @param rno the special-number
 * @return the value
 */
extern uint64_t cpu_getSpecial(int rno);

#endif /* MMIX_CPU_H_ */
