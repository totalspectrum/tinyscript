#ifndef TINYSCRIPT_H
#define TINYSCRIPT_H

#include <stdint.h>

#ifdef __propeller__
// define SMALL_PTRS to use 16 bits for pointers
// useful for machines with <= 64KB of RAM
#define SMALL_PTRS
#endif

// errors
// all the ParseXXX functions return 0 on success, a negative
// error code otherwise
enum {
    TS_ERR_OK = 0,
    TS_ERR_NOMEM = -1,
    TS_ERR_SYNTAX = -2,
    TS_ERR_UNKNOWN_SYM = -3,
    TS_ERR_BADARGS = -4,
    TS_ERR_TOOMANYARGS = -5,
    TS_ERR_OK_ELSE = 1, // special internal condition
};

// we use this a lot
typedef char Byte;

//our target is machines with < 64KB of memory, so 16 bit pointers
//will do
typedef Byte *Ptr;

// strings are represented as (length,ptr) pairs
// this is done so that we can re-use variable names and similar
// substrings directly from the script text, without having
// to insert 0 into them
typedef struct {
#ifdef SMALL_PTRS
    uint16_t len_;
    uint16_t ptr_;
#else
    unsigned len_;
    const char *ptr_;
#endif
} String;

// val has to be able to hold a pointer
typedef intptr_t Val;

static inline unsigned StringGetLen(String s) { return (unsigned)s.len_; }
static inline const char *StringGetPtr(String s) { return (const char *)(intptr_t)s.ptr_; }
#ifdef SMALL_PTRS
static inline void StringSetLen(String *s, unsigned len) { s->len_ = (uint16_t)len; }
static inline void StringSetPtr(String *s, const char *ptr) { s->ptr_ = (uint16_t)(intptr_t)ptr; }
#else
static inline void StringSetLen(String *s, unsigned len) { s->len_ = len; }
static inline void StringSetPtr(String *s, const char *ptr) { s->ptr_ = ptr; }
#endif

// symbols can take the following forms:
#define INT      0x0  // integer
#define STRING   0x1  // string
#define OPERATOR 0x2  // operator; precedence in high 8 bits
#define ARG      0x3  // argument; value is offset on stack
#define BUILTIN  'B'  // builtin: number of operands in high 8 bits
#define USRFUNC  'f'  // user defined a procedure; number of operands in high 8 bits
#define TOK_BINOP 'o'

#define BINOP(x) (((x)<<8)+TOK_BINOP)
#define CFUNC(x) (((x)<<8)+BUILTIN)

typedef struct symbol {
    String name;
    int    type;   // symbol type
    Val    value;  // symbol value, or string ptr
} Sym;

#define MAX_BUILTIN_PARAMS 4

typedef Val (*Cfunc)(Val, Val, Val, Val);
typedef Val (*Opfunc)(Val, Val);

// structure to describe a user function
typedef struct ufunc {
    String body; // pointer to the body of the function
    int nargs;   // number of args
    // names of arguments
    String argName[MAX_BUILTIN_PARAMS];
} UserFunc;

//
// global interface
//
int TinyScript_Init(void *mem, int mem_size);
int TinyScript_Define(const char *name, int toktype, Val value);
int TinyScript_Run(const char *s, int saveStrings, int topLevel);

// provided by our caller
extern int inchar(void);
extern void outchar(int c);

#endif
