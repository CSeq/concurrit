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


#ifndef DSL_H_
#define DSL_H_

#include "common.h"

#include <cstdatomic>

namespace concurrit {

class Scenario;
class Coroutine;

enum TPVALUE { TPFALSE = 0, TPTRUE = 1, TPUNKNOWN = 2, TPINVALID = -1 };
TPVALUE TPNOT(TPVALUE v);
TPVALUE TPAND(TPVALUE v1, TPVALUE v2);
TPVALUE TPOR(TPVALUE v1, TPVALUE v2);

class TransitionPredicate {
public:
	TransitionPredicate() {}
	virtual ~TransitionPredicate() {}
	virtual TPVALUE EvalPreState() = 0;
	virtual bool EvalPostState() = 0;

	static TransitionPredicate* True();
	static TransitionPredicate* False();
};

class TrueTransitionPredicate : public TransitionPredicate {
public:
	TrueTransitionPredicate() : TransitionPredicate() {}
	~TrueTransitionPredicate() {}
	TPVALUE EvalPreState() { return TPTRUE; }
	bool EvalPostState() { return TPTRUE; }
};

class FalseTransitionPredicate : public TransitionPredicate {
public:
	FalseTransitionPredicate() : TransitionPredicate() {}
	~FalseTransitionPredicate() {}
	TPVALUE EvalPreState() { return TPFALSE; }
	bool EvalPostState() { return TPFALSE; }
};

/********************************************************************************/

extern TrueTransitionPredicate __true_transition_predicate__;
extern FalseTransitionPredicate __false_transition_predicate__;

/********************************************************************************/

class NotTransitionPredicate : public TransitionPredicate {
public:
	NotTransitionPredicate(TransitionPredicate* pred) : TransitionPredicate(), pred_(pred) {}
	~NotTransitionPredicate() {}
	TPVALUE EvalPreState() { return TPNOT(pred_->EvalPreState()); }
	bool EvalPostState() { return !(pred_->EvalPostState()); }
private:
	DECL_FIELD(TransitionPredicate*, pred)
};

/********************************************************************************/

enum NAryOp { NAryAND, NAryOR };
class NAryTransitionPredicate : public TransitionPredicate, public std::vector<TransitionPredicate*> {
public:
	NAryTransitionPredicate(NAryOp op = NAryAND, std::vector<TransitionPredicate*>* preds = NULL)
	: TransitionPredicate(), std::vector<TransitionPredicate*>(), op_(op) {
		if(preds != NULL) {
			for(iterator itr = preds->begin(), end = preds->end(); itr != end; ++itr) {
				push_back(*itr);
			}
		}
	}
	~NAryTransitionPredicate() {}

	TPVALUE EvalPreState() {
		safe_assert(!empty());
		TPVALUE v = (op_ == NAryAND) ? TPTRUE : TPFALSE;
		for(NAryTransitionPredicate::iterator itr = begin(); itr != end(); ++itr) {
			// update current
			TransitionPredicate* current = (*itr);
			// update v
			v = (op_ == NAryAND) ? TPAND(v, current->EvalPreState()) : TPOR(v, current->EvalPreState());
			if((op_ == NAryAND && v == TPFALSE) || (op_ == NAryOR && v == TPTRUE)) {
				break;
			}
		}
		return v;
	}

	bool EvalPostState() {
		safe_assert(!empty());
		bool v = (op_ == NAryAND) ? true : false;
		for(NAryTransitionPredicate::iterator itr = begin(); itr != end(); ++itr) {
			// update current
			TransitionPredicate* current = (*itr);
			// update v
			v = (op_ == NAryAND) ? (v && current->EvalPostState()) : (v || current->EvalPostState());
			if((op_ == NAryAND && v == false) || (op_ == NAryOR && v == true)) {
				break;
			}
		}
		return v;
	}
private:
	DECL_FIELD(NAryOp, op)
};

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ExecutionTree;
typedef std::atomic_address ExecutionTreeRef;
typedef std::vector<ExecutionTree*> ExecutionTreeList;

#define for_each_child(child) \
	ExecutionTreeList::iterator __itr__ = children_.begin(); \
	ExecutionTree* child = (__itr__ != children_.end() ? *__itr__ : NULL); \
	for (; __itr__ != children_.end(); child = ((++__itr__) != children_.end() ? *__itr__ : NULL))

class ExecutionTree : public Writable {
public:
	ExecutionTree(ExecutionTree* parent = NULL, int num_children = 0) : parent_(parent), covered_(false) {
		InitChildren(num_children);
		num_nodes_++;
	}

