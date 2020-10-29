#include "mem_alloc.h"
#include "listobj.h"
#include "tupleobj.h"
#define MAXFREELIST 64
#define DEFAULT_SIZE  8
static ListObj *free_list[MAXFREELIST];
static int numfreelist = 0;
//@formatter:off
TypeObj Type_List = {
		{ &Type_Type, 0 },
		0,
		"list",
		sizeof(ListObj),
		0,
		NULL,
		NULL,
		(destructor)list_destroy,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) NULL,
		(initfn) NULL,
		NULL,
		BT_LIST,
};
//@formatter:on
ListObj *list_new(ThreadState *ts, int cap) {
	ListObj *l;
	if (cap == 0)
		cap = DEFAULT_SIZE;
	if (numfreelist > 0) {
		numfreelist--;
		l = free_list[numfreelist];
		l->_base.nref = 1;
	} else {
		l = (ListObj*) object_new(ts, &Type_List);
	}
	l->length = 0;
	l->item = (Object **) mem_alloc(ts, NULL, 0, sizeof(Object *) * cap);
	l->capacity = cap;
	return l;
}
ListObj *list_append(ThreadState *ts, ListObj *list, Object *data) {
	if (list->length >= list->capacity) {
		list_resize(ts, list, -1);
	}
	REF_INC(data);
	list->item[list->length++] = data;
	return list;
}
ListObj *list_resize(ThreadState *ts, ListObj *list, int nsize) {
	if (nsize < 0) {
		int used = list->length;
		if (used >= list->capacity) {
			sky_assert(used == list->capacity);
			if (used < 64)
				nsize = used * 2;
			else {
				nsize = used * 1.5;
			}
			if (nsize < 4)
				nsize = 4;
//			if (nsize < used)
//				skyc_runerror(NULL, "ListObj overflow");
		} else {
			return list;
		}
	}
//	list->item = (Object **) realloc(list->item, sizeof(Object *) * nsize);
	list->item = (Object **) mem_alloc(ts, list->item,
			sizeof(Object *) * list->capacity, sizeof(Object *) * nsize);
	list->capacity = nsize;
	return list;
}

ListObj *list_add(ThreadState *ts, ListObj *l, int index, Object *data) {
	int used = l->length;
	if (index < 0) {
		index += l->length;
		sky_assert(index >= 0);
	}
	sky_assert(index <= used);
	if (used >= l->capacity)
		list_resize(ts, l, -1);
	Object * *ptr = l->item;
	for (; used > index; used--) {
		ptr[used] = ptr[used - 1];
	}
	ptr[index] = data;
	l->length++;
	return l;
}

ListObj *list_remove(ThreadState *ts, ListObj *list, int index) {
	int used = list->length;
	sky_assert(index < used && index >= -used);
	if (index < 0)
		index += used;
	Object * *data = list->item;
	used -= 2;
	for (int i = index; i < used; i++) {
		data[i] = data[i + 1];
	}
	list->length--;
	if ((list->length < list->capacity / 4) && (list->capacity > 8))
		list_resize(ts, list, list->capacity / 2);
	return list;
}

Object *list_at(ThreadState *ts, ListObj *l, size_t index) {
	int used = l->length;
	sky_assert(index < used && index >= -used);
	if (index < 0)
		return l->item[index + used];
	return l->item[index];
}
ListObj *list_addArray(ThreadState *ts, ListObj *list, Object * *a, size_t n) {
	int used = list->length;
	int nused = used + n;
	if (nused > list->capacity) {
		list_resize(ts, list, nused * 1.2);
		sky_check(list->capacity <= nused, "ListObj overflow!");
	}
	Object * *data = list->item + used;
	for (int i = 0; i <= n; i++) {
		data[i] = a[i];
	}
	list->length = nused;
	return list;
}
ListObj *list_addFromVec(ThreadState *ts, ListObj *list, ListObj *a) {
	int used = list->length;
	int nused = used + a->length;
	if (nused >= list->capacity) {
		list_resize(ts, list, nused * 1.2);
		sky_check(list->capacity >= nused, "qvec overflow!");
	}
	Object * *data = list->item + used;
	int n = a->length;
	for (int i = 0; i < n; i++) {
		data[i] = a->item[i];
	}
	list->length = nused;
	return list;
}

void list_destroy(ThreadState *ts, ListObj *l) {
	Object * *data = l->item;
	register int len = l->length;
	for (int i = 0; i < len; i++) {
//		Object *_op = l->item[i];
//		pthread_spin_lock(&_op->spinlock);
//		_op->nref--;
//		if (_op->nref <= 0) {
//			pthread_spin_unlock(&_op->spinlock);
//			assert(_op->nref == 0);
//			pthread_spin_destroy(&_op->spinlock);
//			_op->type->destroy(ts, _op);
//		} else {
//			pthread_spin_unlock(&_op->spinlock);
//		}
		REF_DEC(ts, l->item[i]);
	}
	mem_alloc(ts, data, sizeof(Object *) * l->capacity, 0);
	if (numfreelist < MAXFREELIST) {
		free_list[numfreelist] = l;
		numfreelist++;
	} else {
		pthread_spin_destroy(&l->_base.spinlock);
		mem_alloc(ts, l, sizeof(ListObj), 0);
	}
}
Object *pop_back(ThreadState *ts, ListObj *list) {
	if (list->length > 0) {
		list->length--;
		if ((list->length < list->capacity / 4) && (list->capacity > 16))
			list_resize(ts, list, list->capacity / 2);
		return list->item[list->length];
	}
	return cast(Object *, 0);
}
void list_shrink(ThreadState *ts, ListObj *list) {
	list->item = (Object **) mem_alloc(ts, list->item,
			sizeof(Object *) * list->capacity, sizeof(Object *) * list->length);
	list->capacity = list->length;
}
ListObj *list_clone(ThreadState *ts, ListObj *list) {
	ListObj *newlist = list_new(ts, list->length);
	if (list->length) {
		memcpy(newlist->item, list->item, list->length * sizeof(Object *));
		newlist->length = list->length;
	}
	return newlist;
}
void list_cache_clear(ThreadState *ts) {
	for (int i = 0; i < numfreelist; i++) {
		mem_alloc(ts, free_list[i], sizeof(ListObj), 0);
	}
	numfreelist = 0;
}
Object *list_toTuple(ThreadState *ts, ListObj *l) {
	TupleObj *tuple = (TupleObj*) tuple_new(ts, l->length);
	for (int i = 0; i < l->length; i++) {
		tuple->item[i] = l->item[i];
	}
	return (Object*) tuple;
}

//struct apiVec Arr = { list_new, list_clone, list_resize, list_append,
//		list_append, list_remove, pop_back, list_at,
//		/*IndexOfS,*/list_destroy, list_addArray, list_addFromVec, sort, add,
//		addSort, list_shrink };
