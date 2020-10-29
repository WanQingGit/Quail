/*
 *  Created on: Jul 19, 2019
 *  Author: WanQing<1109162935@qq.com>
 */
#include "intobj.h"
#include "number.h"
#include "mem_alloc.h"
#include "mapobj.h"
#include "qstate.h"
int nfree_num = 0;
Object *free_num[MAX_NUM];
Int int_hash(IntObj *i);
Int int_compare(IntObj *a, IntObj *b);
void int_destroy(ThreadState *ts, IntObj *i);
#define INT_DEFAULT_HASH 715827883
//@formatter:off
TypeObj Type_Int = {
		{ &Type_Type, 0 },
		0,
		"int",
		sizeof(IntObj),
		0,
		(hashfn) int_hash,
		(comparefn) int_compare,
		(destructor)int_destroy,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) NULL,
		(initfn) NULL,
		NULL,
		BT_INT
};
//@formatter:on
Int int_hash(IntObj *i) {
	return i->ival != -1 ? i->ival : INT_DEFAULT_HASH;
}
Object *int_new(ThreadState *ts, Int i) {
	Object *o;
	MapObj *map = (MapObj *) ts->gs->intTable;
	Int pos = i & (map->size - 1);
	EntrySet *entry = map->entry[pos].set;
	while (entry) {
		if (cast(IntObj*,entry->key)->ival == i) {
			o = entry->key;
			REF_INC(o);
			return o;
		}
		entry = entry->next;
	}
	if (nfree_num > 0) {
		o = free_num[--nfree_num];
		o->type = &Type_Int;
		o->nref = 1;
	} else {
		o = object_new(ts, &Type_Int);
	}
	cast(IntObj*,o)->ival = i;
	entry = (EntrySet*) mem_alloc(ts, NULL, 0, sizeof(EntrySet));
	entry->hash = i;
	entry->key = o;
	entry->next = map->entry[pos].set;
	map->entry[pos].set = entry;
	return o;
}
Int int_compare(IntObj *a, IntObj *b) {
	return a->ival - b->ival;
}
void int_destroy(ThreadState *ts, IntObj *i) {
	i->_base.nref = 1;
//	MapObj *map=ts->gs->intTable;
}
