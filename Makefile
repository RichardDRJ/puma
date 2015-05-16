WORKINGDIR	:= $(shell pwd)

SRCDIR		:= src
MODULES		:= $(subst src/,,$(shell find $(SRCDIR)/* -type d))
BINDIR		:= bin
BUILDDIR	:= build
INCDIRS		:= include

BUILDMODS	:= $(addprefix $(BUILDDIR)/,$(MODULES))
BINMODS		:= $(addprefix $(BINDIR)/,$(MODULES))

CSRCS		:= $(shell find $(SRCDIR) -name '*.c')
CXXSRCS		:= $(shell find $(SRCDIR) -name '*.cpp')
SRCS		:= $(CSRCS) $(CXXSRCS)

HEADERS		:= $(shell find $(INCDIRS) -name '*.h')

COBJECTS 	:= $(subst src,$(BUILDDIR),$(CSRCS:%.c=%.c.o))
CXXOBJECTS 	:= $(subst src,$(BUILDDIR),$(CXXSRCS:%.cpp=%.cpp.o))
OBJECTS		:= $(COBJECTS) $(CXXOBJECTS)


INCFLAGS	= $(addprefix -I,$(INCDIRS))

CFLAGS		= -std=gnu99 -Wunused-variable -O2 -g -DNDEBUG -fPIC
CXXFLAGS	= -std=c++11 -Wunused-variable -O2 -g -DNDEBUG -fPIC

LDFLAGS		= -shared -pthread

FOLDERS		= $(BINDIR) $(BINMODS) $(BUILDDIR) $(BUILDMODS)

ifdef PUMA_NODEPAGES
	CFLAGS		+= -DPUMA_NODEPAGES=$(PUMA_NODEPAGES)
	CXXFLAGS	+= -DPUMA_NODEPAGES=$(PUMA_NODEPAGES)
endif

OS := $(shell uname -s)
ifeq ($(OS),Linux)
	CFLAGS		+= -axSSE4.1
	CXXFLAGS	+= -axSSE4.1
	LDFLAGS		+= -lnuma -lrt -static-intel
	CC			= icc -x c
	CXX			= icc -x c++ -cxxlib
	LINKER		= icc
	EXT			= so
else ifeq ($(OS),Darwin)
	CFLAGS 		+= -DNNUMA -DNOVALGRIND
	CXXFLAGS	+= -DNNUMA -DNOVALGRIND
	CC			= gcc
	CXX			= g++
	LINKER		= gcc
	EXT			= dylib
endif

TARGET		:= $(BINDIR)/libpumalist.$(EXT)

include tests/test.mk

.PHONY: all clean no_test

all: $(TARGET) test

no_test: $(TARGET)

$(FOLDERS):
	@mkdir -p $(FOLDERS)

$(TARGET): $(OBJECTS) | $(FOLDERS)
	echo "Linking $@"
	$(LINKER) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.cpp.o: src/%.cpp | $(FOLDERS)
	@echo "Compiling $<"
	@$(CXX) $(INCFLAGS) $(DEFS) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.c.o: src/%.c | $(FOLDERS)
	@echo "Compiling $<"
	@$(CC) $(INCFLAGS) $(DEFS) $(CFLAGS) -c $< -o $@

$(SRCS): $(HEADERS)

clean: test_clean
	@echo "Cleaning working tree"
	@rm -rf $(BUILDDIR) $(BINDIR)
