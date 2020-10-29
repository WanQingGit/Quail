/*
 *  Created on: Jul 25, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#ifndef INCLUDE_FLOATOBJ_H_
#define INCLUDE_FLOATOBJ_H_
#include "typeobj.h"
typedef struct _fltobj {
	OBJ_HEAD
	double fval;
} FloatObj;
FloatObj * float_new(ThreadState *ts, double v);
#endif /* INCLUDE_FLOATOBJ_H_ */
