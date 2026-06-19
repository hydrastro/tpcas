# tpcas — out-of-tree build
#
# ds (github:hydrastro/ds) is provided as an external dependency. Inside
# `nix develop` / the flake build, DS_CFLAGS and DS_LIBS are exported for you.
# Outside Nix, point them at an installed ds, e.g.:
#   make DS_CFLAGS=-I/usr/local/include DS_LIBS='-L/usr/local/lib -lds'
#
# Common targets:
#   make              build build/tpcas and build/libtpcas.a
#   make test         run built-in parser comparison suite
#   make run ARGS='forall x. P(x)'   parse one expression
#   make install PREFIX=/some/prefix
#   make clean        remove build products

CC       ?= cc
CXX      ?= c++
AR       ?= ar
RM       ?= rm -f
RMDIR    ?= rm -rf
MKDIR_P  ?= mkdir -p
INSTALL  ?= install

SRC_DIR    := src
APP_DIR    := app
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

# ds dependency. Defaults assume a standard prefix install; the flake
# overrides these to point at the ds package in the Nix store.
DS_CFLAGS ?=
DS_LIBS   ?= -lds

WARNINGS := -Wall -Wextra -Wpedantic
STD      := -std=c11
CPPFLAGS ?=
CPPFLAGS += -I$(SRC_DIR) $(DS_CFLAGS)
CFLAGS   ?=
CFLAGS   += $(STD) $(WARNINGS)
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic
LDFLAGS  ?=
LDLIBS   ?=
LDLIBS   += $(DS_LIBS)

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

TPCAS_MAIN_SRCS := $(APP_DIR)/tpcas.c

LIB_SRCS := $(TPCAS_LIB_SRCS)
SRCS     := $(LIB_SRCS) $(TPCAS_MAIN_SRCS)
LIB_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))
MAIN_OBJS:= $(patsubst %.c,$(OBJ_DIR)/%.o,$(TPCAS_MAIN_SRCS))
OBJS     := $(LIB_OBJS) $(MAIN_OBJS)
DEPS     := $(patsubst %.c,$(DEP_DIR)/%.d,$(SRCS))

.PHONY: all check test check-cpp run clean format install uninstall help print-vars

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

check: test check-cpp

$(BUILD_DIR)/cpp-smoke: test/cpp_smoke.cpp $(STATIC_LIB)
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@ $< $(STATIC_LIB) $(LDLIBS)

check-cpp: $(BUILD_DIR)/cpp-smoke
	./$(BUILD_DIR)/cpp-smoke

run: $(TARGET)
	./$(TARGET) $(ARGS)

install: all
	$(INSTALL) -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCDIR)
	$(INSTALL) -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)/tpcas
	$(INSTALL) -m 0644 $(STATIC_LIB) $(DESTDIR)$(LIBDIR)/libtpcas.a
	$(INSTALL) -m 0644 $(SRC_DIR)/*.h $(DESTDIR)$(INCDIR)/

uninstall:
	$(RM) $(DESTDIR)$(BINDIR)/tpcas $(DESTDIR)$(LIBDIR)/libtpcas.a
	$(RMDIR) $(DESTDIR)$(INCDIR)

clean:
	$(RMDIR) $(BUILD_DIR)

format:
	clang-format -i $(TPCAS_LIB_SRCS) $(TPCAS_MAIN_SRCS) $(SRC_DIR)/*.h

print-vars:
	@echo "CC=$(CC)"
	@echo "MODE=$(MODE)"
	@echo "TARGET=$(TARGET)"
	@echo "STATIC_LIB=$(STATIC_LIB)"
	@echo "DS_CFLAGS=$(DS_CFLAGS)"
	@echo "DS_LIBS=$(DS_LIBS)"
	@echo "SRCS=$(SRCS)"

help:
	@sed -n '1,20p' Makefile

-include $(DEPS)
