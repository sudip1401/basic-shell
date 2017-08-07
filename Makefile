CC = gcc

all: osh

osh: shell.o parser.o executor.o
	$(CC) -o osh shell.o parser.o executor.o

shell.o: shell.c shell.h defs.h
	$(CC) -c shell.c -o shell.o

parser.o: parser.c parser.h defs.h
	$(CC) -c parser.c -o parser.o

executor.o: executor.c executor.h defs.h
	$(CC) -c executor.c -o executor.o

clean:
	rm -f *.o *.out *~ osh
