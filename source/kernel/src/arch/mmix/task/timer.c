/**
 * $Id: timer.c 900 2011-06-02 20:18:17Z nasmussen $
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

#include <sys/common.h>
#include <sys/task/timer.h>

#define TIMER_BASE			0x8001000000000000

#define TIMER_CTRL			0		/* timer control register */
#define TIMER_DIVISOR		1		/* timer divisor register */

#define TIMER_EXP			0x01	/* timer has expired */
#define TIMER_IEN			0x02	/* enable timer interrupt */

void timer_arch_init(void) {
	ulong *regs = (ulong*)TIMER_BASE;
	/* set frequency */
	regs[TIMER_DIVISOR] = TIMER_FREQUENCY;
	/* enable timer */
	regs[TIMER_CTRL] = TIMER_IEN;
}

void timer_ackIntrpt(void) {
	ulong *regs = (ulong*)TIMER_BASE;
	/* remove expired-flag */
	regs[TIMER_CTRL] = TIMER_IEN;
}
