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
#include <esc/setjmp.h>
#include <esc/fileio.h>
#include <esc/proc.h>
#include <esc/heap.h>
#include <io/streams.h>
#include <io/ofilestream.h>
#include <io/ifilestream.h>
#include <io/ostringstream.h>
#include <io/istringstream.h>
#include <exceptions/exception.h>
#include <exceptions/io.h>
#include <exceptions/outofmemory.h>
#include <mem/heap.h>
#include <util/string.h>
#include <util/cmdargs.h>
#include <errors.h>

#include "tests/tstring.h"
#include "tests/tvector.h"
#include "tests/texceptions.h"
#include "tests/tcmdargs.h"
#include "tests/tfile.h"

int main(int argc,char *argv[]) {
#if 0
	const char *sort = "def";
	s32 num = 10;
	bool flag = false;
	const char *b = "meinb";
	sCmdArgs *a = cmdargs_create(argc,argv);
	a->parse(a,"s=s n=d flag b=s*",&sort,&num,&flag,&b);
	cout->format(cout,"sort=%s num=%d flag=%d b=%s\n",sort,num,flag,b);
	sIterator it = a->getFreeArgs(a);
	cout->format(cout,"Free arguments:\n");
	while(it.hasNext(&it)) {

		char *arg = (char*)it.next(&it);
		cout->format(cout,"	%s\n",arg);
	}
	a->destroy(a);

	char buf[30];
	sOStream *s = osstream_open(buf,sizeof(buf));
	s->format(s,"%-4d: %.2f: %15s",12,-12.45,"test woot?");
	s->close(s);

	cout->writes(cout,"Please enter the file to write to: ");
	cin->readline(cin,buf,sizeof(buf));

	sOStream *f = ofstream_open(buf,IO_CREATE | IO_WRITE);
	f->format(f,"Lets take a look at this... %d %d %d\n",1,2,3);
	f->close(f);

	s32 n;
	u32 u;
	char str[16];
	sIStream *ss = isstream_open("-12, 24000, abcdef");
	ss->format(ss,"%d, %u, %s",&n,&u,str);
	cout->format(cout,"Got n=%d, u=%u, str=%s\n",n,u,str);
	ss->close(ss);

	/*u32 i = 0;
	TRY {
		while(1) {
			heap_alloc(sizeof(u32) * 1024);
			i++;
		}
	}
	CATCH(OutOfMemoryException,e) {
		printf("Got out-of-memory-exception after %d allocated bytes at %s:%d\n",
				i * 1024 * sizeof(u32),e->file,e->line);
	}
	ENDCATCH*/
#endif

	test_register(&tModString);
	test_register(&tModVector);
	test_register(&tModExc);
	test_register(&tModCmdArgs);
	test_register(&tModFile);
	test_start();
	return 0;
}
