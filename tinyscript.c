//
// a very tiny scripting language
//
#include <stdio.h>
#include <string.h>
#include "tinyscript.h"

#define SYMSTACK_SIZE (1024/sizeof(Sym))
#define VALSTACK_SIZE (1024/sizeof(Val))

Sym symstack[SYMSTACK_SIZE];
int symptr;

Val valstack[VALSTACK_SIZE];
int valptr;

Val stringeq(String ai, String bi)
{
    const Byte *a, *b;
    int i, len;
    
    if (ai.len != bi.len) {
        return 0;
    }
    a = ai.ptr;
    b = bi.ptr;
    len = ai.len;
    for (i = 0; i < len; i++) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return 1;
}

String pc;  // instruction pointer

//
// Utility function
//
void
PrintString(String s)
{
    int len = s.len;
    const Byte *ptr = s.ptr;
    while (len > 0) {
        putchar(*ptr);
        ptr++;
        --len;
    }
}

//
// parse an expression or statement
//

#define TOK_SYMBOL 'A'
#define TOK_NUMBER 'N'
#define TOK_STRING 'S'

// fetch the next token
int curToken;
String token;
Val val;

static void ResetToken()
{
    token.len = 0;
    token.ptr = pc.ptr;
}

//
// get next character from the program counter
// returns -1 on end of file
int
GetChar()
{
    int c;
    if (pc.len == 0)
        return -1;
    c = *pc.ptr;
    pc.ptr++;
    pc.len--;
    token.len++;
    return c;
}

//
// undo last getchar
// (except at end of input)
//
void
UngetChar()
{
    if (1) {
        pc.len++;
        pc.ptr--;
        token.len--;
    }
}

int isspace(int c)
{
    return (c == ' ' || c == '\t');
}

int isdigit(int c)
{
    return (c >= '0' && c <= '9');
}
int islower(int c)
{
    return (c >= 'a' && c <= 'z');
}
int isalpha(int c)
{
    return islower(c) || ((c >= 'A' && c <= 'Z'));
}

void
GetSpan(int (*testfn)(int))
{
    int c;
    do {
        c = GetChar();
    } while (testfn(c));
    if (c != -1) {
        UngetChar();
    }
}

int
NextToken()
{
    int c;
    int r = -1;
    
    ResetToken();
    for(;;) {
        c = GetChar();
        if (isspace(c)) {
            ResetToken();
        } else {
            break;
        }
    }

    if (c == '#') {
        // comment
        do {
            c = GetChar();
        } while (c >= 0 && c != '\n');
        r = c;
    } else if (isdigit(c)) {
        GetSpan(isdigit);
        r = TOK_NUMBER;
    } else if ( isalpha(c) ) {
        GetSpan(isalpha);
        r = TOK_SYMBOL;
    } else {
        r = c;
    }
#if 0
    printf("Token[%c] = ", r);
    PrintString(token);
    printf("\n");
#endif
    curToken = r;
    return r;
}

// push a number on the result stack
void
Push(Val x)
{
    valstack[valptr++] = x;
}

// pop a number off the result stack
Val
Pop()
{
    Val r = 0;
    if (valptr > 0) {
        --valptr;
        r = valstack[valptr];
    }
    return r;
}

// convert a string to a number
Val
StringToNum(String s)
{
    Val r = 0;
    int c;
    const Byte *ptr = s.ptr;
    int len = s.len;
    while (len-- > 0) {
        c = *ptr++;
        if (!isdigit(c)) break;
        r = 10*r + (c-'0');
    }
    return r;
}

// look up a symbol by name
Sym *
LookupSym(String name)
{
    Sym *s;
    int n;

    n = symptr;
    while (n > 0) {
        --n;
        s = &symstack[n];
        if (stringeq(s->name, name)) {
            return s;
        }
    }
    return NULL;
}
// define a symbol
Sym *
DefineSym(String name)
{
    Sym *s;
    if (symptr == SYMSTACK_SIZE) {
        printf("too many symbols\n");
        return NULL;
    }
    s = &symstack[symptr++];
    s->name = name;
    s->value = 0;
    return s;
}

extern int ParseExpr();

// parse a value; for now, just a number
// returns 1 if valid, 0 if syntax error

int
ParseVal()
{
    int c;
    int valid;
    c = curToken;
    if (c == '(') {
        NextToken();
        valid = ParseExpr();
        if (valid) {
            c = curToken;
            if (c == ')') {
                NextToken();
                return 1;
            }
        }
        return 0;
    } else if (c == TOK_NUMBER) {
        Push(StringToNum(token));
        NextToken();
        return 1;
    } else if (c == TOK_SYMBOL) {
        Sym *sym = LookupSym(token);
        if (!sym) return 0;
        Push(sym->value);
        NextToken();
        return 1;
    } else {
        return 0;
    }
}

// perform an operation
void
BinaryOperator(int c)
{
    Val b = Pop();
    Val r = Pop();
    
    switch (c) {
    case '*':
        r = (r*b);
        break;
    case '/':
        r = (r/b);
        break;
    case '+':
        r = (r+b);
        break;
    case '-':
        r = (r-b);
        break;
    }
    Push(r);
}

// parse a term (a*b, a/b)
int
ParseTerm()
{
    int valid = 0;
    int c;
    valid = ParseVal();
    if (!valid) return valid;
    c = curToken;
    while (c == '*' || c == '/') {
        NextToken();
        valid = ParseVal();
        if (!valid) {
            Pop();
            return valid;
        }
        BinaryOperator(c);
        c = curToken;
    }
    return 1;
}
int
ParseSimpleExpr()
{
    int valid = 0;
    int c;
    valid = ParseTerm();
    if (!valid) return valid;
    c = curToken;
    while (c == '+' || c == '-') {
        NextToken();
        valid = ParseTerm();
        if (!valid) {
            Pop();
            return valid;
        }
        BinaryOperator(c);
        c = curToken;
    }
    return 1;
}
int
ParseExpr()
{
    int negflag = 0;

    if (curToken == '-') {
        negflag = 1;
        NextToken();
    }
    if (ParseSimpleExpr()) {
        if (negflag) {
            Push(-Pop());
        }
        return 1;
    }
    return 0;
}

static String
DupString(String orig)
{
    String x;
    char *ptr;
    x.len = orig.len;
    ptr = malloc(x.len);
    memcpy(ptr, orig.ptr, x.len);
    x.ptr = ptr;
    return x;
}

int
ParseStmt()
{
    int c;
    String name;
    Val val;

    c = NextToken();
    if (c == TOK_SYMBOL) {
        // is this a=expr?
        Sym *s;
        name = token;
        c = NextToken();
        if (c != '=') return 0;
        NextToken();
        if (!ParseExpr()) {
            return 0;
        }
        val = Pop();
        s = LookupSym(name);
        if (!s) {
            // FIXME? needed for REPL, not elsewhere
            name = DupString(name);
            s = DefineSym(name);
            if (!s) {
                return 0;
            }
        }
        s->value = val;
    } else {
        if (!ParseExpr()) {
            return 0;
        }
        val = Pop();
    }
    printf("%d\n", (int)val);
    return 1;
}

String
Cstring(const char *str)
{
    String x;
    
    x.len = strlen(str);
    x.ptr = str;
    return x;
}

void
TinyScript_Init(void)
{
}

int
TinyScript_Run(const char *buf)
{
    pc = Cstring(buf);
    if (!ParseStmt()) {
        printf("?parse err\n");
        return -1;
    }
    return 0;
}
