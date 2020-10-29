/*
 *  Created on: 2019年3月12日
 *      Author: WanQing
 */
#include "mapobj.h"
#include "mem_alloc.h"
#include "strobj.h"
#include "qstrutils.h"
#define MAP_MINSIZE 8
#define MAXFREEMAP 256
static MapObj *free_map[MAXFREEMAP];
static int numfreeMap = 0;
#define uentry(t) t->entry
#define gentry(t,pos) (t->entry[pos])
#define gentryMap(t,pos) gentry(t,pos).dict
#define gentrySet(t,pos) gentry(t,pos).set

Int map_hash(MapObj *map);
Int map_compare(MapObj *a, MapObj *b);
void map_set_knowHashWithoutCompare(ThreadState *ts, MapObj *t, Object *key,
		Int hash, MapEntry *res);
//@formatter:off
TypeObj Type_Dict = {
		{ &Type_Type, 0 },
		0,
		"dict",
		sizeof(MapObj),
		0,
		(hashfn)map_hash,
		(comparefn)map_compare,
		(destructor)map_destroy,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) NULL,
		(initfn) NULL,
		NULL,
		BT_DICT
};
TypeObj Type_Set = {
		{ &Type_Type, 0 },
		0,
		"set",
		sizeof(MapObj),
		0,
		(hashfn)map_hash,
		(comparefn)map_compare,
		(destructor)map_destroy,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) NULL,
		(initfn) NULL,
		NULL,
		BT_TYPE
};
//@formatter:on

Int map_hash(MapObj *map) {
	Int hash = 0x345678UL;
	hash ^= map->length;
	hash = hash + map->mod * 133 - 177 * map->mapFlag;
	if (hash == -1)
		return 137389;
	return hash;
}
Int map_compare(MapObj *a, MapObj *b) {
	if (a == b)
		return 0;
	int res = a->length - b->length;
	if (res)
		return res;
	if ((res = a->size - b->size) != 0)
		return res;
	return a - b;
}

int map_resize(ThreadState *ts, MapObj *t, unsigned int size) {
	Int oldsize = t->size, hash, newsize;
	for (newsize = MAP_MINSIZE; newsize < size && newsize > 0; newsize <<= 1)
		;
	if (newsize < 0)
		return -1;
	if (newsize != oldsize) {
		MapEntry *oldt = uentry(t);
		uentry(t) = (MapEntry*) mem_alloc(ts, NULL, 0, newsize * sizeof(MapEntry));
		int mod = newsize - 1;
		t->mod = mod;
		for (Int i = 0; i < newsize; i++)
			t->entry[i].set = NULL;
		t->size = newsize;
		if (t->length) {
			EntrySet *node, *temp;
			for (int i = 0; i < oldsize; i++) {
				node = oldt[i].set;
				while (node) {
					hash = node->hash & mod;
					temp = node->next;
					node->next = uentry(t)[hash].set;
					uentry(t)[hash].set = node;
					node = temp;
				}
			}
		}
		if (oldsize)
			mem_alloc(ts, oldt, oldsize * sizeof(MapEntry), 0);
	}
	return 1;
}

