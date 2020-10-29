/*
 *  Created on: Jul 28, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#ifndef INCLUDE_CFUNCOBJ_H_
#define INCLUDE_CFUNCOBJ_H_
#include "typeobj.h"
typedef int (*CFunction)(ThreadState *ts, stkId args,int argc, Object *kwds);
typedef struct _cfuncobj {
	VAR_HEAD
	CFunction fn;
	UpVal *upval[];
} CFuncObj;
Object *cfunc_new(ThreadState *ts, CFunction fn, int nup);
#endif /* INCLUDE_CFUNCOBJ_H_ */
