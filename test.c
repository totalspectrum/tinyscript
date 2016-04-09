#include <stdio.h>
#include "tinyscript.h"

void
REPL()
{
    char buf[128];
    char *s;

    for(;;) {
        printf("> "); fflush(stdout);
        fgets(buf, sizeof(buf), stdin);
        for (s = buf; *s && *s != '\n' && *s != '\r'; s++)
            ;
        *s = 0;
        TinyScript_Run(buf);
    }
}

int
main()
{
    TinyScript_Init();
    REPL();
    return 0;
}

