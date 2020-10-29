/*
 *  Created on: Jul 28, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#include "classobj.h"
#include "mapobj.h"
#include "strobj.h"
#include "tupleobj.h"
#include "qcall.h"
//@formatter:off

//@formatter:on

void class_new(ThreadState *ts, Object *name, stkId base, int argc) {
	TypeObj *super, *meta;
	Object * cls;
	if (argc)
		super = (TypeObj *) base[0];
	else
		super = &Type_Object;
//	TupleObj *tuple = (TupleObj*) tuple_new(ts, 3);
	stack_check(ts, 3);
	stkId args = ts->top;
	args[0] = (Object *) name;
	args[1] = (Object*) super;
	args[2] = map_new(ts, true);
	stack_inc(ts, 3);
	meta = super->_base.type;
	assert(meta->f_call(ts,(Object*)meta, args,3, NULL)==1);
	cls = stack_pop(ts);
	stack_dec(ts, 3);
	*base = (Object*) cls;
}
