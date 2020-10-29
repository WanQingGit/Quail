/*
 *  Created on: Aug 2, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#ifndef INCLUDE_BASEOBJ_H_
#define INCLUDE_BASEOBJ_H_
#include "Quail.h"
Object **object_getDict(Object *o);
int object_setattr(ThreadState *ts, Object *obj, Object *name, Object *v);
int object_getattr(ThreadState *ts, Object *obj, Object *name, stkId ra);
#endif /* INCLUDE_BASEOBJ_H_ */
