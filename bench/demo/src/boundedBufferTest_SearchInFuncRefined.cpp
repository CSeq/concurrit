#include <stdio.h>

#include "concurrit.h"

CONCURRIT_BEGIN_MAIN()

//============================================================//
//============================================================//

CONCURRIT_BEGIN_TEST(BBScenario, "Bounded buffer scenario")

	TESTCASE() {
		MAX_WAIT_TIME(0);

		FUNCT(bounded_buf_get);
		FUNCT(bounded_buf_put);

		TVAR(P1);
		TVAR(C1);
		TVAR(C2);

		WAIT_FOR_THREAD(P1, ENTERS(bounded_buf_put), "Wait for a producer.");

		WAIT_FOR_DISTINCT_THREADS((C1, C2), ENTERS(bounded_buf_get), "Wait for 2 consumers.");

		MAX_WAIT_TIME(3*USECSPERSEC);

		WHILE (!ALL_ENDED(P1, C1, C2)) {

			TVAR(t);

			CHOOSE_THREAD_BACKTRACK(t, (P1, C1, C2), PTRUE, "Select a thread to execute.");

			RUN_THREAD_THROUGH(t, READS() || WRITES() || CALLS() || HITS_PC() || ENDS(), "Run t until any event.");
		}
	}

CONCURRIT_END_TEST(BBScenario)

//============================================================//
//============================================================//


CONCURRIT_END_MAIN()