bool map_del(ThreadState *ts, MapObj *t, Object *key, EntryDict *res) {
	Int hash = key->type->hash(key); //sky_hash(key);
	Int pos = hash & t->mod;
	EntryDict *entry = gentryMap(t, pos);
	EntryDict *prev = NULL;
	while (entry) {
		if ((entry->key->type == key->type)
				&& (entry->key->type->compare(entry->key, key) == 0)) {
			if (prev)
				prev->next = entry->next;
			else
				gentry(t, hash).dict = entry->next;
			if (res) { //keyval指向的变量必须要大于sizeof(void*)
				res->key = entry->key;
				if (t->mapFlag) {
					res->val = entry->val;
				}
			}
			if (t->mapFlag)
				mem_alloc(ts, entry, sizeof(EntryDict), 0);
			else
				mem_alloc(ts, entry, sizeof(EntrySet), 0);
			t->length--;
			return true;
		}
		prev = entry;
		entry = entry->next;
	}
	return false;
}
int map_gset_str(ThreadState *ts, MapObj *t, char *key, int len, bool insert,
		MapEntry *res) {
	if (t->length == 0 && insert == false)
		return false;
	Int hash = str_hash_count(key, len, 111);
	Int pos = hash & t->mod;
	EntrySet *entry = gentrySet(t, pos);
	while (entry) {
		if (hash == entry->hash
				&& memcmp(key, cast(StrObj*,entry->key)->val, len) == 0) {
			res->set = entry;
			return true;
		}
		entry = entry->next;
	}
	if (insert) {
		if (t->length >= t->size) {
			map_resize(ts, t, t->size * 2);
			pos = hash & t->mod;
		}
		if (t->mapFlag) {
			entry = (EntrySet*) mem_alloc(ts, NULL, 0, sizeof(EntryDict));
		} else {
			entry = (EntrySet*) mem_alloc(ts, NULL, 0, sizeof(EntrySet));
		}
		entry->hash = hash;
		entry->next = gentrySet(t, pos);
		gentrySet(t, pos) = entry;
		++t->length;
		res->set = entry;
	}
	return false;
}
int map_gset_int(ThreadState *ts, MapObj *t, Int key, bool insert,
		MapEntry *res) {
	if (t->length == 0 && insert == false)
		return false;
	Int pos = key & t->mod;
	EntrySet *entry = gentrySet(t, pos);
	while (entry) {
		if (key == entry->hash) {
			res->set = entry;
			return true;
		}
		entry = entry->next;
	}
	if (insert) {
		if (t->length >= t->size) {
			map_resize(ts, t, t->size * 2);
			pos = key & t->mod;
		}
		if (t->mapFlag) {
			entry = (EntrySet*) mem_alloc(ts, NULL, 0, sizeof(EntryDict));
		} else {
			entry = (EntrySet*) mem_alloc(ts, NULL, 0, sizeof(EntrySet));
		}
		entry->hash = key;
		entry->next = gentrySet(t, pos);
		gentrySet(t, pos) = entry;
		++t->length;
		res->set = entry;
	}
	return false;
}
int map_gset(ThreadState *ts, MapObj *t, Object *key, bool insert,
		MapEntry *res) {
	if (t->length == 0 && insert == false)
		return false;
	Int hash = key->type->hash(key);
	Int pos = hash & t->mod;
	EntrySet *entry = gentrySet(t, pos);
	while (entry) {
		if ((entry->key->type == key->type)
				&& (entry->key->type->compare(entry->key, key) == 0)) {
			res->set = entry;
			return true;
		}
		entry = entry->next;
	}
	if (insert) {
		if (t->length >= t->size) {
			map_resize(ts, t, t->size * 2);
			pos = hash & t->mod;
		}
		if (t->mapFlag) {
			entry = (EntrySet*) mem_alloc(ts, NULL, 0, sizeof(EntryDict));
		} else {
			entry = (EntrySet*) mem_alloc(ts, NULL, 0, sizeof(EntrySet));
		}
		entry->hash = hash;
		entry->key = key;
		entry->next = gentrySet(t, pos);
		gentrySet(t, pos) = entry;
		++t->length;
		res->set = entry;
	}
	return false;
}
int map_gset_knowHash(ThreadState *ts, MapObj *t, Object *key, Int hash,
		bool insert, MapEntry *res) {
	if (t->length == 0 && insert == false)
		return false;
	Int pos = hash & t->mod;
	EntrySet *entry = gentrySet(t, pos);
	while (entry) {
		if ((entry->key->type == key->type)
				&& (entry->key->type->compare(entry->key, key) == 0)) {
			res->set = entry;
			return true;
		}
		entry = entry->next;
	}
	if (insert) {
		if (t->length >= t->size) {
			map_resize(ts, t, t->size * 2);
			pos = hash & t->mod;
		}
		if (t->mapFlag) {
			entry = (EntrySet*) mem_alloc(ts, NULL, 0, sizeof(EntryDict));
		} else {
			entry = (EntrySet*) mem_alloc(ts, NULL, 0, sizeof(EntrySet));
		}
		entry->hash = hash;
		entry->key = key;
		entry->next = gentrySet(t, pos);
		gentrySet(t, pos) = entry;
		++t->length;
		res->set = entry;
	}
	return false;
}

