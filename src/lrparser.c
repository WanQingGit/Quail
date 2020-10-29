/*
 * lrparser.c
 *
 *  Created on: Jun 28, 2019
 *  Author: WanQing<1109162935@qq.com>
 */
#include "lrparser.h"
#include "qstate.h"
#include "mapobj.h"
#include "strobj.h"
#include "floatobj.h"
#include "intobj.h"
#include "mem_alloc.h"
#include "instruction.h"
#include "listobj.h"
#include "bytelist.h"
#include <typeWord_.h>
#include "typeGram_.h"
#include "funcobj.h"
#include <assert.h>
#include "qmem.h"
#define sizeclosure(n)	(sizeof(LClosure) + \
                         sizeof(char *)*((n)-1))
#define mem_reallocvector(S,v,oldn,n,t) \
   ((v)=cast(t*, mem_alloc(S,v,oldn*sizeof(t),n*sizeof(t))))

#define freeGramdesc(e) do{\
	if(e->u.alltks)\
			Arr.destroy(&e->u.alltks,NULL);\
	skym_alloc_pool(_S,e,sizeof(qgramdesc),0);\
}while(0)
#define freeExpdesc(ts,e) mem_alloc(ts,e,sizeof(expdesc),0)
#define numerable(v) ((v)->k==VKINT||(v)->k==VKFLT)
#define isInt(v) ((v)->k==VKINT)
#define isFlt(v) ((v)->k==VKFLT)
#define getCodePos(fs,i) fs->f->lineinfo[i].pc
#define getCode(fs,i) ((fs)->f->code+getCodePos(fs,i))
void constHook(LRParser *pa, qtk *token);
void statHook(LRAnalyzer *analyzer, LRParser *pa);
expdesc *getGramExpDesc(LRParser *pa, RBTree *exprs, int idx);
qgramdesc *getGramDesc(RBTree *exprs, int idx);
void searchvar(LRParser *pa, int varname, expdesc *var, expkind flag);
void exp_op_handle(LRParser *pa, RuleInfo *info);
void exp_free(FuncInfo *fs, expdesc *e);
void exp_init(expdesc *e, expkind k, int i);
LocVar* var_localGet(FuncInfo *fs, int i);
LocVar* var_globalGet(FuncInfo *fs, int i);
void var_localRemove(FuncInfo *fs, int tolevel);
void var_localActive(FuncInfo *fs, int nvars);
int var_localFind(FuncInfo *fs, int name);
int var_globalFind(FuncInfo *fs, int n);
int var_upbalFind(FuncInfo *fs, int name);
int var_get(FuncInfo *fs, expdesc *e);
void var_indexed(FuncInfo *fs, expdesc *t, int attrname);
void var_global(FuncInfo *fs, expdesc *t, int varname);
void var_store(LRParser *pa, expdesc *var, expdesc *ex);
void reg_reserve(FuncInfo *fs, int n);
void reg_check(FuncInfo *fs, int n);
void reg_free(FuncInfo *fs, int reg);
#define gconst(pa,i) ((pa)->module->consts[i])
void addLocalvar(LRParser* pa, int str);
int addConst(LRParser *pa, Object *key);
int addConstNoHash(LRParser *pa, Object *key);
int addIntConst(LRParser *pa, INT i);
int addStrConst(LRParser *pa, char *s);
int addFltConst(LRParser *pa, FLT f);
int addBoolConst(LRParser *pa, int i);
static Proto *addProto(LRParser *pa);
int code_add(FuncInfo *fs, Instruction* i, int n);
int code_addA(FuncInfo *fs, OpCode o, int a);
int code_addAB(FuncInfo *fs, OpCode o, int a, int b);
int code_addABC(FuncInfo *fs, OpCode o, int a, int b, int c);
int code_addABCD(FuncInfo *fs, OpCode o, int a, int b, int c, int d);
int code_const(FuncInfo *fs, int reg, int k);
int code_ret(FuncInfo *fs, int first, int nret);
bool kToBool(LRParser *parser, expdesc *e);
void varToReg(FuncInfo *fs, expdesc *e, int reg);
void expToAnyReg(FuncInfo *fs, expdesc *e);
void expToNextReg(FuncInfo *fs, expdesc *e);
int expToK(LRParser *pa, expdesc *e);
void expToReg(FuncInfo *fs, expdesc *e, int reg);
void expToRegUp(FuncInfo *fs, expdesc *e);
int expToRK(FuncInfo *fs, expdesc *e);
int jump_offset(FuncInfo *fs, int pc);
static void jump_fix_offset(FuncInfo *fs, int pc, int dest);
static void jump_fix_reg(FuncInfo *fs, int pc, int reg);
static short jump_get_reg(FuncInfo *fs, int pc);
static void jump_fetch(FuncInfo *fs);
void jump_link(FuncInfo *fs, int *l1, int l2);
int jumpifTrue(FuncInfo *s, int reg, expdesc *e);
int jumpifFalse(FuncInfo *s, int reg, expdesc *e);
void func_create(LRParser *pa);
void func_close(LRParser *pa);
void func_load(LRParser *pa, expdesc *v);

static int selfid;
static int initid;
static int newid;
static ListObj _list = { 0 };
void atom_expr(LRParser *pa, qbytes *tokens, qvec tks, expdesc *v) {
	qtk *tk = Bytes_get(tokens, tks->data[0].i, qtk);
	switch (tk->type) {
	case TW_SBL:
	case TW_NAME:
		searchvar(pa, tk->v.i, v, VNORMAL);
		for (int i = 1; i < tks->length;) {
			tk = Bytes_get(tokens, tks->data[i].i, qtk);
			switch (tk->type) {
			case TW_DOT:
			case TW_MBL:
			case TW_SBL:
				break;
			default:
				break;

			}
		}
		break;
	case TW_NUM: {
		assert(tks->length == 1);
		qobj *obj = tk->v.obj;
		if (obj->type == typeInt) {
			exp_init(v, VKINT, 0);
			v->u.ival = obj->val.i;
		} else {
			exp_init(v, VKFLT, 0);
			v->u.nval = obj->val.flt;
		}
		qfree(obj);
		break;
	}
	default:
		break;
	}
}
void scope_new(FuncInfo *fs) {
	BlockCnt *bl = (BlockCnt *) mem_alloc(fs->parser->ts, NULL, 0,
			sizeof(BlockCnt));
	if (fs->bl) {
		bl->nactvar = fs->nactvar;
		bl->previous = fs->bl;
	} else {
		bl->previous = NULL;
		bl->nactvar = 0;
	}
	bl->upval = 0;
	fs->bl = bl;
	fs->usedreg = fs->nactvar;
}
void scope_leave(FuncInfo *fs) {
	BlockCnt *bl = fs->bl;
	if (bl->previous && bl->upval) {
		assert(false);
	}
	fs->bl = bl->previous;
	var_localRemove(fs, bl->nactvar);
	mem_alloc(fs->parser->ts, bl, sizeof(BlockCnt), 0);
	fs->usedreg = fs->nactvar;
}
void func_create(LRParser *pa) {
	FuncInfo *nfs = (FuncInfo*) mem_alloc(pa->ts, NULL, 0, sizeof(FuncInfo));
	memset(nfs, 0, sizeof(FuncInfo));
	Proto* p = addProto(pa); //proto树
	nfs->f = p;
	nfs->f->linedefined = pa->lex->curline;
	nfs->prev = pa->fs; /* linked list of funcstates */
	nfs->parser = pa;
	pa->fs = nfs;
	nfs->jpc = NO_JUMP;
	nfs->firstlocal = pa->dyd->actvar.n;
}
void func_close(LRParser *pa) {
	FuncInfo *fs = pa->fs;
	Proto *f = fs->f;
	if (f->ninstr) {
		Instruction *i = getinstr(f, f->ninstr - 1);
		if (GET_OPCODE(i)!= OP_RETURN) {
			code_ret(fs, 0, 0);
		}
	} else {
		code_ret(fs, 0, 0);
	}
	code_ret(fs, 0, 0);
	mem_reallocvector(pa->ts, f->code, fs->pc, f->ncode, Instruction);
	mem_reallocvector(pa->ts, f->lineinfo, fs->pc2, f->ninstr, linedesc); //这里拷贝越界
	mem_reallocvector(pa->ts, f->p, fs->np, f->np, Proto *);
	mem_reallocvector(pa->ts, f->locvars, fs->nlocvars, f->nlocvars, LocVar);
	mem_reallocvector(pa->ts, f->upvalues, fs->nups, f->nupvalues, Upvaldesc);
	mem_reallocvector(pa->ts, f->glvars, fs->ngl, f->ngl, LocVar);
	assert(fs->bl == NULL);
	pa->fs = fs->prev;
	if (pa->fs)
		mem_alloc(pa->ts, fs, sizeof(FuncInfo), 0);
//	luaC_checkGC(L);
}
void func_load(LRParser *pa, expdesc *v) {
	expdesc e;
	FuncInfo *fs = pa->fs;
	exp_init(&e, VRELOCABLE,
			code_addAB(fs, OP_CLOSURE, VRELOCABLE_REG, fs->f->np - 1));
	var_store(pa, v, &e);
}
void method_load(LRParser *pa, expdesc *v) {
	FuncInfo *fs = pa->fs;
	code_addAB(fs, OP_BUILD_METH, v->k == VMETHOD ? v->u.info : RKMASK(v->u.info),
			fs->f->np - 1);
}

