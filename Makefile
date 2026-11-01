# ucmdline makefile (largely tests)

CC_GCC ?= gcc
CC_CLANG ?= clang

WARN := -Wall -Wextra -Werror -Wshadow -Wpointer-arith -Wstrict-prototypes
CFLAGS := -std=c89 -pedantic $(WARN)

CLANG_EXTRA := -Wno-c99-extensions # Address clang warning on 'bool' (stdbool.h)

BUILD := build
GCCDIR := $(BUILD)/gcc
CLANGDIR := $(BUILD)/clang

.PHONY: test clean

test: $(GCCDIR)/test_ucmdline $(CLANGDIR)/test_ucmdline
	$(GCCDIR)/test_ucmdline
	$(CLANGDIR)/test_ucmdline

$(GCCDIR)/ucmdline.o: ucmdline.c ucmdline.h
	@mkdir -p $(GCCDIR)
	$(CC_GCC) $(CFLAGS) -O2 -c ucmdline.c -o $@

$(GCCDIR)/test_ucmdline: tests/test_ucmdline.c $(GCCDIR)/ucmdline.o
	$(CC_GCC) $(CFLAGS) -O2 -I. tests/test_ucmdline.c $(GCCDIR)/ucmdline.o -o $@

$(CLANGDIR)/ucmdline.o: ucmdline.c ucmdline.h
	@mkdir -p $(CLANGDIR)
	$(CC_CLANG) $(CFLAGS) $(CLANG_EXTRA) -O2 -c ucmdline.c -o $@

$(CLANGDIR)/test_ucmdline: tests/test_ucmdline.c $(CLANGDIR)/ucmdline.o
	$(CC_CLANG) $(CFLAGS) $(CLANG_EXTRA) -O2 -I. tests/test_ucmdline.c $(CLANGDIR)/ucmdline.o -o $@

clean:
	rm -rf $(BUILD)
