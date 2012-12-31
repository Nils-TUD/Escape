/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#pragma once

#define RB_DEFAULT		0x0		/* error if ring-buffer full */
#define RB_OVERWRITE	0x1		/* overwrite if full */

/* the user doesn't need to know the internal structure */
typedef void sRingBuf;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a new ring-buffer with given element-size and -count.
 *
 * @param eSize the size of each element
 * @param eCount the max. number of elements in the ring
 * @param flags RB_DEFAULT or RB_OVERWRITE
 * @return the ring-buffer or NULL if failed
 */
sRingBuf *rb_create(size_t eSize,size_t eCount,uint flags);

/**
 * Destroys the given ring-buffer
 *
 * @param r the ring-buffer
 */
void rb_destroy(sRingBuf *r);

/**
 * @param r the ring-buffer
 * @return the number of elements that can be read
 */
size_t rb_length(sRingBuf *r);

/**
 * Writes the given element into the ring-buffer. If RB_OVERWRITE is set for the ring-buffer
 * and the buffer is full the first unread element will simply be overwritten. If RB_OVERWRITE
 * is not set, this will result in an error.
 *
 * @param r the ring-buffer
 * @param e the element
 * @return true if successfull
 */
bool rb_write(sRingBuf *r,const void *e);

/**
 * Writes <n> elements from <e> into the ring-buffer.
 *
 * @param r the ring-buffer
 * @param e the address of the elements
 * @param n the number of elements to write
 * @return the number of written elements
 */
size_t rb_writen(sRingBuf *r,const void *e,size_t n);

/**
 * Reads the next element from the ring-buffer. If there is no element, false is returned.
 *
 * @param r the ring-buffer
 * @param e the address where to copy the element
 * @return true if successfull
 */
bool rb_read(sRingBuf *r,void *e);

/**
 * Reads <n> elements to <e> from the ring-buffer.
 *
 * @param r the ring-buffer
 * @param e the address where to copy the elements
 * @param n the number of elements to read
 * @return the number of read elements
 */
size_t rb_readn(sRingBuf *r,void *e,size_t n);

/**
 * Moves the next <n> elements from <src> to <dst>. Note that the element-size has to be equal!
 *
 * @param dst the destination-ring-buffer
 * @param src the source-ring-buffer
 * @param n the number of elements to move
 * @return the number of moved elements
 */
size_t rb_move(sRingBuf *dst,sRingBuf *src,size_t n);

/**
 * Prints information about the given ring-buffer
 *
 * @param r the ring-buffer
 */
void rb_print(sRingBuf *r);

#ifdef __cplusplus
}
#endif
