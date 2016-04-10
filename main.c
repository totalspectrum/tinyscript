#include <stdio.h>
#include <stdlib.h>
#include "tinyscript.h"

#ifdef __propeller__
#define ARENA_SIZE 4096
#else
#define ARENA_SIZE 8192
#define MAX_SCRIPT_SIZE 100000
#endif

int inchar() {
    return getchar();
}
void outchar(int c) {
    putchar(c);
}

#ifdef MAX_SCRIPT_SIZE
char script[MAX_SCRIPT_SIZE];

void
runscript(const char *filename)
{
    FILE *f = fopen(filename, "r");
    int r;
    if (!f) {
        perror(filename);
        return;
    }
    r=fread(script, 1, MAX_SCRIPT_SIZE, f);
    fclose(f);
    if (r <= 0) {
        fprintf(stderr, "File read error on %s\n", filename);
        return;
    }
    script[r] = 0;
    r = TinyScript_Run(script, 0, 1);
    if (r != 0) {
        printf("script error %d\n", r);
    }
    exit(r);
}
#endif

// compute a function of two variables
static Val testfunc(Val x, Val y, Val a, Val b)
{
    (void)a;
    (void)b;
    return x*x + y*y;
}

void
REPL()
{
    char buf[128];
    int r;
    
    for(;;) {
        printf("> "); fflush(stdout);
        fgets(buf, sizeof(buf), stdin);
        r = TinyScript_Run(buf, 1, 1);
        if (r != 0) {
            printf("error %d\n", r);
        }
    }
}

char arena[ARENA_SIZE];

int
main(int argc, char **argv)
{
    int err;
    
    err = TinyScript_Init(arena, sizeof(arena));
    err |= TinyScript_Define("dsqr", BUILTIN, (Val)testfunc);

    if (err != 0) {
        printf("Initialization of interpreter failed!\n");
        return 1;
    }
#ifdef __propeller__
    REPL();
#else
    if (argc > 2) {
        printf("Usage: tinyscript [file]\n");
    }
    if (argv[1]) {
        runscript(argv[1]);
    } else {
        REPL();
    }
#endif
    return 0;
}

