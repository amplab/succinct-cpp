CC := g++

SRCDIR := ../benchmark
BUILDDIR := ../build/bench
BINDIR := ../bin
LIBDIR := ../lib
TARGET_S := $(BINDIR)/sbench
TARGET_SS := $(BINDIR)/ssbench
TARGET_SURE := $(BINDIR)/surebench

OBJ_S := $(BUILDDIR)/succinct-bench.o
OBJ_SS := $(BUILDDIR)/succinctserver-bench.o
OBJ_SURE := $(BUILDDIR)/sure-bench.o

CFLAGS := -O3 -std=c++11 -Wall -g
LIB := -L $(LIBDIR) -lsuccinct
LIBT := -L $(LIBDIR) -lsuccinct -levent -lthrift -lthriftnb -lsuccinctclient
INC := -I ../include

all: sbench ssbench surebench

sbench: $(TARGET_S)

ssbench: $(TARGET_SS)

surebench: $(TARGET_SURE)

$(TARGET_S): $(OBJ_S)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(TARGET_S) $(LIB)"; $(CC) $^ -o $(TARGET_S) $(LIB)
	
$(TARGET_SS): $(OBJ_SS)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(TARGET_SS) $(LIB)"; $(CC) $^ -o $(TARGET_SS) $(LIBT)
	
$(TARGET_SURE): $(OBJ_SURE)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(TARGET_SURE) $(LIB)"; $(CC) $^ -o $(TARGET_SURE) $(LIB)
	
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo "Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)
	
.PHONY: clean