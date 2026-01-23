# My Basic Compiler Grammar

Here is the grammar for my statically typed language:

```ebnf
program         ::= { statement }

statement       ::= var_decl
                  | assignment
                  | if_stmt
                  | while_stmt
                  | return_stmt
                  | expr_stmt
                  | print_stmt

var_decl        ::= "VAR" identifier "=" expr "AS" type
type            ::= "INTEGER" | "FLOAT" | "BOOLEAN" | "CHAR" | "STRING"

assignment      ::= identifier "=" expr

expr_stmt       ::= expr

if_stmt         ::= "IF" ( expr ) "THEN" { statement } [ "ELSE" { statement } ] "END"

while_stmt      ::= "WHILE" ( expr ) "DO" { statement } "END"

return_stmt     ::= "RETURN" expr

print_stmt      ::= "PRINT" "(" expr { "," expr } ")"

Expr            ::= expr "+" term
                  | expr "-" term
                  | term

term            ::= term "*" factor
                  | term "/" factor
                  | factor

factor          ::= number
                  | boolean
                  | char
                  | identifier
                  | "(" expr ")"

number          ::= integer | float
boolean         ::= "TRUE" | "FALSE"
string_literal  ::= '"' { any_character } '"'
identifier      ::= letter { letter | digit | "_" }
