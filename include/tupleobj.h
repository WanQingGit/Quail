/*
 *  Created on: Jul 29, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#ifndef INCLUDE_TUPLEOBJ_H_
#define INCLUDE_TUPLEOBJ_H_
#include "typeobj.h"

typedef struct {
	VAR_HEAD
	Object *item[];
} TupleObj;
Object *tuple_new(ThreadState *ts, int n);
extern Object *tupleNil;
#endif /* INCLUDE_TUPLEOBJ_H_ */
