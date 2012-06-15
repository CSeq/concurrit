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
// #include <fullduplex.h>

namespace concurrit {

/********************************************************************************/
#define PIPEDIR	"/tmp/concurrit/pipe"

struct PipeNamePair {
	std::string in_name;
	std::string out_name;
};

inline PipeNamePair PipeNamesForSUT(int id = 0) {
	safe_assert(id >= 0);

	PipeNamePair pair;
	char buff[256];

	snprintf(buff, 256, PIPEDIR "/in_%d", id);
	pair.in_name = std::string(buff);

	snprintf(buff, 256, PIPEDIR "/out_%d", id);
	pair.out_name = std::string(buff);

	return pair;
}

inline PipeNamePair PipeNamesForDSL(int id = 0) {
	PipeNamePair pair = PipeNamesForSUT(id);
	return {pair.out_name, pair.in_name};
}

/********************************************************************************/

class ShadowThread;

class EventPipe {
public:
	EventPipe() {
		Init(NULL, NULL);
	}

	EventPipe(const PipeNamePair& names) {
		Init(names);
	}

	void Init(const PipeNamePair& names) {
		Init(names.in_name.c_str(), names.out_name.c_str());
	}


	void Init(const char* in_name, const char* out_name) {
		in_name_ = in_name;
		if(in_name_ != NULL) {
			in_name_ = strdup(in_name_);
			MkFifo(in_name_);
		}

		out_name_ = out_name;
		if(out_name_ != NULL) {
			out_name_ = strdup(out_name_);
			MkFifo(out_name_);
		}

		in_fd_ = out_fd_ = -1;

		is_open_ = false;
	}

	virtual ~EventPipe() {
		if(is_open_) {
			Close();
		}

		if(in_name_ != NULL) delete in_name_;
		if(out_name_ != NULL) delete out_name_;
	}

	static void MkFifo(const char* name) {
		safe_assert(name != NULL);

		int ret_val = mkfifo(name, 0666);

		if ((ret_val == -1) && (errno != EEXIST)) {
			safe_fail("Error creating the named pipe %s", name);
		}
	}

	virtual void Open() {
		safe_assert(!is_open_);

		if(in_name_ != NULL) {
			in_fd_ = open(in_name_, O_RDONLY);
		}

		if(out_name_ != NULL) {
			out_fd_ = open(out_name_, O_WRONLY);
		}

		is_open_ = true;
	}

