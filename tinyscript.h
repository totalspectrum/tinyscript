#ifndef TINYSCRIPT_H
#define TINYSCRIPT_H

#include <stdint.h>

// we use this a lot
typedef char Byte;

//our target is machines with < 64KB of memory, so 16 bit pointers
//will do
typedef Byte *Ptr;

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
typedef struct {
    unsigned len;
    const char *ptr;
} String;

// a symbol should fit in 64 bits
// 32 bits for the symbol value
// 32 bits for the symbol name
// part of that 32 bits is re-used for the type

typedef struct symbol {
    String name;
    Val value; // symbol value
} Sym;


//
// global interface
//
void TinyScript_Init();
int TinyScript_Run(const char *s);

#endif
