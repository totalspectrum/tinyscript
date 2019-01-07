/* Tinyscript interpreter
 *
 * Copyright 2016 Total Spectrum Software Inc.
 *
 * +--------------------------------------------------------------------
 * ¦  TERMS OF USE: MIT License
 * +--------------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * +--------------------------------------------------------------------
 */

//
// a very tiny scripting language
//
//#define DEBUG
#ifdef DEBUG
#include <stdio.h>
#endif

// comment this symbol out to save a little bit of space on
// the error messages
#define VERBOSE_ERRORS

#define MAX_EXPR_LEVEL 5

#include <string.h>
#include <stdlib.h>
#include "tinyscript.h"

// where our data is stored
// value stack grows from the top of the area to the bottom
// symbol stack grows from the bottom up
static int arena_size;
static Byte *arena;

static Sym *symptr;
static Val *valptr;
static String parseptr;  // acts as instruction pointer

// arguments to functions
static Val fArgs[MAX_BUILTIN_PARAMS];
static Val fResult = 0;

// variables for parsing
static int curToken;  // what kind of token is current
static int tokenArgs; // number of arguments for this token
static String token;  // the actual string representing the token
static Val tokenVal;  // for symbolic tokens, the symbol's value
static Sym *tokenSym;

// compare two Strings for equality
Val stringeq(String ai, String bi)
{
    const Byte *a, *b;
    int i, len;

    len = StringGetLen(ai);
    if (len != StringGetLen(bi)) {
        return 0;
    }
    a = StringGetPtr(ai);
    b = StringGetPtr(bi);
    for (i = 0; i < len; i++) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return 1;
}

//
// Utility functions
//

// print a string
void
PrintString(String s)
{
    unsigned len = StringGetLen(s);
    const char *ptr = (const char *)StringGetPtr(s);
    while (len > 0) {
        outchar(*ptr);
        ptr++;
        --len;
    }
}

// print a newline
void
Newline(void)
{
    outchar('\n');
}

// print a number
// FIXME: should this be delegated to the application?
static void
PrintNumber(Val v)
{
    unsigned long x;
    unsigned base = 10;
    int prec = 1;
    int digits = 0;
    int c;
    char buf[32];
    
    if (v < 0) {
        outchar('-');
        x = -v;
    } else {
        x = v;
    }
    while (x > 0 || digits < prec) {
        c = x % base;
        x = x / base;
        if (c < 10) c += '0';
        else c = (c - 10) + 'a';
        buf[digits++] = c;
    }
    // now output
    while (digits > 0) {
        --digits;
        outchar(buf[digits]);
    }
}

// look up a symbol by name
static Sym *
LookupSym(String name)
{
    Sym *s;

    s = symptr;
    while ((intptr_t)s > (intptr_t)arena) {
        --s;
        if (stringeq(s->name, name)) {
            return s;
        }
    }
    return NULL;
}

static void
outcstr(const char *ptr)
{
    while (*ptr) {
        outchar(*ptr++);
    }
}

#ifdef VERBOSE_ERRORS
//
// some functions to print an error and return
//
static int SyntaxError() {
    outcstr("syntax error before:");
    PrintString(parseptr);
    outchar('\n');
    return TS_ERR_SYNTAX;
}
static int ArgMismatch() {
    outcstr("argument mismatch\n");
    return TS_ERR_BADARGS;
}
static int TooManyArgs() {
    outcstr("too many arguments\n");
    return TS_ERR_TOOMANYARGS;
}
static int OutOfMem() {
    outcstr("out of memory\n");
    return TS_ERR_NOMEM;
}
static int UnknownSymbol() {
    outcstr(": unknown symbol\n");
    return TS_ERR_UNKNOWN_SYM;
}
#else
#define SyntaxError() TS_ERR_SYNTAX
#define ArgMismatch() TS_ERR_BADARGS
#define TooManyArgs() TS_ERR_TOOMANYARGS
#define OutOfMem()    TS_ERR_NOMEM
#define UnknownSymbol() TS_ERR_UNKNOWN_SYM
#endif

//
// parse an expression or statement
//

#define TOK_SYMBOL 'A'
#define TOK_NUMBER 'N'
#define TOK_HEX_NUMBER 'X'
#define TOK_STRING 'S'
#define TOK_IF     'i'
#define TOK_ELSE   'e'
#define TOK_WHILE  'w'
#define TOK_PRINT  'p'
#define TOK_VAR    'v'
#define TOK_VARDEF 'V'
#define TOK_BUILTIN 'B'
#define TOK_BINOP  'o'
#define TOK_FUNCDEF 'F'
#define TOK_SYNTAX_ERR 'Z'
#define TOK_RETURN 'r'

