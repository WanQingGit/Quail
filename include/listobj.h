/*
 *  Created on: Jul 22, 2019
 *  Author: WanQing<1109162935@qq.com>
 */

#ifndef INCLUDE_LISTOBJ_H_
#define INCLUDE_LISTOBJ_H_
#include "typeobj.h"
ListObj *list_new(ThreadState *ts, int cap);
ListObj *list_append(ThreadState *ts, ListObj *list, Object *data);
ListObj *list_resize(ThreadState *ts, ListObj *list, int nsize);
ListObj *list_add(ThreadState *ts, ListObj *l, int index, Object *data);
Object *list_at(ThreadState *ts, ListObj *list, size_t index);
ListObj *list_remove(ThreadState *ts, ListObj *list, int index);
ListObj *list_addArray(ThreadState *ts, ListObj *list, Object * *a, size_t n);
ListObj *list_addFromVec(ThreadState *ts, ListObj *list, ListObj *a);
void list_destroy(ThreadState *ts, ListObj *l);
Object *pop_back(ThreadState *ts, ListObj *list);
void list_shrink(ThreadState *ts, ListObj *list);
ListObj *list_clone(ThreadState *ts, ListObj *list);
Object *list_toTuple(ThreadState *ts, ListObj *l);
void list_cache_clear(ThreadState *ts);

#endif /* INCLUDE_LISTOBJ_H_ */
