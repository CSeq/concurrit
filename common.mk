# variables to set: CONCURRIT_HOME=...

CONCURRIT_SRCDIR=$(CONCURRIT_HOME)/src
CONCURRIT_INCDIR=$(CONCURRIT_HOME)/include
CONCURRIT_BINDIR=$(CONCURRIT_HOME)/bin
CONCURRIT_OBJDIR=$(CONCURRIT_HOME)/obj
CONCURRIT_LIBDIR=$(CONCURRIT_HOME)/lib
CONCURRIT_TESTDIR=$(CONCURRIT_HOME)/tests
CONCURRIT_TPDIR=$(CONCURRIT_HOME)/third_party
CONCURRIT_WORKDIR=$(CONCURRIT_HOME)/work

CXXTESTDIR=$(CONCURRIT_TPDIR)/cxxtest

CONCURRIT_LIB_FLAG=-lconcurrit

# flags to use when compiling concurrit
CONCURRIT_INC_FLAGS=-I$(CONCURRIT_INCDIR) -I$(CONCURRIT_TPDIR)/glog/include -I$(CONCURRIT_TPDIR)/gflags/include -I$(CONCURRIT_TPDIR)/pth/include -I$(BOOST_ROOT)
CONCURRIT_LIB_FLAGS=-L$(CONCURRIT_LIBDIR) -L$(CONCURRIT_TPDIR)/glog/lib -L$(CONCURRIT_TPDIR)/gflags/lib -L$(CONCURRIT_TPDIR)/pth/lib -lpth -lpthread -lglog -lgflags 

# flags to use when compiling tests with concurrit
CONCURRIT_TEST_INC_FLAGS=$(CONCURRIT_INC_FLAGS)
CONCURRIT_TEST_LIB_FLAGS=-L$(CONCURRIT_LIBDIR) $(CONCURRIT_LIB_FLAG) 

# flags to use when compiling programs under test with concurrit
CONCURRIT_PROG_INC_FLAGS=-I$(CONCURRIT_INCDIR)
CONCURRIT_PROG_LIB_FLAGS=-L$(CONCURRIT_LIBDIR) $(CONCURRIT_LIB_FLAG)

# flags to use when compiling the pin tool
CONCURRIT_PINTOOL_INC_FLAGS=-I$(CONCURRIT_INCDIR) -I$(CONCURRIT_TPDIR)/tbb/include -fomit-frame-pointer
CONCURRIT_PINTOOL_LIB_FLAGS=-L$(CONCURRIT_LIBDIR) $(CONCURRIT_LIB_FLAG) -L$(CONCURRIT_TPDIR)/tbb/lib/ia32/cc4.1.0_libc2.4_kernel2.6.16.21 -ltbb 
