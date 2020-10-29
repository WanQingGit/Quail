/*
 *  Created on: Jul 24, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#include "instruction.h"
#include "typeobj.h"
#include "intobj.h"
#include "floatobj.h"
#include "strobj.h"
#include "moduleobj.h"
#include "tupleobj.h"
#define mode(b,c,d) (((b)<<4) | ((c)<<2) | (d))
struct opcode opcodes[] = {
/*iA*/
{ iA, oA, OpN, mode(OpN, OpN, OpN), "NIL" },/* OP_NIL */
/*iAB*/
{ iAB, oAB, OpR, mode(OpR, OpN, OpN), "MOVE" },/* OP_MOVE */
{ iAB, oAB, OpR, mode(OpK, OpN, OpN), "CONST" },/* OP_CONST */
{ iAB, oAB, OpR, mode(OpK, OpN, OpN), "GLOBAL" },/* OP_GLOBAL */
{ iAB, oAB, OpK, mode(OpB, OpN, OpN), "SETGLOBAL" },/* OP_SETGLOBAL */
{ iAB, oAB, OpR, mode(OpB, OpN, OpN), "LOADNIL" },/* OP_LOADNIL */
{ iAB, oAB, OpR, mode(OpN, OpN, OpN), "UPVAL" },/* OP_UPVAL */
{ iAB, oAB, OpN, mode(OpB, OpN, OpN), "SETUPVAL" },/* OP_SETUPVAL */
{ iAB, oAB, OpR, mode(OpN, OpN, OpN), "NEWTABLE" },/* OP_NEWTABLE */
{ iAB, oAB, OpR, mode(OpN, OpN, OpN), "NEWLIST" },/* OP_NEWLIST */
{ iAB, oAB, OpR, mode(OpN, OpN, OpN), "NEWCLASS" },/* OP_NEWCLASS */
{ iAB, oAB, OpR, mode(OpB, OpN, OpN), "UNM" },/* OP_UNM */
{ iAB, oAB, OpR, mode(OpB, OpN, OpN), "BNOT" },/* OP_BNOT */
{ iAB, oAB, OpR, mode(OpB, OpN, OpN), "NOT" },/* OP_NOT */
{ iAB, oAB, OpR, mode(OpB, OpN, OpN), "LEN" },/* OP_LEN */
{ iAB, oAB, OpR, mode(OpN, OpN, OpN), "JMP" },/* OP_JMP */
{ iAB, oAB, OpR, mode(OpB, OpN, OpN), "TEST" },/* OP_TEST */
{ iAB, oAB, OpR, mode(OpB, OpN, OpN), "FORLOOP" },/* OP_FORLOOP */
{ iAB, oAB, OpR, mode(OpB, OpN, OpN), "FORPREP" },/* OP_FORPREP */
{ iAB, oAB, OpR, mode(OpN, OpN, OpN), "RETURN" },/* OP_RETURN */
{ iAB, oAB, OpR, mode(OpB, OpN, OpN), "TFORCALL" },/* OP_TFORCALL */
{ iAB, oAB, OpR, mode(OpB, OpN, OpN), "TFORLOOP" },/* OP_TFORLOOP */
{ iAB, oAB, OpR, mode(OpN, OpN, OpN), "CLOSURE" },/* OP_CLOSURE */
{ iAB, oAB, OpN, mode(OpN, OpN, OpN), "BUILD_METH" },/* OP_BUILD_METH */
{ iAB, oAB, OpR, mode(OpN, OpN, OpN), "VARARG" },/* OP_VARARG */
/*iABC*/
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "LOADBOOL" },/* OP_LOADBOOL */
{ iABC, oABC, OpR, mode(OpN, OpB, OpN), "GETTABUP" },/* OP_GETTABUP */
{ iABC, oABC, OpR, mode(OpB, OpK, OpN), "GET_ATTR" },/* OP_GET_ATTR */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "SETTABUP" },/* OP_SETTABUP */
{ iABC, oABC, OpB, mode(OpK, OpB, OpN), "SET_ATTR" },/* OP_SET_ATTR */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "GETTABGL" },/* OP_GETTABGL */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "SETUP_EXCEPT" },/* OP_SETUP_EXCEPT */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "ADD" },/* OP_ADD */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "SUB" },/* OP_SUB */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "MUL" },/* OP_MUL */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "MOD" },/* OP_MOD */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "POW" },/* OP_POW */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "DIV" },/* OP_DIV */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "IDIV" },/* OP_IDIV */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "BAND" },/* OP_BAND */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "BOR" },/* OP_BOR */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "BXOR" },/* OP_BXOR */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "SHL" },/* OP_SHL */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "SHR" },/* OP_SHR */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "CONCAT" },/* OP_CONCAT */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "EQ" },/* OP_EQ */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "LT" },/* OP_LT */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "LE" },/* OP_LE */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "TESTSET" },/* OP_TESTSET */
{ iABC, oABC, OpR, mode(OpN, OpN, OpN), "CALL" },/* OP_CALL */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "CALL_KW" },/* OP_CALL_KW */
{ iABC, oABC, OpR, mode(OpB, OpB, OpN), "SETLIST" }, /* OP_SETLIST */
{ iABC, oABC, OpR, mode(OpR, OpN, OpN), "JMP_IF_FALSE" }, /* OP_JMP_IF_FALSE */
{ iABC, oABC, OpR, mode(OpR, OpN, OpN), "JMP_IF_TRUE" }, /* OP_JMP_IF_TRUE */
};

