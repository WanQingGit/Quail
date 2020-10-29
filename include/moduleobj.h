/*
 *  Created on: Jul 26, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#ifndef INCLUDE_MODULEOBJ_H_
#define INCLUDE_MODULEOBJ_H_
#include "typeobj.h"
typedef struct _module {
	OBJ_HEAD
	Proto main;
	Object *dict;
	stkId consts;
	Object *name;
	char *source;
	int nconst;
} ModuleObj;
ModuleObj *module_new(ThreadState *ts, Object *name);
#endif /* INCLUDE_MODULEOBJ_H_ */
