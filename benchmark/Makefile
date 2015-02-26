CC := g++

SRCDIR := src
BUILDDIR := ../build/bench
BINDIR := ../bin
LIBDIR := ../lib

TARGET_S := $(BINDIR)/sbench
TARGET_F := $(BINDIR)/fbench
TARGET_SS := $(BINDIR)/ssbench
TARGET_SURE := $(BINDIR)/surebench

OBJ_S := $(BUILDDIR)/shard-bench.o
OBJ_F := $(BUILDDIR)/file-bench.o
OBJ_SS := $(BUILDDIR)/succinctserver-bench.o
OBJ_SURE := $(BUILDDIR)/sure-bench.o

CFLAGS := -O3 -std=c++11 -Wall -g
LIB := -L $(LIBDIR) -lsuccinct
LIBT := -L $(LIBDIR) -lsuccinct -levent -lthrift -lthriftnb -lsuccinctclient
INC := -I ../include -I include

all: sbench fbench ssbench surebench

sbench: $(TARGET_S)

fbench: $(TARGET_F)

ssbench: $(TARGET_SS)

surebench: $(TARGET_SURE)

$(TARGET_S): $(OBJ_S)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(TARGET_S) $(LIB)"; $(CC) $^ -o $(TARGET_S) $(LIB)

$(TARGET_F): $(OBJ_F)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(TARGET_F) $(LIB)"; $(CC) $^ -o $(TARGET_F) $(LIB)
	
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
	@echo " $(RM) -r $(BUILDDIR) $(TARGET_S) $(TARGET_F) $(TARGET_SS) $(TARGET_SURE)";\
		$(RM) -r $(BUILDDIR) $(TARGET_S) $(TARGET_F) $(TARGET_SS) $(TARGET_SURE)
	
.PHONY: clean
