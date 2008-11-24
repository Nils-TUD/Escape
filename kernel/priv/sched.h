/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVSCHED_H_
#define PRIVSCHED_H_

#include "../pub/common.h"
#include "../pub/sched.h"

#if 0
typedef struct {
	sProc *proc;
	s8 name[32];
	u32 hash;
	sSLList *queue;
} tService;
sSLList *services;

/*
 * - register service:
 * 		- pass service name to kernel
 * 			- calculate hash, put into services-list
 *
 * - connect to service:
 * 		- pass service name to kernel
 * 			- search for name in services-list and return hash
 *
 * - send command:
 * 		- pass service hash, command id, data and data-size to kernel
 * 			- look in the service-list for the given hash
 * 			- put caller to sleep
 *			- put all required information in the service-queue
 * 			- if service waiting:
 * 				- put command-id and data on service-stack
 * 				- wakeup service
 *
 * - send reply:
 * 		- pass reply-data and data-size to kernel
 * 		- data has the following format: <ptr1>,<size1>,<ptr2>,<size2>,...,<ptrn>,<sizen>
 * 			- kernel copies data to caller stack
 * 			- remove entry from service-queue
 * 			- if queue empty, put service to sleep
 * 			- else put service on ready-queue
 * 			- wake caller
 *
 * - disconnect from service:
 * 		- pass service hash to kernel
 * 			- anything to do?
 *
 * - unregister service:
 * 		- pass service hash to kernel
 * 			- search in services for hash
 * 			- if there are waiting apps, put error-code on their stack, put all in ready-queue
 * 			- remove service
 *
 *
 * Additional stuff:
 * 	- let the user provide a timeout for a command
 *  - provide an ansynchronous call, too. Some commands simply don't need a reply
 */
#endif

/* the queue for all runnable (but not currently running) processes */
typedef struct sQueueNode sQueueNode;
struct sQueueNode {
	sQueueNode *next;
	sProc *p;
};

#endif /* PRIVSCHED_H_ */
