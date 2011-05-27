/**
 * $Id$
 */

#ifndef BOOT_H_
#define BOOT_H_

#define MAX_PATH_LEN	20
#define PROG_COUNT		1 /* TODO */

/* a program we should load */
typedef struct {
	char path[MAX_PATH_LEN];
	uint id;
	uint start;
	uint size;
} sLoadProg;

#define BL_HDD_ID	0
#define BL_FS_ID	1
#define BL_RTC_ID	2
#define BL_K_ID		3

#endif /* BOOT_H_ */
