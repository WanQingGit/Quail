/*
 *  Created on: Jul 19, 2019
 *  Author: WanQing<1109162935@qq.com>
 */

#ifndef INCLUDE_QSTATE_H_
#define INCLUDE_QSTATE_H_
#include "typeobj.h"
#define MINSTACK	20
#define BASIC_STACK_SIZE        (2*MINSTACK)

//@formatter:off
typedef enum {
	CS_END = (1 << 0),
	CS_HOOKED = (1 << 2),
	CS_CONTINUE = (1 << 3),
	CS_VM = (1 << 4),
} CallStatus;
//@formatter:on
struct CallInfo {
	Object *func; /* function index in the stack */
	stkId top; /* top for this function */
	stkId base; /* base for this function */
	stkId res;
	struct CallInfo *previous, *next; /* dynamic call link */
	union {
		Instruction *pc;
		void *ctx; /* context info. in case of yields */
	} u;
	ptrdiff_t extra;
	int argc;
	short nresults; /* expected number of results from this function */
	CallStatus callstatus :16;
//	bool ismethod :8;
};

GlobalState *GlobalState_New();
ThreadState *ThreadState_New(GlobalState *gs);
int stack_init(ThreadState *ts);
void GlobalState_Init(ThreadState *ts);
void global_init(ThreadState *ts);
#endif /* INCLUDE_QSTATE_H_ */

