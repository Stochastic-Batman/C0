# **C0**
A compiler for an imperative programming language, resembling Pascal but using C syntax

**C0** is a safe subset of C introduced in the textbook:

Wolfgang J. Paul, Christoph Baumann, Petro Lutsyk, and Sabine Schmaltz.  
*System Architecture: An Ordinary Engineering Discipline*.  
Springer International Publishing, 2016.  
DOI: [10.1007/978-3-319-43065-2](https://doi.org/10.1007/978-3-319-43065-2).

The book presents a mathematically rigorous construction of a simple MIPS-based computer system from first principles, including a naive (but correctness-proven) compiler for C0. Its primary focus is on hardware and system architecture rather than advanced compiler techniques.

Since I did not have a dedicated compilers course, I am studying compiler construction using:

Douglas Thain.  
*Introduction to Compilers and Language Design* (Second Edition).  
Independently published, 2020 (freely available as PDF).  
URL: [https://www3.nd.edu/~dthain/compilerbook/compilerbook.pdf](https://www3.nd.edu/~dthain/compilerbook/compilerbook.pdf).

This project applies the techniques from Thain's book to implement a full compiler for C0 in C, targeting MIPS assembly (in the spirit of the original book's code generation chapters).

## **Project Structure**

```text
C0/
├── LL1_check.py  # Check if the transformed C0 CFG is LL1 via First and Follow sets
├── LL1_derivation.pdf  # Derivation of LL(1) C0
├── LL1_derivation.tex  # Source code for the derivation of LL(1) C0
├── README.md
├── Makefile  # Build rules
├── bin/  # Output dir for built executable (git ignored)
├── src/
│   ├── main.c  # Entry point: reads input file, calls scanner/parser/etc.
│   ├── scanner.c  # Scanner implementation
│   ├── scanner.h  # Scanner Header: token types enum, tokenize function prototype
│   ├── parser.c  # Parser implementation
│   └── parser.h  # AST structs, parse function
└── tests/  # Test files
```


## **Running the Tests**

Test files are located in the `tests/` directory. Use the following commands to build and run the test suites.

### 1. Build the Test Executables

```bash
make test_scanner
make test_parser

```


### 2. Scanner Tests

Tokenize input files using the `--scan` flag to print token details (type, lexeme, line, column, and value).

* **Simple Function:** `./bin/test_scanner --scan tests/main_42.c0`
* **Booleans & Numbers:** `./bin/test_scanner --scan tests/scanner_bool_num.c0`
* **Expressions:** `./bin/test_scanner --scan tests/scanner_expr.c0`
* **Error Handling:** `./bin/test_scanner --scan tests/scanner_error.c0`


### 3. Parser Tests

Run with the `--parse` flag to parse the input and print the **Abstract Syntax Tree (AST)** or syntax errors.

* **Simple Return:** `./bin/test_parser --parse tests/main_42.c0`
* **Complex Expression:** `./bin/test_parser --parse tests/parser_expr.c0`
* **Syntax Error:** `./bin/test_parser --parse tests/parser_error.c0`



## **Context-Free Grammar of C0**

**Note**: Dereference changed from `*` to `@` to avoid ambiguity with multiplication in parsing.

| Non-terminal | Production                                                                                  | Description                          |
|--------------|---------------------------------------------------------------------------------------------|--------------------------------------|
| **Lexical**  |                                                                                             |                                      |
| `<Di>`       | `0 \| 1 \| 2 \| 3 \| 4 \| 5 \| 6 \| 7 \| 8 \| 9`                                            | Digit                                |
| `<DiS>`      | `<Di> \| <Di><DiS>`                                                                         | Digit sequence                       |
| `<Le>`       | `a \| ... \| z \| A \| ... \| Z \| _`                                                       | Letter                               |
| `<DiLe>`     | `<Le> \| <Di>`                                                                              | Alphanumeric symbol                  |
| `<DiLeS>`    | `<DiLe> \| <DiLe><DiLeS>`                                                                   | Sequence of alphanumeric symbols     |
| `<Na>`       | `<Le> \| <Le><DiLeS>`                                                                       | Name                                 |
| `<C>`        | `<DiS> \| <DiS>u \| null`                                                                   | int/uint/null constant               |
| `<CC>`       | `'_' \| ... \| '~'`                                                                         | Char-constant with ASCII code        |
| `<BC>`       | `true \| false`                                                                             | Bool-constant                        |
| `<id>`       | `<Na> \| <id>.<Na> \| <id>[<E>] \| <id>@ \| <id>&`                                          | Identifier (field, index, deref, addr) |
| **Expressions** |                                                                                          |                                      |
| `<F>`        | `<id> \| -<F> \| (<E>) \| <C>`                                                              | Factor                               |
| `<T>`        | `<F> \| <T>*<F> \| <T>/<F>`                                                                 | Term                                 |
| `<E>`        | `<T> \| <E>+<T> \| <E>-<T>`                                                                 | Expression                           |
| `<Atom>`     | `<E> > <E> \| <E> >= <E> \| <E> < <E> \| <E> <= <E> \| <E> == <E> \| <E> != <E> \| <BC>`    | Atom                                 |
| `<BF>`       | `<id> \| <Atom> \| !<BF> \| (<BE>)`                                                         | Boolean factor                       |
| `<BT>`       | `<BF> \| <BT> && <BF>`                                                                      | Boolean term                         |
| `<BE>`       | `<BT> \| <BE> \|\| <BT>`                                                                    | Boolean expression                   |
| `<Pa>`       | `<E> \| <BE> \| <CC>`                                                                       | Parameter                            |
| `<PaS>`      | `<Pa> \| <Pa>,<PaS>`                                                                        | Parameter sequence                   |
| **Statements** |                                                                                           |                                      |
| `<St>`       | `<id> = <E> \| <id> = <BE> \| <id> = <CC>`<br>`\| if <BE> { <StS> }`<br>`\| if <BE> { <StS> } else { <StS> }`<br>`\| while <BE> { <StS> }`<br>`\| <id> = <Na>(<PaS>) \| <id> = <Na>()`<br>`\| <id> = new <Na>@` | Assignment / if-then / if-then-else / while / call / alloc |
| `<StS>`      | `<St> \| <St>; <StS>`                                                                       | Statement sequence                   |
| `<rSt>`      | `return <E> \| return <BE> \| return <CC>`                                                  | Return statement                     |
| **Types and Declarations** |                                                                             |                                      |
| `<Ty>`       | `int \| bool \| char \| uint \| <Na>`                                                       | Basic type                           |
| `<VaD>`      | `<Ty> <Na>`                                                                                 | Variable declaration                 |
| `<VaDS>`     | `<VaD> \| <VaD>;<VaDS>`                                                                     | Variable declaration sequence        |
| `<TE>`       | `<Ty>[<DiS>] \| <Ty>@ \| struct { <VaDS> }`                                                 | Type expression (array/pointer/struct) |
| `<TyD>`      | `typedef <TE> <Na>`                                                                         | Type declaration                     |
| `<TyDS>`     | `<TyD> \| <TyD>;<TyDS>`                                                                     | Type declaration sequence            |
| **Functions and Program** |                                                                                |                                      |
| `<body>`     | `<rSt> \|  <StS>;<rSt>`                                                                     | Function body                        |
| `<PaDS>`     | `<VaD> \| <VaD>,<PaDS>`                                                                     | Parameter declarations               |
| `<FuD>`      | `<Ty> <Na>(<PaDS>){<VaDS>;<body>} \| <Ty> <Na>(<PaDS>){<body>}`<br>`\| <Ty> <Na>(){<VaDS>;<body>} \| <Ty> <Na>(){<body>}` | Function declaration                 |
| `<FuDS>`     | `<FuD> \| <FuD>;<FuDS>`                                                                     | Function sequence                    |
| `<prog>`     | `<TyDS>;<VaDS>;<FuDS> \| <VaDS>;<FuDS> \| <TyDS>;<FuDS> \| <FuDS>`                          | Program                              |

The C0 grammar is not LL(1) due to several issues that prevent predictive top-down parsing with one-token lookahead:

1. Primarily, left recursion in non-terminals like `<id>` (e.g., when parsing `x.y`, the parser gets stuck in an infinite loop because it keeps trying the recursive production `<id>.<Na>` without advancing, unable to decide if `x` is the base or part of a chain). This is like an infinite descent in recursive functions without a base case.

2. `FIRST` set conflicts from overlapping starting tokens in alternatives, such as in `<prog>` (e.g., `typedef int x;` overlaps on `typedef` for programs with or without variable sections). Think of `FIRST` sets like the possible first tokens of a production (similar to entry symbols in a DFA state) - when they overlap for different choices under the same non-terminal, the parser can't predict which path to take with just one lookahead token, leading to ambiguity.

3. Further conflicts in recursive sequences like `<VaDS>` (e.g., `int x;` overlaps on `int` because the parser can't tell if it's a single declaration or the start of a list without more lookahead). These are similar to the expression issues but for lists, where the grammar uses left-recursive or unfactored productions that don't clearly separate a single item from a chain, forcing the parser to guess ahead.

### Feasibility of Rewriting to ***LL(1)***
It is possible to transform the **C0** CFG into an ***LL(1)*** grammar by eliminating left recursion, removing ambiguities (the original C0 is unambiguous, so no loss of language), and eliminating common prefixes). The resulting grammar **C0++** maintains the same language but is suitable for recursive descent parsing. 

