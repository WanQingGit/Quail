/*
 *  Created on: Jul 26, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#ifndef INCLUDE_QUAILVM_H_
#define INCLUDE_QUAILVM_H_
#include "typeobj.h"

void vm_call_module(ThreadState *ts, Object *module);
int instr_execute(ThreadState *ts);
#endif /* INCLUDE_QUAILVM_H_ */
