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

/* This is a slightly modified version of: */

/*
 * tinflate  -  tiny inflate
 *
 * Copyright (c) 2003 by Joergen Ibsen / Jibz
 * All Rights Reserved
 *
 * http://www.ibsensoftware.com/
 *
 * This software is provided 'as-is', without any express
 * or implied warranty.  In no event will the authors be
 * held liable for any damages arising from the use of
 * this software.
 *
 * Permission is granted to anyone to use this software
 * for any purpose, including commercial applications,
 * and to alter it and redistribute it freely, subject to
 * the following restrictions:
 *
 * 1. The origin of this software must not be
 *    misrepresented; you must not claim that you
 *    wrote the original software. If you use this
 *    software in a product, an acknowledgment in
 *    the product documentation would be appreciated
 *    but is not required.
 *
 * 2. Altered source versions must be plainly marked
 *    as such, and must not be misrepresented as
 *    being the original software.
 *
 * 3. This notice may not be removed or altered from
 *    any source distribution.
 */

#include <z/deflate.h>

namespace z {

/* special ordering of code length codes */
const unsigned char Deflate::clcidx[] = {
	16,17,18,0,8,7,9,6,
	10,5,11,4,12,3,13,2,
	14,1,15
};

/* ----------------------- *
 * -- utility functions -- *
 * ----------------------- */

/* build extra bits and base tables */
void Deflate::build_bits_base(unsigned char *bits,unsigned short *base,int delta,int first) {
	int i,sum;

	/* build bits table */
	for(i = 0; i < delta; ++i)
		bits[i] = 0;
	for(i = 0; i < 30 - delta; ++i)
		bits[i + delta] = i / delta;

	/* build base table */
	for(sum = first,i = 0; i < 30; ++i) {
		base[i] = sum;
		sum += 1 << bits[i];
	}
}

/* build the fixed huffman trees */
void Deflate::build_fixed_trees(Tree *lt,Tree *dt) {
	int i;

	/* build fixed length tree */
	for(i = 0; i < 7; ++i)
		lt->table[i] = 0;

	lt->table[7] = 24;
	lt->table[8] = 152;
	lt->table[9] = 112;

	for(i = 0; i < 24; ++i)
		lt->trans[i] = 256 + i;
	for(i = 0; i < 144; ++i)
		lt->trans[24 + i] = i;
	for(i = 0; i < 8; ++i)
		lt->trans[24 + 144 + i] = 280 + i;
	for(i = 0; i < 112; ++i)
		lt->trans[24 + 144 + 8 + i] = 144 + i;

	/* build fixed distance tree */
	for(i = 0; i < 5; ++i)
		dt->table[i] = 0;

	dt->table[5] = 32;

	for(i = 0; i < 32; ++i)
		dt->trans[i] = i;
}

/* given an array of code lengths, build a tree */
void Deflate::build_tree(Tree *t,const unsigned char *lengths,unsigned int num) {
	unsigned short offs[16];
	unsigned int i,sum;

	/* clear code length count table */
	for(i = 0; i < 16; ++i)
		t->table[i] = 0;

	/* scan symbol lengths,and sum code length counts */
	for(i = 0; i < num; ++i)
		t->table[lengths[i]]++;

	t->table[0] = 0;

	/* compute offset table for distribution sort */
	for(sum = 0,i = 0; i < 16; ++i) {
		offs[i] = sum;
		sum += t->table[i];
	}

	/* create code->symbol translation table (symbols sorted by code) */
	for(i = 0; i < num; ++i) {
		if(lengths[i])
			t->trans[offs[lengths[i]]++] = i;
	}
}

/* ---------------------- *
 * -- decode functions -- *
 * ---------------------- */

/* get one bit from source stream */
int Deflate::getbit(Data *d) {
	unsigned int bit;

	/* check if tag is empty */
	if(!d->bitcount--) {
		/* load next tag */
		d->tag = d->source->get();
		d->bitcount = 7;
	}

	/* shift bit out of tag */
	bit = d->tag & 0x01;
	d->tag >>= 1;

	return bit;
}

/* read a num bit value from a stream and add base */
unsigned int Deflate::read_bits(Data *d,int num,int base) {
	unsigned int val = 0;

	/* read num bits */
	if(num) {
		unsigned int limit = 1 << (num);
		unsigned int mask;

		for(mask = 1; mask < limit; mask *= 2)
			if(getbit(d)) val += mask;
	}

	return val + base;
}

/* given a data stream and a tree, decode a symbol */
int Deflate::decode_symbol(Data *d,Tree *t) {
	int sum = 0,cur = 0,len = 0;

	/* get more bits while code value is above sum */
	do {
		cur = 2 * cur + getbit(d);

		++len;

		sum += t->table[len];
		cur -= t->table[len];
	}
	while(cur >= 0);

	return t->trans[sum + cur];
}

/* given a data stream, decode dynamic trees from it */
void Deflate::decode_trees(Data *d,Tree *lt,Tree *dt) {
	Tree code_tree;
	unsigned char lengths[288 + 32];
	unsigned int hlit,hdist,hclen;
	unsigned int i,num,length;

	/* get 5 bits HLIT (257-286) */
	hlit = read_bits(d,5,257);

	/* get 5 bits HDIST (1-32) */
	hdist = read_bits(d,5,1);

	/* get 4 bits HCLEN (4-19) */
	hclen = read_bits(d,4,4);

	for(i = 0; i < 19; ++i)
		lengths[i] = 0;

	/* read code lengths for code length alphabet */
	for(i = 0; i < hclen; ++i) {
		/* get 3 bits code length (0-7) */
		unsigned int clen = read_bits(d,3,0);

		lengths[clcidx[i]] = clen;
	}

	/* build code length tree */
	build_tree(&code_tree,lengths,19);

	/* decode code lengths for the dynamic trees */
	for(num = 0; num < hlit + hdist;) {
		int sym = decode_symbol(d,&code_tree);

		switch(sym) {
			case 16:
				/* copy previous code length 3-6 times (read 2 bits) */
				{
					unsigned char prev = lengths[num - 1];
					for(length = read_bits(d,2,3); length; --length)
						lengths[num++] = prev;
				}
				break;
			case 17:
				/* repeat code length 0 for 3-10 times (read 3 bits) */
				for(length = read_bits(d,3,3); length; --length)
					lengths[num++] = 0;
				break;
			case 18:
				/* repeat code length 0 for 11-138 times (read 7 bits) */
				for(length = read_bits(d,7,11); length; --length)
					lengths[num++] = 0;
				break;
			default:
				/* values 0-15 represent the actual code lengths */
				lengths[num++] = sym;
				break;
		}
	}

	/* build dynamic trees */
	build_tree(lt,lengths,hlit);
	build_tree(dt,lengths + hlit,hdist);
}
}

