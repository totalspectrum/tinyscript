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
    <stmt> ::= <vardecl> | <subrdecl> |
               | <assignment> | <ifstmt>
               | <whilestmt> | <subrcall>
               | <printstmt> | <builtincall>

The statements in a program are separated by newlines or ';'.

Either variables or subroutines may be declared.

    <subrdecl> ::= "subr" <symbol> <string>
    <vardecl> ::= "var" <assignment>
    <assignment> ::= <symbol> "=" <expr>

Variables must always be given a value when declared. All variables
simply hold 32 bit quantities, normally interpreted as an integer.
The symbol in an assignment outside of a vardecl must already have
been declared.

Subroutines point to a string. When a subroutine is called, the string
is interpreted as a script (so at that time it is parsed using the
language grammar). If a subroutine is never called then it is never
parsed, so it need not contain legal code if it is not called.

Strings may be enclosed either in double quotes or between { and }.
The latter case is more useful for subroutines and similar code uses,
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


Builtin functions are defined by the runtime.

    <expr1> ::= <expr0> [<binop1> <expr0>]*
    <binop1> ::= "*" | "/"

    <expr2> ::= <expr1> [<binop2> <expr2>]*
    <binop2> ::= "+" | "-"

    <expr3> ::= <expr2> [<binop3><expr3>]*
    <binop3> ::= "&" | "|" | "^" | "<<" | ">>"

    <expr4> ::= <expr3> [<binop4><expr4>]*
    <binop4> ::= "==" | "<>" | ">" | "<" | ">=" | "<="

    <unaryop> ::= <binop2>

The runtime system may choose to create new operators at any of these levels.
