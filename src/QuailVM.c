/*
 *  Created on: Jul 26, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */
#include "QuailVM.h"
#include "funcobj.h"
#include "mapobj.h"
#include "moduleobj.h"
#include "qstate.h"
#include "instruction.h"
#include "mem_alloc.h"
#include "qmath.h"
#include "cfuncobj.h"
#include "classobj.h"
#include "qvector.h"
#include "tupleobj.h"
#include "qcall.h"
#include "baseobj.h"

#define TARGET(op) TARGET_##op:
#define PC ci->u.pc
#define MASKARG MASKN(16,0)
#define ARG_A() (a=cast(ushort,MASKARG&args))
#define ARG_B() (b=cast(ushort,MASKARG&(args>>16)))
#define RK(b)  ISK(b) ?( k + (b-BITRK)) : (base + b)
#define R(b)  (base + b)
#define K(b)  (k + b)
#if __WORDSIZE == 64
typedef unsigned long long WordArg;
#define ARG_C() (c=cast(ushort,MASKARG&(args>>32)))
#define ARG_D() (d=cast(ushort,MASKARG&(args>>48)))
#else
typedef unsigned int WordArg;
#define ARG_C() (args=*cast(WordArg*,PC+5),c=cast(ushort,MASKARG&args))
#define ARG_D() (d=cast(ushort,MASKARG&(args>>16)))
#endif
//#define OP_FETCH
#define OP_NEXT do{\
		PC += opcodes[op].size;\
		op = GET_OPCODE(PC);\
		args=*cast(WordArg*,PC+1);\
		printcode(PC, count++, p);\
		goto *opcode_targets[op];}while(0);

//    case OP_##op:
void vm_call_module(ThreadState *ts, Object *module) {
	ModuleObj *md = (ModuleObj*) module;
	Object *mainFunc = func_new(ts, &md->main, 0);
	if (ts->stack == NULL) {
		assert(stack_init(ts));
	}
	stack_top(ts)=mainFunc;
	if (ts->ci == NULL)
		CI_New(ts);
	assert(call_prepare(ts, stack_top_ptr(ts),NULL,1)>=0);
	instr_execute(ts);
}

