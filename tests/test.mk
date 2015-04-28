TESTDIR			:= tests

TEST_SRCDIR		:= $(TESTDIR)/src
TEST_MODULES	:= $(subst src/,,$(shell find $(SRCDIR)/* -type d))
TEST_BINDIR		:= $(TESTDIR)/bin
TEST_BUILDDIR	:= $(TESTDIR)/build
TEST_INCDIRS	:= $(TESTDIR)/include $(INCDIRS)
TEST_TARGET		:= $(TEST_BINDIR)/test_pumalist

TEST_BUILDMODS	:= $(addprefix $(TEST_BUILDDIR)/,$(TEST_MODULES))
TEST_BINMODS	:= $(addprefix $(TEST_BINDIR)/,$(TEST_MODULES))

TEST_CSRCS		:= $(shell find $(TEST_SRCDIR) -name '*.c')
TEST_CXXSRCS	:= $(shell find $(TEST_SRCDIR) -name '*.cpp')
TEST_SRCS		:= $(TEST_CSRCS) $(TEST_CXXSRCS)
TEST_HEADERS	:= $(shell find $(TEST_INCDIRS) -name '*.h')

TEST_COBJECTS 	:= $(subst $(TEST_SRCDIR),$(TEST_BUILDDIR),$(TEST_CSRCS:%.c=%.c.o))
TEST_CXXOBJECTS	:= $(subst $(TEST_SRCDIR),$(TEST_BUILDDIR),$(TEST_CXXSRCS:%.cpp=%.cpp.o))
TEST_OBJECTS	:= $(TEST_COBJECTS) $(TEST_CXXOBJECTS)

TEST_LDFLAGS	:= -L$(BINDIR) -lpumalist -shared-intel -openmp

TEST_INCFLAGS	:= $(addprefix -I,$(TEST_INCDIRS))

TEST_DEFS		:=

TEST_CFLAGS		= -std=gnu99 -Wall -Wextra -Werror -pedantic -O2
TEST_CXXFLAGS	= -std=c++11 -Wall -Wextra -Werror -pedantic -O2

TEST_FOLDERS	= $(TEST_BINDIR) $(TEST_BINMODS) $(TEST_BUILDDIR) $(TEST_BUILDMODS)

.PHONY: test test_clean

test: $(TEST_TARGET)
	@echo "Running tests..."
	@LD_LIBRARY_PATH=$(LD_LIBRARY_PATH):$(BINDIR) $(TEST_TARGET)

$(TEST_FOLDERS):
	@mkdir -p $(TEST_FOLDERS)

$(TEST_TARGET): $(TEST_OBJECTS) $(TARGET)
	@echo "Linking $@"
	$(LINKER) -o $@ $(TEST_OBJECTS) $(TEST_LDFLAGS)

$(TEST_BUILDDIR)/%.cpp.o: $(TEST_SRCDIR)/%.cpp $(TEST_FOLDERS)
	@echo "Compiling $<"
	@$(CXX) $(TEST_INCFLAGS) $(TEST_DEFS) $(TEST_CXXFLAGS) -c $< -o $@

$(TEST_BUILDDIR)/%.c.o: $(TEST_SRCDIR)/%.c $(TEST_FOLDERS)
	@echo "Compiling $<"
	@$(CC) $(TEST_INCFLAGS) $(TEST_DEFS) $(TEST_CFLAGS) -c $< -o $@

$(TEST_SRCS): $(TEST_HEADERS)

test_clean:
	@rm -rf $(TEST_BUILDDIR) $(TEST_BINDIR)