static void ResetToken()
{
    StringSetLen(&token, 0);
    StringSetPtr(&token, StringGetPtr(parseptr));
}

//
// get next character from the program counter
// returns -1 on end of file
static int
GetChar()
{
    int c;
    unsigned len = StringGetLen(parseptr);
    const char *ptr;
    if (len == 0)
        return -1;
    ptr = StringGetPtr(parseptr);
    c = *ptr++;
    --len;

    StringSetPtr(&parseptr, ptr);
    StringSetLen(&parseptr, len);
    StringSetLen(&token, StringGetLen(token)+1);
    return c;
}

//
// peek n characters forward from the program counter
// returns -1 on end of file
static int
PeekChar(unsigned int n)
{
  if (StringGetLen(parseptr) <= n)
    return -1;
  return *(StringGetPtr(parseptr) + n);
}

// remove the last character read from the token
static void
IgnoreLastChar()
{
    StringSetLen(&token, StringGetLen(token)-1);
}

// remove the last character read from the token
static void
IgnoreFirstChar()
{
    StringSetPtr(&token, StringGetPtr(token) + 1);
    StringSetLen(&token, StringGetLen(token)-1);
}
//
// undo last getchar
//
static void
UngetChar()
{
    StringSetLen(&parseptr, StringGetLen(parseptr)+1);
    StringSetPtr(&parseptr, StringGetPtr(parseptr)-1);
    IgnoreLastChar();
}

// return true if a character is in a string
static int charin(int c, const char *str)
{
    while (*str) {
        if (c == *str++) return 1;
    }
    return 0;
}

// these appear in <ctypes.h> too, but
// we don't want macros and we don't want to
// drag in a table of character flags
static int isspace(int c)
{
    return (c == ' ') || (c == '\t');
}

static int isdigit(int c)
{
    return (c >= '0' && c <= '9');
}
static int ishexchar(int c)
{
    return (c >= '0' && c <= '9') || charin(c, "abcdefABCDEF");
}
static int islower(int c)
{
    return (c >= 'a' && c <= 'z');
}
static int isupper(int c)
{
    return (c >= 'A' && c <= 'Z');
}
static int isalpha(int c)
{
    return islower(c) || isupper(c);
}
static int isidpunct(int c)
{
    return charin(c, ".:_");
}
static int isidentifier(int c)
{
    return isalpha(c) || isidpunct(c);
}

static int notquote(int c)
{
    return (c >= 0) && !charin(c, "\"\n");
}

static void
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
static int
isoperator(int c) {
    return charin(c, "+-/*=<>&|^");
}

static int
doNextToken(int israw)
{
    int c;
    int r = -1;
    Sym *sym = NULL;
    
    tokenSym = NULL;
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
        if (c == '0' && charin(PeekChar(0), "xX") && ishexchar(PeekChar(1))) {
            GetChar();
            IgnoreFirstChar();
            IgnoreFirstChar();
            GetSpan(ishexchar);
            r = TOK_HEX_NUMBER;
        } else {
            GetSpan(isdigit);
            r = TOK_NUMBER;
        }
    } else if ( isalpha(c) ) {
        GetSpan(isidentifier);
        r = TOK_SYMBOL;
        // check for special tokens
        if (!israw) {
            tokenSym = sym = LookupSym(token);
            if (sym) {
                r = sym->type & 0xff;
                tokenArgs = (sym->type >> 8) & 0xff;
                if (r < '@')
                    r = TOK_VAR;
                tokenVal = sym->value;
            }
        }
    } else if (isoperator(c)) {
        GetSpan(isoperator);
        tokenSym = sym = LookupSym(token);
        if (sym) {
            r = sym->type;
            tokenVal = sym->value;
        } else {
            r = TOK_SYNTAX_ERR;
        }
    } else if (c == '{') {
        int bracket = 1;
        ResetToken();
        while (bracket > 0) {
            c = GetChar();
            if (c < 0) {
                return TOK_SYNTAX_ERR;
            }
            if (c == '}') {
                --bracket;
            } else if (c == '{') {
                ++bracket;
            }
        }
        IgnoreLastChar();
        r = TOK_STRING;
    } else if (c == '"') {
        ResetToken();
        GetSpan(notquote);
        c = GetChar();
        if (c < 0) return TOK_SYNTAX_ERR;
        IgnoreLastChar();
        r = TOK_STRING;
    } else {
        r = c;
    }
