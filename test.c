#include <stdio.h>
#include "tinyscript.h"

int inchar() {
    return getchar();
}
void outchar(int c) {
    putchar(c);
}

void
REPL()
{
    char buf[128];
    char *s;
    int r;
    
    for(;;) {
        printf("> "); fflush(stdout);
        fgets(buf, sizeof(buf), stdin);
        for (s = buf; *s && *s != '\n' && *s != '\r'; s++)
            ;
        *s = 0;
        r = TinyScript_Run(buf);
        if (r != 0) {
            printf("error %d\n", r);
        }
    }
}

int
main()
{
    TinyScript_Init();
    REPL();
    return 0;
}

