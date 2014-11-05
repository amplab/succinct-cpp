CC := g++

SRCDIR := ../benchmark
BUILDDIR := ../build/bench
LIBDIR := ../lib
TARGET_S := ../bin/sbench
TARGET_SS := ../bin/ssbench

SRC_S := $(SRCDIR)/succinct-bench.cpp
SRC_SS := $(SRCDIR)/succinctserver-bench.cpp

OBJ_S := $(BUILDDIR)/succinct-bench.o
OBJ_SS := $(BUILDDIR)/succinctserver-bench.o

CFLAGS := -O3 -std=c++11 -Wall -g
LIB := -L $(LIBDIR) -lsuccinct -levent -lthrift -lthriftnb -lsuccinctclient
INC := -I ../include

all: sbench ssbench

sbench: $(TARGET_S)

ssbench: $(TARGET_SS)

$(TARGET_S): $(OBJ_S)
	@echo "Linking..."
	@echo " $(CC) $^ -o $(TARGET_S) $(LIB)"; $(CC) $^ -o $(TARGET_S) $(LIB)
	
$(TARGET_SS): $(OBJ_SS)
	@echo "Linking..."
	@echo " $(CC) $^ -o $(TARGET_SS) $(LIB)"; $(CC) $^ -o $(TARGET_SS) $(LIB)
	
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo "Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)
	
.PHONY: clean