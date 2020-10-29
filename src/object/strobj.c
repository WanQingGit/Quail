/*
 * qstrbuffer.c
 *
 *  Created on: Apr 11, 2019
 *  Author: WanQing<1109162935@qq.com>
 */

#include <strobj.h>
#include <string.h>
#include "qstrutils.h"
#include "mapobj.h"
#include "mem_alloc.h"
#include "qstate.h"
#define STRMAXLEN 32

static StrObj *gshrstr(ThreadState *ts, const char *str, size_t l);
static StrObj *str_newlstr(ThreadState *ts, const char* str, Int len);
StrObj *str_new(ThreadState *ts, const char *str, size_t l);
Int str_hash(StrObj *o);
Int str_compare(StrObj *a, StrObj *b);
//@formatter:off
TypeObj Type_Str = {
		{ &Type_Type, 0 },
		0,
		"str",
		sizeof(StrObj),
		0,
		(hashfn) str_hash,
		(comparefn) str_compare,
		(destructor)NULL,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) NULL,
		(initfn) NULL,
		NULL,
		BT_STR
};
//@formatter:on
StrObj *string_init;
StrObj *string_new;
StrObj *string_this;
#define META_STR(name) "__"#name"__"
#define META_INIT(name) if((string_##name=gshrstr(ts,META_STR(name),strlen(META_STR(name))))==NULL) return 0;
#define KEYSTR_INIT(name) if((string_##name=gshrstr(ts,""#name"",strlen(""#name"")))==NULL) return 0;
int str_meta_init(ThreadState *ts) {
	META_INIT(init);
	META_INIT(new);
	KEYSTR_INIT(this);
	return 1;
}
Int str_hash(StrObj *o) {
	if (o->hash != -1)
		return o->hash;
	Int hash = (Int) str_hash_count(o->val, (size_t) o->length, 111);
	o->hash = hash;
	return hash;
}
Int str_compare(StrObj *a, StrObj *b) {
	if (a == b)
		return 0;
	return (Int) strcmp(a->val, b->val);
}

static StrObj *gshrstr(ThreadState *ts, const char *str, size_t len) {
	MapEntry res;
	MapObj* strt = (MapObj*) ts->gs->strTable;
	if (map_gset_str(ts, strt, (char *) str, len, true, &res)) {
		REF_INC(res.set->key);
		return (StrObj*) res.set->key;
	}
	StrObj *sp = (StrObj*) mem_alloc(ts, NULL, 0,
			sizeof(StrObj) + (len + 1) * sizeof(char));
	sp->_base.type = &Type_Str;
	sp->_base.nref = 1;
	sp->hash = res.dict->hash;
	pthread_spin_init(&sp->_base.spinlock, PTHREAD_PROCESS_PRIVATE);
	memcpy(sp->val, str, len);
	sp->val[len] = '\0';
	res.set->key = (Object*) sp;
	return sp;
}

StrObj *str_new(ThreadState *ts, const char *str, size_t l) {
	if (l <= STRMAXLEN)
		return gshrstr(ts, str, l);
	else {
		return str_newlstr(ts, str, l);
	}
}

static StrObj *str_newlstr(ThreadState *ts, const char* str, Int l) {
	StrObj *sp = (StrObj*) mem_alloc(ts, NULL, 0,
			sizeof(StrObj) + (l + 1) * sizeof(char));
	memcpy(sp->val, str, l);
	sp->val[l] = '\0';
	sp->hash = 0;
	return sp;
}

