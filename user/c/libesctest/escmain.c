/**
 * $Id: lessmain.c 572 2010-03-19 22:46:11Z nasmussen $
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
#include <streams/ofilestream.h>
#include <streams/ifilestream.h>
#include <streams/ostringstream.h>
#include <streams/istringstream.h>
#include <exceptions/exception.h>
#include <exceptions/io.h>
#include <exceptions/outofmemory.h>
#include <mem/heap.h>
#include <errors.h>
/*
static void doStuff(void) {
	int exid;
	printf("Enter exception-number (0..2): ");
	scanf("%d",&exid);
	switch(exid) {
		case 0:
			THROW(IOException,ERR_FILE_EXISTS);
			break;
		case 1:
			THROW(FileNotFoundException);
			break;
		default:
			THROW(OutOfMemoryException);
			break;
	}
}

static void doSomethingElse(void) {
	TRY {
		doStuff();
		printf("don't get here1!\n");
	}
	CATCH(IOException,e) {
		printf("Got IO-Exception, rethrowing1...\n");
		RETHROW(e);
	}
	FINALLY {
		printf("Finally block 4\n");
	}
	ENDCATCH
}

static void doSomething(void) {
	TRY {
		doSomethingElse();
		printf("don't get here2!\n");
	}
	CATCH(IOException,e) {
		printf("Got IO-Exception, rethrowing2...\n");
		RETHROW(e);
	}
	ENDCATCH
}

int main(void) {
	TRY {
		int i = 3;
		i++;
		printf("i=%d\n",i);
		THROW(IOException,ERR_FILE_EXISTS);
	}
	CATCH(IOException,e) {
		printf("a: ex3 @ %s line %d\n",e->file,e->line);
	}
	FINALLY {
		printf("Finally block 2\n");
	}
	ENDCATCH

	TRY {
		printf("TEST!\n");
	}
	FINALLY {
		printf("Do something else\n");
	}
	ENDCATCH

	TRY {
		printf("HIER\n");
		THROW(IOException,-1);
	}
	CATCH(IOException,e) {
		printf("Got another IO-Exception\n");
	}
	FINALLY {
		printf("My second finalizer\n");
		TRY {
			doStuff();
		}
		CATCH(IOException,e) {
			printf("Got IO-Exception\n");
		}
		FINALLY {
			printf("My finalizer\n");
		}
		ENDCATCH
	}
	ENDCATCH

	TRY {
		doSomething();
		printf("don't get here3!\n");
	}
	CATCH(OutOfMemoryException,e) {
		printf("b: ex1 @ %s line %d\n",e->file,e->line);
	}
	CATCH(FileNotFoundException,e) {
		printf("b: ex2 @ %s line %d\n",e->file,e->line);
	}
	FINALLY {
		printf("Finally block 3\n");
	}
	ENDCATCH
	printf("done!\n");
	return 0;
}*/

int main(int argc,char *argv[]) {
	char buf[30];
	sOStream *s = osstream_open(buf,sizeof(buf));
	s->format(s,"%-4d: %.2f: %15s",12,-12.45,"test woot?");
	s->close(s);

	sOStream *out = ofstream_openfd(STDOUT_FILENO);
	out->writes(out,"Please enter the file to write to: ");
	out->flush(out);

	sIStream *in = ifstream_openfd(STDIN_FILENO);
	in->readline(in,buf,sizeof(buf));

	sOStream *f = ofstream_open(buf,IO_CREATE | IO_WRITE);
	f->format(f,"Lets take a look at this... %d %d %d\n",1,2,3);
	f->close(f);

	s32 n;
	u32 u;
	char str[16];
	sIStream *ss = isstream_open("-12, 24000, abcdef");
	ss->format(ss,"%d, %u, %s",&n,&u,str);
	out->format(out,"Got n=%d, u=%u, str=%s\n",n,u,str);
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

	out->close(out);
	in->close(in);
	return 0;
}
