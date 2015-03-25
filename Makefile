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

THRIFTTARGET_SS := $(BINDIR)/sserver
THRIFTTARGET_SH := $(BINDIR)/shandler
THRIFTTARGET_SM := $(BINDIR)/smaster
THRIFTTARGET_SC := $(LIBDIR)/libsuccinctclient.a

THRIFTTARGET_AS := $(BINDIR)/aserver
THRIFTTARGET_AH := $(BINDIR)/ahandler
THRIFTTARGET_AM := $(BINDIR)/amaster
THRIFTTARGET_AC := $(LIBDIR)/libadaptsuccinctclient.a

BUILDDIRS := $(SUCCINCTBUILDDIR) $(BENCHBUILDDIR) $(THRIFTBUILDDIR)
TARGETS := $(SUCCINCTTARGET) $(THRIFTTARGET_SS) $(THRIFTTARGET_SH) $(THRIFTTARGET_SM) $(THRIFTTARGET_SC) $(THRIFTTARGET_AS) $(THRIFTTARGET_AH) $(THRIFTTARGET_AM) $(THRIFTTARGET_AC)

SUCCINCTSRCDIRS := $(shell find $(SUCCINCTSRCDIR) -type d)
SUCCINCTBUILDDIRS := $(subst $(SUCCINCTSRCDIR),$(SUCCINCTBUILDDIR),$(SUCCINCTSRCDIRS))

SUCCINCTSOURCES := $(shell find $(SUCCINCTSRCDIR) -type f -name *.cpp)
SUCCINCTOBJECTS := $(patsubst $(SUCCINCTSRCDIR)/%,$(SUCCINCTBUILDDIR)/%,$(SUCCINCTSOURCES:.cpp=.o))
SUCCINCTCFLAGS := -O3 -std=c++11 -Wall -g
SUCCINCTLIB :=
SUCCINCTINC := -I include

THRIFTSOURCES_GEN := $(THRIFTSRCDIR)/succinct_constants.cpp $(THRIFTSRCDIR)/succinct_types.cpp $(THRIFTSRCDIR)/QueryService.cpp
THRIFTSOURCES_SS := $(THRIFTSRCDIR)/QueryServer.cpp
THRIFTSOURCES_SH := $(THRIFTSRCDIR)/SuccinctServer.cpp $(THRIFTSRCDIR)/SuccinctService.cpp
THRIFTSOURCES_SM := $(THRIFTSRCDIR)/SuccinctMaster.cpp $(THRIFTSRCDIR)/MasterService.cpp $(THRIFTSRCDIR)/SuccinctService.cpp
THRIFTSOURCES_SC := $(THRIFTSRCDIR)/succinct_constants.cpp $(THRIFTSRCDIR)/succinct_types.cpp $(THRIFTSRCDIR)/QueryService.cpp $(THRIFTSRCDIR)/SuccinctService.cpp $(THRIFTSRCDIR)/MasterService.cpp

THRIFTSOURCES_AGEN := $(THRIFTSRCDIR)/adaptive_constants.cpp $(THRIFTSRCDIR)/adaptive_types.cpp $(THRIFTSRCDIR)/AdaptiveQueryService.cpp
THRIFTSOURCES_AS := $(THRIFTSRCDIR)/AdaptiveQueryServer.cpp
THRIFTSOURCES_AH := $(THRIFTSRCDIR)/AdaptiveSuccinctServer.cpp $(THRIFTSRCDIR)/AdaptiveSuccinctService.cpp
THRIFTSOURCES_AM := $(THRIFTSRCDIR)/AdaptiveSuccinctMaster.cpp $(THRIFTSRCDIR)/AdaptiveMasterService.cpp $(THRIFTSRCDIR)/AdaptiveSuccinctService.cpp
THRIFTSOURCES_AC := $(THRIFTSRCDIR)/adaptive_constants.cpp $(THRIFTSRCDIR)/adaptive_types.cpp $(THRIFTSRCDIR)/AdaptiveQueryService.cpp $(THRIFTSRCDIR)/AdaptiveSuccinctService.cpp $(THRIFTSRCDIR)/AdaptiveMasterService.cpp

THRIFTOBJECTS_GEN := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_GEN:.cpp=.o))
THRIFTOBJECTS_SS := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_SS:.cpp=.o))
THRIFTOBJECTS_SH := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_SH:.cpp=.o))
THRIFTOBJECTS_SM := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_SM:.cpp=.o))
THRIFTOBJECTS_SC := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_SC:.cpp=.o))

THRIFTOBJECTS_AGEN := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_AGEN:.cpp=.o))
THRIFTOBJECTS_AS := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_AS:.cpp=.o))
THRIFTOBJECTS_AH := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_AH:.cpp=.o))
THRIFTOBJECTS_AM := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_AM:.cpp=.o))
THRIFTOBJECTS_AC := $(patsubst $(THRIFTSRCDIR)/%,$(THRIFTBUILDDIR)/%,$(THRIFTSOURCES_AC:.cpp=.o))

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

succinct-thrift-components: gen-thrift succinct-server succinct-handler succinct-master succinct-client

adaptive-thrift: build-thrift adaptive-thrift-components

adaptive-thrift-components: gen-thrift-adaptive adaptive-server adaptive-handler adaptive-master adaptive-client

