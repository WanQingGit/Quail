/*
 *  Created on: Jul 26, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */
#include "moduleobj.h"
#include "mapobj.h"
#include "strobj.h"
static Object *name_key;
//@formatter:off
TypeObj Type_Module = {
		{ &Type_Type, 0 },
		0,
		"module",
		sizeof(ModuleObj),
		0,
		(hashfn) NULL,
		(comparefn) NULL,
		(destructor)NULL,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) NULL,
		(initfn) NULL,
		NULL,
		BT_MODULE
};

ModuleObj *module_new(ThreadState *ts,Object *name){
	if(name_key==NULL){
		char *name_str= "__name__";
		name_key=(Object*)str_get(ts,name_str);
	}
	ModuleObj *module=(ModuleObj *)object_new(ts, &Type_Module);
	MapObj *dict=(MapObj*)map_new(ts, true);
	MapEntry res;
	map_gset(ts, dict, name_key, true, &res);
	res.dict->val=name;
	module->dict=(Object*)dict;
	module->name=name;
	return module;
}
//@formatter:on