## ***LL(1)*** CFG of **C0**

**Note**: Dereference remains `@`; lexical terminals as before (ID for <Na>, NUM for <DiS>, etc.). <Expr> now handles both arith and bool, with semantics to check types later.

| Non-terminal | Production | Description |
|--------------|------------|-------------|
| **Types** | | |
| `<Ty>` | `int \| bool \| char \| uint \| ID` | Basic type |
| `<TEprime>` | `[ NUM ] \| @ \| ε` | Type modifier |
| `<TE>` | `<Ty> <TEprime> \| struct { <VaDS> }` | Type expression |
| **Variable Declarations** | | |
| `<VaD>` | `<Ty> ID` | Variable declaration |
| `<VaDS_tail>` | `; <VaD> <VaDS_tail> \| ε` | More var decls |
| `<VaDS>` | `<VaD> <VaDS_tail>` | Var decl sequence |
| **Type Declarations** | | |
| `<TyD>` | `typedef <TE> ID` | Type declaration |
| `<TyDS_tail>` | `; <TyD> <TyDS_tail> \| ε` | More type decls |
| `<TyDS>` | `<TyD> <TyDS_tail>` | Type decl sequence |
| `<TDSO>` | `<TyDS> \| ε` | Optional typedefs |
| **L-values** | | |
| `<lvalue>` | `ID <lvalue_tail>` | L-value |
| `<lvalue_tail>` | `. ID <lvalue_tail> \| [ <Expr> ] <lvalue_tail> \| @ <lvalue_tail> \| & <lvalue_tail> \| ε` | L-value postfix |
| **Expressions** | | |
| `<Primary>` | `ID <primary_tail> \| - <Primary> \| ! <Primary> \| ( <Expr> ) \| <C> \| <CC> \| <BC>` | Primary expression |
| `<primary_tail>` | `( <PSO> ) \| <lvalue_tail>` | Call or postfix ops |
| `<MulExpr>` | `<Primary> <MulExpr_tail>` | Multiplicative expr |
| `<MulExpr_tail>` | `* <Primary> <MulExpr_tail> \| / <Primary> <MulExpr_tail> \| ε` | Mul/div continuation |
| `<AddExpr>` | `<MulExpr> <AddExpr_tail>` | Additive expr |
| `<AddExpr_tail>` | `+ <MulExpr> <AddExpr_tail> \| - <MulExpr> <AddExpr_tail> \| ε` | Add/sub continuation |
| `<RelExpr>` | `<AddExpr> <RelExpr_tail>` | Relational expr |
| `<RelExpr_tail>` | `<rel_op> <AddExpr> <RelExpr_tail> \| ε` | Relational continuation |
| `<AndExpr>` | `<RelExpr> <AndExpr_tail>` | Logical AND expr |
| `<AndExpr_tail>` | `&& <RelExpr> <AndExpr_tail> \| ε` | AND continuation |
| `<Expr>` | `<AndExpr> <Expr_tail>` | Full expression |
| `<Expr_tail>` | `\|\| <AndExpr> <Expr_tail> \| ε` | OR continuation |
| **Call Parameters** | | |
| `<PaS>` | `<Expr> <PaS_tail>` | Parameter sequence |
| `<PaS_tail>` | `, <Expr> <PaS_tail> \| ε` | More parameters |
| `<PSO>` | `<PaS> \| ε` | Optional parameters |
| **Statements** | | |
| `<RHS>` | `<Expr> \| new ID @` | Assignment RHS |
| `<rSt>` | `return <Expr>` | Return statement |
| `<EP>` | `else { <StS> } \| ε` | Optional else |
| `<St>` | `<lvalue> = <RHS> \| if <Expr> { <StS> } <EP> \| while <Expr> { <StS> }` | Statement |
| `<StS_tail>` | `; <St> <StS_tail> \| ε` | More statements |
| `<StS>` | `<St> <StS_tail>` | Statement sequence |
| **Function Body** | | |
| `<locals>` | `local <VaDS> \| ε` | Local declarations |
| `<SSO>` | `<StS> \| ε` | Optional statements |
| `<body>` | `<SSO> <rSt>` | Function body |
| **Function Parameters** | | |
| `<PaDS>` | `<VaD> <PaDS_tail>` | Param declarations |
| `<PaDS_tail>` | `, <VaD> <PaDS_tail> \| ε` | More param decls |
| `<PDSO>` | `<PaDS> \| ε` | Optional param decls |
| **Program** | | |
| `<GDT>` | `; \| ( <PDSO> ) { <locals> <body> }` | Var end or function def |
| `<GD>` | `<Ty> ID <GDT>` | Global declaration |
| `<GDs>` | `<GD> <GDs> \| ε` | Global decl sequence |
| `<prog>` | `<TDSO> <GDs>` | Program |

## Terminals

| Terminal | Description |
|----------|-------------|
| `ID` | Identifier |
| `NUM` | Integer literal (for array sizes) |
| `<C>` | Integer constant |
| `<CC>` | Character constant |
| `<BC>` | Boolean constant (`true`, `false`) |
| `<rel_op>` | Relational operator (`<`, `>`, `<=`, `>=`, `==`, `!=`) |
| `int`, `bool`, `char`, `uint` | Built-in type keywords |
| `struct`, `typedef`, `new` | Type-related keywords |
| `if`, `else`, `while`, `return`, `local` | Control and declaration keywords |
| `+`, `-`, `*`, `/` | Arithmetic operators |
| `&&`, `\|\|`, `!` | Logical operators |
| `@`, `&` | Pointer dereference and address-of |
| `.` | Field access |
| `[`, `]` | Array indexing |
| `(`, `)` | Parentheses |
| `{`, `}` | Braces |
| `;` | Statement/declaration separator |
| `,` | Parameter separator |
| `=` | Assignment |
