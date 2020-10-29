/*
 *  Created on: Jul 28, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */
#include "cfuncobj.h"
#include "mapobj.h"
#include "strobj.h"
int blt_print(ThreadState *ts, stkId args, int argc, Object *kwds) {
	printf("blt_print\n");
	return 0;
}
//@formatter:off
struct cfunclib{
	char *name;
	CFunction fn;
} funclist[] = {
		{ "print", blt_print },
		{ NULL, 	 NULL      }

};
struct bltinType{
	char *name;
	TypeObj *type;
} typelist[] = {
		{ "object", &Type_Object },
		{ "dict",   &Type_Dict   },
		{ "set",    &Type_Set    },
		{ "list",   &Type_List   },
		{ "tuple",  &Type_Tuple  },
		{ "int",    &Type_Int    },
		{ "float",  &Type_Float  },
		{ "str",    &Type_Str    },
		{ "func",   &Type_Func   },
		{ NULL, 	  NULL         }
};
//@formatter:on
void global_init(ThreadState *ts) {
	struct cfunclib *cf = funclist;
	MapObj *glTable = (MapObj*) ts->gs->glTable;
	MapEntry res;
	for (; cf->name; cf++) {
		Object *fo = cfunc_new(ts, cf->fn, 0);
		assert(
				map_gset_str(ts, glTable, cf->name, strlen(cf->name), true, &res)
						== false);
		res.dict->key = (Object*) str_get(ts, cf->name);
		res.dict->val = fo;
	}
	struct bltinType *type = typelist;
	for (; type->name; type++) {
		assert(
				map_gset_str(ts, glTable, type->name, strlen(type->name), true, &res)
						== false);
		res.dict->key = (Object*) str_get(ts, type->name);
		res.dict->val = (Object*) type->type;
	}
}

