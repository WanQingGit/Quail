/*
 *  Created on: Jul 24, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#ifndef INCLUDE_INSTRUCTION_H_
#define INCLUDE_INSTRUCTION_H_
#include "qbase.h"
#include "lrparser.h"
typedef enum {
	/*----------------------------------------------------------------------
	 name							args			description
	 ------------------------------------------------------------------------*/
	OP_NIL, OP_MOVE, /*	A B	R(A) := R(B)					*/
	OP_CONST, /*	A Bx	R(A) := Kst(Bx)					*/
	OP_GLOBAL, /*	A B C	R(A)=G[K(B)] */
	OP_SETGLOBAL, /*	A B  G[K(A)]= RK(B) */
	OP_LOADNIL, /*	A B	R(A), R(A+1), ..., R(A+B) := nil		*/
	OP_UPVAL, /*	A B	R(A) := UpValue[B]				*/
	OP_SETUPVAL, /*	A B	UpValue[B] := R(A)				*/
	OP_NEWTABLE, /*	A B	R(A) := {} (size = B)				*/
	OP_NEWLIST, /*	A B	R(A) := [] (size = B)				*/
	OP_NEWCLASS, /*	A B class name=k(A),*/
	OP_UNM, /*	A B	R(A) := -R(B)					*/
	OP_BNOT, /*	A B	R(A) := ~R(B)					*/
	OP_NOT, /*	A B	R(A) := not R(B)				*/
	OP_LEN, /*	A B	R(A) := length of R(B)				*/
	OP_JMP, /*	A sBx	pc+=sBx; if (A) close all upvalues >= R(A - 1)	*/
	OP_TEST, /*	A C	if not (R(A) <=> C) then pc++			*/
	OP_FORLOOP, /*	A sBx	R(A)+=R(A+2); if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A) }*/
	OP_FORPREP, /*	A sBx	R(A)-=R(A+2); pc+=sBx				*/
	OP_RETURN, /*	A B	return R(A), ... ,R(A+B-2)	(see note)	*/
	OP_TFORCALL, /*	A C	R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2));	*/
	OP_TFORLOOP, /*	A sBx	if R(A+1) ~= nil then { R(A)=R(A+1); pc += sBx }*/
	OP_CLOSURE, /*	A Bx	R(A) := closure(KPROTO[Bx])			*/
	OP_BUILD_METH, /*	A Bx	R(A) := closure(KPROTO[Bx])			*/
	OP_VARARG, /*	A B	R(A), R(A+1), ..., R(A+B-2) = vararg		*/
	OP_LOADBOOL, /*	A B C	R(A) := (Bool)B; if (C) pc++			*/
	OP_GETTABUP, /*	A B C	R(A) := UpValue[B][RK(C)]			*/
	OP_GET_ATTR, /*	A B C	R(A) := RK(B)[K(C)]				*/
	OP_SETTABUP, /*	A B C	UpValue[A][RK(B)] := RK(C)			*/
	OP_SET_ATTR, /*	A B C	RK(A)[RK(B)] := RK(C)	A=K is global			*/
	OP_GETTABGL, /*	A B C	R(A)=G[RK(B-GLBAIS)][RK(C-GLBAIS)] */
	OP_SETUP_EXCEPT, /*	A B C	R(A+1) := R(B); R(A) := R(B)[RK(C)]		*/
	OP_ADD, /*	A B C	R(A) := RK(B) + RK(C)				*/
	OP_SUB, /*	A B C	R(A) := RK(B) - RK(C)				*/
	OP_MUL, /*	A B C	R(A) := RK(B) * RK(C)				*/
	OP_MOD, /*	A B C	R(A) := RK(B) % RK(C)				*/
	OP_POW, /*	A B C	R(A) := RK(B) ^ RK(C)				*/
	OP_DIV, /*	A B C	R(A) := RK(B) / RK(C)				*/
	OP_IDIV, /*	A B C	R(A) := RK(B) // RK(C)				*/
	OP_BAND, /*	A B C	R(A) := RK(B) & RK(C)				*/
	OP_BOR, /*	A B C	R(A) := RK(B) | RK(C)				*/
	OP_BXOR, /*	A B C	R(A) := RK(B) ~ RK(C)				*/
	OP_SHL, /*	A B C	R(A) := RK(B) << RK(C)				*/
	OP_SHR, /*	A B C	R(A) := RK(B) >> RK(C)				*/
	OP_CONCAT, /*	A B C	R(A) := R(B).. ... ..R(C)			*/
	OP_EQ, /*	A B C	if ((RK(B) == RK(C)) ~= A) then pc++		*/
	OP_LT, /*	A B C	if ((RK(B) <  RK(C)) ~= A) then pc++		*/
	OP_LE, /*	A B C	if ((RK(B) <= RK(C)) ~= A) then pc++		*/
	OP_TESTSET, /*	A B C	if (R(B) <=> C) then R(A) := R(B) else pc++	*/
	OP_CALL, /*	A B C	R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1)) */
	OP_CALL_KW, /*	A B C	return R(A)(R(A+1), ... ,R(A+B-1))		*/
	OP_SETLIST, /*	A B C	R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B	*/
	OP_JMP_IF_FALSE,/* R(C)=R(B) if R(B)==false,PC+=A */
	OP_JMP_IF_TRUE, /* R(C)=R(B) if R(B)==true, PC+=A */
	OP_MAX,
} OpCode;

