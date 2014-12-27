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

#include <sys/endian.h>
#include <z/deflate.h>

namespace z {

/* ---------------------- *
 * -- encode functions -- *
 * ---------------------- */

void Deflate::flush(Data *d) {
	if(d->bitcount > 0) {
		d->drain->put(d->tag);
		d->tag = 0;
		d->bitcount = 0;
	}
}

void Deflate::writebit(Data *d,int bit) {
	assert((bit & ~0x1) == 0);
	d->tag |= bit << d->bitcount;

	if(++d->bitcount == 8)
		flush(d);
}

void Deflate::write_bits(Data *d,unsigned int bits,int num) {
	while(num-- > 0) {
		writebit(d,bits & 0x1);
		bits >>= 1;
	}
}

void Deflate::write_bits_reverse(Data *d,unsigned int bits,unsigned int num) {
	while(num-- > 0)
		writebit(d,(bits >> num) & 0x1);
}

void Deflate::write_symbol(Data *d,Tree *t,unsigned int sym) {
	size_t i;
	for(i = 0; i < 4; ++i) {
		if(t->litbase[i] > sym)
			break;
	}
	assert(i > 0);
	i--;

	unsigned int num = t->length[i];
	unsigned int bits = t->codebase[i] + (sym - t->litbase[i]);
	write_bits_reverse(d,bits,num);
}

void Deflate::write_int(Data *d,Tree *t,unsigned short *base,unsigned char *bits,unsigned int value,int add) {
	/* determine code */
	size_t i;
	for(i = 0; i < 30; ++i) {
		if(base[i] > value)
			break;
	}
	assert(i > 0);
	i--;

	/* write code */
	write_symbol(d,t,add + i);
	write_bits(d,value - base[i],bits[i]);
}

void Deflate::write_symbol_chain(Data *d,Tree *lt,Tree *dt,unsigned int len,unsigned int dist) {
	write_int(d,lt,length_base,length_bits,len,257);
	write_int(d,dt,dist_base,dist_bits,dist,0);
}

/* ----------------------------- *
 * -- block deflate functions -- *
 * ----------------------------- */

void Deflate::deflate_fixed_block(Data *d) {
	size_t len = d->source->cached();
	size_t orglen = len;
	while(len > 0) {
		unsigned char sym = d->source->peek(0);

		// first, check if we already had the now following byte-sequence
		int x,maxlen = 0,max = 0;
		// ensure that we don't look more than 29 bytes back and not past the block-start and our
		// current position
		int end = MIN(29,len);
		int begin = -MIN(29,orglen - len);
		for(x = begin; x < 0; ++x) {
			// check how many bytes match
			int y;
			for(y = 0; x + y < 0 && y < end; ++y) {
				if(d->source->peek(x + y) != d->source->peek(y))
					break;
			}
			// if we reached the end, the last one didn't match
			if(x + y >= 0 || y >= end)
				y--;

			// the longest so far?
			if(y > maxlen) {
				maxlen = y;
				max = x;
			}
		}

		// the smallest length we can encode is 3
		if(maxlen >= 3) {
			write_symbol_chain(d,&sltree,&sdtree,maxlen,-max);
			x = maxlen;
		}
		else {
			// first write the symbol
			write_symbol(d,&sltree,sym);

			// now check if the symbol is following again at least 3 times
			for(x = 1; x < end; ++x) {
				if(d->source->peek(x) != sym)
					break;
			}
			x--;
			if(x == end)
				x--;

			if(x >= 3) {
				write_symbol_chain(d,&sltree,&sdtree,x,1);
				x++;
			}
			// adjust x to fetch one byte from source
			else
				x = 1;
		}

		// fetch the bytes from source
		len -= x;
		while(x-- > 0)
			d->source->get();
	}

	/* end of block */
	write_symbol(d,&sltree,256);
}

void Deflate::deflate_uncompressed_block(Data *d) {
	unsigned int length,invlength;
	unsigned int i;

	/* make sure we start next block on a byte boundary */
	flush(d);

	/* get length */
	length = d->source->cached();
	invlength = ~length & 0x0000ffff;
	assert(length <= 0xffff);

	length = cputole16(length);
	invlength = cputole16(invlength);
	d->drain->put(length & 0xff);
	d->drain->put(length >> 8);
	d->drain->put(invlength & 0xff);
	d->drain->put(invlength >> 8);

	/* copy block */
	for(i = le16tocpu(length); i; --i)
		d->drain->put(d->source->get());
}

/* ---------------------- *
 * -- public functions -- *
 * ---------------------- */

Deflate::Deflate() : DeflateBase() {
	/* init static length tree */
	sltree.litbase[0] = 0;
	sltree.length[0] = 8;
	sltree.codebase[0] = 0x30;

	sltree.litbase[1] = 144;
	sltree.length[1] = 9;
	sltree.codebase[1] = 0x190;

	sltree.litbase[2] = 256;
	sltree.length[2] = 7;
	sltree.codebase[2] = 0;

	sltree.litbase[3] = 280;
	sltree.length[3] = 8;
	sltree.codebase[3] = 0xC0;

	/* init static distance tree */
	sdtree.litbase[0] = 0;
	sdtree.length[0] = 5;
	sdtree.codebase[0] = 0;
	// termination
	sdtree.litbase[1] = 32;
}

int Deflate::compress(DeflateDrain *drain,DeflateSource *source,int compr) {
	Data d;
	d.source = source;
	d.bitcount = 0;
	d.tag = 0;
	d.drain = drain;

	while(source->more() || source->cached() > 0) {
		/* final block? */
		writebit(&d,source->more() ? 0 : 1);
		/* compression type */
		write_bits(&d,compr,2);

		/* write data */
		switch(compr) {
			case NONE:
				deflate_uncompressed_block(&d);
				break;
			case FIXED:
				deflate_fixed_block(&d);
				break;
			case DYN:
				// TODO deflate_dynamic_block(&d);
				break;
		}
	}

	flush(&d);
	return 0;
}

}
