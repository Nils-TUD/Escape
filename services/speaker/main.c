/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include <service.h>
#include <proc.h>
#include <messages.h>

#define MSG_SPEAKER_TIMERINTRPT		0xFF

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

static u16 intrptCount = 0;
static u16 intrptTarget = 0;
static sMsgDefHeader msgTimerIntrpt = {
	.id = MSG_SPEAKER_TIMERINTRPT,
	.length = 0
};

s32 main(void) {
	s32 fd,id;

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
		fd = getClient(id);
		if(fd < 0)
			sleep();
		else {
			sMsgDefHeader header;
			while(read(fd,&header,sizeof(sMsgDefHeader)) > 0) {
				switch(header.id) {
					case MSG_SPEAKER_TIMERINTRPT: {
						if(intrptTarget > 0) {
							intrptCount++;
							if(intrptCount == intrptTarget) {
								stopSound();
								/* reset */
								intrptTarget = 0;
								intrptCount = 0;
								remIntrptListener(id,IRQ_TIMER);
							}
						}
					}
					break;

					case MSG_SPEAKER_BEEP: {
						sMsgDataSpeakerBeep data;
						read(fd,&data,sizeof(sMsgDataSpeakerBeep));
						if(data.frequency > 0 && data.duration > 0) {
							/* add timer-interrupt listener */
							if(addIntrptListener(id,IRQ_TIMER,&msgTimerIntrpt,sizeof(sMsgDefHeader)) == 0) {
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

static void playSound(u32 frequency) {
	u32 f;
	u8 tmp;

	/* Set the PIT to the desired frequency */
	f = PIC_FREQUENCY / frequency;
	outb(IOPORT_PIT_CTRL_WORD_REG,0xb6);
	outb(IOPORT_PIT_SPEAKER,(u8)(f));
	outb(IOPORT_PIT_SPEAKER,(u8)(f >> 8));

	/* And play the sound using the PC speaker */
	tmp = inb(IOPORT_KB_CTRL_B);
	if(tmp != (tmp | 3))
		outb(IOPORT_KB_CTRL_B,tmp | 3);
}

static void stopSound(void) {
	u8 tmp = inb(IOPORT_KB_CTRL_B) & 0xFC;
	outb(IOPORT_KB_CTRL_B,tmp);
}
