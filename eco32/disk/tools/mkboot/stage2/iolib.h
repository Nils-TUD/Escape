/*
 * iolib.h -- a very small library of I/O routines
 */


#ifndef _IOLIB_H_
#define _IOLIB_H_


char getc(void);
void putc(char c);
int rwscts(int dskno, int cmd, int sector, int addr, int count);


#endif /* _IOLIB_H_ */
