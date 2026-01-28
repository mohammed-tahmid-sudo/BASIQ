# My Basic Compiler Grammar

```markdown
# BASIC-Like Programming Language Grammar

## 1. Program
```

```
<program> ::= <statement_list>

```

## 2. Statement List
```

<statement_list> ::= <statement>
| <statement> <statement_list>

```

## 3. Statements
```

<statement> ::= <assignment>
| <print_stmt>
| <input_stmt>
| <if_stmt>
| <for_stmt>
| <while_stmt>
| <end_stmt>

```

## 4. Assignment
```

<assignment> ::= <variable> "=" <expression> <variable> ::= [A-Za-z][A-Za-z0-9]*

```

## 5. Expressions
```

<expression> ::= <term>
| <expression> "+" <term>
| <expression> "-" <term>

<term>       ::= <factor>
| <term> "*" <factor>
| <term> "/" <factor>

<factor>     ::= <number>
| <variable>
| "(" <expression> ")"

<number>     ::= [0-9]+

```

## 6. Print Statement
```

<print_stmt> ::= "PRINT" <expression>

```

## 7. Input Statement
```

<input_stmt> ::= "INPUT" <variable>

```

## 8. If Statement
```

<if_stmt> ::= "IF" <condition> "THEN" <statement_list> ["ELSE" <statement_list>] "END IF"

<condition> ::= <expression> <relop> <expression>

<relop> ::= "=" | "<>" | "<" | ">" | "<=" | ">="

```

## 9. For Loop
```

<for_stmt> ::= "FOR" <variable> "=" <expression> "TO" <expression> ["STEP" <expression>] <statement_list> "NEXT" <variable>

```

## 10. While Loop
```

<while_stmt> ::= "WHILE" <condition> <statement_list> "WEND"

```

## 11. End Statement
```

<end_stmt> ::= "END"

```
```
