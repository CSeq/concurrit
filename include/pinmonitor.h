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


#ifndef PINMONITOR_H_
#define PINMONITOR_H_

#include "common.h"
#include "sharedaccess.h"

#define MAX_THREADS 2048

namespace concurrit {

class Coroutine;

/// class to handle pin events
class PinMonitor {
private:
	static PinMonitor* instance_;
	PinMonitor(){
		Reset();
	}

	void Reset();

	Coroutine* GetCoroutineByTid(THREADID tid);

public:
	~PinMonitor(){}

	static PinMonitor* GetInstance();

	MemoryCellBase* GetMemoryCell(void* addr, uint32_t size);
	SharedAccess* GetSharedAccess(AccessType type, MemoryCellBase* cell);

	/******************************************************************************************/

	// callbacks
	void MemWriteBefore(THREADID tid, void* addr, uint32_t size, SourceLocation* loc = NULL);
	void MemWriteAfter(THREADID tid, void* addr, uint32_t size, SourceLocation* loc = NULL);
	void MemReadBefore(THREADID tid, void* addr, uint32_t size, SourceLocation* loc = NULL);
	void MemReadAfter(THREADID tid, void* addr, uint32_t size, SourceLocation* loc = NULL);
	void FuncCall(THREADID threadid, void* addr, bool direct, SourceLocation* loc_src, SourceLocation* loc_target);
	void FuncEnter(THREADID threadid, void* addr, SourceLocation* loc);
	void FuncReturn(THREADID threadid, void* addr, SourceLocation* loc, ADDRINT retval);

private:
	Coroutine* tid_to_coroutine_[MAX_THREADS];
};

typedef uint32_t PinMonitorCallType;
const PinMonitorCallType
	MemWriteBefore = 1,
	MemWriteAfter = 2,
	MemReadBefore = 3,
	MemReadAfter = 4,
	FuncCall = 5,
	FuncEnter = 6,
	FuncReturn = 7;

struct PinMonitorCallInfo {
	PinMonitorCallType type;
	THREADID threadid;
	void* addr;
	uint32_t size;
	bool direct;
	SourceLocation* loc_src;
	SourceLocation* loc_target;
	ADDRINT retval;
};

extern "C" void CallPinMonitor(PinMonitorCallInfo* call_info);

} // end namespace



#endif /* PINMONITOR_H_ */