/*
 * qstrbuffer.h
 *
 *  Created on: Apr 11, 2019
 *      Author: WanQing
 */

#ifndef INCLUDE_STROBJ_H_
#define INCLUDE_STROBJ_H_
#include "typeobj.h"
#define str_get(ts,s)\
	str_new(ts,s,strlen(s))
typedef struct _strobj {
	OBJ_HEAD
	Int length;
	Int hash;
	int type;
	char val[];
} StrObj;
typedef struct _unicodestr {
	StrObj _str;
	Int len_utf;
	uint16_t data[];
} UnicodeStr;
StrObj *str_new(ThreadState *ts, const char *str, size_t l);
int str_meta_init(ThreadState *ts);
extern StrObj *string_init;
extern StrObj *string_new;
#endif /* INCLUDE_STROBJ_H_ */
