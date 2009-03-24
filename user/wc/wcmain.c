/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <heap.h>
#include <io.h>
#include <bufio.h>
#include <string.h>

s32 main(u32 argc,char **argv) {
	s32 ch;
	u32 count,bufSize,bufPos;
	char *buffer;
	bool print = false;

	if(argc > 1 && strcmp(argv[1],"-p") == 0)
		print = true;

	count = 0;
	bufPos = 0;
	if(print) {
		bufSize = 20;
		buffer = (char*)malloc(bufSize + sizeof(char));
		if(buffer == NULL) {
			printLastError();
			return 1;
		}
	}

	while((ch = scanc()) > 0) {
		if(isspace(ch)) {
			if(bufPos > 0) {
				if(print) {
					buffer[bufPos] = '\0';
					printf("Word %d: '%s'\n",count + 1,buffer);
				}
				bufPos = 0;
				count++;
			}
		}
		else {
			if(print) {
				if(bufPos >= bufSize) {
					bufSize *= 2;
					buffer = (char*)realloc(buffer,bufSize * sizeof(char));
					if(buffer == NULL) {
						printLastError();
						return 1;
					}
				}
				buffer[bufPos] = ch;
			}
			bufPos++;
		}
	}

	if(print)
		free(buffer);
	printf("Total: %d\n",count);
	return 0;
}
