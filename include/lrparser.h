/*
 * lrparser.h
 *
 *  Created on: Jun 28, 2019
 *  Author: WanQing<1109162935@qq.com>
 */

#ifndef INCLUDE_LRPARSER_H_
#define INCLUDE_LRPARSER_H_

#include "qlr.h"
#include "mapobj.h"
#include "moduleobj.h"
/* kinds of variables/expressions */
typedef enum {
	VNIL, /* constant nil */
	VNORMAL, VVOID, /* when 'expdesc' describes the last expression a list,
	 this kind means an empty list (so, no expression) */
	VTRUE, /* constant true */
	VFALSE, /* constant false */
	VK, /* constant in 'k'; info = index of constant in 'k' */
	VKFLT, /* floating constant; nval = numerical float value */
	VKINT, /* integer constant; nval = numerical integer value */
	VKSTR, VKBOOL, VNONRELOC, /* expression has its value in a fixed register;
	 info = result register */
	VLOCAL, /* local variable; info = local register */
	VNLOCAL, VNGLOBAL, VUPVAL, /* upvalue variable; info = index of upvalue in 'upvalues' */
	VASSIGN,VINDEXED, /* indexed variable;
	 ind.vt = whether 't' is register or upvalue;
	 ind.t = table register or upvalue;
	 ind.idx = key's R/K index */
	VJMP, /* expression is a test/comparison;
	 info = pc of corresponding jump instruction */
	VRELOCABLE, /* expression can put result in any register;
	 info = instruction pc */
	VCALL, /* expression is a function call; info = instruction pc */
	VVARARG, /* vararg expression; info = instruction pc */
	VGLOBAL, VMETHOD, VSTMETHOD
} expkind;

typedef struct _parser LRParser;

/* dynamic structures used by the parser */
typedef struct Dyndata {
	struct { /* list of active local variables */
		short* idx;/* 相对于f->locvars的位置，偏移在fs->firstlocal中，variable index in stack */
		int n;
		int size;
	} actvar;
//  Labellist gt;  /* list of pending gotos */
//  Labellist label;   /* list of active labels */
} Dyndata;

typedef struct expdesc {
	expkind k;
	union {
		qstr *str;
		long ival; /* for VKINT */
		double nval; /* for VKFLT */
		int info; /* for generic use */
		struct { /* for indexed variables (VINDEXED) */
			ushort idx; /* index (R/K) t2*/
			ushort t; /* table (register or upvalue) t1 */
			expkind vt :16; /* whether 't' is register (VLOCAL) or upvalue (VUPVAL) t3*/
		} ind;
	} u;
	int t; /* patch list of 'exit when true' */
	int f; /* patch list of 'exit when false' */
} expdesc;
struct _parser {
	LRAnalyzer *analyzer;
	qlexer *lex;
	MapObj* consts;	  //常量表
	int nconst;
	ThreadState *ts;	  //thread state
	struct _funcInfo *fs;
	ModuleObj *module;
	Dyndata *dyd;
	expdesc *expinfo;
//	int curline;
};
typedef struct BlockCnt {
	struct BlockCnt *previous; /* chain */
	int firstlabel; /* index of first label in this block */
	int firstgoto; /* index of first pending goto in this block */
	byte nactvar; /* # active locals outside the block */
	byte upval; /* true if some variable in the block is an upvalue */
	byte isloop; /* true if 'block' is a loop */
} BlockCnt;
typedef struct _funcInfo {
	Proto *f; /* current function header */
	struct _funcInfo *prev; /* enclosing function */
	LRParser *parser;	  //  struct LexState *ls;  /* lexical state */
	BlockCnt *bl; /* chain of current blocks */
	int pc; /* 指令占用的单元个数,pc约=3*pc2 next position to code (equivalent to 'ncode') */
	int pc2;/*指令数 */
	int lasttarget; /* 'label' of last 'jump label' */
	int jpc; /* list of pending jumps to 'pc' */
	int np; /* number of elements in 'p' */
	int firstlocal; /* index of first local var (in Dyndata array) */
	int nlocvars; /* number of elements in 'f->locvars' */
	int nactvar; /* number of active local variables */
	int nups; /* number of upvalues */
	int ngl;
	byte usedreg; /* first free register */
} FuncInfo;

LRAnalyzer *loadRules(char *path);
ModuleObj *parserCode(LRAnalyzer *analyzer, ThreadState *ts, char *src);
#endif /* INCLUDE_LRPARSER_H_ */
