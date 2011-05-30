/**
 * $Id$
 */

#include <esc/common.h>
#include <sys/task/timer.h>
#include <sys/util.h>

#define TIMER_BASE_FREQUENCY	1193182
#define IOPORT_TIMER_CTRL		0x43
#define IOPORT_TIMER_CNTDIV		0x40

/* counter to select */
#define TIMER_CTRL_CNT0			0x00
#define TIMER_CTRL_CNT1			0x40
#define TIMER_CTRL_CNT2			0x80
/* read/write mode */
#define TIMER_CTRL_RWLO			0x10	/* low byte only */
#define TIMER_CTRL_RWHI			0x20	/* high byte only */
#define TIMER_CTRL_RWLOHI		0x30	/* low byte first, then high byte */
/* mode */
#define TIMER_CTRL_MODE1		0x02	/* programmable one shot */
#define TIMER_CTRL_MODE2		0x04	/* rate generator */
#define TIMER_CTRL_MODE3		0x06	/* square wave generator */
#define TIMER_CTRL_MODE4		0x08	/* software triggered strobe */
#define TIMER_CTRL_MODE5		0x0A	/* hardware triggered strobe */
/* count mode */
#define TIMER_CTRL_CNTBIN16		0x00	/* binary 16 bit */
#define TIMER_CTRL_CNTBCD		0x01	/* BCD */

void timer_arch_init(void) {
	/* change timer divisor */
	uint freq = TIMER_BASE_FREQUENCY / TIMER_FREQUENCY;
	util_outByte(IOPORT_TIMER_CTRL,TIMER_CTRL_CNT0 | TIMER_CTRL_RWLOHI |
			TIMER_CTRL_MODE2 | TIMER_CTRL_CNTBIN16);
	util_outByte(IOPORT_TIMER_CNTDIV,freq & 0xFF);
	util_outByte(IOPORT_TIMER_CNTDIV,freq >> 8);
}
