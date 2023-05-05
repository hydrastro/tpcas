CC=gcc
CFLAGS=-g -Wall
TARGET=tpcas
SOURCES=main.c pl.c repl.c
OBJECTS=$(SOURCES:.c=.o)
LIBRARIES=-lmpfr

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET) $(LIBRARIES)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

