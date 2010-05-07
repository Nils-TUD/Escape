module tango.stdc.io;

const STDIN_FILENO		= 0;
const STDOUT_FILENO		= 1;
const STDERR_FILENO		= 2;

const IO_READ			= 1;
const IO_WRITE			= 2;
const IO_CREATE			= 4;
const IO_TRUNCATE		= 8;
const IO_APPEND			= 16;

const SEEK_SET			= 0;
const SEEK_CUR			= 1;
const SEEK_END			= 2;

extern (C)
{
	extern int open(char *path,ubyte mode);
	extern int read(int fd,void *buffer,uint count);
	extern int write(int fd,void *buffer,uint count);
	extern int seek(int fd,int offset,uint whence);
	extern void close(int fd);
	extern int unlink(char *path);
	extern int mkdir(char *path);
	extern int rmdir(char *path);
	extern int pipe(int *readFd,int *writeFd);
	extern int dupFd(int fd);
	extern int redirFd(int src,int dst);
	extern bool isterm(int fd);
}
