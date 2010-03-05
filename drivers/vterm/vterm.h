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

#ifndef VTERM_H_
#define VTERM_H_

#include <esc/common.h>
#include <vterm/vtctrl.h>

/**
 * Inits all vterms
 *
 * @param ids the driver-ids
 * @param cfg the global config
 * @return true if successfull
 */
bool vterm_initAll(tDrvId *ids,sVTermCfg *cfg);

/**
 * @param index the index
 * @return the vterm with given index
 */
sVTerm *vterm_get(u32 index);

/**
 * Selects the given vterminal
 *
 * @param index the index of the vterm
 */
void vterm_selectVTerm(u32 index);

/**
 * @return the currently active vterm
 */
sVTerm *vterm_getActive(void);

/**
 * Updates the dirty range
 *
 * @param vt the vterm
 */
void vterm_update(sVTerm *vt);

#endif /* VTERM_H_ */
