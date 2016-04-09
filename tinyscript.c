//
// a very tiny scripting language
//
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// use this a lot
typedef char Byte;

//our target is machines with < 64KB of memory, so 16 bit pointers
//will do
typedef Byte *Ptr;

// strings are represented as pointers with their 
// our language has 4 data types: integer, string, function, and builtin
typedef enum {
    INT,
    STRING,
    FUNCTION,
    BUILTIN
} Type;

// val has to be able to hold a pointer
typedef intptr_t Val;

// strings are represented as (length,ptr) pairs
// the length is limited to 12 bits, leaving 4 bits to hold type info
typedef struct {
    unsigned tag: 4; // for type info or anything else we want
    unsigned len: 12;
    Ptr      ptr;
} String;

// a symbol should fit in 64 bits
// 32 bits for the symbol value
// 32 bits for the symbol name
// part of that 32 bits is re-used for the type

typedef struct symbol {
    String name;
    Val value; // symbol value
} Sym;

#define SYMSTACK_SIZE (1024/sizeof(Sym))
#define VALSTACK_SIZE (1024/sizeof(Val))
#define HEAP_SIZE (2048)

Sym symstack[SYMSTACK_SIZE];
int symptr;

Val valstack[VALSTACK_SIZE];
int valptr;

Byte heap[HEAP_SIZE];
int heapptr;

Val stringeq(String ai, String bi)
{
    Byte *a, *b;
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
    Byte *ptr = s.ptr;
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
    Byte *ptr = s.ptr;
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
    x.len = orig.len;
    x.ptr = malloc(x.len);
    memcpy(x.ptr, orig.ptr, x.len);
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
Cstring(char *str)
{
    String x;
    
    x.len = strlen(str);
    x.ptr = str;
    return x;
}

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
        pc = Cstring(buf);
        if (!ParseStmt()) {
            printf("?parse err\n");
        }
    }
}

int
main()
{
    REPL();
    return 0;
}

