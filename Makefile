# define the library you're using for READLINE by selecting the
# appropriate two lines

# for GNU Readline
#READLINE=-lreadline
#READLINE_DEFS=-DREADLINE

# for linenoise
#READLINE=linenoise.o
#READLINE_DEFS=-DLINENOISE

# for neither, just leave both undefined

OPTS=-g -Og
CC=gcc
CFLAGS=$(OPTS) $(READLINE_DEFS) -Wall

OBJS=main.o tinyscript.o tinyscript_lib.o

tstest: $(OBJS) $(READLINE)
	$(CC) $(CFLAGS) -o tstest $(OBJS) $(READLINE)

clean:
	rm -f *.o *.elf

test: tstest
	(cd Test; ./runtests.sh)

fibo.elf: fibo.c fibo.h tinyscript.c
	propeller-elf-gcc -o fibo.elf -mlmm -Os fibo.c fibo.h tinyscript.c

fibo.h: fibo.ts
	xxd -i fibo.ts > fibo.h
