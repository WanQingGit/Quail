/*
 *  Created on: Jul 26, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */
#include "funcobj.h"
#include "classobj.h"

Object *meth_new(ThreadState *ts, Object *func, Object *self);
Object *func_descr_get(ThreadState *ts, Object *func, Object *obj,
		TypeObj *type);

//@formatter:off
TypeObj Type_Func = {
		{ &Type_Type, 0 },
		0,
		"function",
		sizeof(FuncObj),
		sizeof(UpVal*),
		(hashfn) NULL,
		(comparefn) NULL,
		(destructor)NULL,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) NULL,
		(initfn) NULL,
		NULL,
		BT_FUNCTION
};
TypeObj Type_Unboundmethod = {
		{ &Type_Type, 0 },
		0,
		"unbound method",
		sizeof(FuncObj),
		sizeof(UpVal*),
		(hashfn) NULL,
		(comparefn) NULL,
		(destructor)NULL,
		(descrgetfn) func_descr_get,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) NULL,
		(initfn) NULL,
		NULL,
		BT_FUNCTION
};
TypeObj Type_Staticmethod = {
		{ &Type_Type, 0 },
		0,
		"static method",
		sizeof(FuncObj),
		sizeof(UpVal*),
		(hashfn) NULL,
		(comparefn) NULL,
		(destructor)NULL,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) NULL,
		(initfn) NULL,
		NULL,
		BT_FUNCTION
};
TypeObj Type_Method = {
		{ &Type_Type, 0 },
		0,
		"method",
		sizeof(MethodObj),
		0,
		(hashfn) NULL,
		(comparefn) NULL,
		(destructor)NULL,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) NULL,
		(initfn) NULL,
		NULL,
		BT_METHOD
};
//@formatter:on

Object *func_new(ThreadState *ts, Proto *p, int n) {
	FuncObj *func = (FuncObj*) object_newVar(ts, &Type_Func, n);
	func->p = p;
	return (Object*) func;
}
Object *func_descr_get(ThreadState *ts, Object *func, Object *obj,
		TypeObj *type) {
	return meth_new(ts, func, obj);
}
Object *meth_new(ThreadState *ts, Object *func, Object *self) {
	MethodObj *meth = (MethodObj*) object_new(ts, &Type_Method);
	meth->func = func;
	meth->self = self;
	return (Object*) meth;
}
