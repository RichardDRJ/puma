.DEFAULT_GOAL := all

WORKINGDIR	:= $(shell pwd)

SRCDIR		:= src
MODULES		:= $(subst src/,,$(shell find $(SRCDIR)/* -type d))
BINDIR		:= bin
BUILDDIR	:= build
INCDIRS		:= include

DOCDIR		:= docs

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

CFLAGS		= -std=gnu99 -Wunused-variable -g -fPIC -DNOVALGRIND -DNDEBUG -O2
CXXFLAGS	= -std=c++11 -Wunused-variable -g -fPIC -DNOVALGRIND -DNDEBUG -O2

LDFLAGS		= -shared -pthread

FOLDERS		= $(BINDIR) $(BINMODS) $(BUILDDIR) $(BUILDMODS)

DOXYGEN_CONF	:= doxygen.cfg

ifdef PUMA_NODEPAGES
	CFLAGS		+= -DPUMA_NODEPAGES=$(PUMA_NODEPAGES)
	CXXFLAGS	+= -DPUMA_NODEPAGES=$(PUMA_NODEPAGES)
endif

ifdef STATIC_THREADPOOL
	CFLAGS		+= -DSTATIC_THREADPOOL
	CXXFLAGS	+= -DSTATIC_THREADPOOL
endif

ifdef NOOPENMP
	CFLAGS		+= -DNOOPENMP
else
	CFLAGS		+= -openmp
	LDFLAGS 	+= -openmp
endif

OS := $(shell uname -s)
ifeq ($(OS),Linux)
	CFLAGS		+= -axSSE4.1
	CXXFLAGS	+= -axSSE4.1
	LDFLAGS		+= -lnuma -lrt -shared-intel
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

.PHONY: all clean no_test doc doc_clean

all: $(TARGET) doc test

no_test: $(TARGET)

$(FOLDERS):
	@mkdir -p $(FOLDERS)

doc:
	@echo "Running doxygen"
	@doxygen $(DOXYGEN_CONF) 1> /dev/null

$(TARGET): $(OBJECTS) | $(FOLDERS)
	@echo "Linking $@"
	@$(LINKER) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.cpp.o: src/%.cpp | $(FOLDERS)
	@echo "Compiling $<"
	@$(CXX) $(INCFLAGS) $(DEFS) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.c.o: src/%.c | $(FOLDERS)
	@echo "Compiling $<"
	@$(CC) $(INCFLAGS) $(DEFS) $(CFLAGS) -c $< -o $@

$(SRCS): $(HEADERS)

docs_clean:
	@rm -rf $(DOCDIR)

clean: test_clean doc_clean
	@echo "Cleaning working tree"
	@rm -rf $(BUILDDIR) $(BINDIR)
