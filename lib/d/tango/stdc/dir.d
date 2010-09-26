module tango.stdc.dir;

private import tango.stdc.stat;

const MAX_NAME_LEN	= 50;

struct dirent {
	ino_t nodeNo;
	ushort recLen;
	ushort nameLen;
	char name[MAX_NAME_LEN + 1];
}
alias void DIR;

extern (C)
{
	extern DIR *opendir(char *path);
	extern bool readdir(DIR *dir,dirent *e);
	extern void closedir(DIR *dir);
}