#ifdef DEBUG
    printf("Token[%c / %x] = ", r&0xff, r);
    PrintString(token);
    printf("\n");
#endif
    curToken = r;
    return r;
}

static int NextToken() { return doNextToken(0); }
static int NextRawToken() { return doNextToken(1); }

// push a number on the result stack
// this stack grows down from the top of the arena

void
Push(Val x)
{
    --valptr;
    if ((intptr_t)valptr < (intptr_t)symptr) {
        // out of memory!
        abort();
    }
    *valptr = x;
}

// pop a number off the result stack
Val
Pop()
{
    Val r = 0;
    if ((intptr_t)valptr < (intptr_t)(arena+arena_size)) {
        r = *valptr++;
    }
    return r;
}

// convert a string to a number
Val
StringToNum(String s)
{
    Val r = 0;
    int c;
    const Byte *ptr = StringGetPtr(s);
    int len = StringGetLen(s);
    while (len-- > 0) {
        c = *ptr++;
        if (!isdigit(c)) break;
        r = 10*r + (c-'0');
    }
    return r;
}

// convert a hex string to a number
Val
HexStringToNum(String s)
{
    Val r = 0;
    int c;
    const Byte *ptr = StringGetPtr(s);
    int len = StringGetLen(s);
    while (len-- > 0) {
        c = *ptr++;
        if (!ishexchar(c)) break;
        if (c <= '9')
            r = 16 * r + (c - '0');
        else if (c <= 'F')
            r = 16 * r + (c - 'A' + 10);
        else
            r = 16 * r + (c - 'a' + 10);
    }
    return r;
}

// define a symbol
Sym *
DefineSym(String name, int typ, Val value)
{
    Sym *s = symptr;

    if (StringGetPtr(name) == NULL) {
        return NULL;
    }
    symptr++;
    if ( (intptr_t)symptr >= (intptr_t)valptr) {
        //out of memory
        return NULL;
    }
    s->name = name;
    s->value = value;
    s->type = typ;
    return s;
}

static Sym *
DefineVar(String name)
{
    return DefineSym(name, INT, 0);
}

String
Cstring(const char *str)
{
    String x;
    
    StringSetLen(&x, strlen(str));
    StringSetPtr(&x, str);
    return x;
}

int
TinyScript_Define(const char *name, int typ, Val val)
{
    Sym *s;
    s = DefineSym(Cstring(name), typ, val);
    if (!s) return OutOfMem();
    return TS_ERR_OK;
}

extern int ParseExpr(Val *result);

// parse an expression list, and push the various results
// returns the number of items pushed, or a negative error
static int
ParseExprList(void)
{
    int err;
    int count = 0;
    int c;
    Val v;
    
    do {
        err = ParseExpr(&v);
        if (err != TS_ERR_OK) {
            return err;
        }
        Push(v);
        count++;
        c = curToken;
        if (c == ',') {
            NextToken();
        }
    } while (c==',');
    return count;
}

static int ParseString(String str, int saveStrings, int topLevel);

// parse a function call
// this may be a builtin (if script == NULL)
// or a user defined script

static int
ParseFuncCall(Cfunc op, Val *vp, UserFunc *uf)
{
    int paramCount = 0;
    int expectargs;
    int c;

    if (uf) {
        expectargs = uf->nargs;
    } else {
        expectargs = tokenArgs;
    }
    c = NextToken();
    if (c != '(') return SyntaxError();
    c = NextToken();
    if (c != ')') {
        paramCount = ParseExprList();
        c = curToken;
        if (paramCount < 0) return paramCount;
    }
    if (c!=')') {
        return SyntaxError();
    }
    // make sure the right number of params is on the stack
    if (expectargs != paramCount) {
        return ArgMismatch();
    }
    // we now have "paramCount" items pushed on to the stack
    // pop em off
    while (paramCount > 0) {
        --paramCount;
        fArgs[paramCount] = Pop();
    }
    if (uf) {
        // need to invoke the script here
        // set up an environment for the script
        int i;
        int err;
        Sym* savesymptr = symptr;
        for (i = 0; i < expectargs; i++) {
            DefineSym(uf->argName[i], INT, fArgs[i]);
        }
        err = ParseString(uf->body, 0, 0);
        *vp = fResult;
        symptr = savesymptr;
        return err;
    } else {
        *vp = op(fArgs[0], fArgs[1], fArgs[2], fArgs[3]);
    }
    NextToken();
    return TS_ERR_OK;
}

