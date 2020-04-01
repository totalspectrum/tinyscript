#include <stdio.h>
#include <stdlib.h>
#include "tinyscript.h"
#include "fibo.h"

#include <propeller.h>
#define ARENA_SIZE 4096

int inchar() {
    return -1;
}
void outchar(int c) {
    putchar(c);
}

static Val getcnt_fn()
{
#ifdef CNT
    return CNT;
#else
    return _cnt();
#endif
}
static Val waitcnt_fn(Val when)
{
    waitcnt(when);
    return when;
}
static Val pinout_fn(Val pin, Val onoff)
{
#if defined(__riscv) || defined(__propeller2__)
    _pinw(pin, onoff);
#else
    unsigned mask = 1<<pin;
    DIRA |= mask;
    if (onoff) {
        OUTA |= mask;
    } else {
        OUTA &= ~mask;
    }
#endif    
    return onoff;
}
static Val pinin_fn(Val pin)
{
#if defined(__riscv) || defined(__propeller2__)
    return _pin(pin);
#else
    unsigned mask=1<<pin;
    DIRA &= ~mask;
    return (INA & mask) ? 1 : 0;
#endif
}

struct def {
    const char *name;
    intptr_t val;
} fdefs[] = {
    { "getcnt",    (intptr_t)getcnt_fn },
    { "pinout",    (intptr_t)pinout_fn },
    { "pinin",     (intptr_t)pinin_fn },
    { "waitcnt",   (intptr_t)waitcnt_fn },
    { NULL, 0 }
};

char memarena[ARENA_SIZE];

int
main(int argc, char **argv)
{
    int err;
    int i;

    printf("fibo test program\n");
    err = TinyScript_Init(memarena, sizeof(memarena));
    for (i = 0; fdefs[i].name; i++) {
        err |= TinyScript_Define(fdefs[i].name, BUILTIN, fdefs[i].val);
    }
    if (err != 0) {
        printf("Initialization of interpreter failed!\n");
        return 1;
    }
    err = TinyScript_Run(fibo_ts, 0, 0);
    if (err < 0) {
        printf("error ");
        putchar('0' - err);
        putchar('\n');
    } else {
        printf("ok\n");
    }
    return 0;
}

