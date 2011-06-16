#!/usr/bin/php
<?php
$syscalls = array(
	array("getConf","SYSCALL_GETCONF"),
	array("debugChar","SYSCALL_DEBUGCHAR"),
	array("debug","SYSCALL_DEBUG"),

	# driver
	array("regDriver","SYSCALL_REG"),
	array("getClientId","SYSCALL_GETCLIENTID"),
	array("getClient","SYSCALL_GETCLIENTPROC"),
	array("getWork","SYSCALL_GETWORK"),

	# I/O
	array("open","SYSCALL_OPEN"),
	array("pipe","SYSCALL_PIPE"),
	array("stat","SYSCALL_STAT"),
	array("fstat","SYSCALL_FSTAT"),
	array("tell","SYSCALL_TELL"),
	array("fcntl","SYSCALL_FCNTL"),
	array("seek","SYSCALL_SEEK"),
	array("read","SYSCALL_READ"),
	array("write","SYSCALL_WRITE"),
	array("isterm","SYSCALL_ISTERM"),
	array("send","SYSCALL_SEND"),
	array("receive","SYSCALL_RECEIVE"),
	array("dupFd","SYSCALL_DUPFD"),
	array("redirFd","SYSCALL_REDIRFD"),
	array("link","SYSCALL_LINK"),
	array("unlink","SYSCALL_UNLINK"),
	array("mkdir","SYSCALL_MKDIR"),
	array("rmdir","SYSCALL_RMDIR"),
	array("mount","SYSCALL_MOUNT"),
	array("unmount","SYSCALL_UNMOUNT"),
	array("sync","SYSCALL_SYNC"),
	array("close","SYSCALL_CLOSE"),

	# memory
	array("_changeSize","SYSCALL_CHGSIZE"),
	array("_addRegion","SYSCALL_ADDREGION"),
	array("setRegProt","SYSCALL_SETREGPROT"),
	array("_mapPhysical","SYSCALL_MAPPHYS"),
	array("_createSharedMem","SYSCALL_CREATESHMEM"),
	array("_joinSharedMem","SYSCALL_JOINSHMEM"),
	array("leaveSharedMem","SYSCALL_LEAVESHMEM"),
	array("destroySharedMem","SYSCALL_DESTROYSHMEM"),

	# process
	array("getpid","SYSCALL_PID"),
	array("getppidof","SYSCALL_PPID"),
	array("fork","SYSCALL_FORK"),
	array("exec","SYSCALL_EXEC"),
	array("waitChild","SYSCALL_WAITCHILD"),
	array("getenvito","SYSCALL_GETENVITO"),
	array("getenvto","SYSCALL_GETENVTO"),
	array("setenv","SYSCALL_SETENV"),

	# signals
	array("setSigHandler","SYSCALL_SETSIGH"),
	array("sendSignalTo","SYSCALL_SENDSIG"),

	# thread
	array("gettid","SYSCALL_GETTID"),
	array("getThreadCount","SYSCALL_GETTHREADCNT"),
	array("startThread","SYSCALL_STARTTHREAD"),
	array("_exit","SYSCALL_EXIT"),
	array("getCycles","SYSCALL_GETCYCLES"),
	array("yield","SYSCALL_YIELD"),
	array("sleep","SYSCALL_SLEEP"),
	array("waitm","SYSCALL_WAIT"),
	array("_waitUnlock","SYSCALL_WAITUNLOCK"),
	array("notify","SYSCALL_NOTIFY"),
	array("_lock","SYSCALL_LOCK"),
	array("_unlock","SYSCALL_UNLOCK"),
	array("join","SYSCALL_JOIN"),
	array("suspend","SYSCALL_SUSPEND"),
	array("resume","SYSCALL_RESUME"),
);

foreach($syscalls as $sc) {
	echo ".global ".$sc[0]."\n";
	echo ".type ".$sc[0].", @function\n";
	echo $sc[0].":\n";
	echo "	SET		$7,".$sc[1]."				# set syscall-number\n";
	echo "	TRAP	1,0,0\n";
	echo "	BZ		$7,1f						# no-error?\n";
	echo "	GETA	$2,errno\n";
	echo "	STOU	$7,$2,0\n";
	echo "	SET		$0,$7\n";
	echo "1:\n";
	echo "	POP		1,0							# return value is in $0\n";
	echo "\n";
}

?>
