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

#include <esc/proto/ui.h>
#include <esc/stream/std.h>
#include <esc/cmdargs.h>
#include <esc/util.h>
#include <sys/common.h>
#include <sys/esccodes.h>
#include <sys/keycodes.h>
#include <sys/thread.h>
#include <sys/time.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>

using namespace esc;

static const float PI 		= 3.14159265358979f;
static const int INTERVAL 	= 50000; /* us */

static UI *ui;
static UIEvents *uiev;
static FrameBuffer *fb;
static Screen::Mode mode;

static bool gui = false;
static uint width = 100;
static uint height = 37;

static volatile bool run = true;
static volatile float speed = 0.04f;
static volatile size_t raster = 1;

static float radd = 0;
static float gadd = 2 * PI / 3;
static float badd = 4 * PI / 3;

static const float PI_HALF = PI / 2;
static float sintime, costime;
static float *x1vals, *x2vals, *y1vals, *y2vals;

static const char chartab[] = {
	' ', '.', ',', '_', 'o', 'O', 'H', 'A'
};

static float fastsin(float x) {
    asm ("fsin" : "+t"(x));
    return x;
}
static float fastcos(float x) {
    asm ("fsin" : "+t"(x));
    return x;
}
static float fastsqrt(float x) {
    asm ("fsqrt" : "+t"(x));
    return x;
}

static int inputThread(void *) {
	/* read from uimanager and handle the keys */
	while(1) {
		esc::UIEvents::Event ev;
		*uiev >> ev;
		if(ev.type != esc::UIEvents::Event::TYPE_KEYBOARD || (ev.d.keyb.modifier & STATE_BREAK))
			continue;

		if(ev.d.keyb.keycode == VK_Q) {
			run = false;
			break;
		}

		switch(ev.d.keyb.keycode) {
			case VK_UP:
				if(raster > 1)
					raster /= 2;
				break;
			case VK_DOWN:
				if(raster < 16)
					raster *= 2;
				break;
			case VK_LEFT:
				if(speed > 0)
					speed -= 0.01f;
				break;
			case VK_RIGHT:
				speed += 0.01f;
				break;
			case VK_PGUP:
				radd += 0.05f;
				gadd += 0.1f;
				badd += 0.2f;
				break;
			case VK_PGDOWN:
				radd -= 0.05f;
				gadd -= 0.1f;
				badd -= 0.2f;
				break;
		}
	}
	return 0;
}

static void init() {
	width = gui ? mode.width : mode.cols;
	height = gui ? mode.height : mode.rows;
	x1vals = new float[width];
	x2vals = new float[width];
	y1vals = new float[height];
	y2vals = new float[height];
}

static float getValue(uint ix,uint iy,float x,float y,float time) {
	// return Util::abs(sin(sin(x * 10 + time) + sin(y * 10 + time) + time));
	float v = 0;
	v += x1vals[ix];
	v += y2vals[iy];
	v += fastsin((x * 10 + y * 10 + time) / 2);
	float cx = x2vals[ix];
	float cy = y2vals[iy];
	v += fastsin(fastsqrt(100 * (cx + cy * cy) + 1) + time);
	v = v * PI_HALF;
	return Util::abs(v);
}