void exp_op_handle(LRParser *pa, RuleInfo *info) {
	expdesc *exp, *exp2;
	double e1, e2;
	qgramdesc *desc = info->desc; //, *desc2;
	qvec alltks = desc->u.alltks;
	qbytes *tokens = info->tokens;
	exp = getGramExpDesc(pa, info->exprs, alltks->data[0].i);
	for (int i = 1; i < alltks->length;) {
		qtk *op = Bytes_get(tokens, alltks->data[i++].i, qtk);
//		node = RB.search(info->exprs, alltks->data[i++].i);
		exp2 = getGramExpDesc(pa, info->exprs, alltks->data[i++].i);
		if (numerable(exp) && numerable(exp2)) {
			if (isInt(exp) && isInt(exp2)) {
				switch (op->type) {
				case TW_MUL:
					exp->u.ival *= exp2->u.ival;
					break;
				case TW_DIV: {
					e2 = exp2->u.ival;
					exp->u.nval /= e2;
					exp->k = VKFLT;
					break;
				}
				case TW_MOD:
					exp->u.ival %= exp2->u.ival;
					break;
				case TW_DIV_DIV:
					exp->u.ival /= exp2->u.ival;
					break;
				case TW_ADD:
					exp->u.ival += exp2->u.ival;
					break;
				case TW_SUB:
					exp->u.ival -= exp2->u.ival;
					break;
				default:
					break;
				}

			} else {
				e1 = isInt(exp) ? (exp->k = VKFLT, exp->u.ival) : exp->u.nval;
				e2 = isInt(exp2) ? exp->u.ival : exp2->u.nval;
				switch (op->type) {
				case TW_MUL:
					exp->u.nval = e1 * e2;
					break;
				case TW_DIV: {
					exp->u.nval = e1 / e2;
					break;
				}
				case TW_MOD:
					exp->u.nval = cast(INT, e1) % cast(INT, e2);
					break;
				case TW_DIV_DIV:
					exp->u.nval = cast(INT, e1) / cast(INT, e2);
					break;
				case TW_ADD:
					exp->u.nval = e1 + e2;
					break;
				case TW_SUB:
					exp->u.ival = e1 - e2;
					break;
				default:
					break;
				}
			}
		} else {
			switch (op->type) {
			case TW_LT:
			case TW_GT:
			case TW_EQ_EQ:
			case TW_INV_EQ:
				break;
			default:
				break;
			}
		}
		mem_alloc(pa->ts, exp2, sizeof(expdesc), 0);
	}
	desc->data = exp;
	Arr.destroy(&desc->u.alltks, NULL);
}
void exp_free(FuncInfo *fs, expdesc *e) {
	if (e->k == VNONRELOC)
		reg_free(fs, e->u.info);
}
void exp_init(expdesc *e, expkind k, int i) {
	e->f = e->t = NO_JUMP;
	e->k = k;
	e->u.info = i;
}
void gram_term(LRParser *pa, RuleInfo *info) {
	printf("---gram_term---\n");
	exp_op_handle(pa, info);
}
void gram_arith_expr(LRParser *pa, RuleInfo *info) {
	printf("---gram_arith_expr---\n");
	exp_op_handle(pa, info);
}
void gram_comparison(LRParser *pa, RuleInfo *info) {
	printf("---gram_arith_expr---\n");
	exp_op_handle(pa, info);
}
qgramdesc *getGramDesc(RBTree *exprs, int idx) {
	RBNode *node = RB.search(exprs, idx);
	assert(node && node->val);
	qgramdesc *desc = cast(qgramdesc*, node->val);
	RB.delNode(exprs, node);
	return desc;
}
expdesc *getGramExpDesc(LRParser *pa, RBTree *exprs, int idx) {
	RBNode *node = RB.search(exprs, idx);
	assert(node && node->val);
	qgramdesc *desc = cast(qgramdesc*, node->val);
	if (desc->u.alltks) {
		Arr.destroy(&desc->u.alltks, NULL);
	}
	RB.delNode(exprs, node);
	expdesc *exp = cast(expdesc*, desc->data);
	skym_alloc_pool(_S, desc, sizeof(qgramdesc), 0);
//	mem_alloc(pa->ts, desc, sizeof(qgramdesc), 0);
	return exp;
}
void gram_and_test(LRParser *pa, RuleInfo *info) {
	qgramdesc *desc = info->desc;
	FuncInfo *fs = pa->fs;
	qvec alltks = desc->u.alltks;
	printf("---gram_and_test---\n");
	int nexp = (alltks->length - 1) / 2; //actual nexp +=1
	expdesc *v = getGramExpDesc(pa, info->exprs, alltks->data[2 * nexp].i);
	desc->data = v;
	int regid = fs->usedreg;
//	expToRK(pa->fs, v);
	for (int i = 0; i < nexp; i++) {
		expdesc *e = getGramExpDesc(pa, info->exprs, alltks->data[2 * i].i);
		if (var_get(pa->fs, e)) {
			expToAnyReg(pa->fs, e);
			reg_free(pa->fs, e->u.info);
			int jmppc = jumpifFalse(pa->fs, regid, e);
			jump_link(pa->fs, &v->f, jmppc);
		} else {
			if (kToBool(pa, v) == true) {
				if (i == nexp - 1) {
					expToK(pa, e);
					desc->data = v;
				} else {
					mem_alloc(pa->ts, v, sizeof(expdesc), 0);
				}
			} else {
				desc->data = v;
				for (; i < nexp; i++) {
					v = getGramExpDesc(pa, info->exprs, alltks->data[2 * i].i);
					mem_alloc(pa->ts, v, sizeof(expdesc), 0);
				}
			}
		}
		mem_alloc(pa->ts, e, sizeof(expdesc), 0);
	}
	if (info->exp->length < 3) {
		fs->usedreg += 1;
		expToReg(fs, v, regid);
		fs->jpc = v->f;
	} else {
		if (kToBool(pa, v)) {
//			if()
		} else {
//			expToReg(fs, v, regid);
			if (info->pos != info->exp->length - 1) {
				expToAnyReg(pa->fs, v);
				v->t = jumpifTrue(pa->fs, regid, v);
				fs->jpc = v->f;
			} else {
				v->t = v->f;
			}
		}
	}

	Arr.destroy(&desc->u.alltks, NULL);
}
void gram_or_test(LRParser *pa, RuleInfo *info) {
	qgramdesc *desc = info->desc;
	qvec alltks = desc->u.alltks;
	FuncInfo *fs = pa->fs;
	printf("---gram_or_test---\n");
	int regid = fs->usedreg;
	int nexp = (alltks->length - 1) / 2; //actual nexp +=1
	expdesc *v = getGramExpDesc(pa, info->exprs, alltks->data[2 * nexp].i);
	for (int i = 0; i < nexp; i++) {
		expdesc *e = getGramExpDesc(pa, info->exprs, alltks->data[2 * i].i);
		if (e->f != NO_JUMP) {
			jump_link(fs, &v->t, e->t);
//			fs->jpc = e->f;
//			expToReg(fs, e, regid);
//			short list = e->f, next;
//			if (list != NO_JUMP) {
//				fs->jpc = list;
//				while (list != NO_JUMP) {
//					next = jump_offset(pa->fs, list);
//					jump_fix_reg(pa->fs, list, e->u.info);
//					list = next;
//				}
//			}
		} else if (var_get(pa->fs, e)) {
			expToAnyReg(fs, e);
//			expToReg(fs, e, regid);
			reg_free(pa->fs, e->u.info);
			int jmppc = jumpifTrue(pa->fs, regid, e);
			jump_link(pa->fs, &v->t, jmppc);
		} else {
			if (kToBool(pa, v) == false) {
				if (i == nexp - 1) {
					expToK(pa, e);
					desc->data = v;
				} else {
					mem_alloc(pa->ts, v, sizeof(expdesc), 0);
				}
			} else {
				desc->data = v;
				for (; i < nexp; i++) {
					v = getGramExpDesc(pa, info->exprs, alltks->data[2 * i].i);
					mem_alloc(pa->ts, v, sizeof(expdesc), 0);
				}
			}
		}

		mem_alloc(pa->ts, e, sizeof(expdesc), 0);
	}
	expToReg(fs, v, regid);
	fs->usedreg += 1;
	desc->data = v;
	fs->jpc = v->t; //jpc解决offset问题
	v->f = NO_JUMP;
	Arr.destroy(&desc->u.alltks, NULL);
}

