# tpcas — out-of-tree build
#
# Common targets:
#   make              build build/tpcas
#   make test         run built-in parser comparison suite
#   make run ARGS='forall x. P(x)'   parse one expression
#   make clean        remove build products only
#   make prune-legacy remove known pre-src/ migration garbage from repo root

CC       := cc
AR       ?= ar
RM       ?= rm -f
RMDIR    ?= rm -rf
MKDIR_P  ?= mkdir -p

SRC_DIR    := src
VENDOR_DIR := vendor/ds/lib
BUILD_DIR  := build
OBJ_DIR    := $(BUILD_DIR)/obj
DEP_DIR    := $(BUILD_DIR)/dep
TARGET     := $(BUILD_DIR)/tpcas

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

TPCAS_SRCS := \
  $(SRC_DIR)/arena.c \
  $(SRC_DIR)/ast.c \
  $(SRC_DIR)/lex.c \
  $(SRC_DIR)/print.c \
  $(SRC_DIR)/pratt.c \
  $(SRC_DIR)/pc.c \
  $(SRC_DIR)/combo.c \
  $(SRC_DIR)/main.c

DS_SRCS := \
  $(VENDOR_DIR)/common.c \
  $(VENDOR_DIR)/status.c \
  $(VENDOR_DIR)/error.c \
  $(VENDOR_DIR)/diagnostic.c \
  $(VENDOR_DIR)/context.c \
  $(VENDOR_DIR)/allocators.c \
  $(VENDOR_DIR)/str.c

SRCS := $(TPCAS_SRCS) $(DS_SRCS)
OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))
DEPS := $(patsubst %.c,$(DEP_DIR)/%.d,$(SRCS))

.PHONY: all test run clean distclean prune-legacy format help print-vars

all: $(TARGET)

$(TARGET): $(OBJS)
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(OBJ_DIR)/%.o: %.c
	@$(MKDIR_P) $(dir $@) $(dir $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@))
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF $(patsubst $(OBJ_DIR)/%.o,$(DEP_DIR)/%.d,$@) -c $< -o $@

test: $(TARGET)
	./$(TARGET)

run: $(TARGET)
	./$(TARGET) $(ARGS)

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
	clang-format -i $(TPCAS_SRCS) $(SRC_DIR)/*.h

print-vars:
	@echo "CC=$(CC)"
	@echo "MODE=$(MODE)"
	@echo "TARGET=$(TARGET)"
	@echo "SRCS=$(SRCS)"

help:
	@sed -n '1,14p' Makefile

-include $(DEPS)
