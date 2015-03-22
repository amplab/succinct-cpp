CC := g++

SRCDIR := src
BUILDDIR := ../build/bench
BINDIR := ../bin
LIBDIR := ../lib

TARGET_S := $(BINDIR)/sbench
TARGET_F := $(BINDIR)/fbench
TARGET_SS := $(BINDIR)/ssbench
TARGET_AS := $(BINDIR)/asbench
TARGET_Q := $(BINDIR)/qbench
TARGET_AQS := $(BINDIR)/aqsbench
TARGET_SURE := $(BINDIR)/surebench

OBJ_S := $(BUILDDIR)/shard-bench.o
OBJ_F := $(BUILDDIR)/file-bench.o
OBJ_SS := $(BUILDDIR)/succinctserver-bench.o
OBJ_AS := $(BUILDDIR)/adashard-bench.o
OBJ_Q := $(BUILDDIR)/queue-bench.o
OBJ_AQS := $(BUILDDIR)/aqs-bench.o
OBJ_SURE := $(BUILDDIR)/sure-bench.o

CFLAGS := -O3 -std=c++11 -Wall -g
LIB := -L $(LIBDIR) -lsuccinct
LIBT := -L $(LIBDIR) -lsuccinct -levent -lthrift -lthriftnb -lsuccinctclient
LIBA := -L $(LIBDIR) -lsuccinct -levent -lthrift -lthriftnb -ladaptsuccinctclient
INC := -I ../include -I include

all: sbench fbench ssbench asbench surebench

sbench: $(TARGET_S)

fbench: $(TARGET_F)

ssbench: $(TARGET_SS)

asbench: $(TARGET_AS)

qbench: $(TARGET_Q)

aqsbench: $(TARGET_AQS)

surebench: $(TARGET_SURE)

$(TARGET_S): $(OBJ_S)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(TARGET_S) $(LIB)"; $(CC) $^ -o $(TARGET_S) $(LIB)

$(TARGET_F): $(OBJ_F)
	@echo "Linking.."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(TARGET_F) $(LIB)"; $(CC) $^ -o $(TARGET_F) $(LIB)
	
$(TARGET_SS): $(OBJ_SS)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(TARGET_SS) $(LIBT)"; $(CC) $^ -o $(TARGET_SS) $(LIBT)

$(TARGET_AS): $(OBJ_AS)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(TARGET_AS) $(LIBA)"; $(CC) $^ -o $(TARGET_AS) $(LIBA)

$(TARGET_Q): $(OBJ_Q)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(TARGET_Q) $(LIBA)"; $(CC) $^ -o $(TARGET_Q) $(LIBA)

$(TARGET_AQS): $(OBJ_AQS)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(TARGET_AQS) $(LIBA)"; $(CC) $^ -o $(TARGET_AQS) $(LIBA)

$(TARGET_SURE): $(OBJ_SURE)
	@echo "Linking..."
	@mkdir -p $(BINDIR)
	@echo " $(CC) $^ -o $(TARGET_SURE) $(LIB)"; $(CC) $^ -o $(TARGET_SURE) $(LIB)
	
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo "Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR) $(TARGET_S) $(TARGET_F) $(TARGET_SS) $(TARGET_AS) $(TARGET_SURE) $(TARGET_AQS)";\
		$(RM) -r $(BUILDDIR) $(TARGET_S) $(TARGET_F) $(TARGET_SS) $(TARGET_AS) $(TARGET_SURE) $(TARGET_AQS)
	
.PHONY: clean