// parse a primary value; for now, just a number
// or variable
// returns 0 if valid, non-zero if syntax error
// puts result into *vp
static int
ParsePrimary(Val *vp)
{
    int c;
    int err;
    
    c = curToken;
    if (c == '(') {
        NextToken();
        err = ParseExpr(vp);
        if (err == TS_ERR_OK) {
            c = curToken;
            if (c == ')') {
                NextToken();
                return TS_ERR_OK;
            }
        }
        return err;
    } else if (c == TOK_NUMBER) {
        *vp = StringToNum(token);
        NextToken();
        return TS_ERR_OK;
    } else if (c == TOK_HEX_NUMBER) {
        *vp = HexStringToNum(token);
        NextToken();
        return TS_ERR_OK;
    } else if (c == TOK_VAR) {
        *vp = tokenVal;
        NextToken();
        return TS_ERR_OK;
    } else if (c == TOK_BUILTIN) {
        Cfunc op = (Cfunc)tokenVal;
        return ParseFuncCall(op, vp, NULL);
    } else if (c == USRFUNC) {
        Sym *sym = tokenSym;
        if (!sym) return SyntaxError();
        err = ParseFuncCall(NULL, vp, (UserFunc *)sym->value);
        NextToken();
        return err;
    } else if ( (c & 0xff) == TOK_BINOP ) {
        // binary operator
        Opfunc op = (Opfunc)tokenVal;
        Val v;
        
        NextToken();
        err = ParseExpr(&v);
        if (err == TS_ERR_OK) {
            *vp = op(0, v);
        }
        return err;
    } else {
        return SyntaxError();
    }
}

// parse a level n expression
// level 0 is the lowest level (highest precedence)
// result goes in *vp
static int
ParseExprLevel(int max_level, Val *vp)
{
    int err = TS_ERR_OK;
    int c;
    Val lhs;
    Val rhs;
    
    lhs = *vp;
    c = curToken;
    while ( (c & 0xff) == TOK_BINOP ) {
        int level = (c>>8) & 0xff;
        if (level > max_level) break;
        Opfunc op = (Opfunc)tokenVal;
        NextToken();
        err = ParsePrimary(&rhs);
        if (err != TS_ERR_OK) return err;
        c = curToken;
        while ( (c&0xff) == TOK_BINOP ) {
            int nextlevel = (c>>8) & 0xff;
            if (level <= nextlevel) break;
            err = ParseExprLevel(nextlevel, &rhs);
            if (err != TS_ERR_OK) return err;
            c = curToken;
        }
        lhs = op(lhs, rhs);
    }
    *vp = lhs;
    return err;
}

int
ParseExpr(Val *vp)
{
    int err;

    err = ParsePrimary(vp);
    if (err == TS_ERR_OK) {
        err = ParseExprLevel(MAX_EXPR_LEVEL, vp);
    }
    return err;
}

static char *
stack_alloc(int len)
{
    int mask = sizeof(Val)-1;
    intptr_t base;
    
    len = (len + mask) & ~mask;
    base = ((intptr_t)valptr) - len;
    if (base < (intptr_t)symptr) {
        return NULL;
    }
    valptr = (Val *)base;
    return (char *)base;
}

static String
DupString(String orig)
{
    String x;
    char *ptr;
    unsigned len = StringGetLen(orig);
    ptr = stack_alloc(len);
    if (ptr) {
        memcpy(ptr, StringGetPtr(orig), len);
    }
    StringSetLen(&x, len);
    StringSetPtr(&x, ptr);
    return x;
}

static int ParseString(String str, int saveStrings, int topLevel);

