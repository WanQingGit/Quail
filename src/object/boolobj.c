/*
 *  Created on: Jul 25, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */
#include "intobj.h"

//@formatter:off
TypeObj Type_Bool = {
		{ &Type_Type, 0 },
		0,
		"bool",
		sizeof(IntObj),
		0,
		(hashfn) NULL,
		(comparefn) NULL,
		(destructor)NULL,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) NULL,
		(initfn) NULL,
		&Type_Int,
		BT_BOOL,
		NULL
};
//@formatter:on

IntObj Obj_False = { { &Type_Bool, 1 }, 0 };
IntObj Obj_True = { { &Type_Bool, 1 }, 1 };
