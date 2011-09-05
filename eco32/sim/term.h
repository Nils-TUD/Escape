/*
 * term.h -- terminal simulation
 */


#ifndef _TERM_H_
#define _TERM_H_


#define TERM_RCVR_CTRL		0	/* receiver control register */
#define TERM_RCVR_DATA		4	/* receiver data register */
#define TERM_XMTR_CTRL		8	/* transmitter control register */
#define TERM_XMTR_DATA		12	/* transmitter data register */

#define TERM_RCVR_RDY		0x01	/* receiver has a character */
#define TERM_RCVR_IEN		0x02	/* enable receiver interrupt */
#define TERM_RCVR_MSEC		1	/* input checking interval */

#define TERM_XMTR_RDY		0x01	/* transmitter accepts a character */
#define TERM_XMTR_IEN		0x02	/* enable transmitter interrupt */
#define TERM_XMTR_MSEC		1	/* output speed */


Word termRead(Word addr);
void termWrite(Word addr, Word data);

void termReset(void);
void termInit(int numTerms);
void termExit(void);


#endif /* _TERM_H_ */
