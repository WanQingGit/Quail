/*
 *  Created on: Jul 26, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#ifndef INCLUDE_QUAIL_H_
#define INCLUDE_QUAIL_H_
#include <pthread.h>
#include "qobject.h"

typedef unsigned char Instruction;

#define OBJ_HEAD Object _base;
#define VAR_HEAD OBJ_HEAD Int length;
#define REF_INC(o) do{\
	Object *_op=(Object*)(o);\
	pthread_spin_lock(&_op->spinlock);\
	_op->nref++;\
	pthread_spin_unlock(&_op->spinlock);\
}while(0)
#define Qu_TYPE(obj) cast(Object*,obj)->type
#define stack_push(ts,obj) *(ts->top++)=obj
#define stack_pop(ts) *(--ts->top)
#define REF_DEC(ts,op) do{\
	Object *_op=(Object*)(op);\
	pthread_spin_lock(&_op->spinlock);\
  _op->nref--;\
  if (_op->nref <= 0) {\
  	pthread_spin_unlock(&_op->spinlock);\
  	assert(_op->nref == 0);\
  	pthread_spin_destroy(&_op->spinlock);\
  	_op->type->destroy(ts, _op);\
  } else {\
  	pthread_spin_unlock(&_op->spinlock);\
  }\
}while(0)

typedef enum {
	BT_NIL,
	BT_FUNCTION,
	BT_METHOD,
	BT_CFUNC,
	BT_STR,
	BT_LNGSTR,
	BT_FLOAT,
	BT_INT,
	BT_BOOL,
	BT_USERDATA,
	BT_DICT,
	BT_SET,
	BT_LIST,
	BT_TUPLE,
	BT_CLASS,
	BT_OBJECT,
	BT_THREAD,
	BT_MODULE,
	BT_RBTREE,
	BT_TYPE
} BaseType;
typedef struct _object Object;
typedef struct _type TypeObj;
typedef struct _proto Proto;
typedef struct _glstate GlobalState;
typedef struct _threadstate ThreadState;
typedef struct CallInfo CallInfo;
typedef Object** stkId; // stack index

typedef struct linedesc {
	int line;
	int pc;
} linedesc;
typedef struct LocVar {
	int idx; //在常量表中的位置
	int startpc; /* first point where variable is active */
	int endpc; /* first point where variable is dead */
} LocVar;
typedef struct Upvaldesc {
	int name; /* upvalue name (for debug information) */
	bool write;
	byte instack; /* whether it is in stack (register) */
	byte idx; /* index of upvalue (in stack or in outer function's list) */
} Upvaldesc;
struct _object {
	TypeObj *type;
	Int nref;
	pthread_spinlock_t spinlock;
};
typedef struct UpVal {
	stkId ref; /* points to stack or to its own value */
	size_t nref; /* reference counter */
	union {
		struct { /* (when open) */
			struct UpVal *next; /* linked list */
			int touched; /* mark to avoid cycles with dead threads */
		} open;
		Object *value; /* the value (when closed) */
	} u;
} UpVal;

struct _glstate {
	struct _glstate *next;
	ThreadState *ts_list;
	Object *modules;
	Object *strTable;
	Object *intTable;
	Object *glTable;
	Int GCdebt;
	Int nthread;
};
struct _threadstate {
	struct _threadstate *next;
	GlobalState *gs;
	CallInfo *ci;
	stkId stack;
	stkId stack_last;
	int stacksize;
	stkId top;
	UpVal *open_upval;
	long threadId;
};
typedef Int (*hashfn)(Object *);
typedef Int (*comparefn)(Object *, Object *);
typedef void (*destructor)(ThreadState *ts, Object *);
typedef Object *(*descrgetfn)(ThreadState *ts, Object *func, Object *obj,
		TypeObj *cls);
typedef int (*descrsetfn)(ThreadState *ts, Object *func, Object *obj,
		Object *cls);
typedef Int (*callfn)(ThreadState *ts, Object *self, stkId args, int argc,
		Object *kwds);
typedef Int (*binaryfn)(ThreadState *ts, Object *obj, Object *arg);
typedef Int (*setattrfn)(ThreadState *ts, Object *obj, Object *key, Object *v);
typedef Object* (*getattrfn)(ThreadState *ts, Object *obj, Object *arg);
typedef int (*initfn)(ThreadState *ts, Object *, stkId args, int argc,
		Object *);
typedef Object *(*newfn)(ThreadState *ts, TypeObj *, stkId args, int argc,
		Object *);
struct _type {
	VAR_HEAD
	char *name;
	Int basicsize, itemsize;
	hashfn hash;
	comparefn compare;
	destructor destroy;
	descrgetfn descr_get;
	descrsetfn descr_set;
	callfn f_call;
	newfn f_new;
	initfn f_init;
	struct _type *super;
	BaseType basetype;
	getattrfn f_getattr;
	setattrfn f_setattr;
	Object *dict;
	Object *mro;
	Int dictoffset;
};

typedef struct {
	VAR_HEAD
} VarObject;

struct _proto {
	Object *module;
	Object *defValue;
	Instruction *code; /* opcodes */
	Proto **p; /* functions defined inside the function */
	linedesc *lineinfo; /* map from opcodes to source lines (debug information) */
	LocVar* locvars; /*只增不减，可能含相同的值 information about local variables (debug information) */
	LocVar*glvars; /* information about local variables (debug information) */
	Upvaldesc* upvalues; /* upvalue information */
//  struct LClosure *cache;  /* last-created closure with this prototype */
//  GCObject *gclist;
	int nupvalues; /* size of 'upvalues' */
	int ncode; //~=ninstr*3,pc实际占用的单元
	int ninstr; //~=ncode/3
	int np; /* size of 'p' */
	int nlocvars;
	int ngl;
	int linedefined; /* debug information  */
	int lastlinedefined; /* debug information  */
	short maxstacksize; /* number of registers needed by this function */
	byte nparams; /* number of fixed parameters */
	byte flag_vararg;
};

#endif /* INCLUDE_QUAIL_H_ */
