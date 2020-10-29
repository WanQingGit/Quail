/*
 *  Created on: Jul 24, 2019
 *  Author: WanQing<1109162935@qq.com>
 */

#ifndef INCLUDE_INTOBJ_H_
#define INCLUDE_INTOBJ_H_
#include "typeobj.h"
Object *int_new(ThreadState *ts, Int i);
extern TypeObj Type_Int;
extern TypeObj Type_Bool;
extern IntObj Obj_False;
extern IntObj Obj_True;
#endif /* INCLUDE_INTOBJ_H_ */
