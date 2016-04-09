#ifndef TINYSCRIPT_H
#define TINYSCRIPT_H

#include <stdint.h>

// errors
// all the ParseXXX functions return 0 on success, a non-zero
// error code otherwise
enum {
    TS_ERR_OK = 0,
    TS_ERR_NOMEM = -1,
    TS_ERR_SYNTAX = -2,
    TS_ERR_UNKNOWN_SYM = -3
};

// we use this a lot
typedef char Byte;

//our target is machines with < 64KB of memory, so 16 bit pointers
//will do
typedef Byte *Ptr;

// val has to be able to hold a pointer
typedef intptr_t Val;

// strings are represented as (length,ptr) pairs
typedef struct {
    unsigned len;
    const char *ptr;
} String;

// symbols can take the following forms:
typedef enum {
    INT,
    STRING,
    FUNCTION,  // scripting language func
    TOKEN,     // language token
    BUILTIN_0, // builtin func with no args
    BUILTIN_1, // builtin func with 1 arg
    BUILTIN_2, // builtin func with 2 args
} Type;

// a symbol should fit in 64 bits
// 32 bits for the symbol value
// 32 bits for the symbol name

typedef struct symbol {
    String name;
    Val    value;  // symbol value
    Type   type;   // symbol type
} Sym;


//
// global interface
//
void TinyScript_Init();
int TinyScript_Run(const char *s);

// provided by our caller
extern int inchar(void);
extern void outchar(int c);

#endif