typedef enum OpMode {
	iABC, iAB, iAsB, iA
} OpMode;
typedef enum OpSize {
	oA = 3, oAB = 5, oABC = 7, oABCD = 9
} OpSize;
typedef enum OpArgMask {
	OpB, /* argument is a constant or register */
	OpR, /* argument is a register  */
	OpK, /* argument is a constant */
	OpN /*a jump offset or other*/
} OpArgMask;
struct opcode {
	OpMode mode :4;
	OpSize size :4;
	OpArgMask A :2;
	unsigned BCD :6;
	char name[24];
} opcode;

#define NO_JUMP -1
#define GLBAIS 1
#define SIZE_A		16
#define SIZE_B		16
#define SIZE_Bx		16
#define SIZE_C		16
#define SIZE_D		16
#define MAXARG_A        ((1<<SIZE_A)-1)
#define MAXARG_B        ((1<<SIZE_B)-1)
#define MAXARG_C        ((1<<SIZE_C)-1)
#define MAXARG_D        ((1<<SIZE_D)-1)
#define VRELOCABLE_REG MAXARG_A
#define MAXREGS MAXARG_A
#define BITRK		(1 << (SIZE_B - 1))
#define GET_OPCODE(i)	cast(Instruction*,i)[0]
#define GETARG(i) cast(short*,(cast(Instruction*,i)+1))
#define GETARG_A(i)	GETARG(i)[0]
#define GETARG_B(i)	GETARG(i)[1]
#define GETARG_C(i)	GETARG(i)[2]
#define GETARG_D(i)	GETARG(i)[3]
#define MAXINDEXRK	(BITRK - 1)
#define INDEXK(r)	((int)(r) & ~BITRK)
#define ISK(x)		((x) & BITRK)
#define RKMASK(x)	((x) | BITRK)
#define RKID(rk) rk&cast(ushort,~BITRK)
#define getinstr(p,i) (p->code+p->lineinfo[i].pc)
#define getinstruction(fs,e)	getinstr(fs->f,e->u.info)

#define getBMode(m)	((opcodes[m].BCD >> 4) & 3)
#define getCMode(m)	((opcodes[m].BCD >> 2) & 3)
#define getDMode(m)	((opcodes[m].BCD >> 0) & 3)

extern struct opcode opcodes[];
void printcode(Instruction* i, int pc, Proto *p);
#endif /* INCLUDE_INSTRUCTION_H_ */
