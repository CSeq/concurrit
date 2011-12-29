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

#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include "common.h"
#include "sharedaccess.h"

namespace concurrit {

/*
 * Exceptions
 */

class BacktrackException : public std::exception {
public:
	BacktrackException() throw() : std::exception() {

	}

	virtual ~BacktrackException() throw() {

	}

	virtual const char* what() const throw()
	{
		return "Backtrack";
	}
private:
};

/********************************************************************************/

class AssertionViolationException : public std::exception {
public:
	AssertionViolationException(const char* cond, SourceLocation* loc) throw()
			: std::exception() {
		condition_ = cond == NULL ? "Unknown" : std::string(cond);
		loc_ = loc;
	}

	~AssertionViolationException() throw() {
		delete loc_;
	}

	virtual const char* what() const throw()
	{
		std::stringstream s(std::stringstream::out);
		s << "Assertion violated: " << condition_ << "\n";
		s << "Source location: " << (loc_ != NULL ? loc_->ToString() : "<unknown>") << "\n";
		return s.str().c_str();
	}
private:
	DECL_FIELD(std::string, condition)
	DECL_FIELD(SourceLocation*, loc)
};

/********************************************************************************/

class AssumptionViolationException : public BacktrackException {
public:
	AssumptionViolationException(const char* cond, SourceLocation* loc) throw()
			: BacktrackException() {
		condition_ = cond == NULL ? "Unknown" : std::string(cond);
		loc_ = loc;
	}

	~AssumptionViolationException() throw() {
		delete loc_;
	}

	virtual const char* what() const throw()
	{
		std::stringstream s(std::stringstream::out);
		s << "Assumption violated: " << condition_ << "\n";
		s << "Source location: " << (loc_ != NULL ? loc_->ToString() : "<unknown>") << "\n";
		return s.str().c_str();
	}
private:
	DECL_FIELD(std::string, condition)
	DECL_FIELD(SourceLocation*, loc)
};

/********************************************************************************/

class InternalException : public std::exception {
public:
	InternalException(std::string& msg, SourceLocation* loc) throw() : message_(msg), loc_(loc) {}

	~InternalException() throw() {
		if(loc_ != NULL) {
			delete loc_;
		}
	}

	virtual const char* what() const throw()
	{
		std::stringstream s(std::stringstream::out);
		s << "Internal exception: " << message_ << "\n";
		s << "Source location: " << (loc_ != NULL ? loc_->ToString() : "<unknown>") << "\n";
		return s.str().c_str();
	}
private:
	DECL_FIELD(std::string, message)
	DECL_FIELD(SourceLocation*, loc)
};

/********************************************************************************/

// general counit exception
class CounitException : public std::exception {
public:
	CounitException(std::string& where, std::exception* e) throw() : std::exception() {
		where_ = where;
		cause_ = e;
	}

	CounitException(const char* where, std::exception* e) throw() : std::exception() {
		where_ = where;
		cause_ = e;
	}

	~CounitException() throw() {
		// do not delete the cause!
	}

	virtual const char* what() const throw()
	{
		std::stringstream s(std::stringstream::out);
		s << "Counit exception in " << where_ << "\n";
		s << "Cause: " << cause_->what() << "\n";
		return s.str().c_str();
	}

	bool is_backtrack() {
		return INSTANCEOF(cause_, BacktrackException*);
	}

	bool is_assertion_violation() {
		return INSTANCEOF(cause_, AssertionViolationException*);
	}

	bool is_internal() {
		return INSTANCEOF(cause_, InternalException*);
	}

private:
	DECL_FIELD(std::string, where)
	DECL_FIELD(std::exception*, cause)
};

/********************************************************************************/

extern BacktrackException* __backtrack_exception__;
extern CounitException*    __counit_exception__;

#define TRIGGER_BACKTRACK()					throw CHECK_NOTNULL(__backtrack_exception__)
#define TRIGGER_WRAPPED_EXCEPTION(m, e)		{ __counit_exception__->set_where(m); __counit_exception__->set_cause(e); throw CHECK_NOTNULL(__counit_exception__); }
#define TRIGGER_WRAPPED_BACKTRACK(m)		TRIGGER_WRAPPED_EXCEPTION(m, (__backtrack_exception__))

//#define TRIGGER_BACKTRACK()				throw new BacktrackException()
//#define TRIGGER_WRAPPED_EXCEPTION(m, e)	throw new CounitException(m, e)
//#define TRIGGER_WRAPPED_BACKTRACK(m)		TRIGGER_WRAPPED_EXCEPTION(m, new BacktrackException())


} // end namespace

#endif /* EXCEPTION_H_ */
