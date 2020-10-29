/*
 *  Created on: 2019年2月21日
 *      Author: WanQing
 */
#include "mem_alloc.h"
#include "string.h"
#include "mem_pool.c"
#include "qstate.h"
#ifdef DEBUG_MEM
#include "qio.h"
#include "bytelist.h"
qmap *mrecord;

#define insertMsg(n,str,nsize) \
			if (mrecord != NULL) { \
				meminfo* info; \
				sy_malloc(S, &info, sizeof(meminfo));\
				info->msg = str;\
				info->size = nsize;\
				MapEntry entry;\
				CTRL.stopif(Map.gset(S, mrecord, n, true,&entry));\
				entry._2->val=info;\
			}
#else
#define insertMsg(ptr, msg, nsize) NULL
#endif

//void *mem_alloc(ThreadState *ts,void *ptr, Int osize, Int nsize) {
//	void* n = NULL;
//	assert((osize==0)==(ptr==NULL));
//	if (nsize == 0) {
//		free(ptr);
//	} else {
//		if (osize) {
//			n = realloc(ptr, nsize);
//			if (nsize > osize)
//				memset((char*) n + osize, 0, nsize - osize);
//		} else {
//			n = calloc(1, nsize);
//		}
////		if (n == NULL)
////			skyc_throw(S, ERR_MEM,
////					"Unable to allocate memory,If necessary, free memory manually!");
//	}
//
//	ts->gs->GCdebt += nsize - osize;
//	return n;
//}

void *mem_alloc(ThreadState *ts, void *ptr, Int osize, Int nsize
#ifdef DEBUG_MEM
		, char *msg
#endif
		) {
	void* n = NULL;
	assert((osize==0)==(ptr==NULL));
	ts->gs->GCdebt += nsize - osize;
#ifdef DEBUG_MEM
	if (mrecord != NULL && ptr) {
		qentry2 entry;
		if (Map.del(S, mrecord, ptr, &entry)) {
			meminfo *info = cast(meminfo*, entry.val);
			if (info) {
				skyc_assert(S, info->size == osize);
				if (skym_free(S, info) == false) {
					qfree(info);
				}
			}
		} else {
			n = 0; //do nothing
		}
	}
#endif
	if (nsize == 0) {
		if (osize && mem_free(/*S, */ptr) == false) {
			qfree(ptr);
		}
		return NULL;
	} else { //nsize != 0
		if (osize) {
			if (osize > SMALL_REQUEST_THRESHOLD) {
				if (nsize > SMALL_REQUEST_THRESHOLD) {
					n = realloc(ptr, nsize);
				} else {
					mem_malloc(/*S, */&n, nsize);
					if (n)
						memcpy(n, ptr, nsize);
					qfree(ptr);
				}
			} else { //osize <= SMALL_REQUEST_THRESHOLD
				if (nsize > SMALL_REQUEST_THRESHOLD) {
					n = malloc(nsize);
					memcpy(n, ptr, osize);
				} else { //nsize <= SMALL_REQUEST_THRESHOLD
					if (SIZE2INDEX(nsize) != SIZE2INDEX(osize)) {
						mem_malloc(/*S, */&n, nsize);
						memcpy(n, ptr, osize > nsize ? nsize : osize);
					} else
						return ptr;
				}
				if (n) {
					if (mem_free(ptr)) {
						insertMsg(ptr, msg, nsize);
					} else {
						qfree(ptr);
					}
				}
				return n;
			}
		} else { //osize=0
			if (nsize > SMALL_REQUEST_THRESHOLD) {
				n = malloc(nsize);
			} else {
				mem_malloc(/*S,*/&n, nsize);
			}
		}
		insertMsg(n, msg, nsize);
		return n;
	}
}

void *mem_growArray(ThreadState *ts, void *block, int *size, int n,
		int size_elems) {
	void *newblock;
	int oldsize = *size;
	int newsize = (oldsize + n) * 1.7;
	assert(oldsize < newsize);
	newblock = mem_alloc(ts, block, oldsize * size_elems, newsize * size_elems);
	*size = newsize;
	return newblock;
}

/*void skym_destroy(ThreadState *s, void *p, int size) {
 skym_alloc_pool(s, o2gc(p), size + sizeof(GCObj), 0);
 }*/

//static void skym_gcpro(ThreadState *S, GCObj *o) {
//	gl_state *g = getTS()->g;
//	skyc_assert(S, g->gc.allgc == o);
//	gc2gray(o);
//	g->gc.allgc = o->next;
//	o->next = g->gc.protect;
//	g->gc.protect = o;
//}
#ifdef DEBUG_MEM
void printMeminfo() {
	mapIter iter;
	Map.iterator(mrecord, &iter);
	printf("total allocated size %ld\n", _S->g->gc.GCdebt);
	while (Map.next(&iter)) {
		qentry2 *entry = iter.entry._2;
		uintptr_t ptr = entry->key;
		if (entry->val) {
			meminfo *info = cast(meminfo *, entry->val);
			printf("adrress %llx size %d msg %s\n", ptr, info->size, info->msg);
		} else {
			printf("delete obsolete adrress %x \n", ptr);
			skyc_assert_(Map.del(_S,mrecord,ptr,NULL));
		}
	}
	if (mrecord->length) {
		qbytes *bytes = Bytes.create(1);
		qentry2 res;
		Map.del(_S, mrecord, bytes, &res);
		skyc_assert_(skym_free(_S, res.val));
		Map.del(_S, mrecord, bytes->data, &res);
		skyc_assert_(skym_free(_S, res.val));
		typeMap->serialize(bytes, mrecord);
		FILE *fp = fopen("res/memInfo.dat", "w");
		fwrite(bytes->data, bytes->length, 1, fp);
		fclose(fp);
		Bytes.destroy(&bytes);
	}
}
void loadMeminfo() {
	FILE *fp = fopen("res/memInfo.dat", "r");
	if (fp) {
		fclose(fp);
		char *bytes = file_read("res/memInfo.dat");
		typeMap->deserial(bytes, &mrecord);
	} else {
		mrecord = Map.create(_S, typeInt, true);
	}
}
const struct apiMem Mem = { skym_new, alloc_pool, printMeminfo, loadMeminfo,
		skym_alloc, skym_gcpro, skym_growArray };
#else
//const struct apiMem Mem = { skym_new, alloc_pool, skym_alloc, skym_gcpro,
//		skym_growArray };
#endif

