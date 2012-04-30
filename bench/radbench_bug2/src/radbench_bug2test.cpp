#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(MyScenario, "My scenario")

	TESTCASE() {

	FUNC(f_newcontext, js_NewContext);
	FUNC(f_setcontextthread, js_SetContextThread);
	FUNC(f_clearcontextthread, js_ClearContextThread);
	FUNC(f_beginrequest, JS_BeginRequest);
	FUNC(f_gc, js_GC);
	FUNC(f_endrequest, JS_EndRequest);

		MAX_WAIT_TIME(0);

#define READS_WRITES_OR_ENDS(t)		(READS(ANY_ADDR, t) || WRITES(ANY_ADDR, t) || ENDS(t) || ENTERS(ANY_FUNC, t) || RETURNS(ANY_FUNC, t))

//		TVAR(t);
//		RUN_UNTIL(!IN_FUNC(f_beginrequest), ENTERS(f_setcontextthread), t, "Select t");
//
//		TVAR(t_main);
//		RUN_UNTIL(NOT_BY(t), ENTERS(f_beginrequest), t_main, "Select t_main");
//
//		MAX_WAIT_TIME(10*USECSPERSEC);
//
//		WHILE_STAR {
//			WHILE_STAR {
//				RUN_UNTIL(BY(t), READS_WRITES_OR_ENDS(t), __, "Run t until ...");
//			}
//
//			RUN_UNTIL(BY(t_main), READS_WRITES_OR_ENDS(t_main), __, "Run t_main until ...");
//		}

		WHILE_STAR {
			FORALL(t, PTRUE);
			RUN_UNTIL(BY(t), READS_WRITES_OR_ENDS(t), __);
		}
}

CONCURRIT_END_TEST(MyScenario)

//============================================================//
//============================================================//

CONCURRIT_END_MAIN()
