/*
 *  Created on: Jul 25, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */
#include "floatobj.h"
#include "number.h"
#include "mem_alloc.h"
#include <math.h>
Int float_hash(double v);
Int float_compare(FloatObj *a, FloatObj *b);
void float_destroy(ThreadState *ts, FloatObj *fltobj);
//@formatter:off
TypeObj Type_Float = {
		{ &Type_Type, 0 },
		0,
		"float",
		sizeof(FloatObj),
		0,
		(hashfn) float_hash,
		(comparefn) float_compare,
		(destructor)float_destroy,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) NULL,
		(initfn) NULL,
		NULL,
		BT_FLOAT,
		NULL
};
//@formatter:on
#define HASH_INF 314159
#if __WORDSIZE == 64
#  define HASH_BITS 61
#else
#  define HASH_BITS 31
#endif
#define HASH_MODULUS (((size_t)1 << HASH_BITS) - 1)
FloatObj * float_new(ThreadState *ts, double v) {
	FloatObj *o;
	if (nfree_num > 0) {
		o = (FloatObj *) free_num[--nfree_num];
		o->_base.type = &Type_Float;
		o->_base.nref = 1;
	} else {
		o = (FloatObj*) object_new(ts, &Type_Float);
	}
	o->fval = v;
	return o;
}
Int float_hash(double v) {
	int e, sign;
	double m;
	size_t x, y;

	if (!isfinite(v)) {
		if (isinf(v))
			return v > 0 ? HASH_INF : -HASH_INF;
		else
			return 0;
	}
	m = frexp(v, &e);

	sign = 1;
	if (m < 0) {
		sign = -1;
		m = -m;
	}

	/* process 28 bits at a time;  this should work well both for binary
	 and hexadecimal floating point. */
	x = 0;
	while (m) {
		x = ((x << 28) & HASH_MODULUS) | x >> (HASH_BITS - 28);
		m *= 268435456.0; /* 2**28 */
		e -= 28;
		y = (size_t) m; /* pull out integer part */
		m -= y;
		x += y;
		if (x >= HASH_MODULUS)
			x -= HASH_MODULUS;
	}

	/* adjust for the exponent;  first reduce it modulo HASH_BITS */
	e = e >= 0 ? e % HASH_BITS : HASH_BITS - 1 - ((-1 - e) % HASH_BITS);
	x = ((x << e) & HASH_MODULUS) | x >> (HASH_BITS - e);
	x = x * sign;
	if (x == (size_t) -1)
		x = (size_t) -2;
	return (Int) x;
}
Int float_compare(FloatObj *a, FloatObj *b) {
	double res = a->fval - b->fval;
	if (res > 0)
		return 1;
	else if (res < 0)
		return -1;
	return 0;
}
void float_destroy(ThreadState *ts, FloatObj *fltobj) {
	if (nfree_num < MAX_NUM) {
		free_num[nfree_num++] = (Object*) fltobj;
	} else {
		mem_alloc(ts, fltobj, sizeof(FloatObj), 0);
	}
}
