/*
 *  Created on: Jul 19, 2019
 *  Author: WanQing<1109162935@qq.com>
 */
#include "qstate.h"
#include "mapobj.h"
#include "mem_alloc.h"
#include "tupleobj.h"
#include "strobj.h"
static GlobalState *glStateList = NULL;
ThreadState *currentState;
GlobalState *GlobalState_New() {
	GlobalState *gs = qcalloc(GlobalState);
	if (gs) {
		gs->next = glStateList;
		glStateList = gs;
	}
	return gs;
}
void GlobalState_Init(ThreadState *ts) {
	GlobalState *gs = ts->gs;
	gs->intTable = (Object*) map_new(ts, false);
	gs->strTable = (Object*) map_new(ts, false);
	gs->glTable = (Object*) map_new(ts, true);

	tupleNil = object_newVar(ts, &Type_Tuple, 0);
	str_meta_init(ts);
	global_init(ts);
}
ThreadState *ThreadState_New(GlobalState *gs) {
	ThreadState *ts = qcalloc(ThreadState);
	if (ts != NULL) {
		ts->ci = NULL;
		ts->gs = gs;
		ts->next = gs->ts_list;
		gs->ts_list = ts;
	}
	return ts;
}
int stack_init(ThreadState *ts) {
	if (ts->stack == NULL) {
		ts->stack = (stkId) mem_alloc(ts, NULL, 0,
				sizeof(Object*) * BASIC_STACK_SIZE);
		if (ts->stack) {
			ts->stack_last = ts->stack + BASIC_STACK_SIZE;
			ts->stacksize = BASIC_STACK_SIZE;
			ts->top = ts->stack;
		} else {
			return 0;
		}
	}
	return -1;
}