	virtual void Close() {
		safe_assert(is_open_);

		if(in_name_ != NULL) {
			close(in_fd_);
		}

		if(out_name_ != NULL) {
			close(out_fd_);
		}

		is_open_ = false;
	}

#define DoSend(x)	write(out_fd_, static_cast<void*>(&x), sizeof(x))
#define DoRecv(x)	read(in_fd_, static_cast<void*>(&x), sizeof(x))

#define DECL_SEND_RECV(F) 			\
	void F(EventBuffer* event) {	\
		Do##F(event->type);			\
		Do##F(event->threadid);		\
		switch(event->type) {		\
		case MemRead:				\
		case MemWrite:				\
			Do##F(event->addr);		\
			Do##F(event->size);		\
			break;					\
		case FuncEnter:				\
			Do##F(event->addr);		\
			Do##F(event->arg0);		\
			Do##F(event->arg1);		\
			break;					\
		case FuncReturn:			\
			Do##F(event->addr);		\
			Do##F(event->retval);	\
			break;					\
		case FuncCall:				\
			Do##F(event->addr);		\
			Do##F(event->addr_target); \
			Do##F(event->arg0);		\
			Do##F(event->arg1);		\
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
	ShadowThread(THREADID tid, EventPipe* pipe) : tid_(tid), pipe_(pipe), event_(NULL) {
		memset(&event_, 0, sizeof(EventBuffer));
		sem_.Init(0);
	}

	virtual ~ShadowThread(){}

//	void WaitForEventAndSkip(EventKind type) {
//		int timer = 0;
//		for(pipe_.Recv(&event_); event_.type != type; pipe_.Recv(&event_)) {
//			if(timer > 1000) {
//				safe_fail("Too many iterations for waiting event kind %d!", type);
//			}
//			++timer;
//			SendContinue();
//		}
//		SendContinue();
//	}
//
	void SendContinue() {
		EventBuffer e;
		e.type = Continue;
		e.threadid = tid_;
		Send(&e);
	}

	virtual void* Run() = 0;

	void Send(EventBuffer* e) {
		pipe_->Send(this, e);
	}

	void Recv(EventBuffer* e) {
		pipe_->Recv(this, e);
	}

	void SendRecvContinue(EventBuffer* e) {
		// send the event
		this->Send(e);

		// wait for continue
		this->Recv(e);

		if(e->type != Continue) {
			safe_fail("Unexpected event type: %d. Expected continue.", e->type);
		}
	}

	// used by pipe implementation
	// when thread wants to receive an event, it waits on its semaphore
	void WaitRecv(EventBuffer* event) {
		safe_assert(event_ == NULL);
		safe_assert(event != NULL);
		event_ = event;

		safe_assert(sem_.Get() <= 1);
		sem_.Wait();
	}

	// copy event from argument and signal the semaphore
	void SignalRecv(EventBuffer* event) {
		// copy
		safe_assert(event_ != NULL);
		*event_ = *event;

		safe_assert(tid_ == event_->threadid);

		event_ = NULL;

		// signal
		safe_assert(sem_.Get() <= 0);
		sem_.Signal();
	}

	static void* thread_func(void* arg) {
		safe_assert(arg != NULL);
		ShadowThread* thread = static_cast<ShadowThread*>(arg);
		safe_assert(thread != NULL);
		return thread->Run();
	}

	int SpawnAsThread(bool call_original) {
		pthread_t pt;
		if(call_original) {
			return PthreadOriginals::pthread_create(&pt, NULL, thread_func, this);
		} else {
			return pthread_create(&pt, NULL, thread_func, this);
		}
	}

private:
	DECL_FIELD(THREADID, tid)
	DECL_FIELD(EventPipe*, pipe)
	DECL_FIELD_REF(EventBuffer*, event)
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
	ConcurrentPipe(EventHandler* event_handler = NULL)
	: EventPipe(), event_handler_(event_handler != NULL ? event_handler : new EventHandler()) {}

	ConcurrentPipe(const PipeNamePair& names, EventHandler* event_handler = NULL) : EventPipe(names), event_handler_(event_handler) {}

	~ConcurrentPipe() {}

	void Send(ShadowThread* thread, EventBuffer* event) {
		send_mutex_.Lock();

		bool cancel = event_handler_ == NULL ? false : !event_handler_->OnSend(event);

		if(!cancel) {
			EventPipe::Send(event);
		}

		send_mutex_.Unlock();
	}

	void Recv(ShadowThread* thread, EventBuffer* event) {
		thread->WaitRecv(event);
	}

	/*****************************************************************/

	//override
	void Open() {
		EventPipe::Open();

		// start thread
		worker_thread_ = new Thread(56789, ConcurrentPipe::thread_func, this);
	}

	//override
	void Close() {
		if(worker_thread_ != NULL) {
			worker_thread_->CancelJoin();
		}
		EventPipe::Close();
	}


	static void* thread_func(void* arg) {
		ConcurrentPipe* pipe = static_cast<ConcurrentPipe*>(arg);
		safe_assert(pipe != NULL);

		EventHandler* event_handler = pipe->event_handler();
		safe_assert(event_handler != NULL);

		EventBuffer event;

		for(;;) {
			// do receive
			pipe->EventPipe::Recv(&event);

			bool cancel = !event_handler->OnRecv(&event);

			if(!cancel) {
				// notify receiver
				ShadowThread* thread = pipe->GetShadowThread(event.threadid);
				if(thread != NULL) { // otherwise ignore the message

					thread->SignalRecv(&event);
				}
			}
		}

		return NULL;
	}

	/*****************************************************************/

	void Broadcast(EventBuffer* e) {
		for(TidToShadowThreadMap::const_iterator itr = tid_to_shadowthread_.begin(), end = tid_to_shadowthread_.end(); itr != end; ++itr) {
			ShadowThread* shadowthread = itr->second;

			e->threadid = shadowthread->tid();
			shadowthread->SignalRecv(e);
		}
	}

	void SendContinue(THREADID tid) {
		EventBuffer e;
		e.type = Continue;
		e.threadid = tid;
		EventPipe::Send(&e);
	}

	/*****************************************************************/

	ShadowThread* GetShadowThread(THREADID tid) {
		TidToShadowThreadMap::accessor acc;
		if(tid_to_shadowthread_.find(acc, tid)) {
			return safe_notnull(acc->second);
		}
		return NULL;
	}

	void RegisterShadowThread(ShadowThread* shadowthread) {
		TidToShadowThreadMap::accessor acc;
		if(!tid_to_shadowthread_.find(acc, shadowthread->tid())) {
			tid_to_shadowthread_.insert(acc, shadowthread->tid());
			acc->second = shadowthread;
		}
		safe_assert(acc->second == shadowthread);
	}

	void UnregisterShadowThread(ShadowThread* shadowthread) {
		tid_to_shadowthread_.erase(shadowthread->tid());
	}

private:
	DECL_FIELD_REF(Mutex, send_mutex)
	DECL_FIELD_REF(TidToShadowThreadMap, tid_to_shadowthread)
	DECL_FIELD(Thread*, worker_thread)
	DECL_FIELD(EventHandler*, event_handler)
};

/********************************************************************************/


} // end namespace

#endif /* IPC_H_ */
