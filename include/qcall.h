/*
 *  Created on: Aug 1, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#ifndef INCLUDE_QCALL_H_
#define INCLUDE_QCALL_H_
#include "Quail.h"

#define stack_check(ts,n) \
   if ((ts->top + n) > ts->stack_last) {\
			assert(stack_grow(ts, (ts->stacksize + n) * 1.9));\
		}

#define stack_top_ptr(ts) (ts)->top
#define stack_top(ts) stack_top_ptr(ts)[0]
#define stack_inc(ts,n) do{stack_check(ts,n);ts->top+=n;}while(0)
#define stack_dec(ts,n) do{ts->top-=n;assert(ts->top>=ts->stack);}while(0)
CallInfo *CI_New(ThreadState *ts);
int call_prepare(ThreadState *ts, stkId o, Object *kw, int nres);
int call_finish(ThreadState *ts, CallInfo *ci, stkId firstResult, int nres);
void stack_correct(ThreadState *ts, stkId oldstack);
bool stack_grow(ThreadState *ts, int nsize);
int call_object(ThreadState *ts, Object *o, stkId args, int argc, Object *kw,
		int nres);
#endif /* INCLUDE_QCALL_H_ */
