/**
 * $Id$
 */

#include <esc/common.h>
#include <esc/io.h>
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/thread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "keyboard.h"
#include "window.h"

#define KB_DATA_BUF_SIZE	128

static void handleKbMessage(sWindow *active,uchar keycode,bool isBreak,uchar modifier,char c);

static volatile bool enabled = false;
static int drvId;
static sMsg msg;
static sKmData kbData[KB_DATA_BUF_SIZE];

static void sigUsr1(int sig) {
	UNUSED(sig);
	enabled = !enabled;
}

int keyboard_start(void *drvIdPtr) {
	int km = open("/dev/kmmanager",IO_READ);
	if(km < 0)
		error("Unable to open /dev/kmmanager");

	if(setSigHandler(SIG_USR1,sigUsr1) < 0)
		error("Unable to announce signal-handler");

	enabled = win_isEnabled();
	drvId = *(int*)drvIdPtr;
	while(1) {
		while(!enabled)
			wait(EV_NOEVENT,0);

		ssize_t count = RETRY(read(km,kbData,sizeof(kbData)));
		if(count < 0) {
			if(count != ERR_INTERRUPTED)
				printe("[WINM] Unable to read from kmmanager");
		}
		else {
			sWindow *active = win_getActive();
			if(!active)
				continue;

			sKmData *kbd = kbData;
			count /= sizeof(sKmData);
			while(count-- > 0) {
				handleKbMessage(active,kbd->keycode,kbd->isBreak,kbd->modifier,kbd->character);
				kbd++;
			}
		}
	}
	close(km);
	return 0;
}

static void handleKbMessage(sWindow *active,uchar keycode,bool isBreak,uchar modifier,char c) {
	int aWin;
	msg.args.arg1 = keycode;
	msg.args.arg2 = isBreak;
	msg.args.arg3 = active->id;
	msg.args.arg4 = c;
	msg.args.arg5 = modifier;
	aWin = getClient(drvId,active->owner);
	if(aWin < 0)
		printe("[WINM] Unable to get client %d",active->owner);
	else {
		send(aWin,MSG_WIN_KEYBOARD_EV,&msg,sizeof(msg.args));
		close(aWin);
	}
}
