/*
 *  Created on: Jul 19, 2019
 *  Author: WanQing<1109162935@qq.com>
 */

#ifndef INCLUDE_MEM_ALLOC_H_
#define INCLUDE_MEM_ALLOC_H_
#include "typeobj.h"
#define mem_growvector(ts,v,nelems,size,t,limit,n) \
          if ((nelems)+n > (size)) \
            ((v)=(t*)(mem_growArray(ts,v,&(size),n,sizeof(t))))
void *mem_alloc(ThreadState *ts,void *ptr, Int osize, Int nsize);
void *mem_growArray(ThreadState *ts, void *block, int *size, int n,
		int size_elems);
#endif /* INCLUDE_MEM_ALLOC_H_ */
