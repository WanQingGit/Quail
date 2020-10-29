/*
 *  Created on: Jul 19, 2019
 *  Author: WanQing<1109162935@qq.com>
 */
#include "typeobj.h"
#include "mapobj.h"
#include "mem_alloc.h"
#include "tupleobj.h"
#include "strobj.h"
#include "qcall.h"
static Int type_call(ThreadState *ts, TypeObj *type, stkId args, int argc,
		Object *kwds);
static Object *type_new(ThreadState *ts, TypeObj *metatype, stkId args,
		int argc, Object *kwds);

//@formatter:off
TypeObj Type_Type = {
		{ &Type_Type, 0 },
		0,
		"type",
		sizeof(TypeObj),
		0,
		(hashfn) NULL,
		(comparefn) NULL,
		(destructor)NULL,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) type_call,
		(newfn) type_new,
		(initfn) NULL,
		NULL,
		BT_CLASS
};


TypeObj Type_Null = {
		{ &Type_Type, 0 },
		0,
		"null",
		0,
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
		BT_NIL
};


//@formatter:on
Object Object_Null = { &Type_Null, 1 };
Object *object_new(ThreadState *ts, TypeObj *tp) {
	Object *op = (Object*) mem_alloc(ts, NULL, 0, tp->basicsize);
	if (op != NULL) {
		op->nref = 1;
		op->type = tp;
		pthread_spin_init(&op->spinlock, PTHREAD_PROCESS_PRIVATE);
	}
	return op;
}
Object *object_newVar(ThreadState *ts, TypeObj *tp, int n) {
	VarObject *op = (VarObject*) mem_alloc(ts, NULL, 0,
			tp->basicsize + tp->itemsize * n);
	if (op != NULL) {
		op->length = n;
		op->_base.nref = 1;
		op->_base.type = tp;
		pthread_spin_init(&op->_base.spinlock, PTHREAD_PROCESS_PRIVATE);
	}
	return (Object*) op;
}
int type_inherit(TypeObj *type) {
	TypeObj *base = type->super;
	if (base == NULL) {
		type->super = base = &Type_Object;
	}
	if (type->hash == NULL)
		type->hash = base->hash;
	if (type->compare == NULL)
		type->compare = base->compare;
	if (type->destroy == NULL)
		type->destroy = base->destroy;
	if (type->f_init == NULL)
		type->f_init = base->f_init;
	if (type->f_new == NULL)
		type->f_new = base->f_new;
	if (type->f_setattr == NULL)
		type->f_setattr = base->f_setattr;
	if (type->f_getattr == NULL)
		type->f_getattr = base->f_getattr;
	return 0;
}
static Int type_call(ThreadState *ts, TypeObj *cls, stkId args, int argc,
		Object *kwds) {
	Object *obj = cls->f_new(ts, cls, args, argc, kwds);
//	obj->dict=
	if (cls->f_init != NULL) {
		cls->f_init(ts, obj, args, argc, kwds);
	}
	stack_push(ts, obj);
	return 1;
}
static Object *type_new(ThreadState *ts, TypeObj *metatype, stkId args,
		int argc, Object *kwds) {
	StrObj *name = cast(StrObj*, args[0]);
	TypeObj *base = (TypeObj*) args[1];
	Object *dict = (Object*) args[2];
	TypeObj* obj = (TypeObj*) object_newVar(ts, metatype, 0);
	obj->name = name->val;
	obj->super = base;
	obj->dict = dict;
	obj->dictoffset = base->basicsize;
	Int basesize = base->basicsize + sizeof(Object*);
	obj->basicsize = basesize;
	obj->itemsize = base->itemsize;
	int nmro = 2;
	while (base->super) {
		base = base->super;
		nmro++;
	}
	base = obj;
	TupleObj* tuple = (TupleObj*) tuple_new(ts, nmro);
	for (int i = 0; i < nmro; i++) {
		tuple->item[i] = (Object*) base;
		base = base->super;
	}
	obj->mro = (Object*) tuple;
	obj->basetype = BT_CLASS;
	obj->f_call = metatype->f_call;
	type_inherit(obj);
	return (Object*) obj;
}
Object *lookup_maybe_method(ThreadState *ts, Object *self, Object *name) {
	Object *res = find_name_in_mro(Qu_TYPE(self), name);
	if (res) {
		descrgetfn f = Qu_TYPE(res)->descr_get;
		if (f) {
			res = f(ts, res, self, Qu_TYPE(self));
		}
	}
	return res;
}

Object *find_name_in_mro(TypeObj *type, Object *name) {
	TupleObj *tuple = (TupleObj*) type->mro;
	Int hash = cast(StrObj*,name)->hash;
	MapEntry res;
	for (int i = 0; i < tuple->length; i++) {
		Object *dict = cast(TypeObj*,tuple->item[i])->dict;
		if (dict) {
			if (map_gset_knowHash(NULL, (MapObj*) dict, name, hash, false, &res)) {
				return res.dict->val;
			}
		}
	}
	return NULL;
}