void gram_atom_expr(LRParser *pa, RuleInfo *info) {
	printf("---gram_atom_expr---\n");
	expdesc *v = mem_alloc(pa->ts, NULL, 0, sizeof(expdesc)), *v2;
	qgramdesc *desc = info->desc;
	qvec alltks = desc->u.alltks;
	qbytes *tokens = info->tokens;
	qtk *tk = Bytes_get(tokens, alltks->data[0].i, qtk);
	FuncInfo *fs = pa->fs;
	int expId;
	switch (tk->type) {
	case TW_SBL:
		assert(alltks->length == 3);
		v = getGramExpDesc(pa, info->exprs, alltks->data[1].i);
		break;
	case TW_NAME:
		v = mem_alloc(pa->ts, NULL, 0, sizeof(expdesc));
		if (info->parent != TG_varlist || alltks->length > 1)
			searchvar(pa, tk->v.i, v, VNORMAL);
		else
			searchvar(pa, tk->v.i, v, VVOID);
		for (int i = 1; i < alltks->length;) {
			if ((expId = alltks->data[i++].i) & EXPFLAG) {
				qgramdesc *desc2 = getGramDesc(info->exprs, expId);
				qvec alltks2 = desc2->u.alltks;
				tk = Bytes_get(tokens, alltks2->data[0].i, qtk);
				assert(tk->type == TW_SBL);
				int nparams = 0;
				expToNextReg(pa->fs, v);
				int j = 1;
				MapObj *dict = NULL;
				MapEntry res;
				while ((expId = alltks2->data[j++].i) & EXPFLAG) {
					v2 = getGramExpDesc(pa, info->exprs, expId);
					if (v2->k == VASSIGN) {
						if (dict == NULL)
							dict = (MapObj*) map_new(pa->ts, true);
						assert(
								!map_gset(pa->ts, dict, pa->module->consts[v2->u.ind.t], true,
										&res));
						res.dict->val = int_new(pa->ts, v2->u.ind.idx);
//						list_append(pa->ts, dict, int_new(pa->ts, v2->u.ind.t));
					} else {
						assert(dict==NULL);
						expToNextReg(pa->fs, v2);
						nparams++;
					}
					tk = Bytes_get(tokens, alltks2->data[j++].i, qtk);
					if (tk->type == TW_SBR) {
						break;
					}
				}
				if (dict == NULL) {
					exp_init(v, VCALL, code_addABC(fs, OP_CALL, v->u.info, nparams, 2));
				} else {
//					Object *tupe = list_toTuple(pa->ts, dict);
//					list_destroy(pa->ts, dict);
					exp_init(v2, VK, addConstNoHash(pa, (Object*) dict));
					expToNextReg(fs, v2);
					exp_init(v, VCALL,
							code_addABC(fs, OP_CALL_KW, v->u.info, nparams + 1, 2));
				}
				fs->usedreg = v->u.info + 1;
				freeGramdesc(desc2);
			} else {
				tk = Bytes_get(tokens, expId, qtk);
				switch (tk->type) {
				case TW_DOT:
					tk = Bytes_get(tokens, alltks->data[i++].i, qtk);
					var_indexed(pa->fs, v, tk->v.i);
					break;
				case TW_MBL:
					break;
				case TW_SBL: {
					int nparams = 0;
					expToNextReg(pa->fs, v);
					while ((expId = alltks->data[i++].i) & EXPFLAG) {
						v2 = getGramExpDesc(pa, info->exprs, expId);
						expToNextReg(pa->fs, v2);
						nparams++;
						tk = Bytes_get(tokens, alltks->data[i++].i, qtk);
						if (tk->type == TW_SBR) {
							break;
						}
					}
					fs->usedreg = v->u.info + 1;
					exp_init(v, VCALL, code_addABC(fs, OP_CALL, v->u.info, nparams, 2));
					break;
				}
				default:
					break;
				}
			}
		}
		break;
	case TW_NUM: {
		v = mem_alloc(pa->ts, NULL, 0, sizeof(expdesc));
		assert(alltks->length == 1);
		qobj *obj = tk->v.obj;
		if (obj->type == typeInt) {
			exp_init(v, VKINT, 0);
			v->u.ival = obj->val.i;
		} else {
			exp_init(v, VKFLT, 0);
			v->u.nval = obj->val.flt;
		}
		qfree(obj);
		break;
	}
	case TW_STRING:
		v = mem_alloc(pa->ts, NULL, 0, sizeof(expdesc));
		exp_init(v, VKSTR, 0);
		v->u.str = tk->v.s;
		break;
	default:
		break;
	}
	desc->data = v;
	Arr.destroy(&desc->u.alltks, NULL);
}
void gram_var_expr(LRParser *pa, RuleInfo *info) {
	printf("---gram_var_expr---\n");
}
void gram_expr_stmt(LRParser *pa, RuleInfo *info) {
	printf("---gram_expr_stmt---\n");
	printf("\n");
}

void gram_assign_stmt(LRParser *pa, RuleInfo *info) {
	printf("---gram_expr_stmt---\n");
	qgramdesc *desc = info->desc;
	qvec alltks = desc->u.alltks;
	RBTree *exprs = info->exprs;
	qgramdesc *var = getGramDesc(exprs, alltks->data[0].i);
	qgramdesc *exp = getGramDesc(exprs, alltks->data[2].i);
	qvec varlist = var->u.alltks;
	qvec explist = exp->u.alltks;
	for (int i = 0; i < varlist->length; i += 2) {
		expdesc *v = getGramExpDesc(pa, exprs, varlist->data[i].i);
		expdesc *e = getGramExpDesc(pa, exprs, explist->data[i].i);
		var_store(pa, v, e);
		freeExpdesc(pa->ts, v);
		freeExpdesc(pa->ts, e);
	}
	freeGramdesc(var);
	freeGramdesc(exp);
}
void gram_call_stmt(LRParser *pa, RuleInfo *info) {
	printf("---gram_call_stmt---\n");
	qgramdesc *desc = info->desc;
	qvec alltks = desc->u.alltks;
//	qbytes *tokens = info->tokens;
	expdesc *e = getGramExpDesc(pa, info->exprs, alltks->data[0].int32);
	assert(e->k == VCALL);
	Instruction *i = getinstruction(pa->fs, e);
	GETARG_C(i)=1;
}

void gram_block_head(LRParser *pa, RuleInfo *info) {
	printf("\n\t\tblock start...\n\n");
	scope_new(pa->fs);
}
void gram_block(LRParser *pa, RuleInfo *info) {
	printf("\n\t\tblock end...\n\n");
	scope_leave(pa->fs);
}
void gram_if_stmt(LRParser *pa, RuleInfo *info) {
	printf("---gram_if_stmt---\n");
	qgramdesc *desc = info->desc;
	qvec alltks = desc->u.alltks;
	qbytes *tokens = info->tokens;
	qtk *tk;
	FuncInfo *fs = pa->fs;
	expdesc *if_block = getGramExpDesc(pa, info->exprs, alltks->data[1].i);
	int prevFalse = if_block->f;
	int i = 3;
	for (; i < alltks->length; i += 2) {
		int expid = alltks->data[i].i;
		if (expid & EXPFLAG) {
			expdesc *ifblock = getGramExpDesc(pa, info->exprs, expid);
			jump_link(fs, &if_block->t, ifblock->t);
			jump_fix_offset(fs, prevFalse, getCodePos(fs,ifblock->f)-getCodePos(fs,prevFalse));
			prevFalse = ifblock->f;
			freeExpdesc(pa->ts, ifblock);
		} else {
			tk = Bytes_get(tokens, expid, qtk);
		}
	}
	if (i == alltks->length) {
		qgramdesc *elseinfo = getGramDesc(info->exprs, alltks->data[i - 1].i);
		Int end = (Int) getGramExpDesc(pa, info->exprs,
				elseinfo->u.alltks->data[0].i);
		jump_fix_offset(fs, prevFalse, getCodePos(fs,end)-getCodePos(fs,prevFalse+1)); //有bug，先凑活着用
		freeGramdesc(elseinfo);
	} else {
		jump_link(fs, &if_block->t, prevFalse);
	}
	fs->jpc = if_block->t;
	freeExpdesc(pa->ts, if_block);
}
void gram_condition(LRParser *pa, RuleInfo *info) {
	FuncInfo *fs = pa->fs;
	printf("---gram_condition---\n");
	qgramdesc *desc = info->desc;
	qvec alltks = desc->u.alltks;
//	qbytes *tokens = info->tokens;
	desc->flag = TD_KEEP;
	expdesc *e = getGramExpDesc(pa, info->exprs, alltks->data[0].i);
	if (var_get(fs, e)) {
		expToAnyReg(fs, e);
		e->f = jumpifFalse(fs, -1, e);
	}
	desc->data = e;
}