	virtual ~ExecutionTree();

	bool ContainsChild(ExecutionTree* node);

	void InitChildren(int n);

	ExecutionTree* child(int i = 0);

	void add_child(ExecutionTree* node);

	void set_child(ExecutionTree* node, int i = 0);
	ExecutionTree* get_child(int i);

	bool child_covered(int i = 0);

	virtual void ComputeCoverage(Scenario* scenario, bool recurse);

	virtual void ToStream(FILE* file) {
		fprintf(file, "Num children: %d, %s.", children_.size(), (covered_ ? "covered" : "not covered"));
	}

private:
	DECL_FIELD(ExecutionTree*, parent)
	DECL_FIELD_REF(ExecutionTreeList, children)
	DECL_FIELD(bool, covered)

	DECL_STATIC_FIELD(int, num_nodes)

	friend class ExecutionTreeManager;
};

/********************************************************************************/

class ChildLoc {
public:
	ChildLoc(ExecutionTree* parent = NULL, int child_index = -1) : parent_(parent), child_index_(child_index) {}
	~ChildLoc(){}

	void set(ExecutionTree* node) {
		safe_assert(parent_ != NULL);
		parent_->set_child(node, child_index_);
		if(node != NULL) {
			node->set_parent(parent_);
		}
	}

	ExecutionTree* get() {
		safe_assert(parent_ != NULL);
		ExecutionTree* node = parent_->get_child(child_index_);
		return node;
	}

	bool check(ExecutionTree* node) {
		safe_assert(parent_ != NULL);
		return (parent_->get_child(child_index_) == node) && (node == NULL || node->parent() == parent_);
	}

	ExecutionTree* exchange(ExecutionTree* node) {
		ExecutionTree* old = get();
		set(node);
		return old;
	}

	operator ExecutionTree* () {
		return get();
	}

	bool empty() {
		return parent_ == NULL || child_index_ < 0;
	}
private:
	DECL_FIELD(ExecutionTree*, parent)
	DECL_FIELD(int, child_index);
};

/********************************************************************************/

class EndNode : public ExecutionTree {
public:
	EndNode(ExecutionTree* parent = NULL) : ExecutionTree(parent, 1) {
		covered_ = true;
		set_child(this); // points to itself
		exception_ = NULL;
		old_root_ = NULL;
	}

	void add_exception(std::exception* e, Coroutine* owner, const std::string& where) {
		exception_ = new ConcurritException(e, owner, where, exception_);
	}

	virtual void ToStream(FILE* file) {
		fprintf(file, "EndNode. %s exception.", (exception_ == NULL ? "Has no " : "Has "));
		ExecutionTree::ToStream(file);
	}

private:
	DECL_FIELD(ConcurritException*, exception)
	DECL_FIELD(ExecutionTree*, old_root)
};

/********************************************************************************/

class ChoiceNode : public ExecutionTree {
public:
	ChoiceNode(ExecutionTree* parent = NULL) : ExecutionTree(parent, 2) {}

	virtual void ToStream(FILE* file) {
		fprintf(file, "ChoiceNode.");
		ExecutionTree::ToStream(file);
	}
};

/********************************************************************************/

class ThreadVar {
public:
	ThreadVar(Coroutine* thread = NULL, const std::string& name = "<unknown>")
	: name_(name), thread_(thread) {}
	~ThreadVar() {}

private:
	DECL_FIELD(std::string, name)
	DECL_FIELD(Coroutine*, thread)
};

typedef boost::shared_ptr<ThreadVar> ThreadVarPtr;


/********************************************************************************/

class SelectThreadNode : public ExecutionTree {
public:
	typedef std::map<THREADID, int> TidToIdxMap;
	SelectThreadNode(ThreadVarPtr var, ExecutionTree* parent = NULL)
	: ExecutionTree(parent, 0), var_(var) {
		safe_assert(var_->thread() == NULL);
	}
	~SelectThreadNode() {}

	ChildLoc set_selected_thread(Coroutine* co) {
		int child_index = add_or_get_thread(co);
		// update newnode to point to the proper child index of select thread
		ChildLoc newnode = {this, child_index};
		safe_assert(!newnode.empty());
		// set thread of the variable
		safe_assert(var_->thread() == NULL);
		var_->set_thread(co);
		return newnode;
	}

	ExecutionTree* child_by_tid(THREADID tid) {
		int index = child_index_by_tid(tid);
		return index < 0 ? NULL : child(index);
	}