int call_prepare(ThreadState *ts, stkId o, Object *kw, int nres) {
	CallInfo *ci = ts->ci;
	int argc = ci->top - (o + 1);
	stkId base;
	Object *op = *o;
	switch (op->type->basetype) {
	case BT_METHOD: {
		EntryDict dict;
		MethodObj *meth = cast(MethodObj*, op);
		assert((ci = CI_New(ts))!=NULL);
		FuncObj *func = cast(FuncObj*, meth->func);
		Proto *p = func->p;
		ci->func = (Object*) func;
		ci->res = o;
		base = o;
		ci->base = base;
		*(base++) = meth->self;
		ci->nresults = nres;
		ts->top = ci->top = base + p->maxstacksize;
		ci->u.pc = p->code;
		byte nparams = p->nparams - 1;
		ModuleObj *md = (ModuleObj*) p->module;
		TupleObj *defval = (TupleObj*) p->defValue;
		byte nparamPos = nparams;
		if (defval)
			nparamPos -= defval->length;
		int i = 0;
		if (argc > nparams) {
			if (!(p->flag_vararg & 1))
				return -1;
			int extra = argc - nparams;
			TupleObj *tp_arg = (TupleObj*) tuple_new(ts, extra);
			for (i = 0; i < extra; i++) {
				tp_arg->item[i] = o[i + nparams + 1];
			}
			o[nparams + 1] = (Object*) tp_arg;
		} else if (argc < nparamPos) {
			if (kw == NULL)
				return -1;

			for (; i < nparamPos; i++) {
				LocVar *var = p->locvars + i;
				assert(map_del(ts, (MapObj* )kw, md->consts[var->idx], &dict));
				*(base++) = dict.val;
			}
		}
		if (kw != NULL && cast(MapObj*,kw)->length) {
			assert(p->flag_vararg & 2);
			if (p->flag_vararg & 1)
				o[nparams + 2] = (Object*) kw;
			else
				o[nparams + 1] = (Object*) kw;
		}
		stack_check(ts, 0);
		return 1;
	}
	case BT_FUNCTION: {
		EntryDict dict;
		FuncObj *func = cast(FuncObj*, op);
		Proto *p = func->p;
		byte nparams = p->nparams;
		ModuleObj *md = (ModuleObj*) p->module;
		TupleObj *defval = (TupleObj*) p->defValue;
		byte nparamPos = nparams;
		if (defval)
			nparamPos -= defval->length;
		base = o + argc + 1;
		int nargPos = argc;
		int i = nargPos;
		if (argc > nparams) {
			if (!(p->flag_vararg & 1))
				return -1;
			int extra = argc - nparams;
			TupleObj *tp_arg = (TupleObj*) tuple_new(ts, extra);
			for (i = 0; i < extra; i++) {
				tp_arg->item[i] = o[i + nparams + 1];
			}
			o[nparams + 1] = (Object*) tp_arg;
		} else if (argc < nparamPos) {
			if (kw == NULL)
				return -1;

			for (; i < nparamPos; i++) {
				LocVar *var = p->locvars + i;
				assert(map_del(ts, (MapObj* )kw, md->consts[var->idx], &dict));
				*(base++) = dict.val;
			}
		}
		if (argc < nparams) {
			if (kw != NULL) {
				for (i = nparamPos; i < nparams; i++) {
					LocVar *var = p->locvars + i;
					if (map_del(ts, (MapObj*) kw, md->consts[var->idx], &dict)) {
						*(base++) = dict.val;
					} else {
						*(base++) = defval->item[i - nparamPos];
					}
				}
			} else {
				for (; i < nparams; i++) {
					*(base++) = defval->item[i - nparamPos];
				}
			}
		}
		if (kw != NULL && cast(MapObj*,kw)->length) {
			assert(p->flag_vararg & 2);
			if (p->flag_vararg & 1)
				o[nparams + 2] = (Object*) kw;
			else
				o[nparams + 1] = (Object*) kw;
		}
		assert((ci = CI_New(ts))!=NULL);
		ci->func = op;
		ci->top = o + p->maxstacksize;
		ci->res = o;
		base = o + 1;
		ci->argc = argc;
//		for (; n < p->nparams; n++) {
//			*(ts->top++) = &Object_Null;
//		}
		ci->base = base;
		ci->nresults = nres;
		ts->top = ci->top = base + p->maxstacksize;
		ci->u.pc = p->code;
		stack_check(ts, 0);
		return 1;
	}
	case BT_CFUNC: {
//		TupleObj *tp = (TupleObj*) tuple_new(ts, argc);
		base = o + 1;
//		for (int i = 0; i < argc; i++) {
//			tp->item[i] = base[i];
//		}
		CFuncObj *cfunc = cast(CFuncObj*, *o);
		assert((ci = CI_New(ts))!=NULL);
		ci->nresults = nres;
		ci->func = *o;
		ci->base = base;
		ci->argc = argc;
		ci->res = o;
		ci->top = base + argc;
		CFunction cfun = cfunc->fn;
		int nret = cfun(ts, base, argc, kw);
		call_finish(ts, ci, ts->top - nret, nret);
		return 0;
	}
	default: {
		base = o + 1;
//		TupleObj *tp = (TupleObj*) tuple_new(ts, argc);
//		for (int i = 0; i < argc; i++) {
//			tp->item[i] = base[i];
//		}
		callfn f_call = op->type->f_call;
		assert(f_call);
		assert((ci = CI_New(ts))!=NULL);
		ci->nresults = nres;
		ci->base = base;
		ci->argc = argc;
		ci->res = o;
		int nret = f_call(ts, op, base, argc, kw);
		call_finish(ts, ci, ts->top - nret, nret);
		return 0;
	}
	}
}
int call_finish(ThreadState *ts, CallInfo *ci, stkId firstResult, int nres) {
	stkId res;
	int wanted = ci->nresults;
	res = ci->res; /* res == final position of 1st result 放参数的位置*/
	ts->ci = ci->previous; /* back to caller */
	/* move results to proper place */
//	return moveresults(ts, firstResult, res, nres, wanted);
	switch (wanted) { /* handle typical cases separately */
	case 0:
		break; /* nothing to move */
	case 1: { /* one result needed */
		if (nres == 0) /* no results? */
			*res = &Object_Null; /* adjust with nil */
		else
			*res = *firstResult; /* move it to proper place */
		break;
	}
	default: {
		int i;
		if (wanted <= nres) { /* enough results? */
			for (i = 0; i < wanted; i++) /* move wanted results to correct place */
				*(res + i) = *(firstResult + i);
		} else { /* not enough results; use all of them plus nils */
			for (i = 0; i < nres; i++) /* move all results to correct place */
				*(res + i) = *(firstResult + i);
			for (; i < wanted; i++) /* complete wanted number of results */
				*(res + i) = &Object_Null;
		}
		break;
	}
	}
	ts->top = res + wanted; /* top points after the last result */
	return 1;
}
UpVal *upval_get(ThreadState *ts, stkId upval) {
	UpVal **uvp = &ts->open_upval;
	UpVal *uv;
	while ((uv = *uvp) != NULL && uv->ref >= upval) {
//    lua_assert(upisopen(p));
		if (uv->ref == upval) /* found a corresponding upvalue? */
			return uv; /* return it */
		uvp = &uv->u.open.next;
	}
	uv = (UpVal *) mem_alloc(ts, NULL, 0, sizeof(UpVal));
	uv->nref = 0;
	uv->u.open.next = *uvp; /* link it to list of open upvalues */
	uv->u.open.touched = 1;
	*uvp = uv;
	uv->ref = upval; /* current value lives in the stack */
	return uv;
}
void upvalue_close(ThreadState *ts, stkId base) {
	UpVal *uv;
	while ((uv = ts->open_upval) != NULL && uv->ref >= base) {
		ts->open_upval = uv->u.open.next; /* remove from 'open' list */
		if (uv->nref == 0) /* no references? */
			mem_alloc(ts, uv, sizeof(UpVal), 0); /* free upvalue */
		else {
			uv->u.value = *uv->ref;
			uv->ref = &uv->u.value; /* now current value lives here */
		}
	}
}
void func_make(ThreadState *ts, Proto *p, UpVal **encup, stkId base, stkId ra) {
	int nup = p->nupvalues;
	Upvaldesc *uv = p->upvalues;
	int i;
	FuncObj *nfunc = (FuncObj*) func_new(ts, p, nup);
	*ra = (Object*) nfunc;
	for (i = 0; i < nup; i++) { /* fill in its upvalues */
		if (uv[i].instack) /* upvalue refers to local variable? */
			nfunc->upvals[i] = upval_get(ts, base + uv[i].idx);
		else
			/* get upvalue from enclosing function */
			nfunc->upvals[i] = encup[uv[i].idx];
		nfunc->upvals[i]->nref++;
		/* new closure is white, so we do not need a barrier here */
	}
}