gen-thrift:
	@echo " ./bin/thrift -I include/thrift -gen cpp:include_prefix -out thrift thrift/succinct.thrift";\
		./bin/thrift -I include/thrift -gen cpp:include_prefix -out thrift thrift/succinct.thrift
	@echo " $(MV) thrift/*.cpp src/thrift/ && mv thrift/*.h include/thrift/";\
		$(MV) thrift/*.cpp src/thrift/ && mv thrift/*.h include/thrift/
	@echo " $(RM) src/thrift/*skeleton*"; $(RM) src/thrift/*skeleton*
	
gen-thrift-adaptive:
	@echo " ./bin/thrift -I include/thrift -gen cpp:include_prefix -out thrift thrift/adaptive.thrift";\
		./bin/thrift -I include/thrift -gen cpp:include_prefix -out thrift thrift/adaptive.thrift
	@echo " $(MV) thrift/*.cpp src/thrift/ && mv thrift/*.h include/thrift/";\
		$(MV) thrift/*.cpp src/thrift/ && mv thrift/*.h include/thrift/
	@echo " $(RM) src/thrift/*skeleton*"; $(RM) src/thrift/*skeleton*

build-thrift:
	@echo "Building thrift-0.9.1..."
	@echo " cd external/thrift-0.9.1; ./configure CXXFLAGS='-O3 -std=c++11' --prefix=`pwd`/../../ --exec-prefix=`pwd`/../../ --with-qt4=no --with-c_glib=no --with-csharp=no --with-java=no --with-erlang=no --with-python=no --with-perl=no --with-php=no --with-php_extension=no --with-ruby=no --with-haskell=no --with-go=no --with-d=no --without-tests && make && make install";\
		cd external/thrift-0.9.1; ./configure CXXFLAGS='-O3 -std=c++11' --prefix=`pwd`/../../ --exec-prefix=`pwd`/../../ --with-qt4=no --with-c_glib=no --with-csharp=no --with-java=no --with-erlang=no --with-python=no --with-perl=no --with-php=no --with-php_extension=no --with-ruby=no --with-haskell=no --with-go=no --with-d=no --without-tests && make && make install

succinct-server: succinct $(THRIFTTARGET_SS)

succinct-handler: succinct $(THRIFTTARGET_SH)

succinct-master: succinct $(THRIFTTARGET_SM)

adaptive-server: succinct $(THRIFTTARGET_AS)

adaptive-handler: succinct $(THRIFTTARGET_AH)

adaptive-master: succinct $(THRIFTTARGET_AM)

$(THRIFTTARGET_SS): $(THRIFTOBJECTS_SS) $(THRIFTOBJECTS_GEN)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(THRIFTTARGET_SS) $(THRIFTLIB)";\
		$(CC) $^ -o $(THRIFTTARGET_SS) $(THRIFTLIB)
		
$(THRIFTTARGET_SH): $(THRIFTOBJECTS_SH) $(THRIFTOBJECTS_GEN) 
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(THRIFTTARGET_SH) $(THRIFTLIB)";\
		$(CC) $^ -o $(THRIFTTARGET_SH) $(THRIFTLIB)
		
$(THRIFTTARGET_SM): $(THRIFTOBJECTS_SM) $(THRIFTOBJECTS_GEN)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(THRIFTTARGET_SM) $(THRIFTLIB)";\
		$(CC) $^ -o $(THRIFTTARGET_SM) $(THRIFTLIB)
		
$(THRIFTTARGET_AS): $(THRIFTOBJECTS_AS) $(THRIFTOBJECTS_AGEN) 
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(THRIFTTARGET_AS) $(THRIFTLIB)";\
		$(CC) $^ -o $(THRIFTTARGET_AS) $(THRIFTLIB)
		
$(THRIFTTARGET_AH): $(THRIFTOBJECTS_AH) $(THRIFTOBJECTS_AGEN) 
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(THRIFTTARGET_AH) $(THRIFTLIB)";\
		$(CC) $^ -o $(THRIFTTARGET_AH) $(THRIFTLIB)
		
$(THRIFTTARGET_AM): $(THRIFTOBJECTS_AM) $(THRIFTOBJECTS_AGEN) 
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(THRIFTTARGET_AM) $(THRIFTLIB)";\
		$(CC) $^ -o $(THRIFTTARGET_AM) $(THRIFTLIB)

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

adaptive-client: adaptive-client-lib

adaptive-client-lib: $(THRIFTTARGET_AC)

$(THRIFTTARGET_AC): $(THRIFTOBJECTS_AC)
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

lsbench: succinct
	@echo "Building layered shard benchmark..."
	@echo " cd benchmark; make clean && make lsbench";\
		cd benchmark; make clean && make lsbench

surebench: succinct
	@echo "Building sure benchmark..."
	@echo " cd benchmark; make clean && make surebench";\
		cd benchmark; make clean && make surebench

ssbench: succinct-thrift-components
	@echo "Building succinct-server benchmark..."
	@echo " cd benchmark; make clean && make ssbench";\
		cd benchmark; make clean && make ssbench

asbench: adaptive-thrift-components
	@echo "Building adashard benchmark..."
	@echo " cd benchmark; make clean && make asbench";\
		cd benchmark; make clean && make asbench

dlbench: adaptive-thrift-components
	@echo "Building loadbalancer benchmark..."
	@echo " cd benchmark; make clean && make dlbench";\
		cd benchmark; make clean && make dlbench

qbench: adaptive-thrift-components
	@echo "Building queue benchmark..."
	@echo " cd benchmark; make clean && make qbench";\
		cd benchmark; make clean && make qbench

aqsbench: adaptive-thrift-components
	@echo "Building aqs benchmark..."
	@echo " cd benchmark; make clean && make aqsbench";\
		cd benchmark; make clean && make aqsbench

clean:
	@echo "Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIRS) $(TARGETS)"; $(RM) -r $(BUILDDIRS) $(TARGETS)
	@echo "Cleaning thrift..."
	@echo " cd external/thrift-0.9.1; ./cleanup.sh";\
		cd external/thrift-0.9.1; ./cleanup.sh

.PHONY: clean