void gram_if_block(LRParser *pa, RuleInfo *info) {
	printf("---gram_if_block---\n");
	qgramdesc *desc = info->desc;
	qvec alltks = desc->u.alltks;
	qbytes *tokens = info->tokens;
	FuncInfo *fs = pa->fs;
	qtk *tk = Bytes_get(tokens, info->pos, qtk);
	expdesc *condition = getGramExpDesc(pa, info->exprs, alltks->data[0].i);
	if (tk->type == TW_elif || tk->type == TW_else) {
		condition->t = code_addAB(fs, OP_JMP, NO_JUMP, -1);
	}
	desc->data = condition;
	desc->flag = TD_KEEP;
}
void gram_else_prefix(LRParser *pa, RuleInfo *info) {
	info->desc->data = (void*) cast(Int, pa->fs->f->ninstr);
	info->desc->flag = TD_KEEP;
}

void gram_classdef(LRParser *pa, RuleInfo *info) {
	printf("---gram_classdef---\n");
	expdesc *v = getGramExpDesc(pa, info->exprs,
			info->desc->u.alltks->data[1].int32);
	Proto *p = pa->fs->f;
	for (int i = 0; i < p->ninstr; i++) {
		assert(getinstr(p,i)[0]!=OP_RETURN);
	}
	code_ret(pa->fs, 0, 1);
	scope_leave(pa->fs);
	func_close(pa);
//	if (v->k == VNLOCAL)
//		var_localActive(pa->fs, 1);
	int nparam = v->u.ind.idx;
	int func = pa->fs->usedreg - nparam - 1;
	code_addAB(pa->fs, OP_CLOSURE, func, pa->fs->f->np - 1);
	code_addABC(pa->fs, OP_CALL, func, nparam, 2);
	if (v->k == VLOCAL)
		code_addAB(pa->fs, OP_MOVE, v->u.ind.t, func);
	freeExpdesc(pa->ts, v);
}
void gram_class_head(LRParser *pa, RuleInfo *info) {
	if (selfid == 0) {
		selfid = addConst(pa, (Object*) str_get(pa->ts, "this"));
		newid = addConst(pa, (Object*) str_get(pa->ts, "__new__"));
		initid = addConst(pa, (Object*) str_get(pa->ts, "__init__"));
	}
	FuncInfo *fs = pa->fs;
	printf("---gram_class_head---\n");
	expdesc *v = (expdesc*) mem_alloc(pa->ts, NULL, 0, sizeof(expdesc));
	qgramdesc *desc = info->desc;
	qvec alltks = desc->u.alltks;
	qbytes *tokens = info->tokens;
	int clsname = Bytes_get(tokens, alltks->data[0].i, qtk)->v.i;
	searchvar(pa, clsname, v, VVOID);
	assert(v->k == VNLOCAL || v->k == VLOCAL);
	if (v->k == VNLOCAL)
		var_localActive(fs, 1);
	int nparams = 0;
	if (alltks->length > 1) {
		fs->usedreg = fs->nactvar;
		qgramdesc *desc2 = getGramDesc(info->exprs, alltks->data[1].int32);
		alltks = desc2->u.alltks;
		if (v->k == VLOCAL) {
			expdesc temp = *v;
			expToNextReg(fs, &temp);
		}
		for (int i = 1; i < alltks->length; i += 2) {
			expdesc *e = getGramExpDesc(pa, info->exprs, alltks->data[i].int32);
			expToNextReg(fs, e);
			freeExpdesc(pa->ts, e);
			nparams++;
		}
		freeGramdesc(desc2);
	}
	func_create(pa);
	fs = pa->fs;
	scope_new(fs);
	fs->f->nparams = nparams;
	code_addAB(fs, OP_NEWCLASS, clsname, nparams);
	addLocalvar(pa, selfid);
	var_localActive(fs, 1);
	fs->usedreg = 1;
	v->u.ind.t = (ushort) v->u.info;
	v->u.ind.idx = nparams;
	desc->data = v;
	desc->flag = TD_KEEP;
}

void gram_func_head(LRParser *pa, RuleInfo *info) {
	printf("---gram_func_head---\n");
	int expid, nparam;
	expdesc *v = (expdesc*) mem_alloc(pa->ts, NULL, 0, sizeof(expdesc));
	qbytes *tokens = info->tokens;
	qgramdesc *desc = info->desc;
	int funname = Bytes_get(tokens, desc->u.alltks->data[0].i, qtk)->v.i;
	searchvar(pa, funname, v, VVOID);
	assert(v->k == VNLOCAL || v->k == VLOCAL);
//	if (v->k == VNLOCAL)
//		var_localActive(pa->fs, 1);
	func_create(pa);
	scope_new(pa->fs);
	RBNode *node = RB.search(info->exprs, desc->u.alltks->data[1].i);
	qgramdesc *params = cast(qgramdesc *, node->val);
	qvec alltks = params->u.alltks;
	Proto* p = pa->fs->f;
	if (alltks->length > 2) {
		for (int i = 1; i < alltks->length; i += 2) {
			if ((expid = alltks->data[i].i) & EXPFLAG) {
				expdesc *e = getGramExpDesc(pa, info->exprs, expid);
				list_append(pa->ts, &_list, int_new(pa->ts, e->u.ind.idx));
				addLocalvar(pa, e->u.ind.t);
				freeExpdesc(pa->ts, e);
			} else {
				qtk *tk = Bytes_get(tokens, expid, qtk);
				if (tk->type == TW_MUL) {
					tk = Bytes_get(tokens, alltks->data[++i].i, qtk);
					p->flag_vararg |= 1;
				} else if (tk->type == TW_MUL_MUL) {
					tk = Bytes_get(tokens, alltks->data[++i].i, qtk);
					p->flag_vararg |= 2;
				}
				addLocalvar(pa, tk->v.i);
			}
		}
	}
	if (_list.length) {
		p->defValue = list_toTuple(pa->ts, &_list);
		_list.length = 0;
	}
	nparam = p->nlocvars;
	p->nparams = nparam;
	var_localActive(pa->fs, nparam);
	pa->fs->usedreg = nparam;
	freeGramdesc(params);
	if (p->flag_vararg & 1)
		p->nparams--;
	if (p->flag_vararg & 2)
		p->nparams--;
	RB.delNode(info->exprs, node);
	desc->data = v;
	desc->flag = TD_KEEP;
}
void gram_meth_head(LRParser *pa, RuleInfo *info) {
	printf("---gram_meth_head---\n");
	expdesc *v = (expdesc*) mem_alloc(pa->ts, NULL, 0, sizeof(expdesc));
	qbytes *tokens = info->tokens;
	qgramdesc *desc = info->desc;
	qvec alltks = desc->u.alltks;
	func_create(pa);
	scope_new(pa->fs);
	int pos = 1, funname;
	if (alltks->length == 2) {
		funname = Bytes_get(tokens, alltks->data[0].i, qtk)->v.i;
		v->k = VMETHOD;
		addLocalvar(pa, selfid);
	} else {
		funname = Bytes_get(tokens, alltks->data[1].i, qtk)->v.i;
		pos++;
		v->k = VSTMETHOD;
	}
	v->u.info = funname;
	RBNode *node = RB.search(info->exprs, alltks->data[pos].i);
	qgramdesc *params = cast(qgramdesc *, node->val);
	alltks = params->u.alltks;
	int nparam = (alltks->length - 1) / 2;
	for (int i = 0; i < nparam; i++) {
		qtk *tk = Bytes_get(tokens, alltks->data[1 + 2 * i].i, qtk);
		addLocalvar(pa, tk->v.i);
	}
	if (v->k == VMETHOD)
		nparam++;
	pa->fs->f->nparams = nparam;
	var_localActive(pa->fs, nparam);
	pa->fs->usedreg = pa->fs->nlocvars;
	freeGramdesc(params);
	RB.delNode(info->exprs, node);
	desc->data = v;
	desc->flag = TD_KEEP;
}
void gram_funcdef(LRParser *pa, RuleInfo *info) {
	printf("---gram_funcdef---\n");
	scope_leave(pa->fs);
	func_close(pa);
	expdesc *v = getGramExpDesc(pa, info->exprs,
			info->desc->u.alltks->data[1].int32);
	func_load(pa, v);
	freeExpdesc(pa->ts, v);
}
void gram_methdef(LRParser *pa, RuleInfo *info) {
	printf("---gram_methdef---\n");
	expdesc *v = getGramExpDesc(pa, info->exprs,
			info->desc->u.alltks->data[1].int32);
	Proto *p = pa->fs->f;
	if (v->u.info == initid) {
		assert(v->k == VMETHOD);
		for (int i = 0; i < p->ninstr; i++) {
			assert(getinstr(p,i)[0]!=OP_RETURN);
		}
		code_ret(pa->fs, 0, 1);
	}
	scope_leave(pa->fs);
	func_close(pa);
	method_load(pa, v);
	freeExpdesc(pa->ts, v);
}

