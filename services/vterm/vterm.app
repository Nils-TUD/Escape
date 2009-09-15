name:							"vterm";
source:						"/appsdb";
sourceWritable:		1;
type:							"driver";
ioports:					980..981,233..233,1016..1016,1021..1021;
driver:
	"keyboard",1,0,0,
	"video",0,1,1
;
fs:								0,0;
services:					;
intrpts:					;
physmem:					;
crtshmem:					;
joinshmem:				;
