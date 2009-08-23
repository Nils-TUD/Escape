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

#ifndef SHELLAPP_H_
#define SHELLAPP_H_

#include <esc/common.h>
#include <esc/dir.h>
#include <esc/gui/common.h>
#include <esc/gui/application.h>
#include <ringbuffer.h>
#include "shellcontrol.h"

// the min-size of the buffer before we pass it to the shell-control
#define UPDATE_BUF_SIZE		256
// the total size of the buffer
#define READ_BUF_SIZE		512
// the size of the ring-buffer
#define GUISH_INBUF_SIZE	128

using namespace esc::gui;

class ShellApplication : public Application {
public:
	ShellApplication(tServ sid,u32 no,ShellControl *sh);
	virtual ~ShellApplication();

protected:
	void doEvents();

private:
	void putIn(char *s,u32 len);

private:
	tServ _sid;
	ShellControl *_sh;
	sRingBuf *_inbuf;
	char *rbuffer;
	u32 rbufPos;
};

#endif /* SHELLAPP_H_ */