void gram_parameters(LRParser *pa, RuleInfo *info) {
	printf("---gram_parameters---\n");
//	qgramdesc *desc = info->desc;
//	qvec alltks = desc->u.alltks;
//	qbytes *tokens = info->tokens;
}

void gram_argassign(LRParser *pa, RuleInfo *info) {
	printf("---gram_argassign---\n");
	qgramdesc *desc = info->desc;
	qvec alltks = desc->u.alltks;
	qbytes *tokens = info->tokens;
	int name = Bytes_get(tokens, alltks->data[0].int32, qtk)->v.int32;
	expdesc *v = getGramExpDesc(pa, info->exprs, alltks->data[2].i);
	v->u.ind.idx = expToRK(pa->fs, v);
	v->u.ind.t = name;
	v->k = VASSIGN;
	desc->data = v;
}
void gram_argument_1(LRParser *pa, RuleInfo *info) {
	printf("---gram_argument_1---\n");
	qgramdesc *desc = info->desc;
	qvec alltks = desc->u.alltks;
	qbytes *tokens = info->tokens;
	int name = Bytes_get(tokens, alltks->data[0].int32, qtk)->v.int32;
	expdesc *v = getGramExpDesc(pa, info->exprs, alltks->data[2].i);
	v->u.ind.idx = expToRK(pa->fs, v);
	v->u.ind.t = name;
	v->k = VASSIGN;
	desc->data = v;
}
void gram_argument_2(LRParser *pa, RuleInfo *info) {
	printf("---gram_argument_2---\n");
//	qgramdesc *desc = info->desc;
//	qvec alltks = desc->u.alltks;
//	qbytes *tokens = info->tokens;
//	int name = Bytes_get(tokens, alltks->data[0].int32, qtk)->v.int32;
//	expdesc *v = getGramExpDesc(pa, info->exprs, alltks->data[2].i);
//	v->u.ind.idx = expToRK(pa->fs, v);
//	v->u.ind.t = name;
//	v->k = VASSIGN;
//	desc->data = v;
}
void gram_argument_3(LRParser *pa, RuleInfo *info) {
	printf("---gram_argument_3---\n");
//	qgramdesc *desc = info->desc;
//	qvec alltks = desc->u.alltks;
//	qbytes *tokens = info->tokens;
//	int name = Bytes_get(tokens, alltks->data[0].int32, qtk)->v.int32;
//	expdesc *v = getGramExpDesc(pa, info->exprs, alltks->data[2].i);
//	v->u.ind.idx = expToRK(pa->fs, v);
//	v->u.ind.t = name;
//	v->k = VASSIGN;
//	desc->data = v;
}

void gram_vardef(LRParser *pa, RuleInfo *info) {
	qgramdesc *desc = info->desc;
	qvec alltks = desc->u.alltks;
	qbytes *tokens = info->tokens;
	qtk *tk = Bytes_get(info->tokens, alltks->data[0].i, qtk);
	qgramdesc *vardesc =
			(qgramdesc*) RB.search(info->exprs, alltks->data[1].i)->val, *exprdesc;
	qvec varname = vardesc->u.alltks, exprs;
	expdesc expinfo, *v;
	int nvar = (varname->length + 1) / 2, nexp;
	if (alltks->length > 2) { //alltks->data[2]='='
		exprdesc = (qgramdesc*) RB.search(info->exprs, alltks->data[3].i)->val;
		exprs = exprdesc->u.alltks;
		nexp = (exprs->length + 1) / 2;
		assert(nexp <= nvar);
	} else
		nexp = 0;
	printf("---gram_vardef---\n");
	if (tk->type == TW_var) {
//		printf("var: ");
		for (int i = 0; i < nvar; i++) {
			tk = Bytes_get(tokens, varname->data[i * 2].i, qtk);
//			qstr *name = gconst(pa, tk->v.i)->val.s;
//			printf("%s  ", name->val);
			searchvar(pa, tk->v.i, &expinfo, VLOCAL);
			if (i < nexp) {
				vardesc =
						(qgramdesc*) RB.search(info->exprs, exprs->data[i * 2].i)->val;
				v = vardesc->data;
				var_store(pa, &expinfo, v);
			}
		}
		var_localActive(pa->fs, nvar);
	} else {
		for (int i = 0; i < nvar; i++) {
			tk = Bytes_get(tokens, varname->data[i * 2].i, qtk);
			searchvar(pa, tk->v.i, &expinfo, VGLOBAL);
			if (i < nexp) {
				vardesc =
						(qgramdesc*) RB.search(info->exprs, exprs->data[i * 2].i)->val;
				v = vardesc->data;
				var_store(pa, &expinfo, v);
			}
		}
	}
}

//@formatter:off
struct funlist{
	TypeGram typeGram;
	void *fun;
	CallType type;
}injectlist[]={
		{TG_vardef, 			  gram_vardef,       C_ATONCE },
		{TG_assign_stmt, 	  gram_assign_stmt,  C_ATONCE },
		{TG_call_stmt, 	    gram_call_stmt,    C_ATONCE },
		{TG_if_stmt, 	      gram_if_stmt,      C_ATONCE },
		{TG_condition, 	    gram_condition,    C_ATONCE },
		{TG_if_block, 	    gram_if_block,     C_ATONCE },
		{TG_else_prefix, 	  gram_else_prefix,  C_ATONCE },
		{TG_if_else_block, 	 NULL,             C_NEVER  },
		{TG_block_head, 	  gram_block_head,   C_ATONCE },
		{TG_block, 	        gram_block,        C_ATONCE },
		{TG_varname, 			  NULL,     			   C_NEVER  },
		{TG_varlist, 			  NULL,     			   C_NEVER  },
		{TG_testlist_star_expr, NULL,     		 C_NEVER  },
		{TG_atom_expr_0,    gram_atom_expr,    C_CHILD  },
		{TG_atom_expr_1,    gram_atom_expr,    C_CHILD  },
		{TG_var_expr,       gram_atom_expr,    C_CHILD  },
//		{TG_var_expr, 	    gram_var_expr,     C_CHILD  },
		{TG_func_head, 	    gram_func_head,    C_ATONCE },
		{TG_meth_head, 	    gram_meth_head,    C_ATONCE },
		{TG_funcdef, 	      gram_funcdef,      C_ATONCE },
		{TG_methdef, 	      gram_methdef,      C_ATONCE },
		{TG_classdef, 	    gram_classdef,     C_ATONCE },
		{TG_argassign, 	    gram_argassign,    C_ATONCE },
//		{TG_argument_0, 	  gram_argument,    C_ATONCE },
		{TG_argument_1, 	  gram_argument_1,    C_ATONCE },
		{TG_argument_2, 	  gram_argument_2,    C_ATONCE },
		{TG_argument_3, 	  gram_argument_3,    C_ATONCE },
//		{TG_argone, 	      gram_argone,    C_ATONCE },
//		{TG_argone, 	      gram_argtwo,    C_ATONCE },
		{TG_parameters, 	  gram_parameters,   C_NEVER  },
		{TG_class_head, 	  gram_class_head,   C_ATONCE  },
//		{TG_test_0, 			NULL,     					C_NEVER |C_MERGEABLE   },
		{TG_arglist, 			  NULL,     			   C_NEVER |C_ACCELERABLE },
		{TG_term,					  gram_term,			 	 C_ATONCE|C_ACCELERABLE },
		{TG_arith_expr, 	  gram_arith_expr,   C_ATONCE|C_ACCELERABLE },
		{TG_comparison, 	  gram_comparison,   C_ATONCE|C_ACCELERABLE },
		{TG_and_test, 	    gram_and_test,     C_CHILD |C_ACCELERABLE },
		{TG_or_test, 	      gram_or_test,      C_ATONCE|C_ACCELERABLE },
};
//@formatter:on
LRAnalyzer *loadRules(char *path) {
	LRAnalyzer *analyzer = deserialAnalyzer(path);
	int len = sizeof(injectlist) / sizeof(injectlist[0]);
	struct funlist *funinfo;
	for (int i = 0; i < len; i++) {
		funinfo = &injectlist[i];
		injectFunc(analyzer, funinfo->typeGram, funinfo->fun, funinfo->type);
	}
	return analyzer;
}
static void initLRParser(ThreadState *ts, LRParser *pa) {
	pa->consts = (MapObj*) map_new(pa->ts, true);
	pa->dyd = qcalloc(Dyndata);
}

