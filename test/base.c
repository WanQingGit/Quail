/*
 *  Created on: Jul 22, 2019
 *  Author: WanQing<1109162935@qq.com>
 */
#include "qstate.h"
#include "listobj.h"
#include "intobj.h"
#include "mapobj.h"
#include "strobj.h"
#include "baselib.h"
void list_test(ThreadState *ts) {
	ListObj *l = list_new(ts, 4);
	for (Int i = 0; i < 6; i++) {
		list_append(ts, l, (Object *) int_new(ts, i));
	}
	for (Int i = 0; i < l->length; i++) {
		printf("%lld\n", cast(IntObj*,l->item[i])->ival);
	}
	list_destroy(ts, l);
}
void map_test(ThreadState *ts) {
	MapObj *map = (MapObj *) map_new(ts, true);
	MapEntry res;
	for (Int i = 0; i < 6; i++) {
		IntObj *o = (IntObj*) int_new(ts, i);
		assert(map_gset(ts, map, (Object* )o, true, &res) == false);
		res.dict->val = int_new(ts, i + 1);
	}
	for (Int i = 0; i < 6; i++) {
		IntObj *o = (IntObj*) int_new(ts, i);
		assert(map_gset(ts, map, (Object* )o, true, &res) == true);
		assert(res.dict->val == cast(Object*,int_new(ts, i+1)));
	}
	map_destroy(ts, map);
}
void str_test(ThreadState *ts) {
	char *s = "WanQing";
	int len = strlen(s);
	StrObj *str = str_new(ts, s, len);
	assert(strcmp(s, str->val) == 0);
	StrObj *str2 = str_new(ts, s, len);
	assert(str2 == str);
}

int main() {
	GlobalState *gs = GlobalState_New();
	ThreadState *ts = ThreadState_New(gs);
	GlobalState_Init(ts);
	bltinTypeInit();
	list_test(ts);
	map_test(ts);
	str_test(ts);
	return 0;
}
