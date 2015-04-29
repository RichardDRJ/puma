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

CFLAGS		= -axSSE4.1 -std=gnu99 -Wunused-variable -O2 -DNOVALGRIND -DNDEBUG -openmp -fPIC
CXXFLAGS	= -axSSE4.1 -std=c++11 -Wunused-variable -O2 -DNOVALGRIND -DNDEBUG -openmp -fPIC

LINKER		= icc
LDFLAGS		= -openmp -shared -shared-intel

FOLDERS		= $(BINDIR) $(BINMODS) $(BUILDDIR) $(BUILDMODS)

OS := $(shell uname -s)
ifeq ($(OS),Linux)
	LDFLAGS +=		-lnuma
	CC			= icc -x c
	CXX			= icc -x c++ -cxxlib
	EXT			= so
else ifeq ($(OS),Darwin)
	CFLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk/usr/include
	CXXFLAGS += -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk/usr/include
	LDFLAGS += -L/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk/usr/lib
	CYTHONINCS += /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk/usr/include
	CYTHONLIBS += /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk/usr/lib
	CFLAGS 		+= -DNNUMA
	CC			= icc
	CXX			= icc -cxxlib
	EXT			= dylib
endif

TARGET		:= $(BINDIR)/libpumalist.$(EXT)

include tests/test.mk

.PHONY: all clean

all: $(TARGET) test

$(FOLDERS):
	@mkdir -p $(FOLDERS)

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

clean: test_clean
	@echo "Cleaning working tree"
	@rm -rf $(BUILDDIR) $(BINDIR)
