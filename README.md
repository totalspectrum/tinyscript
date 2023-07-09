Introduction
============

This is tinyscript, a scripting language designed for very tiny
machines. The initial target is boards using the Parallax Propeller,
which has 32KB of RAM, but the code is written in ANSI C so it should
work on any platform (e.g. testing is done on x86-64 Linux).

On the propeller, the interpreter code needs about 3K of memory in CMM
mode or 5K in LMM. On the x86-64 the interpreter code is 6K. The size
of the workspace you give to the interpreter is up to you, although in
practice it would not be very useful to use less than 2K of RAM. The
processor stack is used as well, so it will need some space.

tinyscript is copyright 2016-2021 Total Spectrum Software Inc. and released
under the MIT license. See the COPYING file for details.

The Language
============
The scripting language itself is pretty minimalistic. The grammar for it
looks like:

    <program> ::= <stmt> | <stmt><sep><program>
    <stmt> ::= <vardecl> | <arrdecl> | <funcdecl> |
               | <assignment> | <ifstmt>
               | <whilestmt> | <funccall>
               | <printstmt> | <returnstmt>

The statements in a program are separated by newlines or ';'.

Either variables or functions may be declared.

    <vardecl> ::= "var" <assignment>
    <arrdecl> ::= "array" <symbol> "(" <number> ")" | "array" <symbol>
    <funcdecl> ::= "func" <symbol> "(" <varlist> ")" <string>
    <assignment> ::= <symbol> "=" <expr>
    <varlist> ::= <symbol> [ "," <symbol> ]+
    
Variables must always be given a value when declared (unless they are
arrays). All non-array variables simply hold 32 bit quantities,
normally interpreted as an integer.  The symbol in an assignment
outside of a vardecl must already have been declared.

Arrays are simple one dimensional arrays. Support for arrays does add
a little bit of code, so they are optional (included if ARRAY_SUPPORT
is defined in tinyscript.h). If the array declaration includes a size,
then a new (uninitialized) array is created. If it does not include a
size, then it must match one of the enclosing function's parameters,
in which case that parameter is checked and must be an array.

Array indices start at 0. Array index -1 is special and holds the
length of the array,

Functions point to a string. When a procedure is called, the string
is interpreted as a script (so at that time it is parsed using the
language grammar). If a function is never called then it is never
parsed, so it need not contain legal code if it is not called.

Strings may be enclosed either in double quotes or between { and }.
The latter case is more useful for functions and similar code uses,
since the brackets nest. Also note that it is legal for newlines to
appear in {} strings, but not in strings enclosed by ".

    <ifstmt> ::= "if" <expr> <string> [<elsepart>]
    <elsepart> ::= "else" <string> | "elseif" <expr> [<elsepart>]
    <whilestmt> ::= "while" <expr> <string> [<elsepart>]

As with functions, the strings in if and while statements are parsed
and interpreted on an as-needed basis. Any non-zero expression is
treated as true, and zero is treated as false. As a quirk of the
implementation, it is permitted to add an "else" clause to a while statement;
any such clause will always be executed after the loop exits.

    <returnstmt> ::= "return" <expr>

Return statements are used to terminate a function and return a value
to its caller.

    <printstmt> ::= "print" <printitem> [ "," <printitem>]+
    <printitem> ::= <string> | <expr>

Expressions are built up from symbols, numbers (decimal or hex integers), and
operators. The operators have precedence levels 1-4. Level 0 of expressions is
the most basic, consisting of numbers or variables optionally preceded by a
unary operator:

    <expr0> ::= <symbol> | <number> 
                | <unaryop><expr0> 
                | "(" <expr> ")"
                | <builtincall>
    <funccall> ::= <symbol> "(" [<exprlist>] ")"
    <exprlist> ::= <expr> ["," <expr>]*

    <number> ::= <digit>+ | "0x"<digit>+ | "'"<asciicharsequence>"'"

    <asciicharsequence> ::= <printableasciichar> | "\'" | "\\" | "\n" | "\t" | "\r"

    <printableasciichar> ::= ' ' to '~' excluding ' and \

    <expr1> ::= <expr0> [<binop1> <expr0>]*
    <binop1> ::= "*" | "/" | "%"

    <expr2> ::= <expr1> [<binop2> <expr2>]*
    <binop2> ::= "+" | "-"

    <expr3> ::= <expr2> [<binop3><expr3>]*
    <binop3> ::= "&" | "|" | "^" | "<<" | ">>"

    <expr4> ::= <expr3> [<binop4><expr4>]*
    <binop4> ::= "=" | "<>" | ">" | "<" | ">=" | "<="

    <unaryop> ::= <binop1> | <binop2> | <binop3> | <binop4>

Builtin functions are defined by the runtime, as are operators. The ones
listed above are merely the ones defined by default. Operators may use
any of the characters `!=<>+-*/&|^%`. Any string of the characters
`!=<>&|^` is processed together, but the operator characters `+-*/` may only
appear on their own.

