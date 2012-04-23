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

Coroutine* PinMonitor::tid_to_coroutine_[MAX_THREADS];
volatile bool PinMonitor::enabled_ = false;

/********************************************************************************/


void PinMonitor::Init() {
	// clean tid_to_coroutine
	for(int i = 0; i < MAX_THREADS; ++i) {
		tid_to_coroutine_[i] = NULL;
	}
}

Coroutine* PinMonitor::GetCoroutineByTid(THREADID tid) {
	safe_assert(0 <= tid && tid < MAX_THREADS);
	Coroutine* co = tid_to_coroutine_[tid];
	if(co == NULL) {
		co = Coroutine::Current();
		tid_to_coroutine_[tid] = safe_notnull(co);
	}
	return co;
}


MemoryCellBase* PinMonitor::GetMemoryCell(void* addr, uint32_t size) {
	switch(size) {
	case 1:
		return new MemoryCell<uint8_t>(static_cast<uint8_t*>(addr));
	case 2:
		return new MemoryCell<uint16_t>(static_cast<uint16_t*>(addr));
	case 4:
		return new MemoryCell<uint32_t>(static_cast<uint32_t*>(addr));
	case 8:
		return new MemoryCell<uint64_t>(static_cast<uint64_t*>(addr));
	default:
		safe_assert(false);
		break;
	}
	return NULL;
}

SharedAccess* PinMonitor::GetSharedAccess(AccessType type, MemoryCellBase* cell) {
	return new SharedAccess(type, cell);
}

/******************************************************************************************/

void PinMonitor::Enable() {
	if(!Config::RunUncontrolled) {
		VLOG(2) << ">>> Enabling Pin instrumentation.";
		EnablePinTool();
		enabled_ = true;
	}
}
void PinMonitor::Disable() {
	if(!Config::RunUncontrolled) {
		VLOG(2) << ">>> Disabling Pin instrumentation.";
		DisablePinTool();
		enabled_ = false;
	}
}

/******************************************************************************************/

// callbacks

inline void PinMonitor::MemAccessBefore(Coroutine* current, Scenario* scenario, SourceLocation* loc /*= NULL*/) {
	safe_assert(current != NULL && scenario != NULL);
	safe_assert(loc != NULL);

	scenario->BeforeControlledTransition(current);
}

inline void PinMonitor::MemAccessAfter(Coroutine* current, Scenario* scenario, SourceLocation* loc /*= NULL*/) {
	safe_assert(current != NULL && scenario != NULL);
	safe_assert(loc != NULL);

	scenario->AfterControlledTransition(current);
}

/********************************************************************************/

inline void PinMonitor::MemWrite(Coroutine* current, Scenario* scenario, void* addr, uint32_t size, SourceLocation* loc /*= NULL*/) {
	safe_assert(current != NULL && scenario != NULL);
	safe_assert(loc != NULL);

	// update auxstate
	AuxState::Writes->set(PTR2ADDRINT(addr), size, current->tid());
}

inline void PinMonitor::MemRead(Coroutine* current, Scenario* scenario, void* addr, uint32_t size, SourceLocation* loc /*= NULL*/) {
	safe_assert(current != NULL && scenario != NULL);
	safe_assert(loc != NULL);

	// update auxstate
	AuxState::Reads->set(PTR2ADDRINT(addr), size, current->tid());
}

/********************************************************************************/

inline void PinMonitor::FuncCall(Coroutine* current, Scenario* scenario, void* addr_src, void* addr_target, bool direct, SourceLocation* loc_src, SourceLocation* loc_target, ADDRINT arg0, ADDRINT arg1) {
	safe_assert(loc_src != NULL && loc_target != NULL);

	// update auxstate
	AuxState::CallsFrom->set(PTR2ADDRINT(addr_src), current->tid());
	AuxState::CallsTo->set(PTR2ADDRINT(addr_target), current->tid());

	scenario->OnControlledTransition(current);
}

inline void PinMonitor::FuncEnter(Coroutine* current, Scenario* scenario, void* addr, SourceLocation* loc, ADDRINT arg0, ADDRINT arg1) {
	safe_assert(current != NULL && scenario != NULL);
	safe_assert(loc != NULL);

	// update auxstate
	AuxState::Enters->set(PTR2ADDRINT(addr), true, current->tid());

	int c = AuxState::InFunc->get(PTR2ADDRINT(addr), current->tid());
	safe_assert(c >= 0);
	AuxState::InFunc->set(PTR2ADDRINT(addr), c+1, current->tid());

	c = AuxState::NumInFunc->get(PTR2ADDRINT(addr), current->tid());
	safe_assert(c >= 0);
	AuxState::NumInFunc->set(PTR2ADDRINT(addr), c+1, current->tid());

	scenario->OnControlledTransition(current);
}

