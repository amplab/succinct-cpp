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

BENCHBUILDDIR := $(BUILDDIR)/bench

THRIFTSRCDIR := $(SRCDIR)/thrift
THRIFTBUILDDIR := $(BUILDDIR)/thrift
THRIFTTARGET_AS := $(BINDIR)/aserver
THRIFTTARGET_QS := $(BINDIR)/qserver
THRIFTTARGET_SS := $(BINDIR)/qhandler
THRIFTTARGET_MS := $(BINDIR)/smaster
THRIFTTARGET_SC := $(LIBDIR)/libsuccinctclient.a

SUCCINCTSRCDIRS := $(shell find $(SUCCINCTSRCDIR) -type d)
SUCCINCTBUILDDIRS := $(subst $(SUCCINCTSRCDIR),$(SUCCINCTBUILDDIR),$(SUCCINCTSRCDIRS))

SUCCINCTSOURCES := $(shell find $(SUCCINCTSRCDIR) -type f -name *.cpp)
SUCCINCTOBJECTS := $(patsubst $(SUCCINCTSRCDIR)/%,$(SUCCINCTBUILDDIR)/%,$(SUCCINCTSOURCES:.cpp=.o))
SUCCINCTCFLAGS := -O3 -std=c++11 -Wall -g
SUCCINCTLIB :=
SUCCINCTINC := -I include

THRIFTSOURCES_GEN := $(THRIFTSRCDIR)/succinct_constants.cpp $(THRIFTSRCDIR)/succinct_types.cpp $(THRIFTSRCDIR)/QueryService.cpp $(THRIFTSRCDIR)/AdaptiveQueryService.cpp
THRIFTSOURCES_SS := $(THRIFTSRCDIR)/SuccinctServer.cpp $(THRIFTSRCDIR)/SuccinctService.cpp
THRIFTSOURCES_QS := $(THRIFTSRCDIR)/QueryServer.cpp
THRIFTSOURCES_AS := $(THRIFTSRCDIR)/AdaptiveQueryServer.cpp
THRIFTSOURCES_MS := $(THRIFTSRCDIR)/SuccinctMaster.cpp $(THRIFTSRCDIR)/MasterService.cpp $(THRIFTSRCDIR)/SuccinctService.cpp
THRIFTSOURCES_SC := $(THRIFTSRCDIR)/succinct_constants.cpp $(THRIFTSRCDIR)/succinct_types.cpp $(THRIFTSRCDIR)/SuccinctService.cpp $(THRIFTSRCDIR)/MasterService.cpp $(THRIFTSRCDIR)/AdaptiveQueryService.cpp
THRIFTOBJECTS_GEN := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_GEN:.cpp=.o))
THRIFTOBJECTS_SS := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_SS:.cpp=.o))
THRIFTOBJECTS_QS := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_QS:.cpp=.o))
THRIFTOBJECTS_AS := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_AS:.cpp=.o))
THRIFTOBJECTS_MS := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_MS:.cpp=.o))
THRIFTOBJECTS_SC := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_SC:.cpp=.o))
THRIFTCFLAGS := -O3 -std=c++11 -w -DHAVE_NETINET_IN_H -g
THRIFTLIB := -L $(LIBDIR) -lsuccinct -levent -lthrift
THRIFTINC := -I include

all: succinct

succinct: succinct-lib

succinct-lib: $(SUCCINCTTARGET)

$(SUCCINCTTARGET): $(SUCCINCTOBJECTS)
	@echo "Creating static library..."
	@mkdir -p $(LIBDIR)
	@echo " $(AR) $(ARFLAGS) $@ $^"; $(AR) $(ARFLAGS) $@ $^

$(SUCCINCTBUILDDIR)/%.o: $(SUCCINCTSRCDIR)/%.cpp
	@mkdir -p $(SUCCINCTBUILDDIRS)
	@echo " $(CC) $(SUCCINCTCFLAGS) $(SUCCINCTINC) -c -o $@ $<";\
		$(CC) $(SUCCINCTCFLAGS) $(SUCCINCTINC) -c -o $@ $<

succinct-thrift: build-thrift succinct-thrift-components

succinct-thrift-components: gen-thrift adaptive-query-server query-server succinct-server succinct-master succinct-client

