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
#include <gui/common.h>
#include <gui/application.h>
#include <esc/ringbuffer.h>
#include <esc/dir.h>
#include "shellcontrol.h"

// the min-size of the buffer before we pass it to the shell-control
#define UPDATE_BUF_SIZE		256
// the total size of the buffer
#define READ_BUF_SIZE		512

using namespace gui;

class ShellApplication : public Application {
public:
	ShellApplication(tFD sid,ShellControl *sh);
	virtual ~ShellApplication();

protected:
	void doEvents();

private:
	/* no cloning */
	ShellApplication(const ShellApplication &app);
	ShellApplication &operator=(const ShellApplication &app);

	void putIn(char *s,u32 len);
	void handleKbMsg();
	void handleKeycode();
	void driverMain();

private:
	tFD _sid;
	sWaitObject _waits[2];
	ShellControl *_sh;
	sVTermCfg _cfg;
	char *rbuffer;
	u32 rbufPos;
};

#endif /* SHELLAPP_H_ */