	// override
	virtual void ComputeCoverage(Scenario* scenario, bool recurse);

	virtual void ToStream(FILE* file) {
		fprintf(file, "SelectThreadNode. Selected %s.", ((var_ != NULL && var_->thread() != NULL) ? var_->thread()->name().c_str() : "no thread"));
		ExecutionTree::ToStream(file);
	}

private:

	// returns the index of the new child
	int add_or_get_thread(Coroutine* co) {
		THREADID tid = co->coid();
		int child_index = child_index_by_tid(tid);
		if(child_index < 0) {
			const int sz = children_.size();
			safe_assert(idxToThreadMap_.size() == sz);
			tidToIdxMap_[tid] = sz;
			children_.push_back(NULL);
			idxToThreadMap_.push_back(co);
			return sz;
		} else {
			return child_index;
		}
	}

	int child_index_by_tid(THREADID tid) {
		TidToIdxMap::iterator itr = tidToIdxMap_.find(tid);
		if(itr == tidToIdxMap_.end()) {
			return -1;
		} else {
			return itr->second;
		}
	}

private:
	DECL_FIELD(ThreadVarPtr, var)
	DECL_FIELD_REF(TidToIdxMap, tidToIdxMap)
	DECL_FIELD_REF(std::vector<Coroutine*>, idxToThreadMap)

};

/********************************************************************************/

class TransitionNode : public ExecutionTree {
public:
	TransitionNode(TransitionPredicate* pred, ThreadVarPtr var = boost::shared_ptr<ThreadVar>(), ExecutionTree* parent = NULL)
	: ExecutionTree(parent, 1), pred_(pred), thread_(NULL), var_(var) {}

	virtual void ToStream(FILE* file) {
		fprintf(file, "TransitionNode.");
		ExecutionTree::ToStream(file);
	}
private:
	DECL_FIELD(TransitionPredicate*, pred)
	DECL_FIELD(Coroutine*, thread)
	DECL_FIELD(ThreadVarPtr, var)
};

/********************************************************************************/

class TransferUntilNode : public ExecutionTree {
public:
	TransferUntilNode(TransitionPredicate* pred, ExecutionTree* parent = NULL)
	: ExecutionTree(parent, 1), pred_(pred) {}

private:
	DECL_FIELD(TransitionPredicate*, pred)
};

/********************************************************************************/

enum AcquireRefMode { EXIT_ON_EMPTY = 1, EXIT_ON_FULL = 2, EXIT_ON_LOCK = 3};

class ExecutionTreeManager {
public:
	ExecutionTreeManager();

	void Restart();

inline bool REF_EMPTY(ExecutionTree* n) { return ((n) == NULL); }
inline bool REF_LOCKED(ExecutionTree* n) { return ((n) == (&lock_node_)); }
inline bool REF_ENDTEST(ExecutionTree* n) { return (INSTANCEOF(n, EndNode*)); }

	// set atomic_ref to lock_node, and return the previous node according to mode
	// if atomic_ref is end_node, returns it immediatelly
	ExecutionTree* AcquireRef(AcquireRefMode mode, long timeout_usec = -1);
	// same as AcquireRefEx, but triggers backtrack exception if the node is an end node
	ExecutionTree* AcquireRefEx(AcquireRefMode mode, long timeout_usec = -1);

	// if child_index >= 0, adds the (node,child_index) to path and nullifies atomic_ref
	// otherwise, puts node back to atomic_ref
	void ReleaseRef(ExecutionTree* node = NULL, int child_index = -1);

	void ReleaseRef(ChildLoc& node) {
		ReleaseRef(node.parent(), node.child_index());
	}

	ChildLoc GetLastInPath();
	void AddToPath(ExecutionTree* node, int child_index);

	void EndWithSuccess();
	void EndWithException(Coroutine* current, std::exception* exception, const std::string& where = "<unknown>");
	void EndWithBacktrack(Coroutine* current, const std::string& where);

	bool CheckCompletePath(std::vector<ChildLoc>* path = NULL);

private:
	ExecutionTree* GetRef();
	ExecutionTree* ExchangeRef(ExecutionTree* node);
	void SetRef(ExecutionTree* node);

private:
	DECL_FIELD_REF(ExecutionTree, root_node)
	DECL_FIELD_REF(ExecutionTree, lock_node)
	DECL_FIELD_REF(std::vector<ChildLoc>, current_path)
	DECL_FIELD(unsigned, num_paths)

	ExecutionTreeRef atomic_ref_;
	Semaphore sem_ref_;

