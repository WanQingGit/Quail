/*
 *  Created on: Jul 28, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#ifndef INCLUDE_CLASSOBJ_H_
#define INCLUDE_CLASSOBJ_H_
#include "typeobj.h"

void class_new(ThreadState *ts, Object *name, stkId base, int argc);
#endif /* INCLUDE_CLASSOBJ_H_ */
