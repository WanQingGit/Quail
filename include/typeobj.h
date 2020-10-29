/*
 *  Created on: Jul 18, 2019
 *  Author: WanQing<1109162935@qq.com>
 */

#ifndef INCLUDE_TYPEOBJ_H_
#define INCLUDE_TYPEOBJ_H_

#include "Quail.h"

typedef struct _intobj {
	OBJ_HEAD
	long long ival;
} IntObj;

typedef struct _listobj {
	VAR_HEAD
	Object **item;
	Int capacity;
} ListObj;

extern TypeObj Type_Type;
extern TypeObj Type_Object;
extern TypeObj Type_Null;
extern TypeObj Type_Int;
extern TypeObj Type_Float;
extern TypeObj Type_Str;
extern TypeObj Type_List;
extern TypeObj Type_Dict;
extern TypeObj Type_Set;
extern TypeObj Type_Func;
extern TypeObj Type_Module;
extern TypeObj Type_Tuple;
extern TypeObj Type_Method;
extern TypeObj Type_Unboundmethod;
extern Object Object_Null;
Object *object_new(ThreadState *ts, TypeObj *tp);
Object *object_newVar(ThreadState *ts, TypeObj *tp, int n);
int type_inherit(TypeObj *type);
Object *find_name_in_mro(TypeObj *type, Object *name);
Object *lookup_maybe_method(ThreadState *ts, Object *self, Object *name);
#endif /* INCLUDE_TYPEOBJ_H_ */
