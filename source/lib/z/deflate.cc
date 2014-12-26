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

void Deflate::encode_symbol(Data *d,Tree *t,unsigned int sym) {
	unsigned int num = t->length[sym];
	unsigned int bits = t->code[sym];
	while(num-- > 0)
		writebit(d,(bits >> num) & 0x1);
}

/* ----------------------------- *
 * -- block deflate functions -- *
 * ----------------------------- */

void Deflate::deflate_fixed_block(Data *d) {
	size_t len = d->source->cached();
	while(len-- > 0) {
		unsigned char sym = d->source->get();
		encode_symbol(d,&sltree,sym);
	}

	/* end of block */
	encode_symbol(d,&sltree,256);
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

/* initialize global (static) data */
Deflate::Deflate() {
	for(size_t i = 0; i <= 143; ++i) {
		sltree.code[i] = 48 + i;
		sltree.length[i] = 8;
	}
	for(size_t i = 144; i <= 255; ++i) {
		sltree.code[i] = 400 + i;
		sltree.length[i] = 9;
	}
	for(size_t i = 256; i <= 279; ++i) {
		sltree.code[i] = 0 + i;
		sltree.length[i] = 7;
	}
	for(size_t i = 280; i <= 287; ++i) {
		sltree.code[i] = 192 + i;
		sltree.length[i] = 8;
	}
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
