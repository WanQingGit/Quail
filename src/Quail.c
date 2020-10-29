/*
 *  Created on: Jul 19, 2019
 *  Author: WanQing<1109162935@qq.com>
 */
#include "qprop.h"
#include "qcontrol.h"
#include "qstate.h"
#include "lrparser.h"
#include "baselib.h"
#include "QuailVM.h"
int main() {
	CTRL.init();
	qprop *prop = Prop.read("./res/Quail.conf", "=");
	qstr *s = Prop.get(prop, STR.get("FILE_CODE"), NULL);
	GlobalState *gs = GlobalState_New();
	ThreadState *ts = ThreadState_New(gs);
	GlobalState_Init(ts);
	bltinTypeInit();
	LRAnalyzer *analyzer = loadRules("../parser/res/rules.dat");
	ModuleObj *mainModule = parserCode(analyzer, ts, s->val);
	CTRL.destroy();
	vm_call_module(ts, (Object*) mainModule);
}