void map_set_knowHashWithoutCompare(ThreadState *ts, MapObj *t, Object *key,
		Int hash, MapEntry *res) {
	if (t->length >= t->size) {
		map_resize(ts, t, t->size * 2);
	}
	Int pos = hash & t->mod;
	EntrySet *entry = gentrySet(t, pos);
	if (t->mapFlag) {
		entry = (EntrySet*) mem_alloc(ts, NULL, 0, sizeof(EntryDict));
	} else {
		entry = (EntrySet*) mem_alloc(ts, NULL, 0, sizeof(EntrySet));
	}
	entry->hash = hash;
	entry->key = key;
	entry->next = gentrySet(t, pos);
	gentrySet(t, pos) = entry;
	++t->length;
	res->set = entry;
}

void map_iter(MapObj *t, mapIter *iter) {
	iter->m = t;
	iter->index = 0;
	iter->entry.dict = iter->nextEntry.dict = NULL;
}
bool map_next(mapIter *iter) {
	if (iter->index < iter->m->size) {
		if (iter->nextEntry.dict) {
			iter->entry = iter->nextEntry;
			iter->nextEntry.dict = iter->nextEntry.dict->next;
			return true;
		} else {
			int i;
			for (i = iter->index; i < iter->m->size; i++) {
				if (iter->m->entry[i].dict) {
					iter->entry = iter->m->entry[i];
					iter->nextEntry.dict = iter->entry.dict->next;
					iter->index = i + 1;
					return true;
				}
			}
			iter->index = i;
		}
	}
	return false;
}
Object* map_new(ThreadState *ts, bool isDict) {
	MapObj *t;
	if (numfreeMap) {
		t = free_map[--numfreeMap];
	} else {
		t = cast(MapObj*, object_new(ts, &Type_Dict));
	}
	if (isDict) {
		t->_base.type = &Type_Dict;
		t->mapFlag = true;
	} else {
		t->_base.type = &Type_Set;
		t->mapFlag = false;
	}
	t->length = t->size = 0;
	map_resize(ts, t, MAP_MINSIZE);
	return (Object*) t;
}
Object *map_clone(ThreadState *ts, MapObj *map) {
	MapObj *nmap = (MapObj*) map_new(ts, map->mapFlag);
	EntryDict *dict;
	MapEntry res;
	if (map->mapFlag) {
		for (int i = 0; i < map->size; i++) {
			dict = map->entry[i].dict;
			while (dict) {
				map_set_knowHashWithoutCompare(ts, nmap, dict->key, dict->hash, &res);
				res.dict->val = dict->val;
				dict = dict->next;
			}
		}
	} else {
		for (int i = 0; i < map->size; i++) {
			dict = map->entry[i].dict;
			while (dict) {
				map_set_knowHashWithoutCompare(ts, nmap, dict->key, dict->hash, &res);
				dict = dict->next;
			}
		}
	}
	return (Object*) nmap;
}
void map_destroy(ThreadState *ts, MapObj *map) {
	if (map->length) {
		EntryDict *entry, *ptr;
		if (map->mapFlag) {
			for (int i = 0; i < map->size; i++) {
				entry = map->entry[i].dict;
				while (entry) {
					ptr = entry;
					REF_DEC(ts, entry->key);
					REF_DEC(ts, entry->val);
					entry = entry->next;
					mem_alloc(ts, ptr, sizeof(EntryDict), 0);
				}
			}
		} else {
			for (int i = 0; i < map->size; i++) {
				entry = map->entry[i].dict;
				while (entry) {
					ptr = entry;
					REF_DEC(ts, entry->key);
					entry = entry->next;
					mem_alloc(ts, ptr, sizeof(EntrySet), 0);
				}
			}
		}
	}
	mem_alloc(ts, map->entry, map->size * sizeof(MapEntry), 0);
	if (numfreeMap < MAXFREEMAP) {
		free_map[numfreeMap++] = map;
	} else {
		pthread_spin_destroy(&map->_base.spinlock);
		mem_alloc(ts, map, sizeof(MapObj), 0);
	}
}
//struct apiMap Map = { map_new, map_resize, map_gset, map_del, map_iter,
//		map_next, map_destroy };
