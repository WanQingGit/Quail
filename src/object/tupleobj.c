/*
 *  Created on: Jul 29, 2019
 *  Author: WanQing
 *  E-mail: 1109162935@qq.com
 */

#include "tupleobj.h"
#include "qhash.h"
Int tuple_hash(TupleObj *t);
//@formatter:off
TypeObj Type_Tuple = {
		{ &Type_Type, 0 },
		0,
		"tuple",
		sizeof(TupleObj),
		sizeof(Object*),
		(hashfn)tuple_hash,
		(comparefn)NULL,
		(destructor)NULL,
		(descrgetfn) NULL,
		(descrsetfn) NULL,
		(callfn) NULL,
		(newfn) NULL,
		(initfn) NULL,
		NULL,
		BT_TUPLE,
};
//@formatter:on
Object *tupleNil;
Object *tuple_new(ThreadState *ts, int n) {
	if (n >= 0) {
		TupleObj *tuple = (TupleObj*) object_newVar(ts, &Type_Tuple, n);
		return (Object*) tuple;
	}
	return tupleNil;
}

/*
 * Python-3.7.3/Objects/tupleobject.c
 * The addend 82520, was selected from the range(0, 1000000) for
 * generating the greatest number of prime multipliers for tuples
 * up to length eight:
 *
 *   1082527, 1165049, 1082531, 1165057, 1247581, 1330103, 1082533,
 *   1330111, 1412633, 1165069, 1247599, 1495177, 1577699
 *
 * Tests have shown that it's not worth to cache the hash value, see
 * issue #9685.
 */
Int tuple_hash(TupleObj *t) {
	Int len = t->length;
	Int res = 0x345678UL, hash;
	UInt mult = HASH_MULTIPLIER;
	register Object **o = t->item;
	while (--len >= 0) {
		hash = (*o)->type->hash(*o);
		if (hash == -1)
			return -1;
		res = (res ^ hash) * mult;
		mult += (Int) (82520UL + len + len);
		o++;
	}
	res += 97531UL; //10111110011111011
	if (res == -1)
		return -2;
	return res;
}
