/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

.global fpu_finit
.global fpu_saveState
.global fpu_restoreState

# void fpu_finit(void);
fpu_finit:
	finit
	ret

# void fpu_saveState(sFPUState *state);
fpu_saveState:
	mov			4(%esp),%eax
	fsave		(%eax)
	ret

# void fpu_restoreState(sFPUState *state);
fpu_restoreState:
	mov			4(%esp),%eax
	frstor	(%eax)
	ret
