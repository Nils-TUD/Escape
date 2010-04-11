module tango.stdc.env;

extern (C)
{
	extern bool getEnv(char *value,uint valSize,char *name);
	extern int setEnv(char *name,char *value);
}
