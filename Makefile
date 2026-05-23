CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -g -O0
LDFLAGS =

OBJS = arena.o ast.o lex.o parse.o print.o eval.o transform.o repl.o main.o

all: tpcas

tpcas: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) tpcas

test: tpcas
	@./run_tests.sh

.PHONY: all clean test