inline void PinMonitor::FuncReturn(Coroutine* current, Scenario* scenario, void* addr, SourceLocation* loc, ADDRINT retval) {
	safe_assert(current != NULL && scenario != NULL);
	safe_assert(loc != NULL);

	// update auxstate
	AuxState::Returns->set(PTR2ADDRINT(addr), true, current->tid());

	scenario->BeforeControlledTransition(current);

	int c = AuxState::InFunc->get(PTR2ADDRINT(addr), current->tid());
	safe_assert(c >= 0);
	if(c == 0) return; // TODO(elmas): in general c must be > 1. Sometimes (radbench) it is 0!
	AuxState::InFunc->set(PTR2ADDRINT(addr), c-1, current->tid());

	scenario->AfterControlledTransition(current);
}

/********************************************************************************/

inline void PinMonitor::ThreadEnd(Coroutine* current, Scenario* scenario) {
	safe_assert(current != NULL && scenario != NULL);

	// last controlled transition
	// set predicate for "will end" before this transition
	safe_assert(current->status() == ENABLED);
	THREADID tid = current->tid();

	safe_assert(!AuxState::Ends->isset(tid));
	AuxState::Ends->set(true, tid);
	safe_assert(AuxState::Ends->isset(tid));

	scenario->OnControlledTransition(current);

	safe_assert(AuxState::Ends->isset(tid));
}

/********************************************************************************/

void CallPinMonitor(PinMonitorCallInfo* info) {
	safe_assert(info != NULL);

	if(!PinMonitor::IsEnabled()) {
		return;
	}

	VLOG(2) << "Calling pinmonitor method " << info->type << " threadid " << info->threadid << " addr " << info->addr;

	Coroutine* current = safe_notnull(PinMonitor::GetCoroutineByTid(info->threadid));
	safe_assert(!current->IsMain());

	CoroutineGroup* group = safe_notnull(current->group());
	Scenario* scenario = safe_notnull(group->scenario());

	switch(info->type) {
		case MemAccessBefore:
			PinMonitor::MemAccessBefore(current, scenario, info->loc_src);
			break;
		case MemAccessAfter:
			PinMonitor::MemAccessAfter(current, scenario, info->loc_src);
			break;
		case MemWrite:
			PinMonitor::MemWrite(current, scenario, info->addr, info->size, info->loc_src);
			break;
		case MemRead:
			PinMonitor::MemRead(current, scenario, info->addr, info->size, info->loc_src);
			break;
		case FuncEnter:
			PinMonitor::FuncEnter(current, scenario, info->addr, info->loc_src, info->arg0, info->arg1);
			break;
		case FuncReturn:
			PinMonitor::FuncReturn(current, scenario, info->addr, info->loc_src, info->retval);
			break;
		case FuncCall:
			PinMonitor::FuncCall(current, scenario, info->addr, info->addr_target, info->direct, info->loc_src, info->loc_target, info->arg0, info->arg1);
			break;
		default:
			printf("Call type: %d\n", info->type);
			bool UnrecognizedPinMonitorCallType = false;
			safe_assert(UnrecognizedPinMonitorCallType);
			break;
	}
}

/********************************************************************************/

void EnablePinTool() { VLOG(2) << "Enabling pin instrumentation"; }
void DisablePinTool() { VLOG(2) << "Disabling pin instrumentation"; }

void ThreadRestart() { VLOG(2) << "Restarting thread."; }

/********************************************************************************/

void StartInstrument() { VLOG(2) << "Starting instrumentation."; }
void EndInstrument() {
	VLOG(2) << "Ending instrumentation.";
	// reset aux variables of current thread
	Coroutine* co = safe_notnull(Coroutine::Current());
	co->FinishControlledTransition();
}

void AtPc(int pc) {
	if(!PinMonitor::IsEnabled()) {
		return;
	}

	VLOG(2) << "Thread at pc " << pc;
	// controlled transition
	Coroutine* co = safe_notnull(Coroutine::Current());
	AuxState::Pc->set(pc, co->tid());
	safe_notnull(safe_notnull(co->group())->scenario())->OnControlledTransition(co);
}

void concurritFuncEnter(void* addr) {
	if(!PinMonitor::IsEnabled()) {
		return;
	}
	PinMonitor::FuncEnter(Coroutine::Current(), Scenario::Current(), addr, RECORD_SRCLOC(), 0, 0);
}

void concurritFuncReturn(void* addr) {
	if(!PinMonitor::IsEnabled()) {
		return;
	}
	PinMonitor::FuncReturn(Coroutine::Current(), Scenario::Current(), addr, RECORD_SRCLOC(), 0);
}


void TriggerAssert(const char* expr, const char* filename, const char* funcname, int line) {
	VLOG(1) << "Triggering assertion violation!";
	TRIGGER_ASSERTION_VIOLATION(expr, filename, funcname, line);
}

void concurritThreadEnd() {
	if(!PinMonitor::IsEnabled()) {
		return;
	}
	PinMonitor::ThreadEnd(Coroutine::Current(), Scenario::Current());
}

/********************************************************************************/


} // end namespace


