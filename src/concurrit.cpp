/**
 * Copyright (c) 2010-2011,
 * Tayfun Elmas    <elmas@cs.berkeley.edu>
 * All rights reserved.
 * <p/>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * <p/>
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * <p/>
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * <p/>
 * 3. The names of the contributors may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 * <p/>
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "concurrit.h"

namespace concurrit {

/********************************************************************************/

const char* CONCURRIT_HOME = NULL;

volatile bool Concurrit::initialized_ = false;
main_args Concurrit::driver_args_;
MainFuncType Concurrit::driver_main_;

void Concurrit::Init(int argc /*= -1*/, char **argv /*= NULL*/) {
	safe_assert(!IsInitialized());

	// init work dir
	CONCURRIT_HOME = getenv("CONCURRIT_HOME");

	// init logging
	google::InitGoogleLogging("concurrit");


	//==========================================

	SetupSignalHandler();

	//==========================================
	// separate arguments into two: 1 -- 2

	fprintf(stderr, "Test arguments: %s\n", main_args(argc, argv).ToString().c_str());

	safe_assert(argc > 0);
	std::vector<char*> args;
	int i = 0;
	// 1
	for(; i < argc; ++i) {
		if(strncmp(safe_notnull(argv[i]), "--", 2) == 0) break;
		args.push_back(argv[i]);
	}

	main_args m = ArgVectorToMainArgs(args);
	fprintf(stderr, "Concurrit arguments: %s\n", m.ToString().c_str());

	Config::ParseCommandLine(m.argc_, m.argv_);
	if(Config::OnlyShowHelp) {
		safe_exit(EXIT_SUCCESS);
	}

	// 2
	if(strncmp(argv[i], "--", 2) == 0) {
		++i;
	}
	args.clear();
	args.push_back("<bench-name>");
	for(; i < argc; ++i) {
		args.push_back(argv[i]);
	}
	Concurrit::driver_args_ = ArgVectorToMainArgs(args);
	fprintf(stderr, "Driver arguments: %s\n", Concurrit::driver_args_ .ToString().c_str());

	//==========================================

	LoadTestLibrary();

	//==========================================

// init pth
//	int pth_init_result = pth_init();
//	safe_assert(pth_init_result == TRUE);

	AuxState::Init();

	Originals::initialize();

	Thread::init_tls_key();

	CoroutineGroup::init_main();

	PinMonitor::Init();

	do { // need a fence here
		initialized_ = true;
	} while(false);

	VLOG(2) << "Initialized Concurrit.";
}

void Concurrit::Destroy() {
	safe_assert(IsInitialized());

	CoroutineGroup::delete_main();

	Thread::delete_tls_key();

//	int pth_kill_result = pth_kill();
//	safe_assert(pth_kill_result == TRUE);

	do { // need a fence here
		initialized_ = false;
	} while(false);

	VLOG(2) << "Finalized Concurrit.";
}

/********************************************************************************/

volatile bool Concurrit::IsInitialized() {
	return initialized_;
}

/********************************************************************************/

void Concurrit::LoadTestLibrary() {
	void* handle = NULL;
	if(Config::TestLibraryFile != NULL) {
		handle = dlopen(Config::TestLibraryFile, RTLD_LAZY | RTLD_GLOBAL);
		if(handle == NULL) {
			safe_fail("Cannot load the test library %s!\n", Config::TestLibraryFile);
		}
		// find the driver main function, try next first, and fail if not found
		Concurrit::driver_main_ = (MainFuncType) FuncAddressByName("__main__", handle, true);
		safe_assert(Concurrit::driver_main_ != concurrit::__main__);
		VLOG(1) << "Loaded the test library " << Config::TestLibraryFile;
	} else {
		// find the driver main function, try next first, and fail if not found
		Concurrit::driver_main_ = (MainFuncType) FuncAddressByName("__main__", false, true, true);
		safe_assert(Concurrit::driver_main_ == concurrit::__main__);
		VLOG(1) << "Not loading a test library";
	}
}

/********************************************************************************/

//============================================

// code to add to benchmarks
/*

#ifdef __cplusplus
extern "C" {
#endif
int __main__(int argc, char* argv[]) {
	return main(argc, argv);
}
#ifdef __cplusplus
} // extern "C"
#endif

*/
//============================================

// default, empty implementation of test driver
int __main__ (int, char**) {
	VLOG(2) << "Running default test-driver main function.";
	safe_fail("Default __main__ implementation should not be called!");
}

void* Concurrit::CallDriverMain(void*) {
	MainFuncType main_func = Concurrit::driver_main();
	safe_assert(main_func != NULL);
	if(main_func == concurrit::__main__) {
		safe_fail("Default __main__ implementation should not be called!");
	}

	// call the driver
	safe_assert(driver_args_.check());
	main_func(driver_args_.argc_, driver_args_.argv_);

}

/********************************************************************************/

void Concurrit::SetupSignalHandler() {
	// first set the signal handler
	struct sigaction sigact;

	sigact.sa_sigaction = Concurrit::SignalHandler;
	sigact.sa_flags = 0;
	sigemptyset (&sigact.sa_mask);

	if (sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL) != 0) {
		safe_fail("error setting signal handler for %d (%s)\n",
				SIGSEGV, strsignal(SIGSEGV));
	}

//	if (sigaction(SIGINT, &sigact, (struct sigaction *)NULL) != 0) {
//		fprintf(stderr, "error setting signal handler for %d (%s)\n",
//				SIGINT, strsignal(SIGINT));
//		fflush(stderr);
//		_Exit(UNRECOVERABLE_ERROR);
//	}
}

void Concurrit::SignalHandler(int sig_num, siginfo_t * info, void * ucontext) {
	fprintf(stderr, "Got signal %d: %s!\n", sig_num, strsignal(sig_num));
	switch(sig_num) {
	case SIGSEGV:
		safe_fail("Segmentation fault!");
		break;
//	case SIGINT:
//		Scenario* scenario = Scenario::Current();
//		if(scenario == NULL){
//			fprintf(stderr, "Scenario is null when signal is handled!");
//			fflush(stderr);
//			raise(sig_num);
//		}
//		// send backtrack exception
//		TestStatus status = scenario->test_status();
//		if(BETWEEN(TEST_SETUP, status, TEST_TEARDOWN)) {
//			scenario->exec_tree()->EndWithBacktrack(Coroutine::Current(), SEARCH_ENDS, "SIGINT");
//			return;
//		} else {
//			fprintf(stderr, "Signal sent when not running the test, calling the default handler!");
//			fflush(stderr);
//			raise(sig_num);
//		}
//		break;
	}
	unreachable();
}

} // end namespace