int instr_execute(ThreadState *ts) {
	OpCode op;
	register WordArg args;
#include"opcode_targets.h"
	ushort a, b, c;
	CallInfo *ci = ts->ci;
	ci->callstatus |= CS_END | CS_VM;
	GlobalState *gs = ts->gs;
	MapObj *glTable = (MapObj*) gs->glTable;
	FrameEntry:
	assert(ts->ci == ci);
	FuncObj* func = cast(FuncObj*, ci->func);
	Int count = 0;
	Proto *p = func->p;
	ModuleObj *module = (ModuleObj*) p->module;
	stkId base = ci->base, k = module->consts, ra, rb, rc;
	MapEntry res;

	op = GET_OPCODE(PC);
	printcode(PC, count++, p);
	args = *cast(WordArg*, PC+1);
	goto *opcode_targets[op];
	TARGET(NIL){
	return -1;
}
	TARGET(MOVE){
	ra=base+ARG_A();
	rb=base+ARG_B();
	*ra=*rb;
	OP_NEXT
}
	TARGET(CONST){
	ra=base+ARG_A();
	rb=K(ARG_B());
	*ra=*rb;
	OP_NEXT
}
	TARGET(GLOBAL){ //OP_GLOBAL
	ra=R(ARG_A());
	assert(map_gset(ts, glTable, *K(ARG_B()), false, &res));
	*ra=res.dict->val;
	OP_NEXT
}
	TARGET(SETGLOBAL){
	ra=k+ARG_A();
	rb=RK(ARG_B());
	map_gset(ts, glTable, *ra, true, &res);
	res.dict->val=*rb;
	OP_NEXT
}
	TARGET(LOADNIL){
	OP_NEXT
}
	TARGET(UPVAL){
	base[ARG_A()]=*func->upvals[ARG_B()]->ref;
	OP_NEXT
}
	TARGET(SETUPVAL){
	OP_NEXT
}
	TARGET(NEWTABLE){
	OP_NEXT
}
	TARGET(NEWLIST){
	OP_NEXT
}
	TARGET(NEWCLASS){ //OP_NEWCLASS
	class_new(ts,k[ARG_A()], base, ARG_B());
	OP_NEXT
}
	TARGET(UNM){
	OP_NEXT
}
	TARGET(BNOT){
	OP_NEXT
}
	TARGET(NOT){
	OP_NEXT
}
	TARGET(LEN){
	OP_NEXT
}
	TARGET(JMP){
	OP_NEXT
}
	TARGET(TEST){
	OP_NEXT
}
	TARGET(FORLOOP){
	OP_NEXT
}
	TARGET(FORPREP){
	OP_NEXT
}
	TARGET(RETURN){
	if(p->np>0) {
		upvalue_close(ts, base);
	}
	ra=R(ARG_A());
	c=call_finish(ts, ci, ra, ARG_B()!=0?b-1:ts->top-ra);
	if(ci->callstatus&CS_END) {
		return b;
	}
	if(c) {
		ts->top=ci->top;
	}
	ci=ts->ci;
	goto FrameEntry;
}
	TARGET(TFORCALL){
	OP_NEXT
}
	TARGET(TFORLOOP){
	OP_NEXT
}
	TARGET(CLOSURE){
	ra=R(ARG_A());
	Proto *proto=p->p[ARG_B()];
	func_make(ts, proto, func->upvals, base, ra);
	if(proto->defValue) {
		TupleObj *tp=(TupleObj*)proto->defValue;
		for(int i=0;i<tp->length;i++) {
			IntObj *io=(IntObj*)tp->item[i];
			tp->item[i]=*(RK(io->ival));
		}
	}
	OP_NEXT
}
	TARGET(BUILD_METH){
	if(ISK(ARG_A())) {
		a=INDEXK(a);
	}
	Proto *proto=p->p[ARG_B()];
	ra=base+p->maxstacksize;
	func_make(ts, proto, func->upvals, base, ra);
	(*ra)->type=&Type_Unboundmethod;
	if(proto->defValue) {
		TupleObj *tp=(TupleObj*)proto->defValue;
		for(int i=0;i<tp->length;i++) {
			IntObj *io=(IntObj*)tp->item[i];
			tp->item[i]=*(RK(io->ival));
		}
	}
	map_gset(ts, (MapObj*)cast(TypeObj*,base[0])->dict, k[a], true, &res);
	res.dict->val=*ra;
	OP_NEXT
}
	TARGET(VARARG){
	OP_NEXT
}
	TARGET(LOADBOOL){
	OP_NEXT
}
	TARGET(GETTABUP){
	OP_NEXT
}
	TARGET(GET_ATTR){
	ra=R(ARG_A());
	if(ISK(ARG_B())) {
		assert(map_gset(ts, glTable, k[RKID(b)], false, &res));
		rb=&res.dict->val;
	} else {
		rb=R(b);
	}
	rc=K(ARG_C());
	assert(object_getattr(ts, *rb, *rc, ra)==1);
	OP_NEXT
}
	TARGET(SETTABUP){
	OP_NEXT
}
	TARGET(SET_ATTR){
	ra=R(ARG_A());
	rb=K(ARG_B());
	rc=RK(ARG_C());
	object_setattr(ts, *ra, *rb, *rc);
	OP_NEXT
}
	TARGET(GETTABGL){
	OP_NEXT
}
	TARGET(SETUP_EXCEPT){
	OP_NEXT
}
	TARGET(ADD){
	OP_NEXT
}
	TARGET(SUB){
	OP_NEXT
}
	TARGET(MUL){
	OP_NEXT
}
	TARGET(MOD){
	OP_NEXT
}
	TARGET(POW){
	OP_NEXT
}
	TARGET(DIV){
	OP_NEXT
}
	TARGET(IDIV){
	OP_NEXT
}
	TARGET(BAND){
	OP_NEXT
}
	TARGET(BOR){
	OP_NEXT
}
	TARGET(BXOR){
	OP_NEXT
}
	TARGET(SHL){
	OP_NEXT
}
	TARGET(SHR){
	OP_NEXT
}
	TARGET(CONCAT){
	OP_NEXT
}
	TARGET(EQ){
	OP_NEXT
}
	TARGET(LT){
	OP_NEXT
}
	TARGET(LE){
	OP_NEXT
}
	TARGET(TESTSET){
	OP_NEXT
}
	TARGET(CALL){
	ra=R(ARG_A());
//	if(ARG_B()!=0)
	ci->top=ts->top=ra+ARG_B()+1;
	PC += opcodes[op].size;
	if((a=call_prepare(ts, ra,NULL,ARG_C()-1))==1) {
		ci=ts->ci;
		goto FrameEntry;
	} else {
		base=ci->base;
	}
	OP_NEXT
}
	TARGET(CALL_KW){
	mapIter iter;
	ra=R(ARG_A());
	ci->top=rb=ra+ARG_B();
	MapObj *kw=cast(MapObj*,*rb);
	kw=(MapObj*)map_clone(ts, kw);
	map_iter(kw, &iter);
	while(map_next(&iter)) {
		IntObj *idx=(IntObj *)iter.entry.dict->val;
		iter.entry.dict->val=*(RK(idx->ival));
	}
	*rb=(Object*)kw;
	if((a=call_prepare(ts, ra,*rb,ARG_C()-1))==1) {
		ci=ts->ci;
		goto FrameEntry;
	} else if(a==0) {
		base=ci->base;
		PC += opcodes[op].size;
	}
	OP_NEXT
}
	TARGET(SETLIST){
	OP_NEXT
}
	TARGET(JMP_IF_FALSE){
	OP_NEXT
}
	TARGET(JMP_IF_TRUE){
	OP_NEXT
}
	error: {
//		p->lineinfo
	}
	return 0;
}
