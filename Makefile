OPTS=-g -Og
CC=gcc
CFLAGS=$(OPTS) -Wall

OBJS=main.o tinyscript.o

tstest: $(OBJS)
	$(CC) $(CFLAGS) -o tstest $(OBJS)

clean:
	rm -f *.o *.elf

test: tstest
	(cd Test; ./runtests.sh)

fibo.elf: fibo.c fibo.h tinyscript.c
	propeller-elf-gcc -o fibo.elf -mlmm -Os fibo.c fibo.h tinyscript.c

fibo.h: fibo.ts
	xxd -i fibo.ts > fibo.h
