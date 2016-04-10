Introduction
============
This is tinyscript, a scripting language designed for very tiny
machines. The initial target is boards using the Parallax Propeller,
which has 32KB of RAM.

The Language
============
The scripting language itself is pretty minimalistic. The grammar for it
looks like:

    <program> ::= <stmt> | <stmt><sep><program>
    <stmt> ::= <vardecl> | <procdecl> |
               | <assignment> | <ifstmt>
               | <whilestmt> | <proccall>
               | <printstmt> | <builtincall>

The statements in a program are separated by newlines or ';'.

Either variables or procedures may be declared.

    <procdecl> ::= "proc" <symbol> <string>
    <vardecl> ::= "var" <assignment>
    <assignment> ::= <symbol> "=" <expr>

Variables must always be given a value when declared. All variables
simply hold 32 bit quantities, normally interpreted as an integer.
The symbol in an assignment outside of a vardecl must already have
been declared.

Procedures point to a string. When a procedure is called, the string
is interpreted as a script (so at that time it is parsed using the
language grammar). If a procedure is never called then it is never
parsed, so it need not contain legal code if it is not called.

Strings may be enclosed either in double quotes or between { and }.
The latter case is more useful for procedures and similar code uses,
since the brackets nest. Also note that it is legal for newlines to
appear in {} strings, but not in strings enclosed by ".

    <ifstmt> ::= "if" <expr> <string> [<elsepart>]
    <elsepart> ::= "else" <string>
    <whilestmt> ::= "while" <expr> <string> [<elsepart>]

As with subroutines, the strings in if and while statements are parsed
and interpreted on an as-needed basis. Any non-zero expression is
treated as true, and zero is treated as false. As a quirk of the
implementation, it is permitted to add an "else" clause to a while statement;
any such clause will always be executed after the loop exits.

    <subrcall> ::= <symbol>
    <printstmt> ::= "print" <printitem> [ "," <printitem>]*
    <printitem> ::= <string> | <expr>

Expressions are built up from symbols, numbers, and operators. The
operators have precedence levels 1-4. Level 0 of expressions is the
most basic, consisting of numbers or variables optionally preceded
by a unary operator:

    <expr0> ::= <symbol> | <number> 
                | <unaryop><expr0> 
                | "(" <expr> ")"
                | <builtincall>
    <builtincall> ::= <symbol> "(" [<exprlist>] ")"
    <exprlist> ::= <expr> ["," <expr>]*


    <expr1> ::= <expr0> [<binop1> <expr0>]*
    <binop1> ::= "*" | "/"

    <expr2> ::= <expr1> [<binop2> <expr2>]*
    <binop2> ::= "+" | "-"

    <expr3> ::= <expr2> [<binop3><expr3>]*
    <binop3> ::= "&" | "|" | "^" | "<<" | ">>"

    <expr4> ::= <expr3> [<binop4><expr4>]*
    <binop4> ::= "==" | "<>" | ">" | "<" | ">=" | "<="

    <unaryop> ::= <binop1> | <binop2> | <binop3> | <binop4>

Builtin functions are defined by the runtime, as are operators. The ones
listed above are merely the ones defined by default. Operators may use
any of the characters =<>+-*/&|^. Note that any string of these characters
is processed together, so for example `a*-b` is parsed as `a` `*-` `b`,
which will cause a syntax error, rather than as `a*(-b)`. The latter may
be achieved by putting a space between the `*` and the `-`.

Note that any operator may be used as a unary operator, and in this case
`<op>x` is interpreted as `0 <op> x` for any operator `<op>`. This is useful
for `+` and `-`, less so for other operators.

Interface to C
==============

The C program must initialize the interpreter with `TinyScript_Init` before
making any other calls. It may then define builtin symbols with
`TinyScript_Define(name, BUILTIN, (Val)func)`, where "name" is the name of the
symbol in scripts and "func" is the C function. Technically the function
should have prototype:

    Val func(Val a, Val b, Val c, Val d)

However, most C compiler calling conventions are such that any C function
(other than varargs ones) will work. On the script side, the interpreter
will supply 0 for any arguments the user does not supply, and will silently
ignore arguments given beyond the fourth.

To run a script, use `TinyScript_Run(script, saveStrings, topLevel)`. Here
"script" is a C string, "saveStrings" is 1 if any variable names created
in the script need to be saved in newly allocated memory (this is necessary
if the space "script" is stored in may later be overwritten, e.g. in
a REPL loop by new commands typed by the user. "topLevel" is 1 if the
variables created by the script should be kept after it finishes.

