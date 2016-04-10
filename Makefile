OPTS=-g -Og
CC=gcc
CFLAGS=$(OPTS) -Wall

OBJS=main.o tinyscript.o

tstest: $(OBJS)
	$(CC) $(CFLAGS) -o tstest $(OBJS)

clean:
	rm -f *.o

test: tstest
	(cd Test; ./runtests.sh)
