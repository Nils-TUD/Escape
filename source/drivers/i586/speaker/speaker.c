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

#include <esc/common.h>
#include <esc/arch/i586/ports.h>
#include <esc/io.h>
#include <esc/driver.h>
#include <esc/proc.h>
#include <esc/conf.h>
#include <esc/irq.h>
#include <esc/messages.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

#define PIC_FREQUENCY				1193180
#define IOPORT_PIT_SPEAKER			0x42
#define IOPORT_PIT_CTRL_WORD_REG	0x43
#define IOPORT_KB_CTRL_B			0x61

/**
 * Plays the sound with given frequency for given duration
 *
 * @param freq the frequency
 * @param dur the duration in milliseconds
 */
static void playSound(uint freq,uint dur);

/**
 * Starts the sound with given frequency
 *
 * @param frequency the frequency
 */
static void startSound(uint frequency);

/**
 * Stops the sound
 */
static void stopSound(void);

static sMsg msg;
static long timerFreq;

int main(void) {
	int fd;
	msgid_t mid;
	int id;

	/* register device */
	id = createdev("/dev/speaker",0110,DEV_TYPE_SERVICE,DEV_CLOSE);
	if(id < 0)
		error("Unable to register device 'speaker'");

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

	while(1) {
		fd = getwork(id,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != -EINTR)
				printe("Unable to get work");
		}
		else {
			switch(mid) {
				case MSG_SPEAKER_BEEP: {
					uint freq = msg.args.arg1;
					uint dur = msg.args.arg2;
					if(freq > 0 && dur > 0)
						playSound(freq,dur);
				}
				break;

				case MSG_DEV_CLOSE:
					close(fd);
					break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
		}
	}

	/* clean up */
	relports(IOPORT_KB_CTRL_B,2);
	relports(IOPORT_PIT_SPEAKER,2);
	close(id);

	return EXIT_SUCCESS;
}

static void playSound(uint freq,uint dur) {
	int tsem = semcrtirq(IRQ_SEM_TIMER);
	if(tsem < 0) {
		printe("Unable to create irq-semaphore");
		return;
	}

	startSound(freq);
	// TODO actually, it would be better not to block in this thread, because we make it unusable
	// for other clients during that time
	while(dur > 0) {
		semdown(tsem);
		dur -= 1000 / timerFreq;
	}
	stopSound();
	semdestr(tsem);
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