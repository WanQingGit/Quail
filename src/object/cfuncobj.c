/*
 *  Created on: Jul 28, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */
#include "cfuncobj.h"

//@formatter:off
TypeObj Type_CFunc = {
		{ &Type_Type, 0 },
		0,
		"cfunction",
		sizeof(CFuncObj),
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
		BT_CFUNC
};
//@formatter:on

Object *cfunc_new(ThreadState *ts, CFunction fn, int nup) {
	CFuncObj *cf = (CFuncObj*) object_newVar(ts, &Type_CFunc, nup);
	cf->fn = fn;
	return (Object*) cf;
}