	DECL_FIELD(Mutex, mutex)
	DECL_FIELD(ConditionVar, cv)


	DISALLOW_COPY_AND_ASSIGN(ExecutionTreeManager)
};

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

enum TransitionConst {
	TRANS_MEM_WRITE,
	TRANS_MEM_READ,
	TRANS_FUNC_CALL,
	TRANS_FUNC_RETURN,
	TRANS_FUNC_ENTER,
	TRANS_ENDING
};

// class storing information about a single transition
class TransitionInfo {
public:
	TransitionInfo(TransitionConst type):
		type_(type) {}
	virtual ~TransitionInfo(){}
private:
	DECL_FIELD(TransitionConst, type)
};

/********************************************************************************/

class EndingTransitionInfo : public TransitionInfo {
public:
	EndingTransitionInfo(): TransitionInfo(TRANS_ENDING){}
	~EndingTransitionInfo(){}
};

/********************************************************************************/

class MemAccessTransitionInfo : public TransitionInfo {
public:
	MemAccessTransitionInfo(TransitionConst type, void* addr, uint32_t size, SourceLocation* loc):
		TransitionInfo(type), addr_(addr), size_(size), loc_(loc) {}
	~MemAccessTransitionInfo(){}
private:
	DECL_FIELD(void*, addr)
	DECL_FIELD(uint32_t, size)
	DECL_FIELD(SourceLocation*, loc)
};

/********************************************************************************/

class FuncCallTransitionInfo : public TransitionInfo {
public:
	FuncCallTransitionInfo(void* addr, SourceLocation* loc_src, SourceLocation* loc_target = NULL, bool direct = false):
		TransitionInfo(TRANS_FUNC_CALL), addr_(addr), loc_src_(loc_src), loc_target_(loc_target), direct_(direct) {}
	~FuncCallTransitionInfo(){}
private:
	DECL_FIELD(void*, addr)
	DECL_FIELD(bool, direct)
	DECL_FIELD(SourceLocation*, loc_src)
	DECL_FIELD(SourceLocation*, loc_target)
};

/********************************************************************************/

class FuncReturnTransitionInfo : public TransitionInfo {
public:
	FuncReturnTransitionInfo(void* addr, SourceLocation* loc, ADDRINT retval):
		TransitionInfo(TRANS_FUNC_RETURN), addr_(addr), loc_(loc), retval_(retval) {}
	~FuncReturnTransitionInfo(){}
private:
	DECL_FIELD(void*, addr)
	DECL_FIELD(SourceLocation*, loc)
	DECL_FIELD(ADDRINT, retval)
};

/********************************************************************************/

class FuncEnterTransitionInfo : public TransitionInfo {
public:
	FuncEnterTransitionInfo(void* addr, SourceLocation* loc):
		TransitionInfo(TRANS_FUNC_ENTER), addr_(addr), loc_(loc) {}
	~FuncEnterTransitionInfo(){}
private:
	DECL_FIELD(void*, addr)
	DECL_FIELD(SourceLocation*, loc)
};


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// constraints
class TransitionConstraint : public TransitionPredicate {
public:
	TransitionConstraint(Scenario* scenario);

	virtual ~TransitionConstraint();

private:
	DECL_FIELD(Scenario*, scenario)
};

/********************************************************************************/

class TransitionConstraintAll : public TransitionConstraint {
public:
	TransitionConstraintAll(Scenario* scenario, TransitionPredicate* pred)
	: TransitionConstraint(scenario), pred_(pred) {}

	~TransitionConstraintAll() {}

	virtual TPVALUE EvalPreState() {
		return pred_->EvalPreState();
	}
	virtual bool EvalPostState() {
		return pred_->EvalPostState();
	}

private:
	DECL_FIELD(TransitionPredicate*, pred)
};


/********************************************************************************/

class TransitionConstraintFirst : public TransitionConstraintAll {
public:
	TransitionConstraintFirst(Scenario* scenario, TransitionPredicate* pred)
	: TransitionConstraintAll(scenario, pred), done_(false) {}

	~TransitionConstraintFirst() {}

	virtual TPVALUE EvalPreState() {
		if(!done_) {
			done_ = true;
			return pred_->EvalPreState();
		} else {
			return TPTRUE;
		}
	}

	virtual bool EvalPostState() {
		if(!done_) {
			done_ = true;
			return pred_->EvalPostState();
		} else {
			return TPTRUE;
		}
	}

private:
	DECL_FIELD(bool, done)
};


/********************************************************************************/

} // namespace


#endif /* DSL_H_ */
