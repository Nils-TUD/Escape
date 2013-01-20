#!/usr/bin/php
<?php
$syscalls = array(
	array("sysconf","SYSCALL_GETCONF"),
	array("debugchar","SYSCALL_DEBUGCHAR"),
	array("debug","SYSCALL_DEBUG"),

	# driver
	array("createdev","SYSCALL_CRTDEV"),
	array("getclientid","SYSCALL_GETCLIENTID"),
	array("getclient","SYSCALL_GETCLIENTPROC"),
	array("getwork","SYSCALL_GETWORK"),

	# I/O
	array("open","SYSCALL_OPEN"),
	array("pipe","SYSCALL_PIPE"),
	array("stat","SYSCALL_STAT"),
	array("fstat","SYSCALL_FSTAT"),
	array("chmod","SYSCALL_CHMOD"),
	array("chown","SYSCALL_CHOWN"),
	array("tell","SYSCALL_TELL"),
	array("fcntl","SYSCALL_FCNTL"),
	array("seek","SYSCALL_SEEK"),
	array("read","SYSCALL_READ"),
	array("write","SYSCALL_WRITE"),
	array("send","SYSCALL_SEND"),
	array("receive","SYSCALL_RECEIVE"),
	array("dup","SYSCALL_DUPFD"),
	array("redirect","SYSCALL_REDIRFD"),
	array("link","SYSCALL_LINK"),
	array("unlink","SYSCALL_UNLINK"),
	array("mkdir","SYSCALL_MKDIR"),
	array("rmdir","SYSCALL_RMDIR"),
	array("mount","SYSCALL_MOUNT"),
	array("unmount","SYSCALL_UNMOUNT"),
	array("sync","SYSCALL_SYNC"),
	array("close","SYSCALL_CLOSE"),

	# memory
	array("_chgsize","SYSCALL_CHGSIZE"),
	array("_regadd","SYSCALL_ADDREGION"),
	array("regctrl","SYSCALL_SETREGPROT"),
	array("_regaddphys","SYSCALL_MAPPHYS"),
	array("_regaddmod","SYSCALL_MAPMOD"),
	array("_shmcrt","SYSCALL_CREATESHMEM"),
	array("_shmjoin","SYSCALL_JOINSHMEM"),
	array("shmleave","SYSCALL_LEAVESHMEM"),
	array("shmdel","SYSCALL_DESTROYSHMEM"),

	# process
	array("getpid","SYSCALL_PID"),
	array("getppidof","SYSCALL_PPID"),
	array("getuid","SYSCALL_GETUID"),
	array("setuid","SYSCALL_SETUID"),
	array("geteuid","SYSCALL_GETEUID"),
	array("seteuid","SYSCALL_SETEUID"),
	array("getgid","SYSCALL_GETGID"),
	array("setgid","SYSCALL_SETGID"),
	array("getegid","SYSCALL_GETEGID"),
	array("setegid","SYSCALL_SETEGID"),
	array("getgroups","SYSCALL_GETGROUPS"),
	array("setgroups","SYSCALL_SETGROUPS"),
	array("isingroup","SYSCALL_ISINGROUP"),
	array("fork","SYSCALL_FORK"),
	array("exec","SYSCALL_EXEC"),
	array("waitchild","SYSCALL_WAITCHILD"),
	array("getenvito","SYSCALL_GETENVITO"),
	array("getenvto","SYSCALL_GETENVTO"),
	array("setenv","SYSCALL_SETENV"),

	# signals
	array("signal","SYSCALL_SETSIGH"),
	array("kill","SYSCALL_SENDSIG"),

	# thread
	array("gettid","SYSCALL_GETTID"),
	array("getthreadcnt","SYSCALL_GETTHREADCNT"),
	array("_startthread","SYSCALL_STARTTHREAD"),
	array("_exit","SYSCALL_EXIT"),
	array("getcycles","SYSCALL_GETCYCLES"),
	array("yield","SYSCALL_YIELD"),
	array("alarm","SYSCALL_ALARM"),
	array("sleep","SYSCALL_SLEEP"),
	array("waitmuntil","SYSCALL_WAIT"),
	array("_waitunlock","SYSCALL_WAITUNLOCK"),
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
	echo "	SET		$7,0						# clear error-code\n";
	echo "	TRAP	0,".$sc[1].",0\n";
	echo "	BZ		$7,1f						# no-error?\n";
	echo "	GETA	$3,errno\n";
	echo "	NEG		$1,0,$7\n";
	echo "	STTU	$1,$3,0\n";
	echo "	SET		$0,$7\n";
	echo "1:\n";
	echo "	POP		1,0							# return value is in $0\n";
	echo "\n";
}

?>
