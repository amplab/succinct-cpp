CC := g++

SRCDIR := ../benchmark
BUILDDIR := ../build/bench
LIBDIR := ../lib
TARGET := ../bin/succinct-bench

SRCDIRS := $(shell find $(SRCDIR) -type d)
BUILDDIRS := $(subst $(SRCDIR),$(BUILDDIR),$(SRCDIRS))

SOURCES := $(shell find $(SRCDIR) -type f -name *.cpp)
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.cpp=.o))
CFLAGS := -O3 -std=c++11 -Wall -Werror
LIB := -L $(LIBDIR) -l succinct
INC := -I ../include

$(TARGET) : $(OBJECTS)
	@echo "Linking..."
	@echo " $(CC) $^ -o $(TARGET) $(LIB)"; $(CC) $^ -o $(TARGET) $(LIB)
	
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIRS)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo "Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)
	
.PHONY: clean