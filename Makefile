# tpcas — out-of-tree build
#
# Common targets:
#   make              build build/tpcas and build/libtpcas.a
#   make test         run built-in parser comparison suite
#   make run ARGS='forall x. P(x)'   parse one expression
#   make install PREFIX=/some/prefix
#   make clean        remove build products only
#   make prune-legacy remove known pre-src/ migration garbage from repo root

CC       ?= cc
AR       ?= ar
RM       ?= rm -f
RMDIR    ?= rm -rf
MKDIR_P  ?= mkdir -p
INSTALL  ?= install

SRC_DIR    := src
VENDOR_DIR := vendor/ds/lib
BUILD_DIR  := build
OBJ_DIR    := $(BUILD_DIR)/obj
DEP_DIR    := $(BUILD_DIR)/dep
TARGET     := $(BUILD_DIR)/tpcas
STATIC_LIB := $(BUILD_DIR)/libtpcas.a

PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
LIBDIR  ?= $(PREFIX)/lib
INCDIR  ?= $(PREFIX)/include/tpcas

MODE ?= debug

WARNINGS := -Wall -Wextra -Wpedantic
STD      := -std=c11
CPPFLAGS ?=
CPPFLAGS += -I$(SRC_DIR) -Ivendor/ds
CFLAGS   ?=
CFLAGS   += $(STD) $(WARNINGS)
LDFLAGS  ?=
LDLIBS   ?=

ifeq ($(MODE),release)
  CFLAGS += -O2 -DNDEBUG
else ifeq ($(MODE),asan)
  CFLAGS += -O1 -g3 -fno-omit-frame-pointer -fsanitize=address,undefined
  LDFLAGS += -fsanitize=address,undefined
else
  CFLAGS += -O0 -g3
endif

TPCAS_LIB_SRCS := \
  $(SRC_DIR)/arena.c \
  $(SRC_DIR)/ast.c \
  $(SRC_DIR)/lex.c \
  $(SRC_DIR)/print.c \
  $(SRC_DIR)/pratt.c \
  $(SRC_DIR)/pc.c \
  $(SRC_DIR)/combo.c

TPCAS_MAIN_SRCS := $(SRC_DIR)/main.c

DS_SRCS := \
  $(VENDOR_DIR)/common.c \
  $(VENDOR_DIR)/status.c \
  $(VENDOR_DIR)/error.c \
  $(VENDOR_DIR)/diagnostic.c \
  $(VENDOR_DIR)/context.c \
  $(VENDOR_DIR)/allocators.c \
  $(VENDOR_DIR)/str.c

LIB_SRCS := $(TPCAS_LIB_SRCS) $(DS_SRCS)
SRCS     := $(LIB_SRCS) $(TPCAS_MAIN_SRCS)
LIB_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))
MAIN_OBJS:= $(patsubst %.c,$(OBJ_DIR)/%.o,$(TPCAS_MAIN_SRCS))
OBJS     := $(LIB_OBJS) $(MAIN_OBJS)
DEPS     := $(patsubst %.c,$(DEP_DIR)/%.d,$(SRCS))

.PHONY: all test run clean distclean prune-legacy format install uninstall help print-vars

all: $(TARGET) $(STATIC_LIB)

$(TARGET): $(MAIN_OBJS) $(STATIC_LIB)
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(MAIN_OBJS) $(STATIC_LIB) $(LDLIBS)

$(STATIC_LIB): $(LIB_OBJS)
	@$(MKDIR_P) $(dir $@)
	$(AR) rcs $@ $^

$(OBJ_DIR)/%.o: %.c
	@$(MKDIR_P) $(dir $@) $(dir $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@))
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) -c $< -o $@

test: $(TARGET)
	./$(TARGET)

run: $(TARGET)
	./$(TARGET) $(ARGS)

install: all
	$(INSTALL) -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCDIR) $(DESTDIR)$(INCDIR)/lib
	$(INSTALL) -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)/tpcas
	$(INSTALL) -m 0644 $(STATIC_LIB) $(DESTDIR)$(LIBDIR)/libtpcas.a
	$(INSTALL) -m 0644 $(SRC_DIR)/*.h $(DESTDIR)$(INCDIR)/
	$(INSTALL) -m 0644 $(VENDOR_DIR)/*.h $(DESTDIR)$(INCDIR)/lib/

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/tpcas $(DESTDIR)$(LIBDIR)/libtpcas.a
	$(RMDIR) $(DESTDIR)$(INCDIR)

clean:
	$(RMDIR) $(BUILD_DIR)

# Removes known junk from the old flat tree. The script runs in dry-run mode
# unless CLEAN_APPLY=1 is passed.
prune-legacy:
	@if [ "$(CLEAN_APPLY)" = "1" ]; then \
		sh scripts/clean-tree.sh --apply; \
	else \
		sh scripts/clean-tree.sh; \
	fi

# distclean means generated products + legacy migration junk.
distclean: clean
	$(MAKE) prune-legacy CLEAN_APPLY=1

format:
	clang-format -i $(TPCAS_LIB_SRCS) $(TPCAS_MAIN_SRCS) $(SRC_DIR)/*.h

print-vars:
	@echo "CC=$(CC)"
	@echo "MODE=$(MODE)"
	@echo "TARGET=$(TARGET)"
	@echo "STATIC_LIB=$(STATIC_LIB)"
	@echo "SRCS=$(SRCS)"

help:
	@sed -n '1,15p' Makefile

-include $(DEPS)
