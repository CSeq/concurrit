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


#ifndef IPC_H_
#define IPC_H_

#include "common.h"

#include "thread.h"

#include <sys/stat.h>
#include <fcntl.h>

namespace concurrit {

/********************************************************************************/
#define PIPEDIR	"/tmp/concurrit/pipe"

struct PipeNamePair {
	std::string in_name;
	std::string out_name;
};

PipeNamePair PipeNamesForSUT(int id = 0);
PipeNamePair PipeNamesForDSL(int id = 0);

/********************************************************************************/

class ShadowThread;

class EventPipe {
public:
	EventPipe();

	EventPipe(const PipeNamePair& names);

	void Init(const PipeNamePair& names);

	void Init(const char* in_name, const char* out_name);

	virtual ~EventPipe();

	static void MkFifo(const char* name);

	virtual void Open(bool open_for_read_first);

	virtual void Close();

#define DOSend(x)	write(out_fd_, (&x), sizeof(x))
#define DORecv(x)	read(in_fd_, (&x), sizeof(x))

#define DECL_SEND_RECV(F) 			\
	void F(EventBuffer* event) {	\
		DO##F(event->type);			\
		DO##F(event->threadid);		\
		switch(event->type) {		\
		case MemRead:				\
		case MemWrite:				\
			DO##F(event->addr);		\
			DO##F(event->size);		\
			break;					\
		case FuncEnter:				\
			DO##F(event->addr);		\
			DO##F(event->arg0);		\
			DO##F(event->arg1);		\
			break;					\
		case FuncReturn:			\
			DO##F(event->addr);		\
			DO##F(event->retval);	\
			break;					\
		case FuncCall:				\
			DO##F(event->addr);		\
			DO##F(event->addr_target); \
			DO##F(event->arg0);		\
			DO##F(event->arg1);		\
			break;					\
		default:					\
			break;					\
		}							\
	}								\

	DECL_SEND_RECV(Send)

	DECL_SEND_RECV(Recv)

	virtual void Send(ShadowThread*, EventBuffer*) = 0;

	virtual void Recv(ShadowThread*, EventBuffer*) = 0;

private:
	DECL_FIELD(const char*, in_name)
	DECL_FIELD(const char*, out_name)
	DECL_FIELD(int, in_fd)
	DECL_FIELD(int, out_fd)
	DECL_FIELD(bool, is_open)
};

/********************************************************************************/

class ShadowThread {
public:
	ShadowThread(THREADID tid, EventPipe* pipe);

	virtual ~ShadowThread(){}

	void SendContinue();

	virtual void* Run() = 0;

	void Send(EventBuffer* e);

	void Recv(EventBuffer* e);

	void SendRecvContinue(EventBuffer* e);

	// used by pipe implementation
	// when thread wants to receive an event, it waits on its semaphore
	void WaitRecv(EventBuffer* event);

	// copy event from argument and signal the semaphore
	void SignalRecv(EventBuffer* event);

	static void* thread_func(void* arg);

	int SpawnAsThread();

private:
	DECL_FIELD(THREADID, tid)
	DECL_FIELD(EventPipe*, pipe)
	DECL_FIELD(EventBuffer*, event)
	DECL_FIELD_REF(Semaphore, sem)
};

/********************************************************************************/

// interface
class EventHandler {
public:
	EventHandler(){}
	virtual ~EventHandler(){}
	virtual bool OnSend(EventBuffer* event) { return true; };
	virtual bool OnRecv(EventBuffer* event) { return true; };
};

/********************************************************************************/

class ConcurrentPipe : public EventPipe {
	typedef tbb::concurrent_hash_map<THREADID, ShadowThread*> TidToShadowThreadMap;
public:

	ConcurrentPipe(const PipeNamePair& names, EventHandler* event_handler = NULL);

	~ConcurrentPipe() {}

	void Send(ShadowThread* thread, EventBuffer* event);
	void Recv(ShadowThread* thread, EventBuffer* event);

	/*****************************************************************/

	//override
	void Open(bool open_for_read_first);
	//override
	void Close();


	static void* thread_func(void* arg);

	/*****************************************************************/

	void Broadcast(EventBuffer* e);

	void SendContinue(THREADID tid);

	/*****************************************************************/

	ShadowThread* GetShadowThread(THREADID tid);

	void RegisterShadowThread(ShadowThread* shadowthread);

	void UnregisterShadowThread(ShadowThread* shadowthread);

private:
	DECL_FIELD_REF(Mutex, send_mutex)
	DECL_FIELD_REF(TidToShadowThreadMap, tid_to_shadowthread)
	DECL_FIELD(Thread*, worker_thread)
	DECL_FIELD(EventHandler*, event_handler)
	DECL_FIELD(Mutex, map_mutex)
};

/********************************************************************************/


} // end namespace

#endif /* IPC_H_ */
