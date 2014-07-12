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

#include <sys/common.h>
#include <sys/io.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <shell/history.h>

#define HISTORY_SIZE		30

/* command history (ring buffer) */
static ssize_t histWritePos = 0;
static ssize_t histReadPos = -1;
static ssize_t histSize = 0;
static char *history[HISTORY_SIZE] = {NULL};

void shell_addToHistory(char *line) {
	size_t len = strlen(line);
	ssize_t lastPos = histWritePos == 0 ? HISTORY_SIZE - 1 : histWritePos - 1;
	/* don't add an entry twice in a row and ignore empty lines */
	if(*line == '\n' || (lastPos < histSize && strcmp(history[lastPos],line) == 0)) {
		/* ensure that we start at the beginning on next histUp/histDown */
		histReadPos = histWritePos;
		/* we don't need the line anymore */
		free(line);
		return;
	}

	/* remove \n at the end */
	line[len - 1] = '\0';

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
	ssize_t i;
	printf("History: (write=%zd,read=%zd,size=%zd)\n",histWritePos,histReadPos,histSize);
	for(i = 0; i < histSize; i++)
		printf("%zd: %s\n",i,history[i]);
	printf("\n");
}