gen-thrift:
	@echo " ./bin/thrift -gen cpp:include_prefix -out thrift thrift/succinct.thrift"; ./bin/thrift -I include/thrift -gen cpp:include_prefix -out thrift thrift/succinct.thrift
	@echo " $(MV) thrift/*.cpp src/thrift/ && mv thrift/*.h include/thrift/"; $(MV) thrift/*.cpp src/thrift/ && mv thrift/*.h include/thrift/
	@echo " $(RM) src/thrift/*skeleton*"; $(RM) src/thrift/*skeleton*

build-thrift:
	@echo "Building thrift-0.9.1..."
	@echo " cd external/thrift-0.9.1; ./configure CXXFLAGS='-O3 -std=c++11' --prefix=`pwd`/../../ --exec-prefix=`pwd`/../../ --with-qt4=no --with-c_glib=no --with-csharp=no --with-java=no --with-erlang=no --with-python=no --with-perl=no --with-php=no --with-php_extension=no --with-ruby=no --with-haskell=no --with-go=no --with-d=no --without-tests && make && make install";\
		cd external/thrift-0.9.1; ./configure CXXFLAGS='-O3 -std=c++11' --prefix=`pwd`/../../ --exec-prefix=`pwd`/../../ --with-qt4=no --with-c_glib=no --with-csharp=no --with-java=no --with-erlang=no --with-python=no --with-perl=no --with-php=no --with-php_extension=no --with-ruby=no --with-haskell=no --with-go=no --with-d=no --without-tests && make && make install

adaptive-query-server: succinct $(THRIFTTARGET_AS)

query-server: succinct $(THRIFTTARGET_QS)

succinct-server: succinct $(THRIFTTARGET_SS)

succinct-master: succinct $(THRIFTTARGET_MS)

$(THRIFTTARGET_SS): $(THRIFTOBJECTS_SS) $(THRIFTOBJECTS_GEN)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(THRIFTTARGET_SS) $(THRIFTLIB)";\
		$(CC) $^ -o $(THRIFTTARGET_SS) $(THRIFTLIB)

$(THRIFTTARGET_AS): $(THRIFTOBJECTS_AS) $(THRIFTOBJECTS_GEN) 
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(THRIFTTARGET_AS) $(THRIFTLIB)";\
		$(CC) $^ -o $(THRIFTTARGET_AS) $(THRIFTLIB)
	
$(THRIFTTARGET_QS): $(THRIFTOBJECTS_QS) $(THRIFTOBJECTS_GEN) 
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(THRIFTTARGET_QS) $(THRIFTLIB)";\
		$(CC) $^ -o $(THRIFTTARGET_QS) $(THRIFTLIB)
		
$(THRIFTTARGET_MS): $(THRIFTOBJECTS_MS) $(THRIFTOBJECTS_GEN)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(THRIFTTARGET_MS) $(THRIFTLIB)";\
		$(CC) $^ -o $(THRIFTTARGET_MS) $(THRIFTLIB)

$(THRIFTBUILDDIR)/%.o: $(THRIFTSRCDIR)/%.cpp
	@mkdir -p $(THRIFTBUILDDIR)
	@echo " $(CC) $(THRIFTCFLAGS) $(THRIFTINC) -c -o $@ $<";\
		$(CC) $(THRIFTCFLAGS) $(THRIFTINC) -c -o $@ $<
		
succinct-client: succinct-client-lib

succinct-client-lib: $(THRIFTTARGET_SC)

$(THRIFTTARGET_SC): $(THRIFTOBJECTS_SC)
	@echo "Creating static library..."
	@mkdir -p $(LIBDIR)
	@echo " $(AR) $(ARFLAGS) $@ $^"; $(AR) $(ARFLAGS) $@ $^

tests: build-gtest
	@echo "Testing..."
	@echo " cd test; make"; cd test; make

build-gtest:
	@echo "Building gtest-1.7.0..."
	@echo " cd external/gtest-1.7.0/make; make";\
		cd external/gtest-1.7.0/make; make

fbench: succinct
	@echo "Building file benchmark..."
	@echo " cd benchmark; make clean && make fbench";\
		cd benchmark; make clean && make fbench

sbench: succinct
	@echo "Building shard benchmark..."
	@echo " cd benchmark; make clean && make sbench";\
		cd benchmark; make clean && make sbench

surebench: succinct
	@echo "Building sure benchmark..."
	@echo " cd benchmark; make clean && make surebench";\
		cd benchmark; make clean && make surebench

ssbench: succinct-thrift-components
	@echo "Building succinct-server benchmark..."
	@echo " cd benchmark; make clean && make ssbench";\
		cd benchmark; make clean && make ssbench

asbench: succinct-thrift-components
	@echo "Building adashard benchmark..."
	@echo " cd benchmark; make clean && make asbench";\
		cd benchmark; make clean && make asbench

clean:
	@echo "Cleaning..."; 
	@echo " $(RM) -r $(SUCCINCTBUILDDIR) $(THRIFTBUILDDIR) $(BENCHBUILDDIR) $(SUCCINCTTARGET) $(THRIFTTARGET_AS) $(THRIFTTARGET_QS) $(THRIFTTARGET_MS) $(THRIFTTARGET_SS) $(THRIFTTARGET_SC)";\
		$(RM) -r $(SUCCINCTBUILDDIR) $(THRIFTBUILDDIR) $(BENCHBUILDDIR) $(SUCCINCTTARGET) $(THRIFTTARGET_AS) $(THRIFTTARGET_QS) $(THRIFTTARGET_MS) $(THRIFTTARGET_SS) $(THRIFTTARGET_SC)
	@echo "Cleaning thrift..."
	@echo " cd external/thrift-0.9.1; ./cleanup.sh";\
		cd external/thrift-0.9.1; ./cleanup.sh

.PHONY: clean