ModuleObj *parserCode(LRAnalyzer *analyzer, ThreadState *ts, char *src) {
	LRParser parser = { 0 };
	char *main_name = "__main__";
	LRParser *pa = &parser;
	FuncInfo fs = { 0 };
	StrObj * moduleName = str_new(ts, main_name, strlen(main_name));
	ModuleObj *module = module_new(ts, (Object*) moduleName);
	pa->module = module;
	module->source = src;
	Proto *p = &module->main;
	p->module = (Object*) module;
	expdesc expinfo = { 0 };
	fs.f = p;
	fs.jpc = NO_JUMP;
	fs.parser = pa;
	pa->fs = &fs;
	pa->ts = ts;
	pa->expinfo = &expinfo;
	scope_new(&fs);
	initLRParser(ts, pa);
	analyzer->ctx = pa;
	qlexer *lex = create_lexer_file(src);
	lex->tokenHook = (tkHookfn) constHook;
	analyzer->statHook = (statHookFn) statHook;
	lex->ctx = pa;
	pa->lex = lex;
	analyse(analyzer, lex);
	scope_leave(pa->fs);
	func_close(pa);
	return module;
}
//adjustlocalvars
void var_localActive(FuncInfo *fs, int nvars) {
	fs->nactvar = fs->nactvar + nvars;
	for (; nvars; nvars--) {
		var_localGet(fs, fs->nactvar - nvars)->startpc = fs->f->ninstr;
	}
	if (fs->f->maxstacksize < fs->nactvar)
		fs->f->maxstacksize = fs->nactvar;
}
void var_localRemove(FuncInfo *fs, int tolevel) {
	fs->parser->dyd->actvar.n -= (fs->nactvar - tolevel);
	while (fs->nactvar > tolevel)
		var_localGet(fs, --fs->nactvar)->endpc = fs->f->ninstr;
}
LocVar* var_localGet(FuncInfo *fs, int i) {
	int idx = fs->parser->dyd->actvar.idx[fs->firstlocal + i];
	assert(idx < fs->f->nlocvars);
	return &fs->f->locvars[idx];
}
int var_localFind(FuncInfo *fs, int name) {
	int i;
	for (i = fs->nactvar - 1; i >= 0; i--) {
		if (name == var_localGet(fs, i)->idx)
			return i;
	}
	return -1; /* not found */
}
LocVar* var_globalGet(FuncInfo *fs, int i) {
	return &fs->f->glvars[i];
}
int var_globalFind(FuncInfo *fs, int n) {
	int i;
	for (i = fs->ngl - 1; i >= 0; i--) {
		if (n == var_globalGet(fs, i)->idx)
			return i;
	}
	return -1; /* not found */
}
int var_upbalFind(FuncInfo *fs, int name) {
	int i;
	Upvaldesc *up = fs->f->upvalues;
	for (i = 0; i < fs->nups; i++) {
		if (up[i].idx == name)
			return i;
	}
	return -1; /* not found */
}
void var_indexed(FuncInfo *fs, expdesc *t, int attrname) {
//  assert(!hasjumps(t) && (vkisinreg(t->k) || t->k == VUPVAL));
	if (t->k == VINDEXED)
		expToNextReg(fs, t);
	t->u.ind.t = t->u.info; /* register or upvalue index */
	t->u.ind.idx = attrname; /* K index for key */
	t->u.ind.vt = t->k; //== VUPVAL ? VUPVAL : VLOCAL;
	t->k = VINDEXED;
}
void var_global(FuncInfo *fs, expdesc *t, int varname) {
//	int rk = expToRK(fs, k);
//	if (t->u.ind.t) {
//		if (t->u.ind.idx) {
//			expToNextReg(fs, t);
//			exp_free(fs, t);
//			t->u.ind.t = t->u.info;
//			t->u.ind.idx = rk;
//			t->u.ind.vt = VLOCAL;
//			return;
//		} else {
//			t->u.ind.idx = rk + GLBAIS;
//		}
//	} else {
//		t->u.ind.t = rk + GLBAIS;
//	}
	t->u.info = varname;
	t->k = VGLOBAL;
}
int var_get(FuncInfo *fs, expdesc *e) {
	switch (e->k) {
	case VLOCAL: /* already in a register */
		e->k = VNONRELOC; /* becomes a non-relocatable value */
	case VNONRELOC:
	case VRELOCABLE:
		return true;
	case VUPVAL: { /* move value to some (pending) register */
		e->u.info = code_addAB(fs, OP_UPVAL, VRELOCABLE_REG, e->u.info);
		e->k = VRELOCABLE;
		return true;
	}
	case VGLOBAL:
		e->u.info = code_addAB(fs, OP_GLOBAL, VRELOCABLE_REG, e->u.info);
		e->k = VRELOCABLE;
		return true;
	case VINDEXED: {
		if (e->u.ind.vt == VLOCAL) { /* is 't' in a register? */
			reg_free(fs, e->u.ind.t);
			e->u.info = code_addABC(fs, OP_GET_ATTR, VRELOCABLE_REG, e->u.ind.t,
					e->u.ind.idx);
		} else if (e->u.ind.vt == VGLOBAL) { /* is 't' in a register? */
			//        reg_free(fs, e->u.ind.t);
			e->u.info = code_addABC(fs, OP_GET_ATTR, VRELOCABLE_REG,
					e->u.ind.t | BITRK, e->u.ind.idx);
		} else {
			assert(e->u.ind.vt == VUPVAL);
//			op = OP_GETTABUP; /* 't' is in an upvalue */
		}
		e->k = VRELOCABLE;
		return true;
	}
	case VASSIGN: {
		if (e->u.ind.idx & BITRK)
			return false;
		return true;
	}
//	case VVARARG:
	case VCALL: {
//		luaK_setoneret(fs, e);
		return true;;
	}
	default:
		return false; /* there is one value available (somewhere) */
	}
}
int code_add(FuncInfo *fs, Instruction* i, int n) {
	if (fs->jpc != NO_JUMP)
		jump_fetch(fs);
	Proto *f = fs->f;
	int pc = f->ncode;
	mem_growvector(fs->parser->ts, f->code, f->ncode, fs->pc, Instruction,
			INT_MAX, n);
	for (int j = 0; j < n; j++) {
		f->code[f->ncode++] = i[j];
	}
	/* save corresponding line information */
	mem_growvector(fs->parser->ts, f->lineinfo, f->ninstr, fs->pc2, linedesc,
			INT_MAX, 1);
	f->lineinfo[f->ninstr].line = fs->parser->lex->curline;
	f->lineinfo[f->ninstr].pc = pc;
	printcode(f->code + pc, f->ninstr, f);
	return f->ninstr++;;
}
int code_addA(FuncInfo *fs, OpCode o, int a) {
	Instruction i[oA];
	assert(a <= MAXARG_A);
	sky_check(opcodes[o].size == oA, opcodes[o].name);
	i[0] = o;
	GETARG_A (i)= a;
	return code_add(fs, i, oA);
}
int code_addAB(FuncInfo *fs, OpCode o, int a, int b) {
	Instruction i[oAB];
	sky_check(opcodes[o].size == oAB, opcodes[o].name);
	assert(a <= MAXARG_A && b <= MAXARG_B);
	GET_OPCODE (i)= o;
	GETARG_A (i)= a;
	GETARG_B (i)= b;
	return code_add(fs, i, oAB);
}
int code_addABC(FuncInfo *fs, OpCode o, int a, int b, int c) {
	Instruction i[oABC];
	sky_check(opcodes[o].size == oABC, opcodes[o].name);
//  assert(getOpMode(o) == iABC);
//  assert(getBMode(o) != OpArgN || b == 0);
//  assert(getCMode(o) != OpArgN || c == 0);
	assert(a <= MAXARG_A && b <= MAXARG_B && c <= MAXARG_C);
	GET_OPCODE (i)= o;
	GETARG_A (i)= a;
	GETARG_B (i)= b;
	GETARG_C (i)= c;
	return code_add(fs, i, oABC);
}
int code_addABCD(FuncInfo *fs, OpCode o, int a, int b, int c, int d) {
	Instruction i[oABCD];
	assert(opcodes[o].size == oABCD || opcodes[o].size == oABC);
	assert(a <= MAXARG_A && b <= MAXARG_B && c <= MAXARG_C && d <= MAXARG_D);
	GET_OPCODE (i)= o;
	GETARG_A (i)= a;
	GETARG_B (i)= b;
	GETARG_C (i)= c;
	GETARG_D (i)= d;
	return code_add(fs, i, oABCD);
}
int code_const(FuncInfo *fs, int reg, int k) {
	return code_addAB(fs, OP_CONST, reg, k);
}
int code_ret(FuncInfo *fs, int first, int nret) {
	return code_addAB(fs, OP_RETURN, first, nret + 1);
}
int jumpifFalse(FuncInfo *s, int reg, expdesc *e) {
	return code_addABC(s, OP_JMP_IF_FALSE, NO_JUMP, e->u.info, reg);
}
int jumpifTrue(FuncInfo *s, int reg, expdesc *e) {
	return code_addABC(s, OP_JMP_IF_TRUE, NO_JUMP, e->u.info, reg);
}
/**
 * 获取链接的下一个指令
 */
