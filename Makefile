CC=gcc
CFLAGS=-g -Og -Wall

OBJS=main.o tinyscript.o

tstest: $(OBJS)
	$(CC) $(CFLAGS) -o tstest $(OBJS)

clean:
	rm -f *.o

test: tstest
	(cd Test; ./runtests.sh)
