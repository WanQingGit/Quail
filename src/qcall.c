/*
 *  Created on: Aug 1, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */
#include "qcall.h"
#include "funcobj.h"
#include "cfuncobj.h"
#include "tupleobj.h"
#include "moduleobj.h"
#include "qstate.h"
#include "mapobj.h"
#include "intobj.h"
#include "instruction.h"
#include "mem_alloc.h"
#include "QuailVM.h"
#include "classobj.h"
void stack_correct(ThreadState *ts, stkId oldstack) {
	CallInfo *ci;
	UpVal *up;
	register stkId stack = ts->stack;
	ts->top = (ts->top - oldstack) + stack;
	for (up = ts->open_upval; up != NULL; up = up->u.open.next)
		up->ref = (up->ref - oldstack) + stack;
	for (ci = ts->ci; ci != NULL; ci = ci->previous) {
		ci->top = (ci->top - oldstack) + stack;
		ci->res = (ci->res - oldstack) + stack;
//		ci->func = (ci->func - oldstack) + ts->stack;
//		if (ci->callstatus & CS_VM)
		ci->base = (ci->base - oldstack) + stack;
	}
}
bool stack_grow(ThreadState *ts, int nsize) {
	assert(nsize > ts->stacksize);
	stkId oldstack = ts->stack;
	stkId nstack = (stkId) mem_alloc(ts, ts->stack,
			sizeof(Object*) * ts->stacksize, sizeof(Object*) * nsize);
	if (nstack) {
		ts->stack = nstack;
		ts->stacksize = nsize;
		ts->stack_last = ts->stack + nsize;
		stack_correct(ts, oldstack);
		return true;
	}
	return false;
}
CallInfo *CI_New(ThreadState *ts) {
	CallInfo *ci;
	if (ts->ci && ts->ci->next) {
		ci = ts->ci = ts->ci->next;
	} else {
		ci = (CallInfo *) mem_alloc(ts, NULL, 0, sizeof(CallInfo));
		if (ci) {
			ci->previous = ts->ci;
			ci->next = NULL;
			if (ts->ci)
				ts->ci->next = ci;
			else {
				ci->top = ts->stack + 1;
			}
			ts->ci = ci;
		}
	}
	return ci;
}

#define RK(b)  ISK(b) ?( k + (b-BITRK)) : (base + b)
int call_object(ThreadState *ts, Object *o, stkId args, int argc, Object *kw,
		int nres) {
	CallInfo *ci = ts->ci;
	stkId base = ci->top;
	switch (o->type->basetype) {
	case BT_METHOD: {
		EntryDict dict;
		MethodObj *meth = cast(MethodObj*, o);
		assert((ci = CI_New(ts))!=NULL);
		FuncObj *func = cast(FuncObj*, meth->func);
		Proto *p = func->p;
		ci->func = (Object*) func;
		ci->res = base;
		ci->argc = argc + 1;
		ci->base = base;
		ci->nresults = nres;
		ts->top = ci->top = base + p->maxstacksize;
		stack_check(ts, 0);
		ci->u.pc = p->code;
		byte nparams = p->nparams - 1;
		ModuleObj *md = (ModuleObj*) p->module;
		TupleObj *defval = (TupleObj*) p->defValue;
		byte nparamPos = nparams;
		if (defval)
			nparamPos -= defval->length;
		*(base++) = meth->self;
		int i = 0;
		if (argc > nparams) {
			if (!(p->flag_vararg & 1))
				return -1;
			for (; i < nparams; i++) {
				*(base++) = args[i];
			}
			int extra = argc - nparams;
			TupleObj *tuple = (TupleObj*) tuple_new(ts, extra);
			for (i = 0; i < extra; i++) {
				tuple->item[i] = args[i + nparams];
			}
			*(base++) = (Object*) tuple;
		} else {
			for (; i < argc; i++) {
				*(base++) = args[i];
			}
			if (argc < nparamPos) {
				if (kw == NULL)
					return -1;

				for (; i < nparamPos; i++) {
					LocVar *var = p->locvars + i;
					assert(map_del(ts, (MapObj* )kw, md->consts[var->idx], &dict));
					*(base++) = dict.val;
				}
			}
		}
		if (kw != NULL && cast(MapObj*,kw)->length) {
			assert(p->flag_vararg & 2);
			if (p->flag_vararg & 1)
				ci->base[nparams + 2] = (Object*) kw;
			else
				ci->base[nparams + 1] = (Object*) kw;
		}
//		assert((ci = CI_New(ts))!=NULL);
		stack_check(ts, 0);

		return instr_execute(ts);
	}
	case BT_FUNCTION: {
		EntryDict dict;
		FuncObj *func = cast(FuncObj*, o);
		Proto *p = func->p;
		byte nparams = p->nparams;
		ModuleObj *md = (ModuleObj*) p->module;
		TupleObj *defval = (TupleObj*) p->defValue;
		byte nparamPos = nparams;
		if (defval)
			nparamPos -= defval->length;
//		stkId op = ci->top;
		assert((ci = CI_New(ts))!=NULL);
		ci->res = base;
		*(base++) = (Object*) func;
		ci->func = (Object*) func;
		ci->base = base;
		ci->nresults = nres;
		int i = 0;
		if (argc > nparams) {
			if (!(p->flag_vararg & 1))
				return -1;
			for (; i < nparams; i++) {
				*(base++) = args[i];
			}
			int extra = argc - nparams;
			TupleObj *tuple = (TupleObj*) tuple_new(ts, extra);
			for (i = 0; i < extra; i++) {
				tuple->item[i] = args[i + nparams];
			}
		} else {
			for (; i < argc; i++) {
				*(base++) = args[i];
			}
			if (argc < nparamPos) {
				if (kw == NULL)
					return -1;

				for (; i < nparamPos; i++) {
					LocVar *var = p->locvars + i;
					assert(map_del(ts, (MapObj* )kw, md->consts[var->idx], &dict));
					*(base++) = dict.val;
					//				*(rb++) = *(RK(cast(IntObj*,dict.val)->ival));
				}
			}
		}
		if (kw != NULL && cast(MapObj*,kw)->length) {
			assert(p->flag_vararg & 2);
			if (p->flag_vararg & 1)
				ci->base[nparams + 2] = (Object*) kw;
			else
				ci->base[nparams + 1] = (Object*) kw;
		}
		ts->top = ci->top = base + p->maxstacksize;
		ci->u.pc = p->code;
		stack_check(ts, 0);
		return instr_execute(ts);
	}
	case BT_CFUNC: {
		CFuncObj *cfunc = cast(CFuncObj*, o);
//		assert((ci = CI_New(ts))!=NULL);
//		ci->nresults = nres;
//		ci->func = o;
		CFunction cfun = cfunc->fn;
		int nret = cfun(ts, args, argc, kw);
//		call_finish(ts, ci, ts->top - nret, nret);
		return nret;
	}
	default: {
		callfn fn = o->type->f_call;
		assert(fn);
		int nret = fn(ts, o, args, argc, kw);
		return nret;
	}
	}
}
