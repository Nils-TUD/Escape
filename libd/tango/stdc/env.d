module tango.stdc.env;

extern (C)
{
	extern bool getenvito(char *name,uint nameSize,uint index);
	extern bool getenvto(char *value,uint valSize,char *name);
	extern int setenv(char *name,char *value);
}