static void redraw(size_t factor,float time) {
	uint w = width / factor;
	uint h = height / factor;

	sintime = .5f * fastsin(time / 5);
	costime = .5f * fastcos(time / 3);

	for(uint x = 0; x < w; ++x) {
		float dx = (float)x / w;
		x1vals[x] = fastsin((dx * 10 + time));
		x2vals[x] = (x1vals[x] + sintime) * (x1vals[x] + sintime);
	}
	for(uint y = 0; y < h; ++y) {
		float dy = (float)y / h;
		y1vals[y] = fastsin((dy * 10 + time) / 2);
		y2vals[y] = y1vals[y] + costime;
	}

	float dx, dy = 0;
	float xadd = 1. / w;
	float yadd = 1. / h;
	for(uint y = 0; y < h; ++y, dy += yadd) {
		dx = 0;
		for(uint x = 0; x < w; ++x, dx += xadd) {
			float val = getValue(x,y,dx,dy,time);
			if(gui) {
				uint32_t rgba = 0;
				rgba |= (uint32_t)(255 * (.5f + .5f * fastsin(val + radd))) << 0;
				rgba |= (uint32_t)(255 * (.5f + .5f * fastsin(val + gadd))) << 8;
				rgba |= (uint32_t)(255 * (.5f + .5f * fastsin(val + badd))) << 16;
				rgba |= (uint32_t)(255) << 24;

				uint32_t *px = (uint32_t*)fb->addr() + y * factor * width + x * factor;
				for(size_t j = 0; j < factor; ++j) {
					for(size_t i = 0; i < factor; ++i)
						px[j * width + i] = rgba;
				}
			}
			else {
				float fac = (.5 + .5 * fastsin(val));
				uint16_t value = 0;
				value |= (uint16_t)(chartab[(size_t)(ARRAY_SIZE(chartab) * fac)]) << 0;
				value |= (uint16_t)(16 + 16 * fastsin(fac + radd)) << 8;

				uint16_t *px = (uint16_t*)fb->addr() + y * factor * width + x * factor;
				for(size_t j = 0; j < factor; ++j) {
					for(size_t i = 0; i < factor; ++i)
						px[j * width + i] = value;
				}
			}
		}
	}

	ui->update(0,0,width,height);
	// we want to have a synchronous update
	ui->getMode();
}

static void usage(const char *name) {
	serr << "Usage: " << name << " [-t (gui|tui)] [-s <cols>x<rows>]\n";
	serr << "    (gui|tui) selects the screen type\n";
	serr << "    <cols>x<rows> selects the screen mode\n";
	serr << "\n";
	serr << "Key-combinations:\n";
	serr << "    left/right: decrease/increase speed\n";
	serr << "    up/down:    higher/lower resolution\n";
	serr << "    pgup/pgdn:  change colors\n";
	serr << "    q:          quit\n";
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	std::string scrtype;
	std::string scrsize;

	esc::cmdargs args(argc,argv,esc::cmdargs::NO_FREE);
	try {
		args.parse("t=s s=s",&scrtype,&scrsize);
		if(args.is_help())
			usage(argv[0]);
	}
	catch(const esc::cmdargs_error& e) {
		errmsg("Invalid arguments: " << e.what());
		usage(argv[0]);
	}

	if(!scrtype.empty() && scrtype == "gui") {
		width = 1024;
		height = 700;
		gui = true;
	}
	if(!scrsize.empty()) {
		if(sscanf(scrsize.c_str(),"%dx%d",&width,&height) != 2)
			usage(argv[0]);
	}

	/* open uimanager */
	ui = new UI("/dev/uimng");

	/* find desired mode */
	if(gui) {
		mode = ui->findGraphicsMode(width,height,32);
		if(mode.bitsPerPixel != 32)
			error("Mode %ux%ux%u is not supported",mode.width,mode.height,mode.bitsPerPixel);
	}
	else
		mode = ui->findTextMode(width,height);

	/* attach to input-channel */
	uiev = new UIEvents(*ui);

	/* create framebuffer and set mode */
	int type = gui ? Screen::MODE_TYPE_GUI : Screen::MODE_TYPE_TUI;
	fb = new FrameBuffer(mode,type);
	ui->setMode(type,mode.id,fb->fd(),true);

	ui->hideCursor();

	if(startthread(inputThread,NULL) < 0)
		error("Unable to start input thread");

	init();

	float time = 0;
	uint64_t tscinterval = timetotsc(INTERVAL);
	while(run) {
		uint64_t now = rdtsc();

		redraw(raster,time);
		time += speed;

		uint64_t next = now + tscinterval;
		now = rdtsc();
		if(next > now)
			usleep(tsctotime(next - now));
	}

	join(0);
	return 0;
}
