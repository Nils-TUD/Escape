/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <heap.h>
#include <string.h>
#include <io.h>
#include <bufio.h>
#include "history.h"

#define HISTORY_SIZE		30

/* command history (ring buffer) */
static s16 histWritePos = 0;
static s16 histReadPos = -1;
static u16 histSize = 0;
static char *history[HISTORY_SIZE] = {NULL};

void shell_addToHistory(char *line) {
	u16 lastPos = histWritePos == 0 ? HISTORY_SIZE - 1 : histWritePos - 1;
	/* don't add an entry twice in a row */
	if(lastPos < histSize && strcmp(history[lastPos],line) == 0) {
		/* ensure that we start at the beginning on next histUp/histDown */
		histReadPos = histWritePos;
		/* we don't need the line anymore */
		free(line);
		return;
	}

	/* free occupied places since we overwrite them */
	if(history[histWritePos])
		free(history[histWritePos]);
	history[histWritePos] = line;
	histWritePos = (histWritePos + 1) % HISTORY_SIZE;
	histReadPos = histWritePos;
	if(histSize < HISTORY_SIZE)
		histSize++;
}

char *shell_histUp(void) {
	if(histSize == 0)
		return NULL;

	if(histReadPos > 0)
		histReadPos--;
	else
		histReadPos = histSize - 1;

	return history[histReadPos];
}

char *shell_histDown(void) {
	if(histSize == 0)
		return NULL;

	if(histReadPos < histSize - 1)
		histReadPos++;
	else
		histReadPos = MIN(0,histSize - 1);

	return history[histReadPos];
}

void shell_histPrint(void) {
	s32 i;
	printf("History: (write=%d,read=%d,size=%d)\n",histWritePos,histReadPos,histSize);
	for(i = 0; i < histSize; i++)
		printf("%d: %s\n",i,history[i]);
	printf("\n");
}
