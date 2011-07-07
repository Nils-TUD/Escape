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

#include <esc/common.h>
#include <esc/arch/i586/ports.h>
#include <esc/io.h>
#include <esc/driver.h>
#include <esc/proc.h>
#include <esc/conf.h>
#include <esc/messages.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <errors.h>

#define PIC_FREQUENCY				1193180
#define IOPORT_PIT_SPEAKER			0x42
#define IOPORT_PIT_CTRL_WORD_REG	0x43
#define IOPORT_KB_CTRL_B			0x61

/**
 * Starts the sound with given frequency
 *
 * @param frequency the frequency
 */
static void playSound(uint frequency);

/**
 * Stops the sound
 */
static void stopSound(void);

/**
 * The timer-interrupt-handler
 */
static void timerIntrptHandler(int sig);

static sMsg msg;
static int timerFreq;
static size_t intrptCount = 0;
static size_t intrptTarget = 0;

int main(void) {
	int fd;
	msgid_t mid;
	int id;

	/* register driver */
	id = regDriver("speaker",0);
	if(id < 0)
		error("Unable to register driver 'speaker'");

	timerFreq = getConf(CONF_TIMER_FREQ);
	if(timerFreq < 0)
		error("Unable to get timer-frequency");

	/* request io-ports */
	if(requestIOPorts(IOPORT_PIT_SPEAKER,2) < 0)
		error("Unable to request io-ports %d .. %d",IOPORT_PIT_SPEAKER,IOPORT_PIT_CTRL_WORD_REG);
	/* I have no idea why it is required to reserve 2 ports because we're accessing just 1 byte
	 * but virtualbox doesn't accept the port-usage otherwise. */
	if(requestIOPorts(IOPORT_KB_CTRL_B,2) < 0)
		error("Unable to request io-port %d",IOPORT_KB_CTRL_B);

	while(1) {
		fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != ERR_INTERRUPTED)
				printe("[SPK] Unable to get work");
		}
		else {
			switch(mid) {
				case MSG_SPEAKER_BEEP: {
					uint freq = msg.args.arg1;
					uint dur = msg.args.arg2;
					if(freq > 0 && dur > 0) {
						/* add timer-interrupt listener */
						if(setSigHandler(SIG_INTRPT_TIMER,timerIntrptHandler) == 0) {
							playSound(freq);
							intrptCount = 0;
							intrptTarget = dur;
						}
					}
				}
				break;

				default:
					msg.args.arg1 = ERR_UNSUPPORTED_OP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
		}
	}

	/* clean up */
	releaseIOPorts(IOPORT_KB_CTRL_B,2);
	releaseIOPorts(IOPORT_PIT_SPEAKER,2);
	close(id);

	return EXIT_SUCCESS;
}

static void timerIntrptHandler(int sig) {
	UNUSED(sig);
	if(intrptTarget > 0) {
		intrptCount += 1000 / timerFreq;
		if(intrptCount >= intrptTarget) {
			stopSound();
			/* reset */
			intrptTarget = 0;
			intrptCount = 0;
			if(setSigHandler(SIG_INTRPT_TIMER,SIG_DFL) < 0)
				printe("[SPK] Unable to unset signal-handler");
		}
	}
}

static void playSound(uint frequency) {
	uint f;
	uint8_t tmp;

	/* Set the PIT to the desired frequency */
	f = PIC_FREQUENCY / frequency;
	outByte(IOPORT_PIT_CTRL_WORD_REG,0xb6);
	outByte(IOPORT_PIT_SPEAKER,(uint8_t)(f));
	outByte(IOPORT_PIT_SPEAKER,(uint8_t)(f >> 8));

	/* And play the sound using the PC speaker */
	tmp = inByte(IOPORT_KB_CTRL_B);
	if(tmp != (tmp | 3))
		outByte(IOPORT_KB_CTRL_B,tmp | 3);
}

static void stopSound(void) {
	uint8_t tmp = inByte(IOPORT_KB_CTRL_B) & 0xFC;
	outByte(IOPORT_KB_CTRL_B,tmp);
}
