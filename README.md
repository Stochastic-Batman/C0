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
