/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef FSINTERFACE_H_
#define FSINTERFACE_H_

#if IN_KERNEL
#	include "../../kernel/h/common.h"
#else
#	include <common.h>
#endif

#define MSG_FS_OPEN			0
#define MSG_FS_READ			1
#define MSG_FS_WRITE		2
#define MSG_FS_CLOSE		3

#define MSG_FS_OPEN_RESP	4
#define MSG_FS_READ_RESP	5
#define MSG_FS_WRITE_RESP	6
#define MSG_FS_CLOSE_RESP	7

/* the open-request-data */
typedef struct {
	tPid pid;
	/* read/write */
	u8 flags;
	/* pathname follows */
	s8 path[];
} sMsgDataFSOpenReq;

/* the open-response-data */
typedef struct {
	tPid pid;
	/* may be an error-code */
	tInodeNo inodeNo;
} sMsgDataFSOpenResp;

/* the read-request-data */
typedef struct {
	tPid pid;
	tInodeNo inodeNo;
	u32 offset;
	u32 count;
} sMsgDataFSReadReq;

/* the read-response-data */
typedef struct {
	tPid pid;
	/* may be an error-code */
	s32 count;
	/* data follows */
	u8 data[];
} sMsgDataFSReadResp;

/* the write-request-data */
typedef struct {
	tPid pid;
	tInodeNo inodeNo;
	u32 offset;
	u32 count;
	/* data follows */
	u8 data[];
} sMsgDataFSWriteReq;

/* the write-response-data */
typedef struct {
	tPid pid;
	/* may be an error-code */
	s32 count;
} sMsgDataFSWriteResp;

/* the close-request-data */
typedef struct {
	tInodeNo inodeNo;
} sMsgDataFSCloseReq;

#endif /* FSINTERFACE_H_ */
