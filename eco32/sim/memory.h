/*
 * memory.h -- main memory simulation
 */


#ifndef _MEMORY_H_
#define _MEMORY_H_


Word memoryReadWord(Word pAddr);
Half memoryReadHalf(Word pAddr);
Byte memoryReadByte(Word pAddr);
void memoryWriteWord(Word pAddr, Word data);
void memoryWriteHalf(Word pAddr, Half data);
void memoryWriteByte(Word pAddr, Byte data);

void memoryReset(void);
void memoryInit(char *romImageName, char *progImageName);
void memoryExit(void);


#endif /* _MEMORY_H_ */