int jump_offset(FuncInfo *fs, int pc) {
//	Instruction *i = fs->f->code + pcPos;
	Instruction *i = getCode(fs, pc);
	short offset = GETARG_A(i);
	return offset;
//	if (offset == NO_JUMP) /* point to itself represents end of list */
//		return NO_JUMP; /* end of list */
//	else
//		return pcPos + offset; /* turn offset into absolute position */
}
/**
 * dest为code的位置
 * pc为第几个指令
 */

static void jump_fix_offset(FuncInfo *fs, int pc, int offset) {
	int pcPos = fs->f->lineinfo[pc].pc;
	Instruction *jmp = fs->f->code + pcPos;
	GETARG_A (jmp)= offset;
}
static void jump_fix_reg(FuncInfo *fs, int pc, int reg) {
	int pcPos = fs->f->lineinfo[pc].pc;
//	int destPos = fs->f->lineinfo[dest].pc;
//	assert(dest != NO_JUMP);
//	short offset = destPos - pcPos;
	Instruction *jmp = fs->f->code + pcPos;
	GETARG_C (jmp)= reg;
}
static short jump_get_reg(FuncInfo *fs, int pc) {
	Instruction *jmp = getCode(fs, pc);
	return GETARG_C(jmp);
}
static void jump_fetch(FuncInfo *fs) {
//	patchlistaux(fs, fs->jpc, fs->f->ncode, NO_REG, fs->f->ncode);
	short list = fs->jpc, next;
	int target = fs->f->ncode;
	while (list != NO_JUMP) {
		next = jump_offset(fs, list);
		jump_fix_offset(fs, list,
				(list == fs->f->ninstr - 1) ? 0 : (target - getCodePos(fs, list + 1)));
		list = next;
	}
	fs->jpc = NO_JUMP;
}
void jump_link(FuncInfo *fs, int *l1, int l2) {
	if (l2 == NO_JUMP)
		return; /* nothing to concatenate? */
	else if (*l1 == NO_JUMP) /* no original list? */
		*l1 = l2; /* 'l1' points to 'l2' */
	else {
		int list = *l1;
		int next;
		while ((next = jump_offset(fs, list)) != NO_JUMP) /* find last element */
			list = next;
		jump_fix_offset(fs, list, l2); /* last element links to 'l2' */
	}
}

bool objToBool(Object *o) {
	switch (o->type->basetype) {
	case BT_BOOL:
	case BT_INT:
		return cast(IntObj*,o)->ival != 0;
	case V_NUMFLT:
		return cast(FloatObj*,o)->fval != 0;
	case V_SHRSTR:
		return cast(StrObj*,o)->length > 0;
	default:
		sky_error("objToBool error!", 0);
	}
}
bool kToBool(LRParser *parser, expdesc *e) {
	switch (e->k) {
	case VK:
		return objToBool(gconst(parser, e->u.info));
	case VKBOOL:
	case VKINT:
		return e->u.ival != 0;
	case VKFLT:
		return e->u.nval != 0;
	case VKSTR:
		return e->u.str->len != 0;
	default:
		return false;
	}
	return false;
}

void varToReg(FuncInfo *fs, expdesc *e, int reg) {
	switch (e->k) {
	case VRELOCABLE: { //填充reg
		Instruction *pc = getinstruction(fs, e);
		GETARG_A (pc)= reg;
		printcode(pc, e->u.info, fs->f);
		break;
	}

	case VGLOBAL: {
		code_addABC(fs, OP_GETTABGL, reg, e->u.ind.t, e->u.ind.idx);
		break;
	}
	case VCALL: {
		int ra = GETARG_A(getinstruction(fs, e));
		if (reg != ra)
			code_addAB(fs, OP_MOVE, reg, ra);
		else
			return;
		break;
	}
	case VLOCAL:
	case VNONRELOC: {
		if (reg != e->u.info)
			code_addAB(fs, OP_MOVE, reg, e->u.info);
		break;
	}
	case VASSIGN: {
		if (reg != e->u.ind.idx) {
			code_addAB(fs, OP_MOVE, reg, e->u.ind.idx);
//			e->e->u.ind.idx = reg;
		}
		break;
	}
	default: {
//      assert(e->k == VJMP);
		return; /* nothing to do... */
	}
	}
	e->u.info = reg;
	e->k = VNONRELOC;
}
void expToReg(FuncInfo *fs, expdesc *e, int reg) {
	if (var_get(fs, e)) {
		varToReg(fs, e, reg);
	} else {
		expToK(fs->parser, e);
		code_const(fs, reg, e->u.info);
	}
//	if (e->k == VJMP) /* expression itself is a test? */
//		luaK_concat(fs, &e->t, e->u.info); /*保留正确需要跳转的指令 put this jump in 't' list */
//	if (hasjumps(e)) {
//		int final; /* position after whole expression */
//		int p_f = NO_JUMP; /* position of an eventual LOAD false */
//		int p_t = NO_JUMP; /* position of an eventual LOAD true */
//		if (need_value(fs, e->t) || need_value(fs, e->f)) {
//			int fj = (e->k == VJMP) ? NO_JUMP : luaK_jump(fs); //不是jmp,则到这里得到的不是bool
//			p_f = code_loadbool(fs, reg, 0, 1);				//需插入jmp跳过两个bool指令
//			p_t = code_loadbool(fs, reg, 1, 0);
//			luaK_patchtohere(fs, fj);				//jpc=fj,在下一表达式生成指令前fix jump
//		}
//		final = fs->lasttarget = fs->f->ncode;
//		patchlistaux(fs, e->f, final, reg, p_f);
//		patchlistaux(fs, e->t, final, reg, p_t);
//	}
//	e->f = e->t = NO_JUMP;
//	e->u.info = reg;
//	e->k = VNONRELOC;
}
void expToRegUp(FuncInfo *fs, expdesc *e) {
	if (e->k != VUPVAL) //|| hasjumps(e))
		expToAnyReg(fs, e);
}
int expToK(LRParser *pa, expdesc *e) {
	switch (e->k) { /* move constants to 'k' */
	case VTRUE:
		e->u.info = addBoolConst(pa, 1);
		goto vk;
	case VFALSE:
		e->u.info = addBoolConst(pa, 0);
		goto vk;
	case VNIL:
//		e->u.info = nilK(fs);
		goto vk;
	case VKINT:
		e->u.info = addIntConst(pa, e->u.ival);
		goto vk;
	case VKFLT:
		e->u.info = addFltConst(pa, e->u.nval);
		goto vk;
	case VKSTR:
		e->u.info = addStrConst(pa, e->u.str->val);
		goto vk;
	case VASSIGN:
		if (e->u.ind.idx & BITRK) {
			e->k = VK;
			return RKMASK(e->u.info=RKID(e->u.ind.idx));
		}
		return 0;
	case VK:
		vk: e->k = VK;
		if (e->u.info <= MAXINDEXRK) /* constant fits in 'argC'? */
			return RKMASK(e->u.info); //(e->u.info) | (1 << (9 - 1)),k大于等于256
		else
			return 0;
	default:
		return 0;
	}
}
int expToRK(FuncInfo *fs, expdesc *e) {
	if (expToK(fs->parser, e) == 0) {
		expToAnyReg(fs, e);
		return e->u.info;
	} else {
		return RKMASK(e->u.info);
	}
}
void expToAnyReg(FuncInfo *fs, expdesc *e) {
	if (e->k != VLOCAL && e->k != VNONRELOC)
		expToNextReg(fs, e);
}
void expToNextReg(FuncInfo *fs, expdesc *e) {
//	luaK_dischargevars(fs, e); //VK无视
//	exp_free(fs, e);
	reg_reserve(fs, 1); //占用一个寄存器
	expToReg(fs, e, fs->usedreg - 1);
}
void reg_check(FuncInfo *fs, int n) {
	int newstack = fs->usedreg + n;
	if (newstack > fs->f->maxstacksize) {
		assert(newstack < MAXREGS);
		fs->f->maxstacksize = newstack;
	}
}
void reg_reserve(FuncInfo *fs, int n) {
	reg_check(fs, n);
	fs->usedreg += n;
}
void reg_free(FuncInfo *fs, int reg) {
	if (!ISK(reg) && reg >= fs->nactvar) {
		fs->usedreg--;
//    assert(reg == fs->reg_free);
	}
}
void var_store(LRParser *pa, expdesc *var, expdesc *ex) {
	int e;
	FuncInfo *fs = pa->fs;
	switch (var->k) {
	case VNLOCAL:
		var_localActive(fs, 1);
	case VLOCAL: {
		if (ex->f != NO_JUMP) {
			short list = ex->f, next;
			while (list != NO_JUMP) {
				next = jump_offset(fs, list);
				jump_fix_reg(fs, list, var->u.info);
				list = next;
			}
		}
		exp_free(fs, ex);
		expToReg(fs, ex, var->u.info); /* compute 'ex' into proper place */
		return; //结束以免寄存器被释放
	}
	case VUPVAL: {
		if (ex->f != NO_JUMP) {
			short reg = jump_get_reg(fs, ex->f);
			expToReg(fs, ex, reg);
			code_addAB(fs, OP_SETUPVAL, ex->u.info, var->u.info);
		} else {
			code_addAB(fs, OP_SETUPVAL, expToRK(fs, ex), var->u.info);
		}
		break;
	}
	case VINDEXED: {
		OpCode op;
		e = expToRK(fs, ex);
		if (var->u.ind.vt != VUPVAL) {
			op = OP_SET_ATTR;
		} else {
			op = OP_SETTABUP;
		}
		if (var->u.ind.vt != VGLOBAL)
			code_addABC(fs, op, var->u.ind.t, var->u.ind.idx, e);
		else
			code_addABC(fs, op, RKMASK(var->u.ind.t), var->u.ind.idx, e);
		break;
	}
	case VNGLOBAL:
	case VGLOBAL:
		code_addAB(fs, OP_SETGLOBAL, var->u.info, expToRK(fs, ex));
		break;
	default:
		break; /* invalid var kind to store */
	}
//	exp_free(fs, ex);//???
}

