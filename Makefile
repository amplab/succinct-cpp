CC := g++
AR := ar

MV := mv

LIBDIR := lib
BINDIR := bin
SRCDIR := src
BUILDDIR := build

SUCCINCTSRCDIR := $(SRCDIR)/succinct
SUCCINCTBUILDDIR := $(BUILDDIR)/succinct
SUCCINCTTARGET := $(LIBDIR)/libsuccinct.a

THRIFTSRCDIR := $(SRCDIR)/thrift
THRIFTBUILDDIR := $(BUILDDIR)/thrift
THRIFTTARGET_QS := $(BINDIR)/qserver
THRIFTTARGET_SS := $(BINDIR)/succinct

SUCCINCTSRCDIRS := $(shell find $(SUCCINCTSRCDIR) -type d)
SUCCINCTBUILDDIRS := $(subst $(SUCCINCTSRCDIR),$(SUCCINCTBUILDDIR),$(SUCCINCTSRCDIRS))

SUCCINCTSOURCES := $(shell find $(SUCCINCTSRCDIR) -type f -name *.cpp)
SUCCINCTOBJECTS := $(patsubst $(SUCCINCTSRCDIR)/%,$(SUCCINCTBUILDDIR)/%,$(SUCCINCTSOURCES:.cpp=.o))
SUCCINCTCFLAGS := -O3 -std=c++11 -Wall -Werror
SUCCINCTLIB :=
SUCCINCTINC := -I include

THRIFTSOURCES_GEN := $(THRIFTSRCDIR)/succinct_constants.cpp $(THRIFTSRCDIR)/succinct_types.cpp $(THRIFTSRCDIR)/QueryService.cpp
THRIFTSOURCES_SS := $(THRIFTSRCDIR)/SuccinctServer.cpp $(THRIFTSRCDIR)/SuccinctService.cpp
THRIFTSOURCES_QS := $(THRIFTSRCDIR)/QueryServer.cpp
THRIFTOBJECTS_GEN := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_GEN:.cpp=.o))
THRIFTOBJECTS_SS := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_SS:.cpp=.o))
THRIFTOBJECTS_QS := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_QS:.cpp=.o))
THRIFTCFLAGS := -O3 -std=c++11 -w -DHAVE_NETINET_IN_H
THRIFTLIB := -L $(LIBDIR) -lsuccinct -levent -lthrift
THRIFTINC := -I include

all: succinct

succinct: $(SUCCINCTTARGET)

$(SUCCINCTTARGET): $(SUCCINCTOBJECTS)
	@echo "Creating static library..."
	@echo " $(AR) $(ARFLAGS) $@ $^"; $(AR) $(ARFLAGS) $@ $^

$(SUCCINCTBUILDDIR)/%.o: $(SUCCINCTSRCDIR)/%.cpp
	@mkdir -p $(SUCCINCTBUILDDIRS)
	@echo " $(CC) $(SUCCINCTCFLAGS) $(SUCCINCTINC) -c -o $@ $<";\
		$(CC) $(SUCCINCTCFLAGS) $(SUCCINCTINC) -c -o $@ $<

succinct-thrift: build-thrift query-server succinct-server 
	@echo "test: $(THRIFTTARGET_SS) $(THRIFTTARGET_QS)"

build-thrift:
	@echo "Building thrift-0.9.1..."
	@echo " cd external/thrift-0.9.1; ./configure CXXFLAGS='-O3 -std=c++11' --prefix=`pwd`/../../ --exec-prefix=`pwd`/../../ --with-pic=no --with-qt4=no --with-c_glib=no --with-csharp=no --with-java=no --with-erlang=no --with-python=no --with-perl=no --with-php=no --with-php_extension=no --with-ruby=no --with-haskell=no --with-go=no --with-d=no --without-tests && make && make install";\
		cd external/thrift-0.9.1; ./configure CXXFLAGS='-O3 -std=c++11' --prefix=`pwd`/../../ --exec-prefix=`pwd`/../../ --with-pic=no --with-qt4=no --with-c_glib=no --with-csharp=no --with-java=no --with-erlang=no --with-python=no --with-perl=no --with-php=no --with-php_extension=no --with-ruby=no --with-haskell=no --with-go=no --with-d=no --without-tests && make && make install
	@echo " ./bin/thrift -gen cpp:include_prefix -out thrift thrift/succinct.thrift"; ./bin/thrift -I include/thrift -gen cpp:include_prefix -out thrift thrift/succinct.thrift
	@echo " $(MV) thrift/*.cpp src/thrift/ && mv thrift/*.h include/thrift/"; $(MV) thrift/*.cpp src/thrift/ && mv thrift/*.h include/thrift/
	@echo " $(RM) src/thrift/*skeleton*"; $(RM) src/thrift/*skeleton*
	
query-server: succinct $(THRIFTTARGET_QS)

succinct-server: succinct $(THRIFTTARGET_SS)

$(THRIFTTARGET_SS): $(THRIFTOBJECTS_SS) $(THRIFTOBJECTS_GEN)
	@echo "Linking..."
	@echo " $(CC) $^ -o $(THRIFTTARGET_SS) $(THRIFTLIB)";\
		$(CC) $^ -o $(THRIFTTARGET_SS) $(THRIFTLIB)

$(THRIFTTARGET_QS): $(THRIFTOBJECTS_QS) $(THRIFTOBJECTS_GEN) 
	@echo "Linking..."
	@echo " $(CC) $^ -o $(THRIFTTARGET_QS) $(THRIFTLIB)";\
		$(CC) $^ -o $(THRIFTTARGET_QS) $(THRIFTLIB)

$(THRIFTBUILDDIR)/%.o: $(THRIFTSRCDIR)/%.cpp
	@mkdir -p $(THRIFTBUILDDIR)
	@echo " $(CC) $(THRIFTCFLAGS) $(THRIFTINC) -c -o $@ $<";\
		$(CC) $(THRIFTCFLAGS) $(THRIFTINC) -c -o $@ $<

tests: build-gtest
	@echo "Testing..."
	@echo " cd test; make"; cd test; make

build-gtest:
	@echo "Building gtest-1.7.0..."
	@echo " cd external/gtest-1.7.0/make; make";\
		cd external/gtest-1.7.0/make; make

bench: succinct
	@echo "Building benchmarks..."
	@echo " cd benchmark; make"; cd benchmark; make

clean:
	@echo "Cleaning..."; 
	@echo " $(RM) -r $(SUCCINCTBUILDDIR) $(LIBDIR)/* $(BINDIR)/*"; 
		$(RM) -r $(SUCCINCTBUILDDIR) $(LIBDIR)/* $(BINDIR)/*
	@echo "Cleaning thrift..."
	@echo " cd external/thrift-0.9.1; make clean";\
		cd external/thrift-0.9.1; make clean

.PHONY: clean