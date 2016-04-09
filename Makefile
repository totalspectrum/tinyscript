CC=gcc
CFLAGS=-g -Og -Wall

OBJS=test.o tinyscript.o

test: $(OBJS)
	$(CC) $(CFLAGS) -o test $(OBJS)

clean:
	rm -f *.o