Note that any operator may be used as a unary operator, and in this case
`<op>x` is interpreted as `0 <op> x` for any operator `<op>`. This is useful
for `+` and `-`, less so for other operators.

`%` is the modulo operator, as in C.

Variable Scope
--------------

Variables are dynamically scoped. For example, in:
```
var x=2

func printx() {
  print x
}
func myfunc() {
  var x=3
  printx()
}
```
invoking `myfunc` will cause 3 to be printed, not 2 as in statically scoped
languages.

Variable Types
--------------

Basically the only type that may be held in variables is an integer. If array support is compiled in, then arrays of integers may be created. Some functions may treat arrays of integers as strings, but there is no native support for this, unfortunately.

Interface to C
==============

Environment Requirements
------------------------

The interpreter is quite self-contained. The functions needed to
interface with it are `outchar` (called to print a single character),
and `memcpy`. `outchar` is the only one of these that is
non-standard. It takes a single int as parameter and prints it as a
character. This is the function the interpreter uses for output
e.g. in the `print` statement.

Optionally you can provide a function TinyScript_Stop() to check whether
a running script should stop. To do this, edit the tinyscript.h file
to remove the default definition (0) for TinyScript_Stop().


Configuration
-------------

Language configuration options are in the form of defines at the top
of tinyscript.h:

```
VERBOSE_ERRORS    - gives better error messages (costs a tiny bit of space)
SMALL_PTRS        - use 16 bits for pointers (for very small machines)
ARRAY_SUPPORT     - include support for integer arrays
```

The demo app main.c has some configuration options in the Makefile:

```
READLINE          - use the GNU readline library for entering text
LINENOISE         - use the linenoise.c library for entering text
```

Standard Library
----------------

There is an optional standard library in tinyscript_lib.{c,h} that adds
`strlen` as a standard requirement and requires the user to define two 
functions: `ts_malloc` and `ts_free`. These can be wrappers for `malloc`/`free`
or perhaps `pvPortMalloc` / `vPortFree` on FreeRTOS systems.

Application Usage
-----------------

As mentioned above, the function `outchar` must be defined by the application
to allow for printing. The following definition will work for standard C:

```
#include <stdio.h>
void outchar(int c) { putchar(c); }
```

Embedded systems may want to provide a definition that uses the serial port
or an attached display.

For the optional standard library `ts_malloc` and `ts_free` must be defined by
the application. The following definitions will work for standard C:

```
#include <stdlib.h>
void * ts_malloc(Val size) {
  return malloc(size);
}
void ts_free(void * pointer) {
  free(pointer);
}
```

The application must initialize the interpreter with `TinyScript_Init` before
making any other calls. `TinyScript_Init` takes two parameters: the base
of a memory region the interpreter can use, and the size of that region.
It returns `TS_ERR_OK` on success, or an error on failure. It is recommended
to provide at least 2K of space to the interpreter.

If `TinyScript_Init` succeeds, the application may then define builtin
symbols with `TinyScript_Define(name, BUILTIN, (Val)func)`, where
`name` is the name of the symbol in scripts and `func` is the C
function. Technically the function should have prototype:

    Val func(Val a, Val b, Val c, Val d)

However, most C compiler calling conventions are such that any C function
(other than varargs ones) will work. (Certainly this is true of GCC on
the Propeller). On the script side, the interpreter
will supply 0 for any arguments the user does not supply, and will silently
ignore arguments given beyond the fourth.

In order to use the optional standard library, its functions need to be added
to the Tinyscript context by calling `ts_define_funcs()`. Check the source code
and tests for more information about what is included in the standard library.

To run a script, use `TinyScript_Run(script, saveStrings, topLevel)`. Here
`script` is a C string, `saveStrings` is 1 if any variable names created
in the script need to be saved in newly allocated memory -- this is necessary
if the space `script` is stored in may later be overwritten, e.g. in
a REPL loop by new commands typed by the user. `topLevel` is 1 if the
variables created by the script should be kept after it finishes.

Standard Library
-----------------
The standard library is optional, and is found in the file `tinyscript_lib.c`. It must be initialized with `ts_define_funcs()` before use. Functions provided are:

`not(x)`: return 1 if x == 0, 0 otherwise

`bool(x)`: returns 0 if x == 0, 1 otherwise

`list_new(n)`: returns a handle to a new list which may contain up to `n` elements

`list_dup(x)`: duplicates the list `x`

`list_free(x)`: frees a list

`list_pop(x)`: removes the last element from list `x` and returns it; returns -1 if no elements have been added to the list

`list_push(x, a)`: appends the value `a` to the list whose handle is `x`

`list_get(x, i)`: retrieves the `i`th element of the list `x`, or -1 if there is no such element

`list_set(x, i, a)`: sets the `i`th element of list `x` to `a`.

`list_size(x)`: returns the current length of the list

Acknowledgements
================
I'd like to thank Mickey Delp and Daniel Landau for their contributions to tinyscript. Daniel's bug reports have been invaluable, and he contributed (among other things) the optional standard library, readline support, and hex support. Mickey contributed better error handling, the modulo operator, and optional array support.

