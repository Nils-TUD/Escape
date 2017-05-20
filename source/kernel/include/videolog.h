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

#pragma once

#include <log.h>
#include <ostream.h>
#include <video.h>

class VideoLog : public OStream {
public:
	virtual void writec(char c) override {
		Video::get().writec(c);
		Log::get().writec(c);
	}
	virtual bool escape(const char **fmt) override {
		return Video::get().escape(fmt);
	}
	virtual uchar pipepad() const override {
		return Video::get().pipepad();
	}
};
