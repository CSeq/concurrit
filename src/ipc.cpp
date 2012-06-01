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

void ShadowThread::WaitForEventAndSkip(EventKind type) {
	int timer = 0;
	for(pipe_.Recv(&event_); event_.type != type; pipe_.Recv(&event_)) {
		if(timer > 1000) {
			safe_fail("Too many iterations for waiting event kind %d!", type);
		}
		++timer;
	}
	SendContinue();
}

/********************************************************************************/

ShadowThread::ShadowThread(THREADID threadid, bool is_server) : tid_(threadid), thread_(NULL) {
		memset(&event_, 0, sizeof(EventBuffer));

		PipeNamePair pipe_names = is_server ? PipeNamesForDSL(tid_) : PipeNamesForSUT(tid_);
		pipe_.Init(pipe_names);
		pipe_.Open();
	}

/********************************************************************************/

ShadowThread::~ShadowThread(){
	if(pipe_.is_open()) {
		pipe_.Close();
	}
}

/********************************************************************************/

void ShadowThread::SendContinue() {
	event_.type = Continue;
	event_.threadid = tid_;
	pipe_.Send(&event_);
}

/********************************************************************************/

} // end namespace