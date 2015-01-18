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

#include <sys/cmdargs.h>
#include <sys/common.h>
#include <sys/esccodes.h>
#include <sys/keycodes.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "buffer.h"
#include "display.h"

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [<file>]\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	bool run = true;
	char c;
	int cmd,n1,n2,n3;
	if(argc > 2 || isHelpCmd(argc,argv))
		usage(argv[0]);

	/* init everything */
	buf_open(argc > 1 ? argv[1] : NULL);
	displ_init(buf_get());
	displ_update();

	while(run && (c = fgetc(stdin)) != EOF) {
		if(c == '\033') {
			/* just accept keycode-escapecodes */
			cmd = freadesc(stdin,&n1,&n2,&n3);
			if(cmd != ESCC_KEYCODE || (n3 & STATE_BREAK))
				continue;

			/* insert a char? */
			if(n1 == '\t' || (isprint(n1) && !(n3 & (STATE_CTRL | STATE_ALT)))) {
				int col,row;
				displ_getCurPos(&col,&row);
				buf_insertAt(col,row,n1);
				displ_markDirty(row,1);
				displ_mvCurHor(HOR_MOVE_RIGHT);
			}
			else {
				switch(n2) {
					case VK_X:
						if(n3 & STATE_CTRL) {
							if(buf_get()->modified) {
								char dstFile[MAX_PATH_LEN];
								int res = displ_getSaveFile(dstFile,MAX_PATH_LEN);
								if(res != EOF) {
									buf_store(dstFile);
									run = false;
								}
							}
							else
								run = false;
						}
						break;

					case VK_ENTER: {
						int col,row;
						displ_getCurPos(&col,&row);
						buf_newLine(col,row);
						/* to beginning of next line */
						displ_mvCurVert(1);
						displ_mvCurHor(HOR_MOVE_HOME);
						displ_markDirty(row,buf_getLineCount());
					}
					break;

					case VK_DELETE:
					case VK_BACKSP: {
						int col,row;
						displ_getCurPos(&col,&row);
						if(n2 == VK_DELETE) {
							sLine *cur = buf_getLine(row);
							if(col == (int)cur->length) {
								buf_moveToPrevLine(row + 1);
								displ_markDirty(row,buf_getLineCount() + 1);
							}
							else
								buf_removeCur(col,row);
						}
						else {
							if(col > 0) {
								displ_mvCurHor(HOR_MOVE_LEFT);
								buf_removeCur(col - 1,row);
							}
							else if(row > 0) {
								displ_mvCurHor(HOR_MOVE_LEFT);
								buf_moveToPrevLine(row);
								displ_markDirty(row - 1,buf_getLineCount() + 1);
							}
						}
						displ_markDirty(row,1);
					}
					break;

					case VK_LEFT:
						displ_mvCurHor(HOR_MOVE_LEFT);
						break;
					case VK_RIGHT:
						displ_mvCurHor(HOR_MOVE_RIGHT);
						break;

					case VK_HOME:
						if(n3 & STATE_CTRL)
							displ_mvCurVert(INT_MIN);
						displ_mvCurHor(HOR_MOVE_HOME);
						break;
					case VK_END:
						if(n3 & STATE_CTRL)
							displ_mvCurVert(INT_MAX);
						displ_mvCurHor(HOR_MOVE_END);
						break;

					case VK_UP:
						displ_mvCurVert(-1);
						break;
					case VK_DOWN:
						displ_mvCurVert(1);
						break;

					case VK_PGUP:
						displ_mvCurVertPage(true);
						break;
					case VK_PGDOWN:
						displ_mvCurVertPage(false);
						break;
				}
			}
			displ_update();
		}
	}
	displ_finish();
	return EXIT_SUCCESS;
}
