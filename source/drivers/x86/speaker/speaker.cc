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

#include <sys/arch/x86/ports.h>
#include <esc/ipc/device.h>
#include <esc/proto/speaker.h>
#include <sys/common.h>
#include <sys/conf.h>
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define PIC_FREQUENCY				1193180
#define IOPORT_PIT_SPEAKER			0x42
#define IOPORT_PIT_CTRL_WORD_REG	0x43
#define IOPORT_KB_CTRL_B			0x61

static void playSound(uint freq,uint dur);
static void startSound(uint frequency);
static void stopSound(void);

class SpeakerDevice : public esc::Device {
public:
	explicit SpeakerDevice(const char *path,mode_t mode)
		: esc::Device(path,mode,DEV_TYPE_SERVICE,0) {
		set(MSG_SPEAKER_BEEP,std::make_memfun(this,&SpeakerDevice::beep));
	}

	void beep(esc::IPCStream &is) {
		uint freq,dur;
		is >> freq >> dur;

		if(freq > 0 && dur > 0)
			playSound(freq,dur);
	}
};

static long timerFreq;

int main(void) {
	timerFreq = sysconf(CONF_TIMER_FREQ);
	if(timerFreq < 0)
		error("Unable to get timer-frequency");

	/* request io-ports */
	if(reqports(IOPORT_PIT_SPEAKER,2) < 0)
		error("Unable to request io-ports %d .. %d",IOPORT_PIT_SPEAKER,IOPORT_PIT_CTRL_WORD_REG);
	/* I have no idea why it is required to reserve 2 ports because we're accessing just 1 byte
	 * but virtualbox doesn't accept the port-usage otherwise. */
	if(reqports(IOPORT_KB_CTRL_B,2) < 0)
		error("Unable to request io-port %d",IOPORT_KB_CTRL_B);

	SpeakerDevice dev("/dev/speaker",0110);
	dev.loop();

	/* clean up */
	relports(IOPORT_KB_CTRL_B,2);
	relports(IOPORT_PIT_SPEAKER,2);
	return EXIT_SUCCESS;
}

static void playSound(uint freq,uint dur) {
	startSound(freq);
	// TODO actually, it would be better not to block in this thread, because we make it unusable
	// for other clients during that time
	sleep(dur);
	stopSound();
}

static void startSound(uint frequency) {
	uint f;
	uint8_t tmp;

	/* Set the PIT to the desired frequency */
	f = PIC_FREQUENCY / frequency;
	outbyte(IOPORT_PIT_CTRL_WORD_REG,0xb6);
	outbyte(IOPORT_PIT_SPEAKER,(uint8_t)(f));
	outbyte(IOPORT_PIT_SPEAKER,(uint8_t)(f >> 8));

	/* And play the sound using the PC speaker */
	tmp = inbyte(IOPORT_KB_CTRL_B);
	if(tmp != (tmp | 3))
		outbyte(IOPORT_KB_CTRL_B,tmp | 3);
}

static void stopSound(void) {
	uint8_t tmp = inbyte(IOPORT_KB_CTRL_B) & 0xFC;
	outbyte(IOPORT_KB_CTRL_B,tmp);
}