#define prk(C,c)  do{\
		switch (get##C##Mode(ip)) {\
		case OpR:\
			if (c < p->nlocvars)\
				j += sprintf(strs + j, "  var"#C":%s",cast(StrObj*,k[p->locvars[c].idx])->val);\
			break;\
		case OpK:\
			j += sprintf(strs + j, "  "#C":");\
			j += obj2str(strs + j, k[c]);\
			break;\
		case OpB:\
			if (c & BITRK) {\
				c -= BITRK;\
				j += sprintf(strs + j, "  "#C":");\
				j += obj2str(strs + j, k[c]);\
			} else {\
				if (c < p->nlocvars)\
					j += sprintf(strs + j, "  var"#C":%s", cast(StrObj*,k[p->locvars[c].idx])->val);\
			}\
		}}while(0)

int obj2str(char* s, Object *o) {
	switch (o->type->basetype) {
	case BT_STR:
		return sprintf(s, "%s", cast(StrObj*,o)->val);
	case BT_INT:
		return sprintf(s, "%lld", cast(IntObj*,o)->ival);
	case BT_FLOAT:
		return sprintf(s, "%f", cast(FloatObj*,o)->fval);
	case BT_TUPLE:
		return sprintf(s, "TP%ld", cast(TupleObj*,o)->length);
	default:
		return 0;
	}
}
void printcode(Instruction* i, int pc, Proto *p) {
	ushort b, c, d;
	int j = 0;
	char strs[200];
	ushort a = GETARG_A(i);
	Object **k = cast(ModuleObj*,p->module)->consts;
	if (a == VRELOCABLE_REG)
		return;
	OpCode ip = GET_OPCODE(i);
	if (ip > OP_MAX) {
		printf("unknow code:%d\n", ip);
		return;
	}
	switch (opcodes[ip].size) {
	case oABCD:
		d = GETARG_D(i);
	case oABC:
		c = GETARG_C(i);
	case oAB:
		b = GETARG_B(i);
		break;
	case oA:
		break;
	}
	switch (opcodes[ip].size) {
	case oA:
		j = sprintf(strs, "%-2d %-5s: A %d", pc, opcodes[ip].name, a);
		break;
	case oAB:
		j = sprintf(strs, "%-2d %-5s: A %d,B %d", pc, opcodes[ip].name, a, b);
		break;
	case oABC:
		j = sprintf(strs, "%-2d %-5s: A %d,B %d,C %d", pc, opcodes[ip].name, a, b,
				c);
		break;
	case oABCD:
		j = sprintf(strs, "%-2d %-5s: A %d,B %d,C %d,D %d", pc, opcodes[ip].name, a,
				b, c, d);
		break;
	}
//	if (opcodes[ip].A == OpB)
//		a -= 1;
	switch (ip) {
	case OP_GETTABGL:
	case OP_CALL:
		b -= GLBAIS;
		c -= GLBAIS;
		break;
	case OP_RETURN:
		b -= 1;
		break;
	default:
		break;
	}
	if (opcodes[ip].A == OpB && ISK(a)) {
		a -= BITRK;
		j += sprintf(strs + j, "  A:");
		j += obj2str(strs + j, k[a]);
	} else if ((opcodes[ip].A == OpR || opcodes[ip].A == OpB)
			&& a < p->nlocvars) {
		j += sprintf(strs + j, "  varA:%s",
		cast(StrObj*,k[p->locvars[a].idx])->val);
	} else if (opcodes[ip].A == OpK) {
		j += sprintf(strs + j, "  A:");
		j += obj2str(strs + j, k[a]);
	}
	if (opcodes[ip].size >= oAB)
		prk(B, b);
	if (opcodes[ip].size >= oABC && c >= 0)
		prk(C, c);
	if (opcodes[ip].size >= oABCD)
		prk(D, d);
	switch (opcodes[ip].size) {
	case oA:
		j += sprintf(strs + j, " | %d", a);
		break;
	case oAB:
		j += sprintf(strs + j, " | %d %d", a, b);
		break;
	case oABC:
		j += sprintf(strs + j, " | %d %d %d", a, b, c);
		break;
	case oABCD:
		j += sprintf(strs + j, " | %d %d %d %d", a, b, c, d);
		break;
	}
	strs[j] = '\0';
	printf("%s\n", strs);
	return;
}
