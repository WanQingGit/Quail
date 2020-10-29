/*
 *  Created on: Jul 19, 2019
 *  Author: WanQing<1109162935@qq.com>
 */

#ifndef INCLUDE_MAPOBJ_H_
#define INCLUDE_MAPOBJ_H_
#include "typeobj.h"

typedef struct EntryDict {
	struct EntryDict *next;
	Int hash;
	Object *key;
	Object *val;
} EntryDict;
typedef struct EntrySet {
	struct EntrySet *next;
	Int hash;
	Object *key;
} EntrySet;

typedef union _entry {
	EntryDict *dict;
	EntrySet *set;
} MapEntry;

typedef struct _mapobj {
	VAR_HEAD
	MapEntry *entry;
	Int size;
	Int mod;
//	TypeObj *keyType;
//	TypeObj *valType;
	int mapFlag;
#ifdef QDEBUG
	uint nfilled;
#endif
} MapObj;

typedef struct mapIter {
	MapObj *m;
	int index;
	MapEntry entry;
	MapEntry nextEntry;
} mapIter;
int map_resize(ThreadState *ts, MapObj *t, unsigned int size);
bool map_del(ThreadState *ts, MapObj *t, Object *key, EntryDict *res);
int map_gset(ThreadState *ts, MapObj *t, Object *key, bool insert,
		MapEntry *res);
int map_gset_str(ThreadState *ts, MapObj *t, char *key, int len, bool insert,
		MapEntry *res);
int map_gset_int(ThreadState *ts, MapObj *t, Int key, bool insert,
		MapEntry *res);
int map_gset_knowHash(ThreadState *ts, MapObj *t, Object *key, Int hash,
		bool insert, MapEntry *res);
void map_iter(MapObj *t, mapIter *iter);
bool map_next(mapIter *iter);
Object* map_new(ThreadState *ts, bool isDict);
Object *map_clone(ThreadState *ts, MapObj *map);
void map_destroy(ThreadState *ts, MapObj *map);
#endif /* INCLUDE_MAPOBJ_H_ */
