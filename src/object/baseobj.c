/*
 *  Created on: Aug 2, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */
#include "baseobj.h"
#include "typeobj.h"
#include "mapobj.h"
#include "strobj.h"
#include "qcall.h"
static Object *obj_new(ThreadState *ts, TypeObj *cls, Object *args,
		Object *kwds);
int slot_init(ThreadState *ts, Object *self, stkId args, int argc, Object *kwds);
int obj_setattr(ThreadState *ts, Object *obj, Object *name, Object *v);

Object* obj_getattr(ThreadState *ts, Object *obj, Object *name);
//@formatter:off
TypeObj Type_Object = {
		{ &Type_Type, 0 },
		0,
		"object",
		sizeof(Object),
		0,
		(hashfn) NULL,
		(comparefn) NULL,
		(destructor)NULL,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) obj_new,
		(initfn) slot_init,
		NULL,
		BT_OBJECT,
		(getattrfn) obj_getattr,
		(setattrfn) obj_setattr,
};
//@formatter:on
Object **object_getDict(Object *o) {
	TypeObj *tp = Qu_TYPE(o);
	Int offset = tp->dictoffset;
	if (offset == 0)
		return NULL;
	return (Object**) ((char*) o + offset);
}

int slot_init(ThreadState *ts, Object *self, stkId args, int argc, Object *kwds) {
	Object *meth = lookup_maybe_method(ts, self, (Object*) string_init);
	call_object(ts, meth, args, argc, kwds, 1);
	return 1;
}

static Object *obj_new(ThreadState *ts, TypeObj *cls, Object *args,
		Object *kwds) {
	Object* obj = object_new(ts, cls);
	return obj;
}
int object_setattr(ThreadState *ts, Object *obj, Object *name, Object *v) {
	TypeObj *tp = Qu_TYPE(obj);
	if (tp->f_setattr) {
		return tp->f_setattr(ts, obj, name, v);
	}
	return -1;
}
int object_getattr(ThreadState *ts, Object *obj, Object *name, stkId ra) {
	TypeObj *tp = Qu_TYPE(obj);
	if (tp->f_getattr) {
		Object *v = tp->f_getattr(ts, obj, name);
		if (v) {
			*ra = v;
			return 1;
		}
	}
	return -1;
}
int obj_setattr(ThreadState *ts, Object *obj, Object *name, Object *v) {
	TypeObj *tp = Qu_TYPE(obj);
	Object *descr, **dict_ptr, *dict;
	descrsetfn f;
	MapEntry entry;
	descr = find_name_in_mro(tp, name);
	if (descr != NULL) {
		f = descr->type->descr_set;
		if (f != NULL) {
			return f(ts, descr, obj, v);
		}
	}
	dict_ptr = object_getDict(obj);
	dict = *dict_ptr;
	if (dict == NULL) {
		*dict_ptr = map_new(ts, true);
		dict = *dict_ptr;
	}
	map_gset(ts, (MapObj*) dict, name, true, &entry);
	entry.dict->val = v;
	return 1;
}
Object* obj_getattr(ThreadState *ts, Object *obj, Object *name) {
	Object *descr, *res;
	TypeObj *tp = Qu_TYPE(obj);
	descrgetfn f;
	MapEntry entry;
	descr = find_name_in_mro(tp, name);
	if (descr != NULL) {
		f = descr->type->descr_get;
		if (f != NULL && descr->type->descr_set) {
			res = f(ts, descr, obj, tp);
			return res;
		}
	}
	Object **dict_ptr = object_getDict(obj);
	Object *dict = *dict_ptr;
	if (dict_ptr != NULL && dict != NULL) {
		if (map_gset(ts, (MapObj*) dict, name, false, &entry)) {
			res = entry.dict->val;
			return res;
		}
	}
	if (f != NULL) {
		res = f(ts, descr, obj, tp);
		return res;
	}
	if (descr)
		return descr;
	return NULL;
}