/* ----------------------------- *
 * -- block inflate functions -- *
 * ----------------------------- */

/* given a stream and two trees, inflate a block of data */
int Deflate::inflate_block_data(Data *d,Tree *lt,Tree *dt) {
	while(1) {
		int sym = decode_symbol(d,lt);

		/* check for end of block */
		if(sym == 256)
			return OK;

		if(sym < 256)
			d->drain->put(sym);
		else {
			int length,dist,offs;
			int i;

			sym -= 257;

			/* possibly get more bits from length code */
			length = read_bits(d,length_bits[sym],length_base[sym]);

			dist = decode_symbol(d,dt);

			/* possibly get more bits from distance code */
			offs = read_bits(d,dist_bits[dist],dist_base[dist]);

			/* copy match */
			for(i = 0; i < length; ++i)
				d->drain->put(d->drain->get(offs));
		}
	}
}

/* inflate an uncompressed block of data */
int Deflate::inflate_uncompressed_block(Data *d) {
	unsigned int length,invlength;
	unsigned int i;

	/* get length */
	length = d->source->get();
	length = length + 256 * d->source->get();

	/* get one's complement of length */
	invlength = d->source->get();
	invlength = invlength + 256 * d->source->get();

	/* check length */
	if(length != (~invlength & 0x0000ffff))
		return FAILED;

	/* copy block */
	for(i = length; i; --i)
		d->drain->put(d->source->get());

	/* make sure we start next block on a byte boundary */
	d->bitcount = 0;

	return OK;
}

/* inflate a block of data compressed with fixed huffman trees */
int Deflate::inflate_fixed_block(Data *d) {
	/* decode block using fixed trees */
	return inflate_block_data(d,&sltree,&sdtree);
}

/* inflate a block of data compressed with dynamic huffman trees */
int Deflate::inflate_dynamic_block(Data *d) {
	/* decode trees from stream */
	decode_trees(d,&d->ltree,&d->dtree);

	/* decode block using decoded trees */
	return inflate_block_data(d,&d->ltree,&d->dtree);
}

/* ---------------------- *
 * -- public functions -- *
 * ---------------------- */

/* initialize global (static) data */
Deflate::Deflate() {
	/* build fixed huffman trees */
	build_fixed_trees(&sltree,&sdtree);

	/* build extra bits and base tables */
	build_bits_base(length_bits,length_base,4,3);
	build_bits_base(dist_bits,dist_base,2,1);

	/* fix a special case */
	length_bits[28] = 0;
	length_base[28] = 258;
}

/* inflate stream from source to dest */
int Deflate::uncompress(Drain *drain,Source *source) {
	Data d;
	int bfinal;

	/* initialise data */
	d.source = source;
	d.bitcount = 0;

	d.drain = drain;

	do {
		unsigned int btype;
		int res;

		/* read final block flag */
		bfinal = getbit(&d);

		/* read block type (2 bits) */
		btype = read_bits(&d,2,0);

		/* decompress block */
		switch(btype) {
			case 0:
				/* decompress uncompressed block */
				res = inflate_uncompressed_block(&d);
				break;
			case 1:
				/* decompress block with fixed huffman trees */
				res = inflate_fixed_block(&d);
				break;
			case 2:
				/* decompress block with dynamic huffman trees */
				res = inflate_dynamic_block(&d);
				break;
			default:
				return FAILED;
		}

		if(res != OK)
			return FAILED;

	}
	while(!bfinal);

	return 0;
}

}
