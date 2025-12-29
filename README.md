# **C0**
Compiler for **C0** written in C

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
├── README.md
├── Makefile  # Build rules
├── src/  # All source code
│   ├── main.c  # Entry point: reads input file, calls scanner/parser/etc., outputs assembly
│   ├── scanner.c  # Scanner implementation
│   ├── scanner.h  # Scanner Header: token types enum, tokenize function prototype
│   ├── parser.c  # Parser implementation
│   ├── parser.h  # AST structs, parse function
│   ├── types.c  # Type checking
│   ├── types.h
│   ├── codegen.c  # Code generation to MIPS assembly
│   ├── codegen.h
│   └── utils.c  # Helper functions (e.g., error reporting, string handling)
│   └── utils.h
├── tests/  # Test files
└── bin/  # Output dir for built executable (git ignore this)
```

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
