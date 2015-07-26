CC := g++

SRCDIR := src
BUILDDIR := ../build/test
LIBDIR := ../lib
TARGETDIR := ../bin

SRCDIRS := $(shell find $(SRCDIR) -type d)
BUILDDIRS := $(subst $(SRCDIR),$(BUILDDIR),$(SRCDIRS))

SOURCES := $(shell find $(SRCDIR) -type f -name *.cpp)
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.cpp=.o))
CFLAGS := -O3 -std=c++11 -Wall -Werror -g
LIB := -L $(LIBDIR) -l gtest_main -l succinct
INC := -I ../include -I ../external/gtest-1.7.0/include

all: $(TARGETDIR)/core-test
	@echo "Running Core test..."
	@echo " $(TARGETDIR)/core-test"; $(TARGETDIR)/core-test

$(TARGETDIR)/core-test : $(OBJECTS)
	@echo "Linking..."
	@echo " $(CC) $^ -o $(TARGETDIR)/core-test $(LIB)";\
		$(CC) $^ -o $(TARGETDIR)/core-test $(LIB)
	
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIRS)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<";\
		$(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo "Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR) $(TARGETDIR)/*-test";\
		$(RM) -r $(BUILDDIR) $(TARGETDIR)/*-test
	
.PHONY: clean
