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

#pragma once

#include <esc/common.h>
#include <esc/syscalls.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Request the given IO-ports
 *
 * @param start the start-port
 * @param count the number of ports to reserve
 * @return a negative error-code or 0 if successfull
 */
A_CHECKRET static inline int reqports(uint16_t start,size_t count) {
	return syscall2(SYSCALL_REQIOPORTS,start,count);
}

/**
 * Request the given IO-port
 *
 * @param port the port
 * @return a negative error-code or 0 if successfull
 */
A_CHECKRET static inline int reqport(uint16_t port) {
	return reqports(port,1);
}

/**
 * Releases the given IO-ports
 *
 * @param start the start-port
 * @param count the number of ports to reserve
 * @return a negative error-code or 0 if successfull
 */
static inline int relports(uint16_t start,size_t count) {
	return syscall2(SYSCALL_RELIOPORTS,start,count);
}

/**
 * Releases the given IO-port
 *
 * @param port the port
 * @return a negative error-code or 0 if successfull
 */
static inline int relport(uint16_t port) {
	return relports(port,1);
}

/**
 * Outputs the byte <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
static inline void outbyte(uint16_t port,uint8_t val) {
	__asm__ volatile (
		"out	%%al,%%dx"
		: : "a"(val), "d"(port)
	);
}

/**
 * Outputs the word <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
static inline void outword(uint16_t port,uint16_t val) {
	__asm__ volatile (
		"out	%%ax,%%dx"
		: : "a"(val), "d"(port)
	);
}

/**
 * Outputs the dword <val> to the I/O-Port <port>
 *
 * @param port the port
 * @param val the value
 */
static inline void outdword(uint16_t port,uint32_t val) {
	__asm__ volatile (
		"out	%%eax,%%dx"
		: : "a"(val), "d"(port)
	);
}

/**
 * Outputs <count> words at <addr> to port <port>
 *
 * @param port the port
 * @param addr an array of words
 * @param count the number of words to output
 */
static inline void outwords(uint16_t port,const void *addr,size_t count) {
	__asm__ volatile (
		"rep; outsw"
		: : "S"(addr), "c"(count), "d"(port)
	);
}

/**
 * Reads a byte from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
static inline uint8_t inbyte(uint16_t port) {
	uint8_t res;
	__asm__ volatile (
		"in	%%dx,%%al"
		: "=a"(res) : "d"(port)
	);
	return res;
}

/**
 * Reads a word from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
static inline uint16_t inword(uint16_t port) {
	uint16_t res;
	__asm__ volatile (
		"in	%%dx,%%ax"
		: "=a"(res) : "d"(port)
	);
	return res;
}

/**
 * Reads a dword from the I/O-Port <port>
 *
 * @param port the port
 * @return the value
 */
static inline uint32_t indword(uint16_t port) {
	uint32_t res;
	__asm__ volatile (
		"in	%%dx,%%eax"
		: "=a"(res) : "d"(port)
	);
	return res;
}

/**
 * Reads <count> words to <addr> from port <port>
 *
 * @param port the port
 * @param addr the array to write to
 * @param count the number of words to read
 */
static inline void inwords(uint16_t port,void *addr,size_t count) {
	__asm__ volatile (
		"rep; insw"
		: : "D"(addr), "c"(count), "d"(port)
	);
}

#ifdef __cplusplus
}
#endif
