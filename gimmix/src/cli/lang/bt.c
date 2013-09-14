/**
 * $Id$
 */

#include "common.h"
#include "cli/lang/bt.h"
#include "cli/lang/btsimple.h"

int bt_getStackLevel(void) {
	return btsimple_getStackLevel(0);
}

void bt_fillForAdd(sBacktraceData *data,bool isFunc,octa instrCount,
		octa oldPC,octa newPC,octa threadId,int ex,octa bits,size_t argCount,octa *args) {
	data->instrCount = instrCount;
	data->isFunc = isFunc;
	data->oldPC = oldPC;
	data->newPC = newPC;
	data->threadId = threadId;
	if(isFunc) {
		data->d.func.argCount = argCount;
		for(size_t i = 0; i < argCount; i++)
			data->d.func.args[i] = args[i];
	}
	else {
		data->d.intrpt.ex = ex;
		data->d.intrpt.bits = bits;
	}
}

void bt_fillForRemove(sBacktraceData *data,bool isFunc,octa instrCount,octa oldPC,octa threadId,
                      size_t argCount,octa *args) {
	data->instrCount = instrCount;
	data->isFunc = isFunc;
	data->oldPC = oldPC;
	data->threadId = threadId;
	if(isFunc) {
        data->d.func.argCount = argCount;
        for(size_t i = 0; i < argCount; i++)
            data->d.func.args[i] = args[i];
	}
}
