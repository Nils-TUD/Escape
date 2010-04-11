module tango.stdc.dir;

private import tango.stdc.stat;

const MAX_NAME_LEN	= 50;

struct dirent {
	ino_t nodeNo;
	ushort recLen;
	ushort nameLen;
	char name[MAX_NAME_LEN + 1];
}

extern (C)
{
	extern int opendir(char *path);
	extern bool readdir(dirent *e,int fd);
	extern void closedir(int fd);
}