int addConst(LRParser *pa, Object *key) {
	ModuleObj *m = pa->module;
	MapEntry entry;
	IntObj *idx /*= Map.gset(pa->ts, pa->consts, key, &entry)*/;
	int k;
	if (map_gset(pa->ts, pa->consts, key, true, &entry)) {
		idx = (IntObj*) entry.dict->val;
		k = idx->ival;
		assert(k < m->nconst);
		return k;
	}
	k = m->nconst;
	entry.dict->val = (Object*) int_new(pa->ts, k);
	mem_growvector(pa->ts, m->consts, m->nconst, pa->nconst, Object*, MAXARG_Ax,
			1);
	m->consts[k] = key;
	return m->nconst++;
}
int addConstNoHash(LRParser *pa, Object *key) {
	ModuleObj *m = pa->module;
	m->consts[m->nconst] = key;
	return m->nconst++;
}

int addStrConst(LRParser *pa, char *s) {
	StrObj *str = str_new(pa->ts, s, strlen(s));
	return addConst(pa, (Object*) str);
}
int addBoolConst(LRParser *pa, int i) {
	return addConst(pa, cast(Object*, i ? &Obj_True : &Obj_False));
}
int addIntConst(LRParser *pa, INT i) {
	return addConst(pa, (Object*) int_new(pa->ts, i));
}
int addFltConst(LRParser *pa, FLT f) {
	return addConst(pa, (Object *) float_new(pa->ts, f));
}
void addLocalvar(LRParser* pa, int str) {
	Dyndata *dyd = pa->dyd;
	FuncInfo *fs = pa->fs;
	Proto *f = fs->f;
	mem_growvector(pa->ts, f->locvars, f->nlocvars, fs->nlocvars, LocVar,
			SHRT_MAX, 1);
	f->locvars[f->nlocvars].idx = str;
//  luaC_objbarrier(pa->ts, f, varname);
	short reg = f->nlocvars++; //局部变量数
	mem_growvector(pa->ts, dyd->actvar.idx, dyd->actvar.n, dyd->actvar.size,
			short, SHRT_MAX, 1);
	dyd->actvar.idx[dyd->actvar.n++] = reg;
}
static Proto *addProto(LRParser *pa) {
	Proto *newproto;
	FuncInfo *fs = pa->fs;
	Proto *f = fs->f; /* prototype of current function */
	mem_growvector(pa->ts, f->p, f->np, fs->np, Proto*, 0, 1);
	f->p[f->np++] = newproto = (Proto *) mem_alloc(pa->ts, NULL, 0,
			sizeof(Proto));
	memset(newproto, 0, sizeof(Proto));
	newproto->module = (Object*) pa->module;
	return newproto;
}

int addUpvalue(FuncInfo *fs, int name, expdesc *v) {
	Proto *f = fs->f;
	mem_growvector(fs->parser->ts, f->upvalues, f->nupvalues, fs->nups, Upvaldesc,
			MAXUPVAL, 1);
	f->upvalues[f->nupvalues].instack = (v->k == VLOCAL);
	f->upvalues[f->nupvalues].idx = v->u.info;
	f->upvalues[f->nupvalues].name = name;
	return f->nupvalues++;
}
void statHook(LRAnalyzer *analyzer, LRParser *pa) {
	pa->fs->usedreg = pa->fs->nactvar;
}
void constHook(LRParser *pa, qtk *token) {
	int idx;
	switch (token->type) {
	case TW_NAME:
		idx = addStrConst(pa, token->v.s->val);
//		token->type = VK;
		token->v.i = idx;
		break;
	case TW_STRING:
//		token->type = VKSTR;
		break;
	case TW_NUM: {
		break;
	}
	case TW_BOOL:
//		token->type = VKBOOL;
		break;
	default:
		assert(0);
		break;
	}
}
/**
 * base:
 *  0 up
 * 1 local
 * 2 global,不查找up链
 */
void searchByChain(FuncInfo *fs, int name, expdesc *var, int base) {
	if (fs == NULL) { /* no more levels? */
		exp_init(var, VVOID, 0); /* default is global */
		return;
	}
	int idx = var_localFind(fs, name); /* look up locals at current level */
	if (idx >= 0) { /* found? */
		exp_init(var, VLOCAL, idx); /* variable is local */
//			if (!base)
//				markupval(fs, v); /* local will be used as an upval */
		return;
	} /* not found as local at current level; try upvalues */
	if (base) {
		idx = var_globalFind(fs, name);
		if (idx >= 0) {
			exp_init(var, VGLOBAL, idx);
			return;
		}
	}
	idx = var_upbalFind(fs, name); /* try existing upvalues */
	if (idx < 0) { /* not found? */
		if (base != 2) {
			searchByChain(fs->prev, name, var, 0); /* try upper levels */
			if (var->k == VVOID) /* not found? */
				return; /* it is a global */
			idx = addUpvalue(fs, name, var); /* 递归形成up链 */
		} else {
			exp_init(var, VVOID, 0); /* default is global */
			return;
		}
	}
	exp_init(var, VUPVAL, idx); /* new or old upvalue */
}
/**
 *  flag = VLOCAL 普通赋值,没找到则生成局部变量,左边
 *  flag= VGLOBAL 全局变量,检测是否冲突,左边
 *  flag= VVOID 没找到就生成局部变量,左边
 *  flag=VNORMAL 找寻变量,右边，没找到就从全局表中找
 */
void searchvar(LRParser *pa, int varname, expdesc *var, expkind flag) {
	FuncInfo *fs = pa->fs;
	searchByChain(fs, varname, var, flag == VNORMAL ? 1 : 2);
	switch (flag) {
	case VLOCAL:
		assert(var->k == VVOID);
		addLocalvar(pa, varname);
//		activelocalvars(pa->fs, 1);
		exp_init(var, VNLOCAL, fs->f->nlocvars - 1);
		break;
	case VGLOBAL:
//		if (var->k != VVOID)
//			error("全局变量名冲突!")
		assert(var->k == VVOID);
		exp_init(var, VNGLOBAL, varname);
		break;
	case VNORMAL: {
		if (var->k == VVOID) {
//			expdesc key;
//			exp_init(&key, VK, varname);
//			var_global(pa->fs, var, &key);
			var_global(pa->fs, var, varname);
		}
		break;
	}
	case VVOID: {
		if (var->k == VVOID) {
			addLocalvar(pa, varname);
			//		activelocalvars(pa->fs, 1);
			exp_init(var, VNLOCAL, fs->f->nlocvars - 1);
		}
		break;
	}
	default:
		break;
	}
}