//
// this is slightly different in that it may return the non-erro TS_ERR_ELSE
// to signify that the condition was false
//
static int ParseIf()
{
    String ifpart, elsepart;
    int haveelse = 0;
    Val cond;
    int c;
    int err;
    
    c = NextToken();
    err = ParseExpr(&cond);
    if (err != TS_ERR_OK) {
        return err;
    }
    c = curToken;
    if (c != TOK_STRING) {
        return SyntaxError();
    }
    ifpart = token;
    c = NextToken();
    if (c == TOK_ELSE) {
        c = NextToken();
        if (c != TOK_STRING) {
            return SyntaxError();
        }
        elsepart = token;
        haveelse = 1;
        NextToken();
    }
    if (cond) {
        err = ParseString(ifpart, 0, 0);
    } else if (haveelse) {
        err = ParseString(elsepart, 0, 0);
    }
    if (err == TS_ERR_OK && !cond) err = TS_ERR_OK_ELSE;
    return err;
}

static int
ParseVarList(UserFunc *uf, int saveStrings)
{
    int c;
    int nargs = 0;
    c = NextRawToken();
    for(;;) {
        if (c == TOK_SYMBOL) {
            String name = token;
            if (saveStrings) {
                name = DupString(name);
            }
            if (nargs >= MAX_BUILTIN_PARAMS) {
                return TooManyArgs();
            }
            uf->argName[nargs] = name;
            nargs++;
            c = NextToken();
            if (c == ')') break;
            if (c == ',') {
                c = NextToken();
            }
        } else if (c == ')') {
            break;
        } else {
            return SyntaxError();
        }
    }
    uf->nargs = nargs;
    return nargs;
}

static int
ParseFuncDef(int saveStrings)
{
    Sym *sym;
    String name;
    String body;
    int c;
    int nargs = 0;
    UserFunc *uf;
    
    c = NextRawToken(); // do not interpret the symbol
    if (c != TOK_SYMBOL) return SyntaxError();
    name = token;
    c = NextToken();
    uf = (UserFunc *)stack_alloc(sizeof(*uf));
    if (!uf) return OutOfMem();
    uf->nargs = 0;
    if (c == '(') {
        nargs = ParseVarList(uf, saveStrings);
        if (nargs < 0) return nargs;
        c = NextToken();
    }
    if (c != TOK_STRING) return SyntaxError();
    body = token;

    if (saveStrings) {
        // copy the strings into safe memory
        name = DupString(name);
        body = DupString(body);
    }
    uf->body = body;
    sym = DefineSym(name, USRFUNC | (nargs<<8), (intptr_t)uf);
    if (!sym) return OutOfMem();
    NextToken();
    return TS_ERR_OK;
}

// handle print statement
static int
ParsePrint()
{
    int c;
    int err = TS_ERR_OK;

print_more:
    c = NextToken();
    if (c == TOK_STRING) {
        PrintString(token);
        NextToken();
    } else {
        Val val;
        err = ParseExpr(&val);
        if (err != TS_ERR_OK) {
            return err;
        }
        PrintNumber(val);
    }
    if (curToken == ',') {
        goto print_more;
    }
    Newline();
    return err;
}

static int
ParseReturn()
{
    int err;
    NextToken();
    err = ParseExpr(&fResult);
    // terminate the script
    StringSetLen(&parseptr, 0);
    return err;
}

static int
ParseWhile()
{
    int err;
    String savepc = parseptr;

again:
    err = ParseIf();
    if (err == TS_ERR_OK_ELSE) {
        return TS_ERR_OK;
    } else if (err == TS_ERR_OK) {
        parseptr = savepc;
        goto again;
    }
    return err;
}

//
// parse one statement
// 1 is true if we need to save strings we encounter (we've been passed
// a temporary string)
//
static int
ParseStmt(int saveStrings)
{
    int c;
    String name;
    Val val;
    int err = TS_ERR_OK;
    
    c = curToken;
    
    if (c == TOK_VARDEF) {
        // a definition var a=x
        c=NextRawToken(); // we want to get VAR_SYMBOL directly
        if (c != TOK_SYMBOL) return SyntaxError();
        if (saveStrings) {
            name = DupString(token);
        } else {
            name = token;
        }
        tokenSym = DefineVar(name);
        if (!tokenSym) {
            return TS_ERR_NOMEM;
        }
        c = TOK_VAR;
        /* fall through */
    }
    if (c == TOK_VAR) {
        // is this a=expr?
        Sym *s;
        name = token;
        s = tokenSym;
        c = NextToken();
        // we expect the "=" operator
        // verify that it is "="
        if (StringGetPtr(token)[0] != '=' || StringGetLen(token) != 1) {
            return SyntaxError();
        }
        if (!s) {
#ifdef VERBOSE_ERRORS
            PrintString(name);
#endif
            return UnknownSymbol(); // unknown symbol
        }
        NextToken();
        err = ParseExpr(&val);
        if (err != TS_ERR_OK) {
            return err;
        }
        s->value = val;
    } else if (c == TOK_BUILTIN || c == USRFUNC) {
        err = ParsePrimary(&val);
        return err;
    } else if (tokenSym && tokenVal) {
        int (*func)(int) = (void *)tokenVal;
        err = (*func)(saveStrings);
    } else {
        return SyntaxError();
    }
    if (err == TS_ERR_OK_ELSE) {
        err = TS_ERR_OK;
    }
    return err;
}

