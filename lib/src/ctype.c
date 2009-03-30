/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#if IN_KERNEL
#	include "../../kernel/h/common.h"
#else
#	include "../../libc/esc/h/common.h"
#endif
#include "../h/ctype.h"

bool isalnum(s32 c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

bool isalpha(s32 c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool iscntrl(s32 c) {
	return c < ' ';
}

bool isdigit(s32 c) {
	return (c >= '0' && c <= '9');
}

bool islower(s32 c) {
	return (c >= 'a' && c <= 'z');
}

bool isprint(s32 c) {
	return c >= ' ' && c <= '~';
}

bool ispunct(s32 c) {
	return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '`') ||
		(c >= '{' && c <= '~');
}

bool isspace(s32 c) {
	return (c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\f' || c == '\v');
}

bool isupper(s32 c) {
	return (c >= 'A' && c <= 'Z');
}

bool isxdigit(s32 c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

s32 tolower(s32 ch) {
	if(ch >= 'A' && ch <= 'Z')
		return ch - ('A' - 'a');
	return ch;
}

s32 toupper(s32 ch) {
	if(ch >= 'a' && ch <= 'z')
		return ch + ('A' - 'a');
	return ch;
}
