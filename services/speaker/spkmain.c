/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include <ports.h>
#include <service.h>
#include <proc.h>
#include <messages.h>
#include <signals.h>

#define PIC_FREQUENCY				1193180
#define IOPORT_PIT_SPEAKER			0x42
#define IOPORT_PIT_CTRL_WORD_REG	0x43
#define IOPORT_KB_CTRL_B			0x61

/**
 * Starts the sound with given frequency
 *
 * @param frequency the frequency
 */
static void playSound(u32 frequency);

/**
 * Stops the sound
 */
static void stopSound(void);

/**
 * The timer-interrupt-handler
 */
static void timerIntrptHandler(tSig sig,u32 data);

static u16 intrptCount = 0;
static u16 intrptTarget = 0;

int main(void) {
	tFD fd;
	tServ id,client;

	/* register service */
	id = regService("speaker",SERVICE_TYPE_SINGLEPIPE);
	if(id < 0) {
		printLastError();
		return 1;
	}

	/* request io-ports */
	if(requestIOPort(IOPORT_PIT_CTRL_WORD_REG) < 0 || requestIOPort(IOPORT_PIT_SPEAKER) < 0) {
		printLastError();
		return 1;
	}
	if(requestIOPort(IOPORT_KB_CTRL_B) < 0) {
		printLastError();
		return 1;
	}

	while(1) {
		fd = getClient(&id,1,&client);
		if(fd < 0)
			sleep(EV_CLIENT);
		else {
			sMsgHeader header;
			while(read(fd,&header,sizeof(sMsgHeader)) > 0) {
				switch(header.id) {
					case MSG_SPEAKER_BEEP: {
						sMsgDataSpeakerBeep data;
						read(fd,&data,sizeof(sMsgDataSpeakerBeep));
						if(data.frequency > 0 && data.duration > 0) {
							/* add timer-interrupt listener */
							if(setSigHandler(SIG_INTRPT_TIMER,timerIntrptHandler) == 0) {
								playSound(data.frequency);
								intrptTarget = data.duration;
							}
						}
					}
					break;
				}
			}
		}
	}

	/* clean up */
	releaseIOPort(IOPORT_KB_CTRL_B);
	releaseIOPort(IOPORT_PIT_SPEAKER);
	releaseIOPort(IOPORT_PIT_CTRL_WORD_REG);
	unregService(id);

	return 0;
}

static void timerIntrptHandler(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	if(intrptTarget > 0) {
		intrptCount++;
		if(intrptCount == intrptTarget) {
			stopSound();
			/* reset */
			intrptTarget = 0;
			intrptCount = 0;
			unsetSigHandler(SIG_INTRPT_TIMER);
		}
	}
}

static void playSound(u32 frequency) {
	u32 f;
	u8 tmp;

	/* Set the PIT to the desired frequency */
	f = PIC_FREQUENCY / frequency;
	outByte(IOPORT_PIT_CTRL_WORD_REG,0xb6);
	outByte(IOPORT_PIT_SPEAKER,(u8)(f));
	outByte(IOPORT_PIT_SPEAKER,(u8)(f >> 8));

	/* And play the sound using the PC speaker */
	tmp = inByte(IOPORT_KB_CTRL_B);
	if(tmp != (tmp | 3))
		outByte(IOPORT_KB_CTRL_B,tmp | 3);
}

static void stopSound(void) {
	u8 tmp = inByte(IOPORT_KB_CTRL_B) & 0xFC;
	outByte(IOPORT_KB_CTRL_B,tmp);
}