static int
ParseString(String str, int saveStrings, int topLevel)
{
    String savepc = parseptr;
    Sym* savesymptr = symptr;
    int c;
    int r;
    
    parseptr = str;
    for(;;) {
        c = NextToken();
        while (c == '\n' || c == ';') {
            c = NextToken();
        }
        if (c < 0) break;
        r = ParseStmt(saveStrings);
        if (r != TS_ERR_OK) return r;
        c = curToken;
        if (c == '\n' || c == ';' || c < 0) {
            /* ok */
        } else {
            return SyntaxError();
        }
    }
    parseptr = savepc;
    if (!topLevel) {
        // restore variable context
        symptr = savesymptr;
    }
    return TS_ERR_OK;
}

//
// builtin functions
//
static Val prod(Val x, Val y) { return x*y; }
static Val quot(Val x, Val y) { return x/y; }
static Val sum(Val x, Val y) { return x+y; }
static Val diff(Val x, Val y) { return x-y; }
static Val bitand(Val x, Val y) { return x&y; }
static Val bitor(Val x, Val y) { return x|y; }
static Val bitxor(Val x, Val y) { return x^y; }
static Val shl(Val x, Val y) { return x<<y; }
static Val shr(Val x, Val y) { return x>>y; }
static Val equals(Val x, Val y) { return x==y; }
static Val ne(Val x, Val y) { return x!=y; }
static Val lt(Val x, Val y) { return x<y; }
static Val le(Val x, Val y) { return x<=y; }
static Val gt(Val x, Val y) { return x>y; }
static Val ge(Val x, Val y) { return x>=y; }

static struct def {
    const char *name;
    int toktype;
    intptr_t val;
} defs[] = {
    // keywords
    { "if",    TOK_IF, (intptr_t)ParseIf },
    { "else",  TOK_ELSE, 0 },
    { "while", TOK_WHILE, (intptr_t)ParseWhile },
    { "print", TOK_PRINT, (intptr_t)ParsePrint },
    { "var",   TOK_VARDEF, 0 },
    { "func",  TOK_FUNCDEF, (intptr_t)ParseFuncDef },
    { "return", TOK_RETURN, (intptr_t)ParseReturn },
    // operators
    { "*",     BINOP(1), (intptr_t)prod },
    { "/",     BINOP(1), (intptr_t)quot },
    { "+",     BINOP(2), (intptr_t)sum },
    { "-",     BINOP(2), (intptr_t)diff },
    { "&",     BINOP(3), (intptr_t)bitand },
    { "|",     BINOP(3), (intptr_t)bitor },
    { "^",     BINOP(3), (intptr_t)bitxor },
    { ">>",    BINOP(3), (intptr_t)shr },
    { "<<",    BINOP(3), (intptr_t)shl },
    { "=",     BINOP(4), (intptr_t)equals },
    { "<>",    BINOP(4), (intptr_t)ne },
    { "<",     BINOP(4), (intptr_t)lt },
    { "<=",    BINOP(4), (intptr_t)le },
    { ">",     BINOP(4), (intptr_t)gt },
    { ">=",    BINOP(4), (intptr_t)ge },

    { NULL, 0, 0 }
};

int
TinyScript_Init(void *mem, int mem_size)
{
    int i;
    int err;
    
    arena = (Byte *)mem;
    arena_size = mem_size;
    symptr = (Sym *)arena;
    valptr = (Val *)(arena + arena_size);
    for (i = 0; defs[i].name; i++) {
        err = TinyScript_Define(defs[i].name, defs[i].toktype, defs[i].val);
        if (err != TS_ERR_OK)
            return err;
    }
    return TS_ERR_OK;
}

int
TinyScript_Run(const char *buf, int saveStrings, int topLevel)
{
    return ParseString(Cstring(buf), saveStrings, topLevel);
}
