//
// a very tiny scripting language
//
#include <stdio.h>
#include <stdint.h>

// use this a lot
typedef uint8_t Byte;

//our target is machines with < 64KB of memory, so 16 bit pointers
//will do
typedef uint16_t Ptr;

// strings are represented as pointers with their 
// our language has 4 data types: integer, string, function, and builtin
typedef enum {
    INT,
    STRING,
    FUNCTION,
    BUILTIN
} Type;

// we allow a full 32 bit range for values
typedef int32_t Val;

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
Ptr symptr;

Val valstack[VALSTACK_SIZE];
Ptr valptr;

Byte heap[HEAP_SIZE];
Ptr heapptr;

Val stringeq(String ai, String bi)
{
    Byte *a, *b;
    int i, len;
    
    if (ai.len != bi.len) {
        return 0;
    }
    a = &heap[ai.ptr];
    b = &heap[bi.ptr];
    len = ai.len;
    for (i = 0; i < len; i++) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return 1;
}

//
// parse an expression or statement
//
String pc;  // instruction pointer

// get next character from the program counter
// returns -1 on end of file
int
PeekNext()
{
    if (pc.len == 0)
        return -1;
    return heap[pc.ptr];
}

// move to next character
void
NextChar()
{
    if (pc.len == 0) return;
    pc.len--;
    pc.ptr++;
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

// parse a number; put the result on the stack
int
ParseNum()
{
    Val r = 0;
    int c;
    int valid = 0;
    for(;;) {
        c = PeekNext();
        if (c < '0' || c > '9') break;
        r = 10*r + (c-'0');
        valid = 1;
        NextChar();
    }
    if (valid)
        Push(r);
    return valid;
}

extern int ParseExpr();

// parse a value; for now, just a number
int
ParseVal()
{
    int c;
    int valid;
    c = PeekNext();
    if (c == '(') {
        NextChar();
        valid = ParseExpr();
        if (valid) {
            c = PeekNext();
            if (c == ')') {
                NextChar();
                return 1;
            }
        }
        return 0;
    }
    return ParseNum();
}

// perform an operation
void
BinaryOperator(int c)
{
    Val b = Pop();
    Val a = Pop();
    Val r;
    
    switch (c) {
    case '*':
        r = (a*b);
        break;
    case '/':
        r = (a/b);
        break;
    case '+':
        r = (a+b);
        break;
    case '-':
        r = (a-b);
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
    c = PeekNext();
    while (c == '*' || c == '/') {
        NextChar();
        valid = ParseVal();
        if (!valid) {
            Pop();
            return valid;
        }
        BinaryOperator(c);
        c = PeekNext();
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
    c = PeekNext();
    while (c == '+' || c == '-') {
        NextChar();
        valid = ParseTerm();
        if (!valid) {
            Pop();
            return valid;
        }
        BinaryOperator(c);
        c = PeekNext();
    }
    return 1;
}
int
ParseExpr()
{
    return ParseSimpleExpr();
}
String
HeapPutCstring(char *str)
{
    String x;
    Byte *dst;
    int len;
    
    x.tag = 0;
    x.ptr = heapptr;
    dst = &heap[heapptr];
    len = 0;
    while (*str) {
        len++;
        *dst++ = (Byte)*str++;
    }
    x.len = len;
    return x;
}
       
void
REPL()
{
    char buf[128];
    char *s;
    Ptr heapsave;

    for(;;) {
        printf("> "); fflush(stdout);
        fgets(buf, sizeof(buf), stdin);
        for (s = buf; *s && *s != '\n' && *s != '\r'; s++)
            ;
        *s = 0;
        heapsave = heapptr;
        pc = HeapPutCstring(buf);
        if (!ParseExpr()) {
            printf("?parse err\n");
        } else {
            Val x = Pop();
            printf("%d\n", x);
        }
        heapptr = heapsave;
    }
}

int
main()
{
    REPL();
    return 0;
}
