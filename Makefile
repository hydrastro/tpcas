CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -g -O0
LDFLAGS =

OBJS = arena.o ast.o lex.o print.o pratt.o pc.o combo.o main.o

tpcas: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o tpcas $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

arena.o:  arena.c arena.h
ast.o:    ast.c ast.h
lex.o:    lex.c lex.h ast.h
print.o:  print.c print.h ast.h
pratt.o:  pratt.c pratt.h lex.h ast.h arena.h
pc.o:     pc.c pc.h ast.h arena.h
combo.o:  combo.c combo.h pc.h ast.h arena.h
main.o:   main.c pratt.h combo.h print.h ast.h arena.h

test: tpcas
	./tpcas

clean:
	rm -f *.o tpcas

.PHONY: test clean
