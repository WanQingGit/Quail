/*
 *  Created on: Jul 26, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#ifndef INCLUDE_FUNCOBJ_H_
#define INCLUDE_FUNCOBJ_H_
#include "typeobj.h"
typedef struct _funcobj {
	VAR_HEAD
	Object *arg_default;
	Proto *p;
	UpVal *upvals[];
} FuncObj;
typedef struct {
	OBJ_HEAD
	Object *func;
	Object *self;
} MethodObj;
Object *func_new(ThreadState *ts, Proto *p, int n);
#endif /* INCLUDE_FUNCOBJ_H_ */
