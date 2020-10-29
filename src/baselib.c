/*
 *  Created on: Jul 25, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */
#include "typeobj.h"
#include "intobj.h"
#include "baselib.h"
void bltinTypeInit() {
	type_inherit(&Type_Bool);
}
